#include "bus.h"

void init_bus(core_T * core[CORE_COUNT]){
    for(int i = 0; i < CORE_COUNT; i++){
        system_bus.cpu_cache[i] = &(core[i]->cache);
        system_bus.bus_interface[i] = &(core[i]->bus_interface);
    }
}

void send_bus_read_request(core_T * core, uint32_t address, bool exclusive){
    if (core->bus_interface.has_pending_request) return;

    core->bus_interface.request.bus_origin_id = core->id;
    core->bus_interface.request.bus_addr = address;
    core->bus_interface.request.bus_cmd = exclusive ? BUS_RDX : BUS_RD;
    core->bus_interface.has_pending_request = true;
    core->bus_interface.request_done = false;
}

void bus_handler(){
    // Reset bus wire if idle
    if (!system_bus.busy) {
        system_bus.bus_cmd = BUS_NOCMD;
        system_bus.bus_origin_id = 0;
        system_bus.bus_addr = 0;
        system_bus.bus_data = 0;
        system_bus.bus_shared = false;
        system_bus.flush_post_state_valid = false;
    }

    // Active transaction
    if (system_bus.busy) {
        // Cooldown for latency
        if (system_bus.cooldown_timer > 0) {
            system_bus.cooldown_timer--;
            return;
        }

        // Processing Transfer
        int requester = system_bus.bus_origin_id;
        int cache_idx = (system_bus.bus_addr >> 3) & 0x3F;
        
        // Align address to the start of the block (Critical Fix)
        // Assuming CACHE_BLOCK_SIZE is a power of 2 for convenience
        uint32_t mem_block_addr = system_bus.bus_addr & ~(CACHE_BLOCK_SIZE - 1); 
        uint32_t mem_physical_addr = mem_block_addr + system_bus.word_offset;

        if (system_bus.bus_cmd == BUS_RD || system_bus.bus_cmd == BUS_RDX) {
            // Read from Main Memory -> Bus -> cache_T
            
            uint32_t data = system_bus.system_memory[mem_physical_addr];
            system_bus.max_memory_accessed = (mem_physical_addr > system_bus.max_memory_accessed) ? mem_physical_addr : system_bus.max_memory_accessed;
            system_bus.bus_data = data;
            system_bus.cpu_cache[requester]->dsram[cache_idx].word[system_bus.word_offset] = data;
        
        } else if (system_bus.bus_cmd == BUS_FLUSH) {
            // Write from cache_T -> Bus -> Main Memory
            uint32_t data = system_bus.cpu_cache[requester]->dsram[cache_idx].word[system_bus.word_offset];
            system_bus.bus_data = data;
            system_bus.system_memory[mem_physical_addr] = data;
            system_bus.max_memory_accessed = (mem_physical_addr > system_bus.max_memory_accessed) ? mem_physical_addr : system_bus.max_memory_accessed;
        }

        system_bus.word_offset++;

        // Transaction Complete
        if (system_bus.word_offset >= CACHE_BLOCK_SIZE) {
            
            // Update MESI States
            if (system_bus.bus_cmd == BUS_RD) {
                mesi_state_T new_state = system_bus.bus_shared ? MESI_SHARED : MESI_EXCLUSIVE;
                system_bus.cpu_cache[requester]->tsram[cache_idx].mesi_state = new_state;
                system_bus.cpu_cache[requester]->tsram[cache_idx].tag = (system_bus.bus_addr >> 9) & 0xFFF;
            } 
            else if (system_bus.bus_cmd == BUS_RDX) {
                system_bus.cpu_cache[requester]->tsram[cache_idx].mesi_state = MESI_MODIFIED;
                system_bus.cpu_cache[requester]->tsram[cache_idx].tag = (system_bus.bus_addr >> 9) & 0xFFF;
            } 
            else if (system_bus.bus_cmd == BUS_FLUSH) {
                // Apply the correct post-FLUSH MESI state.
                // Eviction flush: INVALID
                // Snoop flush on BUS_RD: SHARED (M->S)
                // Snoop flush on BUS_RDX: INVALID (M->I)
                mesi_state_T post = MESI_INVALID;
                if (system_bus.flush_post_state_valid) {
                    post = system_bus.flush_post_state;
                }
                system_bus.cpu_cache[requester]->tsram[cache_idx].mesi_state = post;
                system_bus.flush_post_state_valid = false;
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

    // Arbitration
    int start = (system_bus.last_granted_device + 1) % CORE_COUNT;
    for (int i = 0; i < CORE_COUNT; i++) {
        int id = (start + i) % CORE_COUNT;
        bus_interface_T *bi = system_bus.bus_interface[id];

        if (bi->has_pending_request && !bi->request_done) {

            // Each new BusRd/BusRdX transaction starts with bus_shared = 0
            system_bus.bus_shared = false;

            // Replacement policy: direct-mapped.
            // If the requester is about to replace a MODIFIED line (different tag),
            // we must FLUSH it to main memory BEFORE granting the new request.
            uint32_t req_idx = (bi->request.bus_addr >> 3) & 0x3F;
            uint32_t req_tag = (bi->request.bus_addr >> 9) & 0xFFF;
            tsram_line_T *rline = &system_bus.cpu_cache[id]->tsram[req_idx];

            if (rline->mesi_state == MESI_MODIFIED && rline->tag != req_tag) {
                // Flush the old block (tag/index -> word address)
                // This is an eviction flush: the line is being replaced, so it becomes INVALID.
                uint32_t old_block_addr = ((rline->tag & 0xFFF) << 9) | (req_idx << 3);
                system_bus.flush_post_state = MESI_INVALID;
                system_bus.flush_post_state_valid = true;
                system_bus.busy = true;
                system_bus.bus_origin_id = id;
                system_bus.bus_cmd = BUS_FLUSH;
                system_bus.bus_addr = old_block_addr;
                system_bus.cooldown_timer = 0;
                system_bus.word_offset = 0;
                return;
            }
            
            // Snooping: other cores respond / invalidate
            uint32_t tag = req_tag;
            uint32_t idx = req_idx;

            for(int c = 0; c < CORE_COUNT; c++) {
                if (c == id) continue; // Don't snoop self
                
                tsram_line_T *line = &system_bus.cpu_cache[c]->tsram[idx];
                if (line->mesi_state != MESI_INVALID && line->tag == tag) {
                    system_bus.bus_shared = true; // Signal shared

                    // MESI fix: if another core issues BUS_RD while we hold the line in EXCLUSIVE,
                    // we must downgrade to SHARED.
                    if (bi->request.bus_cmd == BUS_RD && line->mesi_state == MESI_EXCLUSIVE) {
                        line->mesi_state = MESI_SHARED;
                    }

                    if (line->mesi_state == MESI_MODIFIED) {
                        // Found a MODIFIED line in another cache. Must FLUSH it so main memory is up-to-date.
                        // Post-FLUSH state depends on requester cmd:
                        // - BUS_RD  : M -> S
                        // - BUS_RDX : M -> I
                        system_bus.flush_post_state = (bi->request.bus_cmd == BUS_RD) ? MESI_SHARED : MESI_INVALID;
                        system_bus.flush_post_state_valid = true;
                        system_bus.busy = true;
                        system_bus.bus_origin_id = c; // The flusher
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
            system_bus.bus_origin_id = id;
            system_bus.bus_cmd = bi->request.bus_cmd;
            system_bus.bus_addr = bi->request.bus_addr;
            system_bus.cooldown_timer = BUS_DELAY;
            system_bus.word_offset = 0;
            return;
        }
    }
}