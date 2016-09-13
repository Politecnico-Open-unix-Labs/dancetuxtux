#ifndef TIMER_UTILS_H
#define TIMER_UTILS_H

#include <stdint.h> // uint8_t, uint16_t
#include <stddef.h> // NULL pointer

// THIS SOURCE WORKS ONLY WITH Atmega32u4, so check that
#include "cpu.h"
#ifndef __AVR_ATmega32U4__
#    error This library works only with Atmega32u4
#endif

// This code must be different from
// any following timer configuration constant
#define TIMER_USE_DEFAULT ((uint8_t)0xff) // -1
#define TIMER_RUNNING TIMER_USE_DEFAULT

// Timer id. Why not an enum? enums are int-types, this way uses uint8_t
// binary format: 0 0 0 0 x x x x
// where x-es are the timer id
#define TIMER_ID_0 ((uint8_t)0x00)
#define TIMER_ID_1 ((uint8_t)0x01)
#define TIMER_ID_3 ((uint8_t)0x03)

// Comparator registers
// binary format: 0 0 0 1 x x x x
// where x-es are the coparator id
#define TIMER_COMP_A   ((uint8_t)0x10) // For timers 0, 1, 3
#define TIMER_COMP_B   ((uint8_t)0x11) // Only for timers 1, 3
#define TIMER_COMP_C   ((uint8_t)0x12) // Only for timers 1, 3
#define TIMER_COMP_ICR ((uint8_t)0x13) // Only for timers 1, 3

// Timer input soruces
// binary format: 0 0 1 0 x x x x
// where x-es are the CS (Clock Source) id
#define TIMER_SOURCE_NONE      ((uint8_t)0x20)
#define TIMER_SOURCE_CLK       ((uint8_t)0x21)
#define TIMER_SOURCE_CLK_8     ((uint8_t)0x22)
#define TIMER_SOURCE_CLK_64    ((uint8_t)0x23)
#define TIMER_SOURCE_CLK_256   ((uint8_t)0x24)
#define TIMER_SOURCE_CLK_1024  ((uint8_t)0x25)
#define TIMER_SOURCE_EXT_FALL  ((uint8_t)0x26)
#define TIMER_SOURCE_EXT_RISE  ((uint8_t)0x27)

// Timer modes
// binary format 0 0 1 1 x x x x
// Where x-es are the mode id
#define TIMER_MODE_NORMAL         ((uint8_t)0x30)
#define TIMER_MODE_PWM_8BIT       ((uint8_t)0x31)
#define TIMER_MODE_PWM_9BIT       ((uint8_t)0x32)
#define TIMER_MODE_PWM_10BIT      ((uint8_t)0x33)
#define TIMER_MODE_PWM_ICR        ((uint8_t)0x34)
#define TIMER_MODE_PWM_OCR        ((uint8_t)0x35)
#define TIMER_MODE_FREQ_PWM_ICR   ((uint8_t)0x36)
#define TIMER_MODE_FREQ_PWM_OCR   ((uint8_t)0x37)
#define TIMER_MODE_FAST_PWM_8BIT  ((uint8_t)0x38)
#define TIMER_MODE_FAST_PWM_9BIT  ((uint8_t)0x39)
#define TIMER_MODE_FAST_PWM_10BIT ((uint8_t)0x3A)
#define TIMER_MODE_FAST_PWM_ICR   ((uint8_t)0x3B)
#define TIMER_MODE_FAST_PWM_OCR   ((uint8_t)0x3C)
#define TIMER_MODE_CTC_ICR        ((uint8_t)0x3D)
#define TIMER_MODE_CTC_OCR        ((uint8_t)0x3E)

// Output modes
// NOTE: Do not change the values, they are studied to be or-ed toghether
// binary format: 0 1 x x x x x x 
// first and second bits are constant
// then two are for C, two for B, and last two for A
#define OUT_MODE_NORMAL_A             ((uint8_t)0x40)
#define OUT_MODE_TOGGLE_ON_COMPARE_A  ((uint8_t)0x41)
#define OUT_MODE_CLEAR_ON_COMPARE_A   ((uint8_t)0x42)
#define OUT_MODE_SET_ON_COMPARE_A     ((uint8_t)0x43)
#define OUT_MODE_NORMAL_B             ((uint8_t)0x40)
#define OUT_MODE_TOGGLE_ON_COMPARE_B  ((uint8_t)0x44)
#define OUT_MODE_CLEAR_ON_COMPARE_B   ((uint8_t)0x48)
#define OUT_MODE_SET_ON_COMPARE_B     ((uint8_t)0x4C)
#define OUT_MODE_NORMAL_C             ((uint8_t)0x40)
#define OUT_MODE_TOGGLE_ON_COMPARE_C  ((uint8_t)0x50)
#define OUT_MODE_CLEAR_ON_COMPARE_C   ((uint8_t)0x60)
#define OUT_MODE_SET_ON_COMPARE_C     ((uint8_t)0x70)

// Not used by the user
#define OUT_MODE_MASK_A ((uint8_t)0x03)
#define OUT_MODE_MASK_B ((uint8_t)0x0C)
#define OUT_MODE_MASK_C ((uint8_t)0x30)

// Timer interrupt enable flags
// NOTE: Do not change the values, they are studied to be or-ed toghether
// binary format: 1 0 0 x x x x x
// first and second bits are constant
// then one bit is for TOI, one for C, one for B, and one for A, and last for ICI
#define TIMER_INTERRUPT_MODE_ICI   ((uint8_t)0x81)
#define TIMER_INTERRUPT_MODE_OCIA  ((uint8_t)0x82)
#define TIMER_INTERRUPT_MODE_OCIB  ((uint8_t)0x84)
#define TIMER_INTERRUPT_MODE_OCIC  ((uint8_t)0x88)
#define TIMER_INTERRUPT_MODE_TOI   ((uint8_t)0x90)

// A set of useful timer functions

// ===== TIMER SETUP FUNCTIONS ======
void timer_init(uint8_t timer_id, uint8_t source, uint8_t mode, uint8_t out_mode);
void timer_init_interrupt(uint8_t timer_id, uint8_t interrupt_mode);

// ===== TIMER CONTROLS =====
uint8_t timer_start(uint8_t timer_id); // immediately starts the timer (no computations if timer_id is compile-time constant)
uint8_t timer_stop(uint8_t timer_id); // immediately stops the timer (no computations if timer_id is compile-time constant)
uint8_t is_timer_running(uint8_t timer_id); // 1 if the timer is going, 0 if it is stopped
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
