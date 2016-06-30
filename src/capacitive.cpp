/*
    Capacitive low level functions
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

#include "cpu.h" // Almost all the needed to work with AVR controller
#include "capacitive.h" // function defined in this code
#include "settings.h" // capacitive settings
#include <stdint.h> // uint8_t type
#include <stddef.h> // NULL pointer

// Declared in config.h, included in main. Do not include config.h here
extern const uint8_t inputs[];
extern const uint8_t inputs_len;

static uint8_t portb_bitmask, portc_bitmask, portd_bitmask, portf_bitmask; // input bitmask (1 input, 0 no input)

typedef struct {
    volatile uint8_t * port, * ddr, * pin; // pointers to port variable
    uint8_t bitmask; // bitmask for that port
} pin_t;

static inline uint8_t check_pin(pin_t pin) __attribute__((always_inline)); // Some utils
static inline pin_t pin_to_port(uint8_t pin) __attribute__((always_inline)); // always_inline forces inlining

// Maybe add this into an util.cpp file ?
static inline pin_t pin_to_port(uint8_t pin) { // Arduino pin to atmega port id
    pin_t ret_val;

    switch (pin) {
        // Pins connected to PORTB
        case 8:  ret_val.bitmask = _BV(4); ret_val.port = &PORTB; break;
        case 9:  ret_val.bitmask = _BV(5); ret_val.port = &PORTB; break;
        case 10: ret_val.bitmask = _BV(6); ret_val.port = &PORTB; break;
        case 11: ret_val.bitmask = _BV(7); ret_val.port = &PORTB; break;   
        // Pins mapped on PORTC
        case 5:  ret_val.bitmask = _BV(6); ret_val.port = &PORTC; break;
        case 13: ret_val.bitmask = _BV(7); ret_val.port = &PORTC; break;
        // Pins mapped on PORTD
        case 3:  ret_val.bitmask = _BV(0); ret_val.port = &PORTD; break;
        case 2:  ret_val.bitmask = _BV(1); ret_val.port = &PORTD; break;
        case 0:  ret_val.bitmask = _BV(2); ret_val.port = &PORTD; break;
        case 1:  ret_val.bitmask = _BV(3); ret_val.port = &PORTD; break;
        case 4:  ret_val.bitmask = _BV(4); ret_val.port = &PORTD; break;
        case 12: ret_val.bitmask = _BV(6); ret_val.port = &PORTD; break;
        case 6:  ret_val.bitmask = _BV(7); ret_val.port = &PORTD; break;    
        // Pins mapped on PORTF (A0, A1, ... respectively 14, 15, ...)
        case 19: ret_val.bitmask = _BV(0); ret_val.port = &PORTF; break;
        case 18: ret_val.bitmask = _BV(1); ret_val.port = &PORTF; break;
        case 17: ret_val.bitmask = _BV(4); ret_val.port = &PORTF; break;
        case 16: ret_val.bitmask = _BV(5); ret_val.port = &PORTF; break;
        case 15: ret_val.bitmask = _BV(6); ret_val.port = &PORTF; break;
        case 14: ret_val.bitmask = _BV(7); ret_val.port = &PORTF; break;
        default: // Non standard pins
            ret_val.port = NULL;
            ret_val.ddr  = NULL;
            ret_val.pin  = NULL;
            ret_val.bitmask = 0;
    }

    // Adjust all the other port settings
    if (ret_val.port == &PORTB) {
        ret_val.ddr  = &DDRB;
        ret_val.pin  = &PINB;
    } else if (ret_val.port == &PORTC) {
        ret_val.ddr  = &DDRC;
        ret_val.pin  = &PINC;
    } else if (ret_val.port == &PORTD) {
        ret_val.ddr  = &DDRD;
        ret_val.pin  = &PIND;
    } else if (ret_val.port == &PORTF) {
        ret_val.ddr  = &DDRF;
        ret_val.pin  = &PINF;
    }

    return ret_val;
}

// Note: inline will not work between multiple files unless LTO is enabled (compile with -flto)
void inint_inputs(void)
{
    pin_t temp; // temporany, to switch pins
    uint8_t i; // counter
    
    // Automatically sets up input ports bitmask
    portd_bitmask = portb_bitmask = portc_bitmask = portf_bitmask = 0; // sets all to zero
    for (i = 0; i < inputs_len; i++) { // for each input
        temp = pin_to_port(inputs[i]);
        if (temp.port == &PORTB)
            portb_bitmask |= temp.bitmask;
        if (temp.port == &PORTC)
            portc_bitmask |= temp.bitmask;
        if (temp.port == &PORTD)
            portd_bitmask |= temp.bitmask;
        if (temp.port == &PORTF)
            portf_bitmask |= temp.bitmask;
    }
}

void discharge_ports(void)
{ 
    // Write 0 on the keyboard pins
    PORTB &= ~(portb_bitmask);
    PORTC &= ~(portc_bitmask);
    PORTD &= ~(portd_bitmask);
    PORTF &= ~(portf_bitmask);
    _MemoryBarrier();

    // and make output the input (to remove internal resistance)
    DDRB |= portb_bitmask;
    DDRC |= portc_bitmask;
    DDRD |= portd_bitmask;
    DDRF |= portf_bitmask;
    _MemoryBarrier();

    // wait some time, this way the capacitor connected get discharged
    _delay_us(DISCHARGE_TIME);
}

unsigned char check_port(uint8_t in)
{
    pin_t pin;
    uint8_t ret_val; // return value
    uint8_t tmp_bitmask, old_SREG = SREG;
    
    pin = pin_to_port(in); // Gets an usable pin data structure
    
    // Safety code: check the pin is enabled for capacitive
    if (pin.port == &PORTB)
        tmp_bitmask = portb_bitmask;
    else if (pin.port == &PORTC)
        tmp_bitmask = portc_bitmask;
    else if (pin.port == &PORTD)
        tmp_bitmask = portd_bitmask;
    else if (pin.port == &PORTF)
        tmp_bitmask = portf_bitmask;
    else // Should never happen
        tmp_bitmask = 0; // This will lead to an error
    
    if (!(tmp_bitmask & pin.bitmask))
        return -1; // Error, port was not enabled for capacitive

    // ports should be already discharged
    SREG = 0; // disables interrupts for a while
    _MemoryBarrier(); // Do not go on until SREG is zero
    ret_val = check_pin(pin); // time critical section
    SREG = old_SREG; // re-enable interrupts (if enabled)

    return ret_val;
}

// ====== ALL THE HARD WORK IS DONE HERE ======
static inline uint8_t check_pin(pin_t pin)
{
    uint8_t ret_val;
    
    // _MemoryBarrier function forces r/w to follow the order. It should not slow down execution
    *(pin.ddr) &= ~(pin.bitmask); // Make the port an input (connect internal resistor)
    _MemoryBarrier();
    *(pin.port) |= pin.bitmask; // writes 1 on the pin (port is a pull-up). Capacitor charging starts now
    _MemoryBarrier();

    // Now have to wait some time
    _delay_us(1); // TODO: use timers instead

    if (*(pin.pin) & pin.bitmask) // if pin is HIGH key was not pressed
        ret_val = 0;
    else // if pin is LOW key was pressed
        ret_val = 1;

    // discharge port, it is important to leave the pis low if you want to do multiple readings
    // discharge port function should do the same thing, but it is safer to do it also here
    _MemoryBarrier();
    *(pin.port) &= ~(pin.bitmask); // wirtes 0
    _MemoryBarrier();
    *(pin.ddr) |= pin.bitmask; // port is now an output (disconnect internal resistor)
    _MemoryBarrier();

    return ret_val;
}
