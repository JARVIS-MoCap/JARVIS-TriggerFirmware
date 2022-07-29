#ifndef _TRIGGERPINS_SELECTOR
#define _TRIGGERPINS_SELECTOR

#ifdef ARDUINO_TEENSY35
#include "triggerpins_teensy35.h"
#elif ARDUINO_TEENSY36
#include "triggerpins_teensy36.h"
#elif ARDUINO_TEENSY36
#include "triggerpins_teensy36.h"
#elif ARDUINO_TEENSY41
#include "triggerpins_teensy41.h"
#elif ARDUINO_AVR_NANO
#include "triggerpins_arduino_nano.h"
#elif ARDUINO_AVR_UNO
#include "triggerpins_arduino_uno.h"
#else
#error "Error: Unknown Board"
#endif

#endif