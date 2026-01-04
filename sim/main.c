#include "general_utils.h"
#include "file_io.h"
#include "pipeline.h"
int main(int argc, char ** argv){
    // Please add the follwing files to the visual studio thingy 
    // (Right click on soruce -> add existing source -> add the file)
    // core.h, core.c, pipeline.h

    // SimFiles sim_files;
    // Core cores[CORE_COUNT];
    // get_arguments(argc, argv, &sim_files);
    Core * cores[CORE_COUNT];

    while(true){
        for(int i = 0; i < CORE_COUNT; i++){
            fetch_stage(cores[i]);
            decode_stage(cores[i]);
            execute_stage(cores[i]);
            memory_stage(cores[i]);
            writeback_stage(cores[i]);
        }
        bus_handler();
        // This is one clock cycle
    }

    DEBUG_PRINT("Hello, debug!\n");
    return 0;

}

