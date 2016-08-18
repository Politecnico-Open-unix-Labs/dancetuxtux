/*
    DanceTuxTux Board Project
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

#include "cpu.h" // All the needed to work with the microcontroller
#include "config.h" // Necessary configuration file
#include "capacitive.h" // To read capacitive pins

int main() {

    DDRC = _BV(7); // output
    _MemoryBarrier(); // Forces executing r/w ops in order

    inint_inputs(inputs, inputs_len);
    while (1) {
        if (check_port(8))
            PORTC |= _BV(7);
        else
            PORTC &= ~(_BV(7));
        discharge_ports();
    }
    return 0;
}
