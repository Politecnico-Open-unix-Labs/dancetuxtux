/*
    Timer utils function, simpify the use of ATmega timers
    Copyright (C) 2016  Serraino Alessio

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
/*
    If you enjoy this work and you would like to support this project,
    please, consider donating me some bitcoins
          ------ 1NFZcpPRPchSfWAHCBqXrKAzXKrYVHL9ca ------
    Any amount, also a single satoshi, is always well accepted!
    You will get fantastic updates!
*/

#include <stdint.h> // uint8_t
#include <stddef.h> // NULL pointer
#include <limits.h> // CHAR_BIT

#include "cpu.h" // processor adress registers
#include "timer_utils.h" // timer defines

#include <avr/power.h> // power enable/disable timers

// This function works as follow:
// For each bit, if relative bitmask bit is 1 then is set to first not used new_val
// Else, if relative bitmask is 0, bit it is left untouched
static inline // dest is a pointer to arrays of max 4 bytes
void set_bits(uint8_t * dest, uint32_t mod_mask, uint32_t new_val) {
    uint32_t current_bitmask;
    uint32_t current_new_val_bitmask;
    uint8_t i;

    _Static_assert(CHAR_BIT == 8, "System not supported"); // This code won't work if it is not true
    i = (uint8_t)(-1); // Iterator counter
    current_new_val_bitmask = 1;
    for (current_bitmask = 1; current_bitmask; current_bitmask <<= 1) {
        i++;
        if ((mod_mask & current_bitmask) == 0) // Must not modify this bit
            continue;
    
        if (new_val & current_new_val_bitmask) // Set to 1
            dest[i / 8] |= (current_bitmask >> (i & 0xF8)); // Equiv (i/8)*8
        else // Set to 0
            dest[i / 8] &= ~(current_bitmask >> (i & 0xF8));
        current_new_val_bitmask <<= 1;
    }
}

// This function works as follow:
// For each bit, if relative bitmask is bit 1 then is set to the respective bit in new_val
// Else, if relative bitmask bit is 0, it is left untouched
static inline
void set_default_bits(uint8_t * dest, uint32_t mod_mask, uint32_t old_val) {
    uint32_t new_mask = 0;
    uint32_t current_bitmask;
    uint32_t current_new_bitmask;

    current_new_bitmask = 1;
    for (current_bitmask = 1; current_bitmask; current_bitmask <<= 1) {
        if ((mod_mask & current_bitmask) == 0) // Must not modify this bit
            continue;
    
        if (old_val & current_bitmask) // Set to 1
            new_mask |= current_new_bitmask;
        else // Set to 0
            new_mask &= ~(current_bitmask);
        current_new_bitmask <<= 1;
    }

    set_bits(dest, mod_mask, new_mask);
}

// Small utility, does some computation to retreive parameters    
static inline
void switch_out_mode(const uint8_t out_mode,
                     uint8_t * out_mode_a, uint8_t * out_mode_b, uint8_t * out_mode_c)
{
    if (out_mode & OUT_MODE_NORMAL_A) // Port A mode set
        *out_mode_a = (out_mode & OUT_MODE_MASK_A) | OUT_MODE_NORMAL_A;
    else // Port A mode left unset, set to normal
        *out_mode_a = OUT_MODE_NORMAL_A;
    
    if (out_mode & OUT_MODE_NORMAL_B) // Port B mode set
        *out_mode_b = (out_mode & OUT_MODE_MASK_B) | OUT_MODE_NORMAL_B;
    else // Port B mode left unset, set to normal
        *out_mode_b = OUT_MODE_NORMAL_B;
    
    if (out_mode & OUT_MODE_NORMAL_C) // Port C mode set
        *out_mode_c = (out_mode & OUT_MODE_MASK_C) | OUT_MODE_NORMAL_C;
    else // Port C mode left unset, set to normal
        *out_mode_c = OUT_MODE_NORMAL_C;
}


// =============================================
// ===   TIMERS BIT MANIPULATION FUNCTIONS   ===
// =============================================

// This allows on/off feature without setting every time the source
volatile static // Accessible only in this code. They might be accessed in ISR
uint8_t timer0_source = 0,
        timer1_source = 0,
        timer3_source = 0;

// Now a single function handles a single timer
static inline
void timer0_init(const uint8_t source, const uint8_t mode,
                 const uint8_t out_mode_a, const uint8_t out_mode_b) {
    uint8_t new_tccr0[2] = {0, 0};
    // bit    |   7      6      5      4      3      2      1      0
    // field  | FOC0A  FOC0B  ------ ------ WGM02  CS02   CS01   CS00     TCCR0B
    // bit    |   7      6      5      4      3      2      1      0  
    // field  | COM0A1 COM0A0 COM0B1 COM0B1 ------ ------ WGM01  WGM00    TCCR0A
    
    // Switches between modes, sets WGM0 bits
    uint32_t mode_bits = 0;
    mode_bits |= _BV(WGM02); // WGM02, should be bit 3
    mode_bits <<= CHAR_BIT; // moves to next byte
    mode_bits |= _BV(WGM01) | _BV(WGM00); // WGM01 and WGM00, should be bits 1 and 0
    if (mode == TIMER_MODE_NORMAL)             set_bits(new_tccr0, mode_bits, 0x00);
    else if (mode == TIMER_MODE_PWM_8BIT)      set_bits(new_tccr0, mode_bits, 0x01);
    else if (mode == TIMER_MODE_CTC_OCR)       set_bits(new_tccr0, mode_bits, 0x02);
    else if (mode == TIMER_MODE_FAST_PWM_8BIT) set_bits(new_tccr0, mode_bits, 0x03);
    else if (mode == TIMER_MODE_PWM_OCR)       set_bits(new_tccr0, mode_bits, 0x05);
    else if (mode == TIMER_MODE_FAST_PWM_OCR)  set_bits(new_tccr0, mode_bits, 0x07);
    else // Leaves this configuration as it is
        set_default_bits(new_tccr0, mode_bits, TCCR0B << CHAR_BIT | TCCR0A);

    // Now switches between output mode A
    uint32_t out_mode_a_bits = 0;
    out_mode_a_bits |= _BV(COM0A1) | _BV(COM0A0);
    if (out_mode_a == OUT_MODE_NORMAL_A)                 set_bits(new_tccr0, out_mode_a_bits, 0x00);
    else if (out_mode_a == OUT_MODE_TOGGLE_ON_COMPARE_A) set_bits(new_tccr0, out_mode_a_bits, 0x01);
    else if (out_mode_a == OUT_MODE_CLEAR_ON_COMPARE_A)  set_bits(new_tccr0, out_mode_a_bits, 0x02);
    else if (out_mode_a == OUT_MODE_SET_ON_COMPARE_A)    set_bits(new_tccr0, out_mode_a_bits, 0x03);
    else // Leaves this configuration as it is
        set_default_bits(new_tccr0, out_mode_a_bits, TCCR0B << CHAR_BIT | TCCR0A);
    
    // Now switches between output mode B
    uint32_t out_mode_b_bits = 0;
    out_mode_b_bits |= _BV(COM0B1) | _BV(COM0B0);
    if (out_mode_b == OUT_MODE_NORMAL_B)                 set_bits(new_tccr0, out_mode_b_bits, 0x00);
    else if (out_mode_b == OUT_MODE_TOGGLE_ON_COMPARE_B) set_bits(new_tccr0, out_mode_b_bits, 0x01);
    else if (out_mode_b == OUT_MODE_CLEAR_ON_COMPARE_B)  set_bits(new_tccr0, out_mode_b_bits, 0x02);
    else if (out_mode_b == OUT_MODE_SET_ON_COMPARE_B)    set_bits(new_tccr0, out_mode_b_bits, 0x03);
    else // Leaves this configuration as it is
        set_default_bits(new_tccr0, out_mode_b_bits, TCCR0B << CHAR_BIT | TCCR0A);
    
    // Now stores clock select info
    uint32_t cs_bits = 0;
    cs_bits = _BV(CS02) | _BV(CS01) | _BV(CS00);
    cs_bits <<= CHAR_BIT; // Bits are in TCCR0B
    set_default_bits(new_tccr0, cs_bits, TCCR0B << CHAR_BIT | TCCR0A); // Uses actual config, Timer CS
                                                                       // Will be set on startup
    _MemoryBarrier(); // Do not mix the operations before and after!
    TCCR0A = new_tccr0[0];
    TCCR0B = new_tccr0[1]; // Now sets all the new fields all together
    if (source != TIMER_USE_DEFAULT) {
        if (is_timer_running(TIMER_ID_0)) { // Timer is going
            timer0_source = source;
            timer_start(TIMER_ID_0); // Starting an already running timer only updates source
        } else { // Timer is not going
            timer0_source = source; // stores sources, sets when timer_start() is called
        }
    } // else leaves timer prescaler as it is, if timer was running leaves it running, etc..
}

#include "USB.h" // TODO: debug function
#include <stdio.h>

static inline
void timer1_init(const uint8_t source, const uint8_t mode,
                 const uint8_t out_mode_a, const uint8_t out_mode_b, const uint8_t out_mode_c) { 
    // Note: this sowtware won't set TCCR1C, it is reported for completeness
    // bit    |   7      6      5      4      3      2      1      0
    // field  | FOC1A  FOC1B  FOC1C  ------ ------ ------ ------ ------   TCCR1C

    // bit    |   7      6      5      4      3      2      1      0
    // field  | ICNC1  ICES1  ------  WGM13 WGM12  CS12   CS11   CS10     TCCR1B
    // bit    |   7      6      5      4      3      2      1      0  
    // field  | COM1A1 COM1A0 COM1B1 COM1B1 COM1C1 COM1C2 WGM11  WGM10    TCCR1A
    
    uint8_t new_tccr1[2] = {0, 0}; // Temporany

    uint32_t mode_bits = 0;
    mode_bits |= _BV(WGM12) | _BV(WGM13); // two bits in TCCR1B
    mode_bits <<= CHAR_BIT; // moves to next byte
    mode_bits |= _BV(WGM11) | _BV(WGM10); // two bits in TCCR1A
    if (mode == TIMER_MODE_NORMAL)              set_bits(new_tccr1, mode_bits, 0x00);
    else if (mode == TIMER_MODE_PWM_8BIT)       set_bits(new_tccr1, mode_bits, 0x01);
    else if (mode == TIMER_MODE_PWM_9BIT)       set_bits(new_tccr1, mode_bits, 0x02);
    else if (mode == TIMER_MODE_PWM_10BIT)      set_bits(new_tccr1, mode_bits, 0x03);
    else if (mode == TIMER_MODE_PWM_ICR)        set_bits(new_tccr1, mode_bits, 0x0A);
    else if (mode == TIMER_MODE_PWM_OCR)        set_bits(new_tccr1, mode_bits, 0x0B);
    else if (mode == TIMER_MODE_FREQ_PWM_ICR)   set_bits(new_tccr1, mode_bits, 0x08);
    else if (mode == TIMER_MODE_FREQ_PWM_OCR)   set_bits(new_tccr1, mode_bits, 0x09);
    else if (mode == TIMER_MODE_FAST_PWM_8BIT)  set_bits(new_tccr1, mode_bits, 0x05);
    else if (mode == TIMER_MODE_FAST_PWM_9BIT)  set_bits(new_tccr1, mode_bits, 0x06);
    else if (mode == TIMER_MODE_FAST_PWM_10BIT) set_bits(new_tccr1, mode_bits, 0x07);
    else if (mode == TIMER_MODE_FAST_PWM_ICR)   set_bits(new_tccr1, mode_bits, 0x0E);
    else if (mode == TIMER_MODE_FAST_PWM_OCR)   set_bits(new_tccr1, mode_bits, 0x0F);
    else if (mode == TIMER_MODE_CTC_ICR)        set_bits(new_tccr1, mode_bits, 0x0C);
    else if (mode == TIMER_MODE_CTC_OCR)        set_bits(new_tccr1, mode_bits, 0x04);
    else // Leaves this configuration as it is
        set_default_bits(new_tccr1, mode_bits, TCCR1B << CHAR_BIT | TCCR1A);

    // Now switches between output mode A
    uint32_t out_mode_a_bits = 0;
    out_mode_a_bits |= _BV(COM1A1) | _BV(COM1A0); // All of this bits are in TCCR1A
    if (out_mode_a == OUT_MODE_NORMAL_A)                 set_bits(new_tccr1, out_mode_a_bits, 0x00);
    else if (out_mode_a == OUT_MODE_TOGGLE_ON_COMPARE_A) set_bits(new_tccr1, out_mode_a_bits, 0x01);
    else if (out_mode_a == OUT_MODE_CLEAR_ON_COMPARE_A)  set_bits(new_tccr1, out_mode_a_bits, 0x02);
    else if (out_mode_a == OUT_MODE_SET_ON_COMPARE_A)    set_bits(new_tccr1, out_mode_a_bits, 0x03);
    else // Leaves this configuration as it is
        set_default_bits(new_tccr1, out_mode_a_bits, TCCR1B << CHAR_BIT | TCCR1A);
    
    // Now switches between output mode B
    uint32_t out_mode_b_bits = 0;
    out_mode_b_bits |= _BV(COM1B1) | _BV(COM1B0);
    if (out_mode_b == OUT_MODE_NORMAL_B)                 set_bits(new_tccr1, out_mode_b_bits, 0x00);
    else if (out_mode_b == OUT_MODE_TOGGLE_ON_COMPARE_B) set_bits(new_tccr1, out_mode_b_bits, 0x01);
    else if (out_mode_b == OUT_MODE_CLEAR_ON_COMPARE_B)  set_bits(new_tccr1, out_mode_b_bits, 0x02);
    else if (out_mode_b == OUT_MODE_SET_ON_COMPARE_B)    set_bits(new_tccr1, out_mode_b_bits, 0x03);
    else // Leaves this configuration as it is
        set_default_bits(new_tccr1, out_mode_b_bits, TCCR1B << CHAR_BIT | TCCR1A);
    
    // Now switches between output mode C
    uint32_t out_mode_c_bits = 0;
    out_mode_c_bits |= _BV(COM1C1) | _BV(COM1C0);
    if (out_mode_c == OUT_MODE_NORMAL_C)                 set_bits(new_tccr1, out_mode_c_bits, 0x00);
    else if (out_mode_c == OUT_MODE_TOGGLE_ON_COMPARE_C) set_bits(new_tccr1, out_mode_c_bits, 0x01);
    else if (out_mode_c == OUT_MODE_CLEAR_ON_COMPARE_C)  set_bits(new_tccr1, out_mode_c_bits, 0x02);
    else if (out_mode_c == OUT_MODE_SET_ON_COMPARE_C)    set_bits(new_tccr1, out_mode_c_bits, 0x03);
    else // Leaves this configuration as it is
        set_default_bits(new_tccr1, out_mode_c_bits, TCCR1B << CHAR_BIT | TCCR1A);
    
    // Now stores clock select info
    uint32_t cs_bits = 0;
    cs_bits = _BV(CS12) | _BV(CS11) | _BV(CS10);
    cs_bits <<= CHAR_BIT; // Bits are in TCCR1A
    set_default_bits(new_tccr1, cs_bits, TCCR1B << CHAR_BIT | TCCR1A); // Uses actual config, Timer CS
                                                                       // Will be set on startup
    _MemoryBarrier(); // Do not mix the operations before and after!
    TCCR1A = new_tccr1[0];
    TCCR1B = new_tccr1[1]; // Now sets all the new fields all together
    if (source != TIMER_USE_DEFAULT) {
        if (is_timer_running(TIMER_ID_1)) { // Timer is going
            timer1_source = source;
            timer_start(TIMER_ID_1); // Starting an already running timer only updates source
        } else { // Timer is not going
            timer1_source = source; // stores sources, sets when timer_start() is called
        }
    } // else leaves timer prescaler as it is, if timer was running leaves it running, etc..
}

static inline
void timer3_init(const uint8_t source, const uint8_t mode,
                 const uint8_t out_mode_a, const uint8_t out_mode_b, const uint8_t out_mode_c) { 
    // Note: this sowtware won't set TCCR3C, it is reported for completeness
    // bit    |   7      6      5      4      3      2      1      0
    // field  | FOC3A  FOC3B  FOC3C  ------ ------ ------ ------ ------   TCCR3C

    // bit    |   7      6      5      4      3      2      1      0
    // field  | ICNC3  ICES3  ------  WGM33 WGM32  CS32   CS31   CS30     TCCR3B
    // bit    |   7      6      5      4      3      2      1      0  
    // field  | COM3A1 COM3A0 COM3B1 COM3B1 COM3C1 COM3C2 WGM31  WGM30    TCCR3A

    uint8_t new_tccr3[2] = {0, 0}; // Temporany

    uint32_t mode_bits = 0;
    mode_bits |= _BV(WGM32) | _BV(WGM33); // two bits in TCCR3B
    mode_bits <<= CHAR_BIT; // moves to next byte
    mode_bits |= _BV(WGM31) | _BV(WGM30); // two bits in TCCR3A
    if (mode == TIMER_MODE_NORMAL)              set_bits(new_tccr3, mode_bits, 0x00);
    else if (mode == TIMER_MODE_PWM_8BIT)       set_bits(new_tccr3, mode_bits, 0x01);
    else if (mode == TIMER_MODE_PWM_9BIT)       set_bits(new_tccr3, mode_bits, 0x02);
    else if (mode == TIMER_MODE_PWM_10BIT)      set_bits(new_tccr3, mode_bits, 0x03);
    else if (mode == TIMER_MODE_PWM_ICR)        set_bits(new_tccr3, mode_bits, 0x0A);
    else if (mode == TIMER_MODE_PWM_OCR)        set_bits(new_tccr3, mode_bits, 0x0B);
    else if (mode == TIMER_MODE_FREQ_PWM_ICR)   set_bits(new_tccr3, mode_bits, 0x08);
    else if (mode == TIMER_MODE_FREQ_PWM_OCR)   set_bits(new_tccr3, mode_bits, 0x09);
    else if (mode == TIMER_MODE_FAST_PWM_8BIT)  set_bits(new_tccr3, mode_bits, 0x05);
    else if (mode == TIMER_MODE_FAST_PWM_9BIT)  set_bits(new_tccr3, mode_bits, 0x06);
    else if (mode == TIMER_MODE_FAST_PWM_10BIT) set_bits(new_tccr3, mode_bits, 0x07);
    else if (mode == TIMER_MODE_FAST_PWM_ICR)   set_bits(new_tccr3, mode_bits, 0x0E);
    else if (mode == TIMER_MODE_FAST_PWM_OCR)   set_bits(new_tccr3, mode_bits, 0x0F);
    else if (mode == TIMER_MODE_CTC_ICR)        set_bits(new_tccr3, mode_bits, 0x0C);
    else if (mode == TIMER_MODE_CTC_OCR)        set_bits(new_tccr3, mode_bits, 0x04);
    else // Leaves this configuration as it is
        set_default_bits(new_tccr3, mode_bits, TCCR3B << CHAR_BIT | TCCR3A);

    // Now switches between output mode A
    uint32_t out_mode_a_bits = 0;
    out_mode_a_bits |= _BV(COM3A0) | _BV(COM3A0); // All of this bits are in TCCR3A
    if (out_mode_a == OUT_MODE_NORMAL_A)                 set_bits(new_tccr3, out_mode_a_bits, 0x00);
    else if (out_mode_a == OUT_MODE_TOGGLE_ON_COMPARE_A) set_bits(new_tccr3, out_mode_a_bits, 0x01);
    else if (out_mode_a == OUT_MODE_CLEAR_ON_COMPARE_A)  set_bits(new_tccr3, out_mode_a_bits, 0x02);
    else if (out_mode_a == OUT_MODE_SET_ON_COMPARE_A)    set_bits(new_tccr3, out_mode_a_bits, 0x03);
    else // Leaves this configuration as it is
        set_default_bits(new_tccr3, out_mode_a_bits, TCCR3B << CHAR_BIT | TCCR3A);
    
    // Now switches between output mode B
    uint32_t out_mode_b_bits = 0;
    out_mode_b_bits |= _BV(COM3B1) | _BV(COM3B0);
    if (out_mode_b == OUT_MODE_NORMAL_B)                 set_bits(new_tccr3, out_mode_b_bits, 0x00);
    else if (out_mode_b == OUT_MODE_TOGGLE_ON_COMPARE_B) set_bits(new_tccr3, out_mode_b_bits, 0x01);
    else if (out_mode_b == OUT_MODE_CLEAR_ON_COMPARE_B)  set_bits(new_tccr3, out_mode_b_bits, 0x02);
    else if (out_mode_b == OUT_MODE_SET_ON_COMPARE_B)    set_bits(new_tccr3, out_mode_b_bits, 0x03);
    else // Leaves this configuration as it is
        set_default_bits(new_tccr3, out_mode_b_bits, TCCR3B << CHAR_BIT | TCCR3A);
    
    // Now switches between output mode C
    uint32_t out_mode_c_bits = 0;
    out_mode_c_bits |= _BV(COM3C1) | _BV(COM3C0);
    if (out_mode_c == OUT_MODE_NORMAL_C)                 set_bits(new_tccr3, out_mode_c_bits, 0x00);
    else if (out_mode_c == OUT_MODE_TOGGLE_ON_COMPARE_C) set_bits(new_tccr3, out_mode_c_bits, 0x01);
    else if (out_mode_c == OUT_MODE_CLEAR_ON_COMPARE_C)  set_bits(new_tccr3, out_mode_c_bits, 0x02);
    else if (out_mode_c == OUT_MODE_SET_ON_COMPARE_C)    set_bits(new_tccr3, out_mode_c_bits, 0x03);
    else // Leaves this configuration as it is
        set_default_bits(new_tccr3, out_mode_c_bits, TCCR3B << CHAR_BIT | TCCR3A);
    
    // Now stores clock select info
    uint32_t cs_bits = 0;
    cs_bits = _BV(CS32) | _BV(CS31) | _BV(CS30);
    cs_bits <<= CHAR_BIT; // Bits are in TCCR3A
    set_default_bits(new_tccr3, cs_bits, TCCR3B << CHAR_BIT | TCCR3A); // Uses actual config, Timer CS
                                                                       // Will be set on startup
    _MemoryBarrier(); // Do not mix the operations before and after!
    TCCR3A = new_tccr3[0];
    TCCR3B = new_tccr3[1]; // Now sets all the new fields all together
    if (source != TIMER_USE_DEFAULT) {
        if (is_timer_running(TIMER_ID_3)) { // Timer is going
            timer3_source = source;
            timer_start(TIMER_ID_3); // Starting an already running timer only updates source
        } else { // Timer is not going
            timer1_source = source; // stores sources, sets when timer_start() is called
        }
    } // else leaves timer prescaler as it is, if timer was running leaves it running, etc..
}

static inline
void timer0_init_interrupt(const uint8_t interrupt_mode_switch) { 
    // bit   |   7     6     5     4      3     2      1     0
    // field | ----- ----- ----- ----- ----- OCIE0B OCIE0A TOIE0     TIMSK0
    // bit   |   7     6     5     4      3     2      1     0
    // field | ----- ----- ----- ----- -----  OCF0B  OCF0A  TOF0     TIFR0
    TIFR0 = 0; // Always reset. This contains 1 if the interrupt is going to be executed
    if (interrupt_mode_switch & TIMER_INTERRUPT_MODE_OCIB) // Output compare interrupt B
                TIMSK0 |= _BV(OCIE0B); // set bit
    else        TIMSK0 &= ~(_BV(OCIE0B)); // clear bit
    if (interrupt_mode_switch & TIMER_INTERRUPT_MODE_OCIA) // Output compare interrupt A
                TIMSK0 |= _BV(OCIE0A); // set bit
    else        TIMSK0 &= ~(_BV(OCIE0A)); // clear bit
    if (interrupt_mode_switch & TIMER_INTERRUPT_MODE_TOI) // Timer overflow interrupt
                TIMSK0 |= _BV(TOIE0); // set bit
    else        TIMSK0 &= ~(_BV(TOIE0)); // clear bit
}

static inline
void timer1_init_interrupt(const uint8_t interrupt_mode_switch) {
    // bit   |   7     6     5     4      3     2      1     0
    // field | ----- ----- ICIE1 ----- OCIE1C OCIE1B OCIE1A TOIE1     TIMSK1
    // bit   |   7     6     5     4      3     2      1     0
    // field | ----- ----- ICF1  -----  OCF1C OCF1B  OCF1A  TOF1      TIFR1
   
    TIFR1 = 0; // Always reset. This contains 1 if the interrupt is going to be executed
    if (interrupt_mode_switch & TIMER_INTERRUPT_MODE_ICI) // Input Compare interrupt
                TIMSK1 |= _BV(ICIE1); // set bit
    else        TIMSK1 &= ~(_BV(ICIE1)); // clear bit
    if (interrupt_mode_switch & TIMER_INTERRUPT_MODE_OCIC) // Output compare interrupt C
                TIMSK1 |= _BV(OCIE1C); // set bit
    else        TIMSK1 &= ~(_BV(OCIE1C)); // clear bit
    if (interrupt_mode_switch & TIMER_INTERRUPT_MODE_OCIB) // Output compare interrupt B
                TIMSK1 |= _BV(OCIE1B); // set bit
    else        TIMSK1 &= ~(_BV(OCIE1B)); // clear bit
    if (interrupt_mode_switch & TIMER_INTERRUPT_MODE_OCIA) // Output compare interrupt A
                TIMSK1 |= _BV(OCIE1A); // set bit
    else        TIMSK1 &= ~(_BV(OCIE1A)); // clear bit
    if (interrupt_mode_switch & TIMER_INTERRUPT_MODE_TOI) // Timer overflow interupt
                TIMSK1 |= _BV(TOIE1); // set bit
    else        TIMSK1 &= ~(_BV(TOIE1)); // clear bit
}

static inline // This is exactly as Timer1. Timer1 and Timer3 are almost indentical
void timer3_init_interrupt(const uint8_t interrupt_mode_switch) {
    // bit   |   7     6     5     4      3     2      1     0
    // field | ----- ----- ICIE3 ----- OCIE3C OCIE3B OCIE3A TOIE3     TIMSK3
    // bit   |   7     6     5     4      3     2      1     0
    // field | ----- ----- ICF3  -----  OCF3C OCF3B  OCF3A  TOF3      TIFR3
   
    TIFR3 = 0; // Always reset. This contains 1 if the interrupt is going to be executed
    if (interrupt_mode_switch & TIMER_INTERRUPT_MODE_ICI) // Input Compare interrupt
                TIMSK3 |= _BV(ICIE3); // set bit
    else        TIMSK3 &= ~(_BV(ICIE3)); // clear bit
    if (interrupt_mode_switch & TIMER_INTERRUPT_MODE_OCIC) // Output compare interrupt C
                TIMSK3 |= _BV(OCIE3C); // set bit
    else        TIMSK3 &= ~(_BV(OCIE3C)); // clear bit
    if (interrupt_mode_switch & TIMER_INTERRUPT_MODE_OCIB) // Output compare interrupt B
                TIMSK3 |= _BV(OCIE3B); // set bit
    else        TIMSK3 &= ~(_BV(OCIE3B)); // clear bit
    if (interrupt_mode_switch & TIMER_INTERRUPT_MODE_OCIA) // Output compare interrupt A
                TIMSK3 |= _BV(OCIE3A); // set bit
    else        TIMSK3 &= ~(_BV(OCIE3A)); // clear bit
    if (interrupt_mode_switch & TIMER_INTERRUPT_MODE_TOI) // Timer overflow interupt
                TIMSK3 |= _BV(TOIE3); // set bit
    else        TIMSK3 &= ~(_BV(TOIE3)); // clear bit
}

static inline
void timer0_force_start(void) {
    uint16_t new_bits; // Holds 3 bits, one per nibble (improves readability)

    if (timer0_source == TIMER_SOURCE_NONE)          new_bits = 0x000;
    else if (timer0_source == TIMER_SOURCE_CLK)      new_bits = 0x001;
    else if (timer0_source == TIMER_SOURCE_CLK_8)    new_bits = 0x010;
    else if (timer0_source == TIMER_SOURCE_CLK_64)   new_bits = 0x011;
    else if (timer0_source == TIMER_SOURCE_CLK_256)  new_bits = 0x100;
    else if (timer0_source == TIMER_SOURCE_CLK_1024) new_bits = 0x101;
    else if (timer0_source == TIMER_SOURCE_EXT_FALL) new_bits = 0x110;
    else if (timer0_source == TIMER_SOURCE_EXT_RISE) new_bits = 0x111;
    else // Invalid value (should not happen), leave parameter as it is
        return; // Nothing more to be done here
    
    // This instruction will actually start the timer
    TCCR0B |= _BV(CS00)*!!(new_bits & 0x000f) |
              _BV(CS01)*!!(new_bits & 0x00f0) |
              _BV(CS02)*!!(new_bits & 0x0f00);
        
    if (timer0_source == TIMER_SOURCE_NONE) // Source note actually stops the timer
        timer0_source = 0; // Timer is not running
    else // Provided valid non NONE source
        timer0_source = TIMER_RUNNING; // timer is now running
}

static inline
void timer1_force_start(void) {
    uint16_t new_bits; // Holds 3 bits, one per nibble (improves readability)

    if (timer1_source == TIMER_SOURCE_NONE)          new_bits = 0x000;
    else if (timer1_source == TIMER_SOURCE_CLK)      new_bits = 0x001;
    else if (timer1_source == TIMER_SOURCE_CLK_8)    new_bits = 0x010;
    else if (timer1_source == TIMER_SOURCE_CLK_64)   new_bits = 0x011;
    else if (timer1_source == TIMER_SOURCE_CLK_256)  new_bits = 0x100;
    else if (timer1_source == TIMER_SOURCE_CLK_1024) new_bits = 0x101;
    else if (timer1_source == TIMER_SOURCE_EXT_FALL) new_bits = 0x110;
    else if (timer1_source == TIMER_SOURCE_EXT_RISE) new_bits = 0x111;
    else // Invalid value (should not happen), leave parameter as it is
        return; // Nothing more to be done here
    
    // This instruction will actually start the timer
    TCCR1B |= _BV(CS10)*!!(new_bits & 0x000f) |
              _BV(CS11)*!!(new_bits & 0x00f0) |
              _BV(CS12)*!!(new_bits & 0x0f00);
    
    if (timer1_source == TIMER_SOURCE_NONE) // Source note actually stops the timer
        timer1_source = 0; // Timer is not running
    else // Provided valid non NONE source
        timer1_source = TIMER_RUNNING; // timer is now running
}

static inline
void timer3_force_start(void) {
    uint16_t new_bits; // Holds 3 bits, one per nibble (improves readability)

    if (timer3_source == TIMER_SOURCE_NONE)          new_bits = 0x000;
    else if (timer3_source == TIMER_SOURCE_CLK)      new_bits = 0x001;
    else if (timer3_source == TIMER_SOURCE_CLK_8)    new_bits = 0x010;
    else if (timer3_source == TIMER_SOURCE_CLK_64)   new_bits = 0x011;
    else if (timer3_source == TIMER_SOURCE_CLK_256)  new_bits = 0x100;
    else if (timer3_source == TIMER_SOURCE_CLK_1024) new_bits = 0x101;
    else if (timer3_source == TIMER_SOURCE_EXT_FALL) new_bits = 0x110;
    else if (timer3_source == TIMER_SOURCE_EXT_RISE) new_bits = 0x111;
    else // Invalid value (should not happen), leave parameter as it is
        return; // Nothing more to be done here
    
    // This instruction will actually start the timer
    TCCR3B |= _BV(CS30)*!!(new_bits & 0x000f) |
              _BV(CS31)*!!(new_bits & 0x00f0) |
              _BV(CS32)*!!(new_bits & 0x0f00);

    if (timer3_source == TIMER_SOURCE_NONE) // Source note actually stops the timer
        timer3_source = 0; // Timer is not running
    else // Provided valid non NONE source
        timer3_source = TIMER_RUNNING; // timer is now running
}

static inline
void timer0_force_stop(void) {
    uint8_t mode; // Stores current timer mode
    
    // simply clears the source bit to stop the timer
    mode = TCCR0B & (_BV(CS02) | _BV(CS01) | _BV(CS00)); // Gets the status
    if (mode == 0x00)      mode = TIMER_SOURCE_NONE; // Timer stopped
    else if (mode == 0x01) mode = TIMER_SOURCE_CLK;
    else if (mode == 0x02) mode = TIMER_SOURCE_CLK_8;
    else if (mode == 0x03) mode = TIMER_SOURCE_CLK_64;
    else if (mode == 0x04) mode = TIMER_SOURCE_CLK_256;
    else if (mode == 0x05) mode = TIMER_SOURCE_CLK_1024;
    else if (mode == 0x06) mode = TIMER_SOURCE_EXT_FALL;
    else if (mode == 0x07) mode = TIMER_SOURCE_EXT_RISE;
    // else should be impossible
    TCCR0B &= ~(_BV(CS02) | _BV(CS01) | _BV(CS00)); // Clears all the clock source bits
    // This will set source to NONE, i.e will stop the timer
    timer0_source = mode; // stores timer mode
}

static inline
void timer1_force_stop(void) {
    uint8_t mode; // Stores current timer mode
    
    // simply clears the source bit to stop the timer
    mode = TCCR1B & (_BV(CS12) | _BV(CS11) | _BV(CS10)); // Gets the status
    if (mode == 0x00)      mode = TIMER_SOURCE_NONE; // Timer stopped
    else if (mode == 0x01) mode = TIMER_SOURCE_CLK;
    else if (mode == 0x02) mode = TIMER_SOURCE_CLK_8;
    else if (mode == 0x03) mode = TIMER_SOURCE_CLK_64;
    else if (mode == 0x04) mode = TIMER_SOURCE_CLK_256;
    else if (mode == 0x05) mode = TIMER_SOURCE_CLK_1024;
    else if (mode == 0x06) mode = TIMER_SOURCE_EXT_FALL;
    else if (mode == 0x07) mode = TIMER_SOURCE_EXT_RISE;
    // else should be impossible
    TCCR1B &= ~(_BV(CS12) | _BV(CS11) | _BV(CS10)); // Clears all the clock source bits
    // This will set source to NONE, i.e will stop the timer
    timer1_source = mode; // stores timer mode
}

static inline
void timer3_force_stop(void) {
    uint8_t mode; // Stores current timer mode
    
    // simply clears the source bit to stop the timer
    mode = TCCR3B & (_BV(CS32) | _BV(CS31) | _BV(CS30)); // Gets the status
    if (mode == 0x00)      mode = TIMER_SOURCE_NONE; // Timer stopped
    else if (mode == 0x01) mode = TIMER_SOURCE_CLK;
    else if (mode == 0x02) mode = TIMER_SOURCE_CLK_8;
    else if (mode == 0x03) mode = TIMER_SOURCE_CLK_64;
    else if (mode == 0x04) mode = TIMER_SOURCE_CLK_256;
    else if (mode == 0x05) mode = TIMER_SOURCE_CLK_1024;
    else if (mode == 0x06) mode = TIMER_SOURCE_EXT_FALL;
    else if (mode == 0x07) mode = TIMER_SOURCE_EXT_RISE;
    // else should be impossible
    TCCR3B &= ~(_BV(CS32) | _BV(CS31) | _BV(CS30)); // Clears all the clock source bits
    // This will set source to NONE, i.e will stop the timer
    timer3_source = mode; // stores timer mode
}

// ====================================
// ===   User interface functions   ===
// ====================================

void timer_init(const uint8_t timer_id, const uint8_t source,
                const uint8_t mode, const uint8_t out_mode)
{
    uint8_t out_mode_a, out_mode_b, out_mode_c; // Temporanies
    
    switch_out_mode(out_mode, &out_mode_a, &out_mode_b, &out_mode_c);
    switch (timer_id) {
        case TIMER_ID_0:
            timer0_init(source, mode, out_mode_a, out_mode_b);
            break;
        case TIMER_ID_1: 
            timer1_init(source, mode, out_mode_a, out_mode_b, out_mode_c);
            break;
        case TIMER_ID_3:
            timer3_init(source, mode, out_mode_a, out_mode_b, out_mode_c);
            break;
    } // End switch
}

void timer_init_interrupt(const uint8_t timer_id, const uint8_t interrupt_mode) {
    const uint8_t interrupt_mode_mask = TIMER_INTERRUPT_MODE_OCIB & // mask of common fields
                                        TIMER_INTERRUPT_MODE_OCIA & // used to eliminate commons!
                                        TIMER_INTERRUPT_MODE_OCIA &
                                        TIMER_INTERRUPT_MODE_ICI  &
                                        TIMER_INTERRUPT_MODE_TOI;
    const uint8_t interrupt_mode_switch = interrupt_mode & (~interrupt_mode_mask); // Takes only
    switch (timer_id) {
        case TIMER_ID_0:
            timer0_init_interrupt(interrupt_mode_switch);
            break;
        case TIMER_ID_1:
            timer1_init_interrupt(interrupt_mode_switch);
            break;
        case TIMER_ID_3:
            timer3_init_interrupt(interrupt_mode_switch);
            break;
    }
}

uint8_t timer_start(uint8_t timer_id)
{
    if (is_timer_running(timer_id)) // Timer is already going
        return 0; // unsuccessful operation
    
    if (timer_id == TIMER_ID_0) { // Timer 0 
        timer0_force_start(); // Timer is not running, so forcing start has no side effects
    } else if (timer_id == TIMER_ID_1) {
        timer1_force_start(); // Timer is not running, so forcing start has no side effects
    } else if (timer_id == TIMER_ID_3) {
        timer3_force_start(); // Timer is not running, so forcing start has no side effects
    } else { // Else wrong timer, do nothing
        return 0;
    }
    return 1; // Operation succeded
}

uint8_t timer_stop(uint8_t timer_id)
{
    if (!is_timer_running(timer_id)) // Timer is already stopped
        return 0; // unsuccessfull operation

    if (timer_id == TIMER_ID_0) {
        timer0_force_stop();
    } else if (timer_id == TIMER_ID_1) {
        timer1_force_stop();
    } else if (timer_id == TIMER_ID_3) {
        timer3_force_stop();
    } else {
        return 0; // Wrong timer
    }
    return 1; // Operation succeded
}

uint8_t is_timer_running(uint8_t timer_id) {
    if (timer_id == TIMER_ID_0)
        return timer0_source == TIMER_RUNNING;
    else if (timer_id == TIMER_ID_1)
        return timer1_source == TIMER_RUNNING;
    else if (timer_id == TIMER_ID_3)
        return timer3_source == TIMER_RUNNING;
    else // Invalid timer
        return (uint8_t)(-1);
}

// ===============================
// ===   POWERSAVE FUNCTIONS   ===
// ===============================

void timer_enable(uint8_t timer_id) {
    if (timer_id == TIMER_ID_0)
        power_timer0_enable();
    else if (timer_id == TIMER_ID_1)
        power_timer1_enable();
    else if (timer_id == TIMER_ID_3)
        power_timer3_enable();
}

void timer_disable(uint8_t timer_id) {
    if (timer_id == TIMER_ID_0)
        power_timer0_disable();
    else if (timer_id == TIMER_ID_1)
        power_timer1_disable();
    else if (timer_id == TIMER_ID_3)
        power_timer3_disable();
}

// ==============================================================================
// ===== get/set timer counter, i.e. number of tick counted by the timer   ======
// ==============================================================================

uint8_t timer0_count(const uint8_t new_count[]) { // The array makes the pointer constant
    uint8_t cnt;
    cnt = TCNT0; // Reads the ticks
    if (new_count) // Replaces the counter
       TCNT0 = new_count[0];
    return cnt; // returns the old value
}

uint16_t timer1_count(const uint16_t new_count[]) {
    uint16_t cnt;
    cnt = TCNT1;
    if (new_count)
        TCNT1 = new_count[0];
    return cnt;
}

// Note: there is not a timer2

uint16_t timer3_count(const uint16_t new_count[]) {
    uint16_t cnt;
    cnt = TCNT3;
    if (new_count)
        TCNT3 = new_count[0];
    return cnt;
}


// =======================================================================
// ===== get/set compare values, comparation is done automatically  ======
// =======================================================================

uint8_t timer0_compare(const uint8_t comp_id, const uint8_t new_val[]) {
    uint8_t old_comp = 0;
    if (comp_id == TIMER_COMP_A) {
        old_comp = OCR0A;
        OCR0A = new_val[0];
    } else if (comp_id == TIMER_COMP_B) {
        old_comp = OCR0B;
        OCR0B = new_val[0];
    }
    return old_comp;
}

uint16_t timer1_compare(const uint8_t comp_id, const uint16_t new_val[]) {
    uint16_t old_comp = 0;
    if (comp_id == TIMER_COMP_A) {
        old_comp = OCR1A;
        OCR1A = new_val[0];
    } else if (comp_id == TIMER_COMP_B) {
        old_comp = OCR1B;
        OCR1B = new_val[0];
    } else if (comp_id == TIMER_COMP_C) {
        old_comp = OCR1C;
        OCR1C = new_val[0];
    } else if (comp_id == TIMER_COMP_ICR) {
        old_comp = ICR1;
        ICR1 = new_val[0];
    }
    return old_comp;
}

uint16_t timer3_compare(const uint8_t comp_id, const uint16_t new_val[]) {
    uint16_t old_comp = 0;
    if (comp_id == TIMER_COMP_A) {
        old_comp = OCR3A;
        OCR3A = new_val[0];
    } else if (comp_id == TIMER_COMP_B) {
        old_comp = OCR3B;
        OCR3B = new_val[0];
    } else if (comp_id == TIMER_COMP_C) {
        old_comp = OCR3C;
        OCR3C = new_val[0];
    } else if (comp_id == TIMER_COMP_ICR) {
        old_comp = ICR3;
        ICR3 = new_val[0];
    }
    return old_comp;
}
