#ifndef TIMER_UTILS_H
#define TIMER_UTILS_H

#include <stdint.h> // uint8_t, uint16_t
#include <stddef.h> // NULL pointer

// THIS SOURCE WORKS ONLY WITH Atmega32u4, so check that
#include "cpu.h"
#ifndef __AVR_ATmega32U4__
#    error This library works only with Atmega32u4
#endif

// Timer id
#define TIMER_ID_0 ((uint8_t)0x00)
#define TIMER_ID_1 ((uint8_t)0x01)
#define TIMER_ID_3 ((uint8_t)0x03)
// Comparator registers
#define TIMER_COMP_A ((uint8_t)0x10)
#define TIMER_COMP_B ((uint8_t)0x11)
#define TIMER_COMP_C ((uint8_t)0x12)

// A set of useful timer functions

// get/set timer counter, i.e. number of tick counted by the timer
uint8_t  timer_count0(const uint8_t  * new_count);
uint16_t timer_count1(const uint16_t * new_count);
uint16_t timer_count3(const uint16_t * new_count);

// get/set compare values, comparation is done automatically
uint8_t  timer_compare0(uint8_t comp_id, const uint8_t  * new_val);
uint16_t timer_compare1(uint8_t comp_id, const uint16_t * new_val);
uint16_t timer_compare3(uint8_t comp_id, const uint16_t * new_val);

#endif
