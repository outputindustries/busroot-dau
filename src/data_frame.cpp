#include "data_frame.h"

const uint32_t SDRAM_START_ADDRESS_4 = ((uint32_t)0x38000000); // USING THE AHB SRAM4 DOMAIN SPACE
const uint32_t SDRAM_ALLOCATION_PROTECTED_BUFFER_SIZE = sizeof(DATA_FRAME_BUFFER) + 1000; // Buffer size + extra margin

// Pointer to circular buffer in SDRAM
volatile DATA_FRAME_BUFFER *data_frame_buffer_sdram = (volatile DATA_FRAME_BUFFER *)SDRAM_START_ADDRESS_4;