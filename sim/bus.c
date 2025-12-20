#include "bus.h"

extern SystemBus system_bus;

bool send_bus_read_request(Core * core, uint32_t address, bool exclusive){
    bool was_bus_busy = system_bus.busy;

    // This function will send a request to the bus if it is ready to accept one
    // If it is not, this function won't do anything besides return that the bus was busy
    
    /*
    system_bus.bus_cmd = BUS_RDX;
    system_bus.bus_addr = address;
    system_bus.bus_orig_id = core_id;
    */

    return was_bus_busy; // This function returns whether the bus was busy or not
}




void invalidate_cache_block(Core * core[CORE_COUNT], int block_index, int safe_core_index){
    if(safe_core_index > CORE_COUNT - 1){
        // We do not have THAT many cores
        return;
    }

    // Invalidate the rest of the core's cache blocks whose index is block_index
}