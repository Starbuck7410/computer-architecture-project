#pragma once
#include "general-utils.h"


void fetch(core_T * core);
void decode(core_T * core);
void execute(core_T * core);
void memory(core_T * core);
void writeback(core_T * core);