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

#include <stdint.h> // uint8_t type
#include <stddef.h> // NULL pointer

#include "cpu.h" // Almost all the needed to work with AVR controller
#include "capacitive.h" // function defined in this code
#include "pin_utils.h" // id_to_pin function
#include "settings.h" // capacitive settings

// Note: inline will not work between multiple files unless LTO is enabled (compile with -flto)
static uint8_t portb_bitmask, portc_bitmask, portd_bitmask, portf_bitmask; // input bitmask (1 input, 0 no input)
static uint8_t portb_used_mask, portc_used_mask, // automagic discharge when necessary 
               portd_used_mask, portf_used_mask;
               

/* check pin function */
static inline uint8_t check_pin(pin_t pin) __attribute__((always_inline));

void inint_inputs(const uint8_t inputs[], const uint8_t inputs_len)
{
    pin_t temp; // temporany, to switch pins
    uint8_t i; // counter
    
    // Automatically sets up input ports bitmask
    portb_bitmask = portc_bitmask = portd_bitmask = portf_bitmask = 0; // sets all to zero
    for (i = 0; i < inputs_len; i++) { // for each input
        temp = id_to_pin(inputs[i]);
        if (temp.port == &PORTB)
            portb_bitmask |= temp.bitmask;
        if (temp.port == &PORTC)
            portc_bitmask |= temp.bitmask;
        if (temp.port == &PORTD)
            portd_bitmask |= temp.bitmask;
        if (temp.port == &PORTF)
            portf_bitmask |= temp.bitmask;
    }

    discharge_ports(); // complete the setup by making ports readable
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
    
    // ports are now usable
    portb_used_mask = portc_used_mask = portd_used_mask = portf_used_mask = 0;
}

uint8_t check_port(uint8_t in)
{
    pin_t pin; // comfortable pin structure
    uint8_t ret_val; // return value
    uint8_t old_SREG; // to store interrupt configuration
    uint8_t * tmp_bitmask, * tmp_used_bitmask;
    
    pin = id_to_pin(in); // Gets an usable pin data structure

    // Safety code: check the pin is enabled for capacitive
    if (pin.port == &PORTB) {
        tmp_bitmask = &portb_bitmask;
        tmp_used_bitmask = &portb_used_mask;
    } else if (pin.port == &PORTC) {
        tmp_bitmask = &portc_bitmask;
        tmp_used_bitmask = &portc_used_mask;
    } else if (pin.port == &PORTD) {
        tmp_bitmask = &portd_bitmask;
        tmp_used_bitmask = &portd_used_mask;
    } else if (pin.port == &PORTF) {
        tmp_bitmask = &portf_bitmask;
        tmp_used_bitmask = &portf_used_mask;
    } else { // Should never happen
        tmp_bitmask = NULL; // This will lead to an error
        tmp_used_bitmask = NULL;
    }

    if ((tmp_bitmask == NULL) // pin does not exists
            || !(*tmp_bitmask & pin.bitmask)) // pin not enabled
        return -1; // Cannot use this port for capacitive sensor, error

    // Now check if can read the port
    if ((tmp_used_bitmask == NULL) // This check might be unuseful
            || (*tmp_used_bitmask & pin.bitmask)) // if port was used
        discharge_ports(); // Cannot read data after use without a prior reset
                           // so reset here

    // Now we are ready to actually read
    _MemoryBarrier();
    old_SREG = SREG; // Stores interrupt configuration
    _MemoryBarrier();
    SREG = 0; // disables interrupts for a while
    _MemoryBarrier(); // Do not go on until SREG is zero
    ret_val = check_pin(pin); // time critical section, readng
    SREG = old_SREG; // re-enable interrupts (if enabled)

    // sets the port as used
    if (tmp_used_bitmask != NULL)
        *tmp_used_bitmask |= pin.bitmask;

    return !!ret_val; // binary return, or 1, or 0
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
