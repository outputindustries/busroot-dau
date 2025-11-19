#include "SDRAM.h"
#include "data_frame.h"
#include <Watchdog.h>
#include "Arduino.h"

unsigned long sendInterval = 5000;
unsigned long previousMillis = 0;

unsigned long debounceDelay = 50;

unsigned int counter_BTN_USER = 0;
unsigned int currentState_BTN_USER = 0;
unsigned int lastState_BTN_USER = 0;
unsigned long lastDebounceTime_BTN_USER = 0;

unsigned int pins[] = {A0, A1, A2, A3, A4, A5, A6, A7};
unsigned int counters[] = {0, 0, 0, 0, 0, 0, 0, 0};
unsigned int currentStates[] = {0, 0, 0, 0, 0, 0, 0, 0};
unsigned int lastStates[] = {0, 0, 0, 0, 0, 0, 0, 0};
unsigned long lastDebounceTimes[] = {0, 0, 0, 0, 0, 0, 0, 0};
int analogs[] = {0, 0};

void setup()
{
  mbed::Watchdog::get_instance().start();

  SDRAM.begin(SDRAM_START_ADDRESS_4);

  // Initialize circular buffer
  data_frame_buffer_sdram->head = 0;
  data_frame_buffer_sdram->tail = 0;
  data_frame_buffer_sdram->count = 0;

  pinMode(LEDB, OUTPUT);

  for (int i = 0; i < 8; i++)
  {
    pinMode(pins[i], INPUT);
  }

  analogReadResolution(12);

  // This is to allow M7 to have started. Otherwise the buffer is filled with multiple messages.
  // Reduced from 20s to 10s, with watchdog kicks
  for (int i = 0; i < 10; i++)
  {
    mbed::Watchdog::get_instance().kick();
    delay(1000);
  }
}

void readInputs()
{
  unsigned long currentMillis = millis();

  // BTN_USER
  pin_size_t state_BTN_USER = 1 - digitalRead(BTN_USER);

  if (state_BTN_USER == 1)
  {
    digitalWrite(LEDB, HIGH);
  }

  if (state_BTN_USER != lastState_BTN_USER)
    lastDebounceTime_BTN_USER = currentMillis;
  if (currentMillis - lastDebounceTime_BTN_USER > debounceDelay && currentState_BTN_USER != state_BTN_USER)
  {
    if (currentState_BTN_USER > state_BTN_USER)
    {
      counter_BTN_USER++;
      digitalWrite(LEDB, LOW);
    }
    currentState_BTN_USER = state_BTN_USER;
  }
  lastState_BTN_USER = state_BTN_USER;

  // Pins
  for (int i = 0; i < 6; i++)
  {
    pin_size_t state = digitalRead(pins[i]); // Read the pin.
    if (state != lastStates[i])
      lastDebounceTimes[i] = currentMillis; // If state has changed since last read, reset debounce time.
    if (currentMillis - lastDebounceTimes[i] > debounceDelay && currentStates[i] != state)
    { // If time since last change (debounce time) exceeds the set debounce delay, proceed...
      if (currentStates[i] > state)
      { // Only count falling edges.
        counters[i]++;
      }
      currentStates[i] = state;
    }
    lastStates[i] = state;
  }

  for (int i = 0; i < 2; i++)
  {
    int state = analogRead(pins[i + 6]);

    analogs[i] = state;
  }
}

void loop()
{
  unsigned long currentMillis = millis();

  mbed::Watchdog::get_instance().kick();

  readInputs();

  if (currentMillis - previousMillis >= sendInterval)
  {
    // Invalidate cache before reading buffer state
    invalidateSharedMemoryCache();

    // Check if buffer is full
    if (data_frame_buffer_sdram->count >= DATA_FRAME_BUFFER_SIZE)
    {
      // Buffer is full - drop the oldest frame by moving tail forward
      data_frame_buffer_sdram->tail = (data_frame_buffer_sdram->tail + 1) % DATA_FRAME_BUFFER_SIZE;
      data_frame_buffer_sdram->count--;
    }

    // Get the current head position
    unsigned int writeIndex = data_frame_buffer_sdram->head;

    // Write data to buffer at head position
    data_frame_buffer_sdram->frames[writeIndex].userButtonCount = counter_BTN_USER;
    data_frame_buffer_sdram->frames[writeIndex].input1Count = counters[0];
    data_frame_buffer_sdram->frames[writeIndex].input2Count = counters[1];
    data_frame_buffer_sdram->frames[writeIndex].input3Count = counters[2];
    data_frame_buffer_sdram->frames[writeIndex].input4Count = counters[3];
    data_frame_buffer_sdram->frames[writeIndex].input5Count = counters[4];
    data_frame_buffer_sdram->frames[writeIndex].input6Count = counters[5];
    data_frame_buffer_sdram->frames[writeIndex].userButtonState = lastState_BTN_USER;
    data_frame_buffer_sdram->frames[writeIndex].input1State = lastStates[0];
    data_frame_buffer_sdram->frames[writeIndex].input2State = lastStates[1];
    data_frame_buffer_sdram->frames[writeIndex].input3State = lastStates[2];
    data_frame_buffer_sdram->frames[writeIndex].input4State = lastStates[3];
    data_frame_buffer_sdram->frames[writeIndex].input5State = lastStates[4];
    data_frame_buffer_sdram->frames[writeIndex].input6State = lastStates[5];
    data_frame_buffer_sdram->frames[writeIndex].input7Analog = analogs[0];
    data_frame_buffer_sdram->frames[writeIndex].input8Analog = analogs[1];

    // Move head forward and increment count
    data_frame_buffer_sdram->head = (data_frame_buffer_sdram->head + 1) % DATA_FRAME_BUFFER_SIZE;
    data_frame_buffer_sdram->count++;

    // Clean cache to make writes visible to M7
    cleanSharedMemoryCache();

    // Reset counters immediately - data is already safely in SDRAM
    // New pulses will accumulate in fresh counters for next transmission
    counter_BTN_USER = 0;
    for (int i = 0; i < 8; i++)
    {
      counters[i] = 0;
    }

    previousMillis = currentMillis;
  }
}
