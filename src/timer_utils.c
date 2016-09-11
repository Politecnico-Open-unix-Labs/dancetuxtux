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


// =============================================
// ===   TIMERS BIT MANIPULATION FUNCTIONS   ===
// =============================================

// This allows on/off feature without setting every time the source
static uint8_t timer0_source = 0,
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
    else if (mode == TIMER_MODE_PWM)           set_bits(new_tccr0, mode_bits, 0x01);
    else if (mode == TIMER_MODE_CTC)           set_bits(new_tccr0, mode_bits, 0x02);
    else if (mode == TIMER_MODE_FAST_PWM)      set_bits(new_tccr0, mode_bits, 0x03);
    else if (mode == TIMER_MODE_PWM_OCRA)      set_bits(new_tccr0, mode_bits, 0x05);
    else if (mode == TIMER_MODE_FAST_PWM_OCRA) set_bits(new_tccr0, mode_bits, 0x07);
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
        set_default_bits(new_tccr0, out_mode_a_bits, TCCR0B << CHAR_BIT | TCCR0A);
    
    // Now stores clock select info
    uint32_t cs_bits = 0;
    cs_bits = _BV(CS02) | _BV(CS01) | _BV(CS00);
    cs_bits <<= CHAR_BIT; // Bits are in TCCR0A
    set_default_bits(new_tccr0, cs_bits, TCCR0B << CHAR_BIT | TCCR0A);

    _MemoryBarrier(); // Do not mix the operations before and after!
    TCCR0A = new_tccr0[0];
    TCCR0B = new_tccr0[1]; // Now sets all the fields
    if (source != TIMER_USE_DEFAULT) {
        if (timer0_source == TIMER_USE_DEFAULT) { // Timer is going
            timer0_source = source;
            timer_start(0); // Starting an already running timer only updates source
        } else { // Timer is not going
            timer0_source = source; // stores sources, sets when timer_start() is called
        }
    } // else leaves timer prescaler as it is
    
}

static inline
void timer0_init_interrupt(const uint8_t interrupt_mode_switch) { 
    // bit   |   7     6     5     4      3     2      1     0
    // field | ----- ----- ----- ----- ----- OCIE0B OCIE0A TOIE0     TIMSK
    TIFR0 = 0; // Always reset. This contains 1 if the interrupt is going to be executed
    if (interrupt_mode_switch & TIMER_INTERRUPT_MODE_OCIB) // Output compare B
                TIMSK0 |= _BV(OCIE0B);
    else        TIMSK0 &= ~(_BV(OCIE0B));
    if (interrupt_mode_switch & TIMER_INTERRUPT_MODE_OCIA) // Output compare A
                TIMSK0 |= _BV(OCIE0A);
    else        TIMSK0 &= ~(_BV(OCIE0A));
    if (interrupt_mode_switch & TIMER_INTERRUPT_MODE_TOI) // Timer overflow
                TIMSK0 |= _BV(TOIE0);
    else        TIMSK0 &= ~(_BV(TOIE0));
}

void timer_init(const uint8_t timer_id, const uint8_t source, const uint8_t mode,
                const uint8_t out_mode_a, const uint8_t out_mode_b) {
    
// >>>> TODO <<<< REMOVE THIS CODE !!!
// Uses the variables
(void)timer1_source;
(void)timer3_source;
    
    switch (timer_id) {
        case TIMER_ID_0:
            timer0_init(source, mode, out_mode_a, out_mode_b);
            break;
        case TIMER_ID_1:
            // TODO: ...
            break;
        case TIMER_ID_3:
            // TODO: ....
            break;
    } // End switch
}

void timer_init_interrupt(const uint8_t timer_id, const uint8_t interrupt_mode) {
    const uint8_t interrupt_mode_mask = TIMER_INTERRUPT_MODE_OCIB & // mask of common fields
                                        TIMER_INTERRUPT_MODE_OCIA & // used to eliminate commons!
                                        TIMER_INTERRUPT_MODE_TOI;
    const uint8_t interrupt_mode_switch = interrupt_mode - interrupt_mode_mask; // only different bits
    switch (timer_id) {
        case TIMER_ID_0:
            timer0_init_interrupt(interrupt_mode_switch);
            break;
        case TIMER_ID_1:
            // TODO: ...
            break;
        case TIMER_ID_3:
            // TODO: ....
            break;
    }
}

void timer_start(uint8_t timer_id) {
    uint8_t timer_source_tmp;
    if (timer_id == TIMER_ID_0) { // Timer 0
        // Sets the source, this way the timer starts
        timer_source_tmp = timer0_source;
        timer0_source = -1; // timer is running
        if (timer_source_tmp == TIMER_SOURCE_NONE)
            TCCR0B |= _BV(2)*0 + _BV(1)*0 + _BV(0)*0;
        else if (timer_source_tmp == TIMER_SOURCE_CLK)
            TCCR0B |= _BV(2)*0 + _BV(1)*0 + _BV(0)*1;
        else if (timer_source_tmp == TIMER_SOURCE_CLK_8)
            TCCR0B |= _BV(2)*0 + _BV(1)*1 + _BV(0)*0;
        else if (timer_source_tmp == TIMER_SOURCE_CLK_64)
            TCCR0B |= _BV(2)*0 + _BV(1)*1 + _BV(0)*1;
        else if (timer_source_tmp == TIMER_SOURCE_CLK_256)
            TCCR0B |= _BV(2)*1 + _BV(1)*0 + _BV(0)*0;
        else if (timer_source_tmp == TIMER_SOURCE_CLK_1024)
            TCCR0B |= _BV(2)*1 + _BV(1)*0 + _BV(0)*1;
        else if (timer_source_tmp == TIMER_SOURCE_EXT_FALL)
            TCCR0B |= _BV(2)*1 + _BV(1)*1 + _BV(0)*0;
        else if (timer_source_tmp == TIMER_SOURCE_EXT_RISE)
            TCCR0B |= _BV(2)*1 + _BV(1)*1 + _BV(0)*1;
        else
            timer0_source = timer_source_tmp; // timer has not started, restores
    } else if (timer_id == TIMER_ID_1) {
        // TODO:
    } else if (timer_id == TIMER_ID_3) {
        // TODO:
    }
}

void timer_stop(uint8_t timer_id) {
    uint8_t mode;
    if (timer_id == TIMER_ID_0) {
        // simply clears the source bit
        mode = TCCR0B & (_BV(2) | _BV(1) | _BV(0)); // Gets the status
        if (mode == 0x00)
            mode = TIMER_SOURCE_NONE;
        else if (mode == 0x01)
            mode = TIMER_SOURCE_CLK;
        else if (mode == 0x02)
            mode = TIMER_SOURCE_CLK_8;
        else if (mode == 0x03)
            mode = TIMER_SOURCE_CLK_64;
        else if (mode == 0x04)
            mode = TIMER_SOURCE_CLK_256;
        else if (mode == 0x05)
            mode = TIMER_SOURCE_CLK_1024;
        else if (mode == 0x06)
            mode = TIMER_SOURCE_EXT_FALL;
        else if (mode == 0x07)
            mode = TIMER_SOURCE_EXT_RISE;
        // else is impossible
        TCCR0B &= ~(_BV(2) | _BV(1) | _BV(0)); // Clears the bits
    } else if (timer_id == TIMER_ID_1) {
        // TODO: this
    } else if (timer_id == TIMER_ID_3) {
        // TODO: this
    }
}

// ===============================
// ===   POWERSAVE FUNCTIONS   ===
// ===============================

void timer_enable(uint8_t timer_id) {
    // TODO: this
}
void timer_disable(uint8_t timer_id) {

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
    }
    return old_comp;
}
