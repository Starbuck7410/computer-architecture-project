#include "bus.h"

extern SystemBus system_bus;

bool send_bus_read_request(Core * core, uint32_t address, bool exclusive){
    bool was_bus_busy = system_bus.busy;

    if (!was_bus_busy) {
        if (exclusive) {
            system_bus.bus_cmd = BUS_RDX;
        }
        else{
            system_bus.bus_cmd = BUS_RD;
        }
        system_bus.bus_addr = address;
        system_bus.bus_orig_id = core->id;
        system_bus.busy = true;

        // clear previous data (?)
        system_bus.bus_data = 0;
        system_bus.bus_shared = false;

    }

    return was_bus_busy; // This function returns whether the bus was busy or not
    // True = bus was busy, request failed
    // False = bus wasn't busy, request sent
}




void invalidate_cache_block(Core * core[CORE_COUNT], int block_index, int safe_core_index){
    if(safe_core_index > CORE_COUNT - 1){
        // We do not have THAT many cores
        return;
    }

    // Invalidate the rest of the core's cache blocks whose index is block_index
}