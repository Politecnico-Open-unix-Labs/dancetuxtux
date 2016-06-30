#ifndef __cplusplus
#    error "You need a C++ compiler to build this file, try using g++ instead of gcc"
#endif
#if __cplusplus < 201100L
#    error "You need at least C++11 to compile this file. Try compiling with --std=c++11"
#endif

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
#include <util/delay.h> // delay_us
#include <util/atomic.h>
