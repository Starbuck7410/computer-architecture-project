#include "bus.h"


void send_bus_read_request(Core * core, uint32_t address, bool exclusive){
    // This calls the bus handler, not the system bus itself
    //bool was_bus_busy = system_bus.busy;
    BusRequest read_request;
    read_request.bus_orig_id = core->id;
    read_request.bus_addr = address;
    read_request.bus_data = 0; // not sure if we want to clear previous data or not
    if (exclusive) {
        read_request.bus_cmd = BUS_RDX;
    }
    else {
        read_request.bus_cmd = BUS_RD;
    }
    bus_handler;
    // both cases calling bus_handler and it will handle the request
    // we dont care if bus busy or not here (i think)
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