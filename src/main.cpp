/*******************************************************************************
 * File:        main.cpp
 * Created:     18. July 2022
 * Author:      Timo Hueser
 * Contact:     timo.hueser@gmail.com
 * Author:      Louis Frank
 * Contact:     singulosta@gmail.com
 * Copyright:   2021 Timo Hueser & Louis Frank
 * License:     LGPL v3.0
 ******************************************************************************/

/*
  # Warnings about certain microcontrollers
  External Trigger Code to be used with AcquisitionTest.py and Adafruit Feather M0 board with trigger input connected to Pin 6
  Pin 5 can be connected to LED to see if trigger pulse is active and o check synchronisation
  Camera GPIO operateds on 3.3V so do not use this with Arduino UNO (5V Logic might damage Cameras)


  Correction from "TECHNICAL REFERENCE - FLIRCHAMELEON®3 - USB3 Vision" chapter 6.7 "GPIO Electrical Characteristics" (page 35):
  ###################################
  Table 6.2: Operating Range
  | Description                         | Minimum | Maximum |
  |-------------------------------------|---------|---------|
  | Non-opto-isolated Voltage           | 0 V     | 24 V    |
  | Opto-isolated Input Voltage         | 0 V     | 30 V    |
  | Opto-isolated Output Voltage        | 0 V     | 24 V    |
  | Non-opto-isolated Sinking Current   |         | 25 mA   |
  | Opto-isolated Output Current        |         | 25 mA   |
  | 3.3 V Output Current                |         | 200 mA  |


  Table 6.3: Absolute Maximum Ratings
  | Description                         | Minimum | Maximum |
  | Non-opto-isolated Voltage           | -24 V   | 42 V    |
  | Opto-isolated Input Voltage         | -70 V   | 40 V    |
  | Opto-isolated Output Voltage        | -24 V   | 24 V    |
  ###################################
  Source: https://flir.app.boxcn.net/s/xobncd08w5w3oc72tmvs33dnttqfpjw9/file/416905133542
*/

#include <Arduino.h>
#include "serial_peer.h"
#include <PacketSerial.h>
#include "triggerpins_selector.h"

#define BAUDRATE 115200
#define SERIAL_START_DELAY 100

// Delays
#define SECOUND 1e6

enum INPUT_MASK
{
  IN0_MASK = 1 << 0,
  IN1_MASK = 1 << 1,
  IN2_MASK = 1 << 2,
  IN3_MASK = 1 << 3,
  IN4_MASK = 1 << 4,
  IN5_MASK = 1 << 5,
  IN6_MASK = 1 << 6,
  IN7_MASK = 1 << 7,
};

uint32_t pulse_count = 0;

// Input handler
#define makeInputInterrupt(pin, pullup, mask) \
  {                                           \
    if (digitalPinToPort(pin) != 0)           \
    {                                         \
      pinMode(pin, pullup);                   \
      attachInterrupt(                        \
          digitalPinToInterrupt(pin),         \
          [] { handleInput(pin, mask); },     \
          CHANGE);                            \
      handleInput(pin, mask);                 \
    }                                         \
  }

uint8_t inputs_state = 0;
uint32_t inputs_changetime_us = 0;
void handleInput(uint16_t pin, INPUT_MASK mask)
{
  if (digitalRead(pin))
  {
    inputs_state |= mask; // setting bit
  }
  else
  {
    inputs_state &= ~(mask); // clearing bit
  }
  inputs_changetime_us = micros();
}

// Communication
PacketSerial packet_serial;
SerialPeer serial_peer;
SetupStruct setup_struct;

// Input handler
void sendCOM(const uint8_t *send_buffer, size_t size)
{
  packet_serial.send(send_buffer, size);
}

// Serial echo
#define DEBUG_COM false
// DEBUG_COM needs 323 bytes of RAM and 204 bytes of Flash
#if DEBUG_COM
#define MAX_MAIN_BUFFER_SIZE 0xFF
uint8_t echo_buffer_len = 0;
uint8_t echo_buffer[MAX_MAIN_BUFFER_SIZE];
#endif

void handleCOM(const uint8_t *incoming, size_t size)
{
#if DEBUG_COM
  echo_buffer_len = 0;

  // Copy buffer
  memcpy(echo_buffer, incoming, size);
  echo_buffer_len = size;

  sendCOM(echo_buffer, echo_buffer_len);

#endif

  // Call handler
  if (size > 1) // ignore one byte messages
    serial_peer.handleMessage((uint8_t *)incoming, size);
}

void setup()
{
  // Serial
  Serial.begin(BAUDRATE);    // USB is always 12 or 480 Mbit/sec
  delay(SERIAL_START_DELAY); // Time for USB Terminal to start

  // Outputs
  // note: pinmode does not generate an error for invalid pins
  pinMode(OUT00_PIN, OUTPUT);
  pinMode(OUT01_PIN, OUTPUT);
  pinMode(OUT02_PIN, OUTPUT);
  pinMode(OUT03_PIN, OUTPUT);
  pinMode(OUT04_PIN, OUTPUT);
  pinMode(OUT05_PIN, OUTPUT);
  pinMode(OUT06_PIN, OUTPUT);
  pinMode(OUT07_PIN, OUTPUT);
  pinMode(OUT08_PIN, OUTPUT);
  pinMode(OUT09_PIN, OUTPUT);
  pinMode(OUT10_PIN, OUTPUT);
  pinMode(OUT11_PIN, OUTPUT);
  pinMode(OUT12_PIN, OUTPUT);
  pinMode(OUT13_PIN, OUTPUT);
  pinMode(OUT14_PIN, OUTPUT);
  pinMode(OUT15_PIN, OUTPUT);

  // Inputs
  // note: makeInputInterrupt does not generate an error for invalid pins
  makeInputInterrupt(IN00_PIN, INPUT_PULLUP, IN0_MASK);
  makeInputInterrupt(IN01_PIN, INPUT_PULLUP, IN1_MASK);
  makeInputInterrupt(IN02_PIN, INPUT_PULLUP, IN2_MASK);
  makeInputInterrupt(IN03_PIN, INPUT_PULLUP, IN3_MASK);
  makeInputInterrupt(IN04_PIN, INPUT_PULLUP, IN4_MASK);
  makeInputInterrupt(IN05_PIN, INPUT_PULLUP, IN5_MASK);
  makeInputInterrupt(IN06_PIN, INPUT_PULLUP, IN6_MASK);
  makeInputInterrupt(IN07_PIN, INPUT_PULLUP, IN7_MASK);

  // Communication
  packet_serial.setStream(&Serial);
  packet_serial.setPacketHandler(&handleCOM);

  serial_peer.setPacketSender(&sendCOM);
}

void loop()
{
  static uint32_t square_wave_timer_last_time_us = micros();
  static uint8_t wave_state = LOW;
  static uint8_t sync_rising_edge = true;
  static uint32_t setup_start_timer_last_time_us = micros();
  static uint32_t setup_start_timer_delay_us = 0;
  static uint8_t setup_start_timer_enable = false;

  static uint8_t pulse_hz = 0;
  static uint8_t req_pulse_hz = 0;
  static uint32_t pulse_limit = 0;

  // Generate pulses
  uint32_t current_us = micros();
  if (
      pulse_hz &&
      (current_us - square_wave_timer_last_time_us) > SECOUND / (pulse_hz * 2))
  {
    square_wave_timer_last_time_us = current_us;
    pulse_count += wave_state; // increment on falling edge
    wave_state = !wave_state;

    // # square_wave_timer code:
    // note: digitalWrite does not generate an error for invalid pins
    digitalWrite(OUT00_PIN, wave_state);
    digitalWrite(OUT01_PIN, wave_state);
    digitalWrite(OUT02_PIN, wave_state);
    digitalWrite(OUT03_PIN, wave_state);
    digitalWrite(OUT04_PIN, wave_state);
    digitalWrite(OUT05_PIN, wave_state);
    digitalWrite(OUT06_PIN, wave_state);
    digitalWrite(OUT07_PIN, wave_state);
    digitalWrite(OUT08_PIN, wave_state);
    digitalWrite(OUT09_PIN, wave_state);
    digitalWrite(OUT10_PIN, wave_state);
    digitalWrite(OUT11_PIN, wave_state);
    digitalWrite(OUT12_PIN, wave_state);
    digitalWrite(OUT13_PIN, wave_state);
    digitalWrite(OUT14_PIN, wave_state);
    digitalWrite(OUT15_PIN, wave_state);

    if (pulse_count <= 1 && wave_state ^ !sync_rising_edge)
    {
      // Send first input on rising edge/falling edge (bool sync_rising_edge) to enable triggerdata/camera-metadata synchronisation
      inputs_changetime_us = current_us;
    }

    if (pulse_limit && pulse_count >= pulse_limit)
    {
      req_pulse_hz = 0;
    }

    // Stop pulsing only after falling edge
    if (wave_state == LOW)
    {
      pulse_hz = req_pulse_hz;
    }
  }

  // Send inputs on changes
  static uint32_t inputs_changetime_us_before = 0;
  if (inputs_changetime_us_before != inputs_changetime_us)
  {
    noInterrupts();
    inputs_changetime_us_before = inputs_changetime_us;
    uint8_t temp_inputs_state = inputs_state;
    interrupts();

    serial_peer.sendInputs(inputs_changetime_us_before, pulse_count, temp_inputs_state);
  }

  // Communication
  packet_serial.update();
  // Check for a receive buffer overflow.
  if (packet_serial.overflow())
  {
    // Send an alert via a pin (e.g. make an overflow LED) or return a
    // user-defined packet to the sender.
    //
    // Ultimately you may need to just increase your recieve buffer via the
    // template parameters.
    serial_peer.sendError((uint8_t *)"PacketSerial overflow error", 28);
  }

  // Handle setup
  if (serial_peer.getSetup(&setup_struct))
  {
#if DEBUG_COM
    echo_buffer_len = snprintf((char *)echo_buffer, MAX_MAIN_BUFFER_SIZE, "getSetup, pulse_hz: %d, pulse_limit: %lu, setup_struct.delay_us: %lu", setup_struct.pulse_hz, setup_struct.pulse_limit, setup_struct.delay_us);
    serial_peer.sendTxt(echo_buffer, echo_buffer_len);
#endif

    setup_start_timer_last_time_us = current_us;
    setup_start_timer_delay_us = setup_struct.delay_us;
    setup_start_timer_enable = true;
  }

  if (
      setup_start_timer_enable &&
      (uint32_t)(current_us - setup_start_timer_last_time_us) > setup_start_timer_delay_us)
  {
    setup_start_timer_enable = false;
    // # setup_start_timer code:

    // Setup
    sync_rising_edge = setup_struct.flags & SYNC_RISING_EDGE;

    req_pulse_hz = setup_struct.pulse_hz;
    pulse_limit = setup_struct.pulse_limit;

    if (setup_struct.flags & RESET_COUNTER)
    {
      pulse_count = 0;
    }
  }
  if (pulse_hz == 0)
  {
    pulse_hz = req_pulse_hz;
  }
}
