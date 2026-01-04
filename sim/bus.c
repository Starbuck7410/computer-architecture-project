#include "bus.h"
extern SystemBus system_bus;


bool send_bus_read_request(Core * core, uint32_t address, bool exclusive){
    // This calls the bus handler, not the system bus itself, we must rewrite this one
    bool was_bus_busy = system_bus.busy;

    if (!was_bus_busy) {
        if (exclusive) {
            system_bus.request.bus_cmd = BUS_RDX;
        }
        else{
            system_bus.request.bus_cmd = BUS_RD;
        }
        system_bus.request.bus_addr = address;
        system_bus.request.bus_orig_id = core->id;
        system_bus.cooldown_timer = BUS_DELAY;
        system_bus.busy = true;

        // clear previous data
        system_bus.request.bus_data = 0;
        system_bus.bus_shared = false;

    }

    return was_bus_busy; // This function returns whether the bus was busy or not
    // True = bus was busy, request failed
    // False = bus wasn't busy, request sent
}




// void invalidate_cache_block(int block_index, int safe_core_index){
//     if(safe_core_index > CORE_COUNT - 1){
//         DEBUG_PRINT("invalidate_cache_block() -> Too many cores!");
//         return;
//     }

//     for (int i = 0; i < CORE_COUNT; ++i) {
//         if (i == safe_core_index) continue;
//         system_bus.cpu_cache[i]->tsram[block_index].mesi_state = MESI_INVALID;
//     }
// }

void init_bus(Core * core[CORE_COUNT]){
    for(int i = 0; i < CORE_COUNT; i++){
        system_bus.cpu_cache[i] = &(core[i]->cache);
    }
    // Maybe add more things for the cache init here later
}

void bus_request_handler(){
    // This function happens every clock cycle;
    // If the bus is on cooldown, we must simply wait.
    // If not, we must send one word per clock cycle to the cache using the 
    // bus_data line, or in the case of a flush, we must flush the entire
    // cache to memory If the bus finished it's request, this function must 
    // clear the system_bus.busy flag.

    // In the case of a BUS_RD/X, before reading from memory into the cache
    // we must ensure the cache is clean in all the other cores, and if not, 
    // flush it directly to the system memory before actually initializing the 
    // BUS_RD/X request. This is done with ANOTHER 16 clock cycle timer,
    // so it should be handled carefully.
    
    if (system_bus.cooldown_timer > 0) {
        system_bus.cooldown_timer --;
        return;
    }

    if(system_bus.request.bus_cmd == BUS_RD){
        // read
    }
    // etc etc...
}


void bus_handler(){
    /*
    If request is being handled, keep handling.

    I not, check all 4 interfaces for a request and handle a request
    using round robin
    */
}