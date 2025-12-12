#include "general_utils.h"
#include "core.h"

int main(){
    // Please add the follwing files to the visual studio thingy 
    // (Right click on soruce -> add existing source -> add the file)
    // core.h, core.c, pipeline.h

    Core core_0, core_1, core_2, core_3;
    FILE * imem_0_file = fopen("imem", "rb");

    init_core(&core_0, 0, imem_0_file); // Implement this in core.c

    DEBUG_PRINT("Hello, debug!\n");
    return 0;

}

