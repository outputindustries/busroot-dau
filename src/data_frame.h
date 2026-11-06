#ifndef DATA_FRAME_H
#define DATA_FRAME_H

#include <Arduino.h>

// How often the M4 core writes a frame to the buffer (ms).
#define SEND_INTERVAL_MS 5000

// Trace mode (commissioning only): build with -DTRACE_INTERVAL_MS=<ms> in the shared
// [env] section of platformio.ini, so both cores agree on the frame layout. The M4
// samples the raw input states every TRACE_INTERVAL_MS and each frame carries them.
#ifdef TRACE_INTERVAL_MS
#define TRACE_MAX_SAMPLES (SEND_INTERVAL_MS / TRACE_INTERVAL_MS + 1) // +1 absorbs timing jitter
static_assert(TRACE_INTERVAL_MS >= 20 && TRACE_MAX_SAMPLES <= 64, "TRACE_INTERVAL_MS out of range");
#define TRACE_BIT_BTN_USER 6 // Bits 0-5 = inputs 1-6, bit 6 = user button
#endif

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
#ifdef TRACE_INTERVAL_MS
  unsigned int traceCount;          // Number of valid samples in trace[]
  uint8_t trace[TRACE_MAX_SAMPLES]; // One packed sample per TRACE_INTERVAL_MS (see TRACE_BIT_BTN_USER)
#endif
};

// Circular buffer configuration
// Max frames to fill ~64KB AHB SRAM4: (65536 - 12 bytes overhead) / 64 bytes per frame ≈ 1024
// Using 1000 for safety margin = ~64KB total = 83 minutes @ 5s intervals.
// Trace frames are larger (~96 bytes at 200 ms), so fewer fit: 650 = ~54 minutes.
#ifdef TRACE_INTERVAL_MS
#define DATA_FRAME_BUFFER_SIZE 650
#else
#define DATA_FRAME_BUFFER_SIZE 1000  // Number of frames that can be buffered
#endif

struct DATA_FRAME_BUFFER
{
  volatile unsigned int head;  // Index where M4 writes next frame
  volatile unsigned int tail;  // Index where M7 reads next frame
  volatile unsigned int count; // Number of frames in buffer
  DATA_FRAME_SEND frames[DATA_FRAME_BUFFER_SIZE];
};

static_assert(sizeof(DATA_FRAME_BUFFER) <= 65536, "Frame buffer exceeds the 64KB AHB SRAM4");

extern const uint32_t SDRAM_START_ADDRESS_4; // USING THE AHB SRAM4 DOMAIN SPACE
extern const uint32_t SDRAM_ALLOCATION_PROTECTED_BUFFER_SIZE; // for my own use, malloc will allocate after the size of this variable, increase if we need more than 10KB for core to core variables

// Pointer to circular buffer in SDRAM
extern volatile DATA_FRAME_BUFFER *data_frame_buffer_sdram;

// Cache coherency helpers for dual-core communication
inline void cleanSharedMemoryCache()
{
#ifdef CORE_CM7
  // M7 has D-Cache - clean it to ensure writes are visible to M4
  SCB_CleanDCache_by_Addr((uint32_t *)SDRAM_START_ADDRESS_4, sizeof(DATA_FRAME_BUFFER));
#endif
  // Memory barriers ensure proper ordering on both cores
  __DSB(); // Data Synchronization Barrier
  __ISB(); // Instruction Synchronization Barrier
}

inline void invalidateSharedMemoryCache()
{
#ifdef CORE_CM7
  // M7 has D-Cache - invalidate it to ensure we read fresh data from SDRAM
  SCB_InvalidateDCache_by_Addr((uint32_t *)SDRAM_START_ADDRESS_4, sizeof(DATA_FRAME_BUFFER));
#endif
  // Memory barriers ensure proper ordering on both cores
  __DSB(); // Data Synchronization Barrier
  __ISB(); // Instruction Synchronization Barrier
}

// True if head/tail/count are in range. SRAM4 holds random values after a
// power-on boot, and an out-of-range index reads or writes outside the buffer.
// Call after invalidateSharedMemoryCache().
inline bool isDataFrameBufferValid()
{
  return data_frame_buffer_sdram->head < DATA_FRAME_BUFFER_SIZE &&
         data_frame_buffer_sdram->tail < DATA_FRAME_BUFFER_SIZE &&
         data_frame_buffer_sdram->count <= DATA_FRAME_BUFFER_SIZE;
}

// Empty the buffer and make the header visible to the other core.
inline void resetDataFrameBuffer()
{
  data_frame_buffer_sdram->head = 0;
  data_frame_buffer_sdram->tail = 0;
  data_frame_buffer_sdram->count = 0;
  cleanSharedMemoryCache();
}

#endif // DATA_FRAME_H