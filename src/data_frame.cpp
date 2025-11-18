#include "data_frame.h"

const uint32_t SDRAM_START_ADDRESS_4 = ((uint32_t)0x38000000); // USING THE AHB SRAM4 DOMAIN SPACE
const uint32_t SDRAM_ALLOCATION_PROTECTED_BUFFER_SIZE = 10000;  

// Pointers to memory locations
volatile byte *send_frame_ready_flag = (volatile byte *)SDRAM_START_ADDRESS_4;
volatile DATA_FRAME_SEND *data_frame_send_sdram = (volatile DATA_FRAME_SEND *)(SDRAM_START_ADDRESS_4 + (2 * sizeof(byte)));