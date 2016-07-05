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

#include "cpu.h" // processor adress registers
#include "timer_utils.h" // timer types

void timer_set_compare0(uint8_t count_id, uint8_t new_val) {
    if (comp_id == 0)
        OCR0A = new_val;
    else
        OCR0B = new_val;
}

void timer_set_compare1(uint8_t count_id, uint16_t new_val) {
    if (comp_id == 0)
        OCR1A = new_val;
    else
        OCR1B = new_val;
}

void timer_set_compare3(uint8_t count_id, uint8_t new_val) {
    if (comp_id == 0)
        OCR3A = new_val;
    else
        OCR3B = new_val;
}
