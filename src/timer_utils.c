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

#include <stdint.h> // uint8_t
#include <stddef.h> // NULL pointer

#include "cpu.h" // processor adress registers
#include "timer_utils.h" // timer defines

void init_timer(const uint8_t timer_id, const uint8_t source, const uint8_t mode) {
    // TODO: 
}

// ==============================================================================
// ===== get/set timer counter, i.e. number of tick counted by the timer   ======
// ==============================================================================

uint8_t timer_count0(const uint8_t new_count[]) { // The array makes the pointer constant
    uint8_t cnt;
    cnt = TCNT0; // Reads the ticks
    if (new_count) // Replaces the counter
       TCNT0 = new_count[0];
    return cnt; // returns the old value
}

uint16_t timer_count1(const uint16_t new_count[]) {
    uint16_t cnt;
    cnt = TCNT1;
    if (new_count)
        TCNT1 = new_count[0];
    return cnt;
}

uint16_t timer_count3(const uint16_t new_count[]) {
    uint16_t cnt;
    cnt = TCNT3;
    if (new_count)
        TCNT3 = new_count[0];
    return cnt;
}


// =======================================================================
// ===== get/set compare values, comparation is done automatically  ======
// =======================================================================

uint8_t timer_compare0(uint8_t comp_id, const uint8_t new_val[]) {
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

uint16_t timer_compare1(uint8_t comp_id, const uint16_t new_val[]) {
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

uint16_t timer_compare3(uint8_t comp_id, const uint16_t new_val[]) {
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
