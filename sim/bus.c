#include "bus.h"

void init_bus(Core * core[CORE_COUNT]){
    for(int i = 0; i < CORE_COUNT; i++){
        system_bus.cpu_cache[i] = &(core[i]->cache);
        system_bus.bus_interface[i] = &(core[i]->bus_interface);
    }
}

void send_bus_read_request(Core * core, uint32_t address, bool exclusive){
    if (core->bus_interface.has_pending_request) return;

    core->bus_interface.request.bus_orig_id = core->id;
    core->bus_interface.request.bus_addr = address;
    core->bus_interface.request.bus_cmd = exclusive ? BUS_RDX : BUS_RD;
    core->bus_interface.has_pending_request = true;
    core->bus_interface.request_done = false;
}

void bus_handler(){
    // Reset bus wire if idle
    if (!system_bus.busy) {
        system_bus.bus_cmd = BUS_NOCMD;
        system_bus.bus_orig_id = 0;
        system_bus.bus_addr = 0;
        system_bus.bus_data = 0;
        system_bus.bus_shared = false;
    }

    // 1. ACTIVE TRANSACTION
    if (system_bus.busy) {
        // Cooldown for latency
        if (system_bus.cooldown_timer > 0) {
            system_bus.cooldown_timer--;
            return;
        }

        // Processing Transfer
        int requester = system_bus.bus_orig_id;
        int cache_idx = (system_bus.bus_addr >> 3) & 0x3F;
        
        // Align address to the start of the block (Critical Fix)
        // System is Word Addressed. Block is 8 words. Mask last 3 bits.
        uint32_t mem_block_addr = system_bus.bus_addr & ~(CACHE_BLOCK_SIZE - 1); 
        
        if (system_bus.bus_cmd == BUS_RD || system_bus.bus_cmd == BUS_RDX) {
            // Read from Main Memory -> Bus -> Cache
            uint32_t data = system_bus.system_memory[mem_block_addr + system_bus.word_offset];
            system_bus.bus_data = data;
            system_bus.cpu_cache[requester]->dsram[cache_idx].word[system_bus.word_offset] = data;
        
        } else if (system_bus.bus_cmd == BUS_FLUSH) {
            // Write from Cache -> Bus -> Main Memory
            uint32_t data = system_bus.cpu_cache[requester]->dsram[cache_idx].word[system_bus.word_offset];
            system_bus.bus_data = data;
            system_bus.system_memory[mem_block_addr + system_bus.word_offset] = data;
        }

        system_bus.word_offset++;

        // Transaction Complete
        if (system_bus.word_offset >= CACHE_BLOCK_SIZE) {
            
            // Update MESI States
            if (system_bus.bus_cmd == BUS_RD) {
                MESI_State new_state = system_bus.bus_shared ? MESI_SHARED : MESI_EXCLUSIVE;
                system_bus.cpu_cache[requester]->tsram[cache_idx].mesi_state = new_state;
                system_bus.cpu_cache[requester]->tsram[cache_idx].tag = (system_bus.bus_addr >> 9) & 0xFFF;
            } 
            else if (system_bus.bus_cmd == BUS_RDX) {
                system_bus.cpu_cache[requester]->tsram[cache_idx].mesi_state = MESI_MODIFIED;
                system_bus.cpu_cache[requester]->tsram[cache_idx].tag = (system_bus.bus_addr >> 9) & 0xFFF;
            } 
            else if (system_bus.bus_cmd == BUS_FLUSH) {
                // Flushed block becomes Invalid (or Shared, but usually Invalid in MIPS sim)
                system_bus.cpu_cache[requester]->tsram[cache_idx].mesi_state = MESI_INVALID;
            }

            // Important: Only clear the pending flag if this was a requested op, not a forced snoop flush
            if (system_bus.bus_cmd != BUS_FLUSH) {
                 system_bus.bus_interface[requester]->request_done = true;
                 system_bus.bus_interface[requester]->has_pending_request = false;
            }
            
            system_bus.busy = false;
            system_bus.last_granted_device = requester;
            system_bus.word_offset = 0;
        }
        return;
    }

    // 2. ARBITRATION
    int start = (system_bus.last_granted_device + 1) % CORE_COUNT;
    for (int i = 0; i < CORE_COUNT; i++) {
        int id = (start + i) % CORE_COUNT;
        BusInterface *bi = system_bus.bus_interface[id];

        if (bi->has_pending_request && !bi->request_done) {
            
            // SNOOPING: Check for collisions
            // bool flush_needed = false;
            uint32_t tag = (bi->request.bus_addr >> 9) & 0xFFF;
            uint32_t idx = (bi->request.bus_addr >> 3) & 0x3F;

            for(int c = 0; c < CORE_COUNT; c++) {
                if (c == id) continue; // Don't snoop self
                
                TSRAM_Line *line = &system_bus.cpu_cache[c]->tsram[idx];
                if (line->mesi_state != MESI_INVALID && line->tag == tag) {
                    system_bus.bus_shared = true; // Signal shared

                    if (line->mesi_state == MESI_MODIFIED) {
                        // Found Modified line! Must FLUSH.
                        system_bus.busy = true;
                        system_bus.bus_orig_id = c; // The flusher
                        system_bus.bus_cmd = BUS_FLUSH;
                        system_bus.bus_addr = bi->request.bus_addr;
                        system_bus.cooldown_timer = 0; 
                        system_bus.word_offset = 0;
                        return; // Start flush immediately
                    }
                    
                    if (bi->request.bus_cmd == BUS_RDX) {
                        line->mesi_state = MESI_INVALID; // Invalidate others on Write
                    }
                }
            }

            // Grant Bus
            system_bus.busy = true;
            system_bus.bus_orig_id = id;
            system_bus.bus_cmd = bi->request.bus_cmd;
            system_bus.bus_addr = bi->request.bus_addr;
            system_bus.cooldown_timer = BUS_DELAY;
            system_bus.word_offset = 0;
            return;
        }
    }
}