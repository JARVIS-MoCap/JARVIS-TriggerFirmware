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
#define SERIAL_START_DELAY 2000

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
    if (digitalPinToPort(pin) != NOT_A_PIN)   \
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
  pinMode(OUT0_PIN, OUTPUT);
  pinMode(OUT1_PIN, OUTPUT);
  pinMode(OUT2_PIN, OUTPUT);
  pinMode(OUT3_PIN, OUTPUT);
  pinMode(OUT4_PIN, OUTPUT);
  pinMode(OUT5_PIN, OUTPUT);
  pinMode(OUT6_PIN, OUTPUT);
  pinMode(OUT7_PIN, OUTPUT);

  // Inputs
  // note: makeInputInterrupt does not generate an error for invalid pins
  makeInputInterrupt(IN0_PIN, INPUT_PULLUP, IN0_MASK);
  makeInputInterrupt(IN1_PIN, INPUT_PULLUP, IN1_MASK);
  makeInputInterrupt(IN2_PIN, INPUT_PULLUP, IN2_MASK);
  makeInputInterrupt(IN3_PIN, INPUT_PULLUP, IN3_MASK);
  makeInputInterrupt(IN4_PIN, INPUT_PULLUP, IN4_MASK);
  makeInputInterrupt(IN5_PIN, INPUT_PULLUP, IN5_MASK);
  makeInputInterrupt(IN6_PIN, INPUT_PULLUP, IN5_MASK);
  makeInputInterrupt(IN7_PIN, INPUT_PULLUP, IN5_MASK);

  // Communication
  packet_serial.setStream(&Serial);
  packet_serial.setPacketHandler(&handleCOM);

  serial_peer.setPacketSender(&sendCOM);
}

void loop()
{
  static uint32_t square_wave_timer_last_time_us = micros();
  static uint8_t wave_state = LOW;
  static uint32_t setup_start_timer_last_time_us = micros();
  static uint32_t setup_start_timer_delay_us = 0;
  static uint8_t setup_start_timer_enable = false;

  static uint8_t pulse_hz = 0;
  static uint32_t pulse_limit = 0;

  // Generate pulses
  uint32_t current_us = micros();
  if (pulse_hz && (uint32_t)(current_us - square_wave_timer_last_time_us) > SECOUND / (pulse_hz * 2))
  {
    square_wave_timer_last_time_us = current_us;
    pulse_count += wave_state;
    wave_state = !wave_state;

    // # Timer0 code:
    // note: digitalWrite does not generate an error for invalid pins
    digitalWrite(OUT0_PIN, wave_state);
    digitalWrite(OUT1_PIN, wave_state);
    digitalWrite(OUT2_PIN, wave_state);
    digitalWrite(OUT3_PIN, wave_state);
    digitalWrite(OUT4_PIN, wave_state);
    digitalWrite(OUT5_PIN, wave_state);
    digitalWrite(OUT6_PIN, wave_state);
    digitalWrite(OUT7_PIN, wave_state);

    if (pulse_limit && pulse_count >= pulse_limit)
    {
      pulse_hz = 0;
    }
  }

  // Send inputs on changes
  static uint8_t inputs_state_before = 0;
  if (inputs_state_before != inputs_state)
  {
    inputs_state_before = inputs_state;
    serial_peer.sendInputs(current_us, pulse_count, inputs_state);
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

  if (setup_start_timer_enable && (uint32_t)(current_us - setup_start_timer_last_time_us) > setup_start_timer_delay_us)
  {
    setup_start_timer_enable = false;

    // Setup
    if (setup_struct.flags & RESET_COUNTER)
    {
      pulse_count = 0;
      wave_state = LOW;
    }
    pulse_hz = setup_struct.pulse_hz;
    pulse_limit = setup_struct.pulse_limit;
    serial_peer.sendInputs(current_us, pulse_count, inputs_state); // Send states on start
  }
}
