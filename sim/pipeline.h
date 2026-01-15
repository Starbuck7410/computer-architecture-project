#pragma once
#include "general_utils.h"


void fetch_stage(Core * core);
void decode_stage(Core * core);
void execute_stage(Core * core);
void memory_stage(Core * core);
void writeback_stage(Core* core);