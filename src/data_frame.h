#ifndef DATA_FRAME_H
#define DATA_FRAME_H

#include <Arduino.h>

struct DATA_FRAME_SEND
{
  unsigned int userButtonCount;
  unsigned int input1Count;
  unsigned int input2Count;
  unsigned int input3Count;
  unsigned int input4Count;
  unsigned int input5Count;
  unsigned int input6Count;
  unsigned int userButtonState;
  unsigned int input1State;
  unsigned int input2State;
  unsigned int input3State;
  unsigned int input4State;
  unsigned int input5State;
  unsigned int input6State;
  unsigned int input7Analog;
  unsigned int input8Analog;
};

extern const uint32_t SDRAM_START_ADDRESS_4; // USING THE AHB SRAM4 DOMAIN SPACE
extern const uint32_t SDRAM_ALLOCATION_PROTECTED_BUFFER_SIZE; // for my own use, malloc will allocate after the size of this variable, increase if we need more than 10KB for core to core variables

// Pointers to memory locations
extern volatile byte *send_frame_ready_flag;
extern volatile DATA_FRAME_SEND *data_frame_send_sdram;

// Cache coherency helpers for dual-core communication
inline void cleanSharedMemoryCache()
{
#ifdef CORE_CM7
  // M7 has D-Cache - clean it to ensure writes are visible to M4
  SCB_CleanDCache_by_Addr((uint32_t *)SDRAM_START_ADDRESS_4, sizeof(DATA_FRAME_SEND) + sizeof(byte) * 2);
#endif
  // Memory barriers ensure proper ordering on both cores
  __DSB(); // Data Synchronization Barrier
  __ISB(); // Instruction Synchronization Barrier
}

inline void invalidateSharedMemoryCache()
{
#ifdef CORE_CM7
  // M7 has D-Cache - invalidate it to ensure we read fresh data from SDRAM
  SCB_InvalidateDCache_by_Addr((uint32_t *)SDRAM_START_ADDRESS_4, sizeof(DATA_FRAME_SEND) + sizeof(byte) * 2);
#endif
  // Memory barriers ensure proper ordering on both cores
  __DSB(); // Data Synchronization Barrier
  __ISB(); // Instruction Synchronization Barrier
}

#endif // DATA_FRAME_H