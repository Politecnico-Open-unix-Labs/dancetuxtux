// This project needs an Atmega32u4, other processor are not supported
#ifndef __AVR_ATmega32U4__
#    define __AVR_ATmega32U4__  // microcontroller
#endif
#ifndef F_CPU
#    define F_CPU 16000000      // frequency of the cpu
#endif

#include <avr/io.h> // POTD, PORTB, PORTC & friends
#include <avr/cpufunc.h> // memory barriers
#include <avr/sfr_defs.h> // _BV macro
#ifndef _BV // if not defined, define one itself
#    define _BV(bit) (1 << (bit))
#endif
#include <avr/power.h> // Powersaver functions
#include <avr/sleep.h>
#include <util/delay.h> // delay_us
#include <util/delay_basic.h>
#include <util/atomic.h>
