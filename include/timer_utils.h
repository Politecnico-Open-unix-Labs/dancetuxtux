#ifndef TIMER_UTILS_H
#define TIMER_UTILS_H

#include <stdint.h> // uint8_t, uint16_t
#include <stddef.h> // NULL pointer

// THIS SOURCE WORKS ONLY WITH Atmega32u4, so check that
#include "cpu.h"
#ifndef __AVR_ATmega32U4__
#    error This library works only with Atmega32u4
#endif

// Timer id. Why not an enum? enums are int-types, this way uses uint8_t
#define TIMER_ID_0 ((uint8_t)0x00)
#define TIMER_ID_1 ((uint8_t)0x01)
#define TIMER_ID_3 ((uint8_t)0x03)
// Comparator registers
#define TIMER_COMP_A ((uint8_t)0x10)
#define TIMER_COMP_B ((uint8_t)0x11)
#define TIMER_COMP_C ((uint8_t)0x12)

#define TIMER_USE_DEFAULT ((uint8_t)0xff) // -1

// Timer input soruces
#define TIMER_SOURCE_NONE      ((uint8_t)0x20)
#define TIMER_SOURCE_CLK       ((uint8_t)0x21)
#define TIMER_SOURCE_CLK_8     ((uint8_t)0x22)
#define TIMER_SOURCE_CLK_64    ((uint8_t)0x23)
#define TIMER_SOURCE_CLK_256   ((uint8_t)0x24)
#define TIMER_SOURCE_CLK_1024  ((uint8_t)0x25)
#define TIMER_SOURCE_EXT_FALL  ((uint8_t)0x26)
#define TIMER_SOURCE_EXT_RISE  ((uint8_t)0x27)

// Timer modes
#define TIMER_MODE_NORMAL          ((uint8_t)0x30)
#define TIMER_MODE_CTC             ((uint8_t)0x31)
#define TIMER_MODE_PWM             ((uint8_t)0x32)
#define TIMER_MODE_PWM_OCRA        ((uint8_t)0x33)
#define TIMER_MODE_FAST_PWM        ((uint8_t)0x34)
#define TIMER_MODE_FAST_PWM_OCRA   ((uint8_t)0x35)

// Output modes
#define OUT_MODE_NORMAL_A             ((uint8_t)0x40)
#define OUT_MODE_TOGGLE_ON_COMPARE_A  ((uint8_t)0x41)
#define OUT_MODE_CLEAR_ON_COMPARE_A   ((uint8_t)0x42)
#define OUT_MODE_SET_ON_COMPARE_A     ((uint8_t)0x43)
#define OUT_MODE_NORMAL_B             ((uint8_t)0x50)
#define OUT_MODE_TOGGLE_ON_COMPARE_B  ((uint8_t)0x51)
#define OUT_MODE_CLEAR_ON_COMPARE_B   ((uint8_t)0x52)
#define OUT_MODE_SET_ON_COMPARE_B     ((uint8_t)0x53)

// Timer interrupt enable flags
#define TIMER_INTERRUPT_MODE_OCIA  ((uint8_t)0x74)
#define TIMER_INTERRUPT_MODE_OCIB  ((uint8_t)0x72)
#define TIMER_INTERRUPT_MODE_OCIC  ((uint8_t)0x71)
#define TIMER_INTERRUPT_MODE_TOI   ((uint8_t)0x70)

// A set of useful timer functions

// ===== TIMER SETUP FUNCTIONS ======
void timer_init(uint8_t timer_id, uint8_t source, uint8_t mode, uint8_t out_mode_a, uint8_t out_mode_b);
void timer_init_interrupt(uint8_t timer_id, uint8_t interrupt_mode);

// ===== TIMER CONTROLS =====
void timer_start(uint8_t timer_id); // immediately starts the timer (no computations if timer_id is compile-time constant)
void timer_stop(uint8_t timer_id); // immediately stops the timer (no computations if timer_id is compile-time constant)
void timer_enable(uint8_t timer_id); // enables the timer, this way it can start
void timer_disable(uint8_t timer_id); // disables the timer, this way saves energy

// get/set timer counter, i.e. number of tick counted by the timer
// Each of this function returns the count, then resets it to the value pointed by new cont, if not null.
// Call with null argument to not set the timer.
uint8_t  timer0_count(const uint8_t  * new_count);
uint16_t timer1_count(const uint16_t * new_count);
uint16_t timer3_count(const uint16_t * new_count);

// get/set compare values, comparation is done automatically. Same interface as above
uint8_t  timer0_compare(uint8_t comp_id, const uint8_t  * new_val);
uint16_t timer1_compare(uint8_t comp_id, const uint16_t * new_val);
uint16_t timer3_compare(uint8_t comp_id, const uint16_t * new_val);

#endif
