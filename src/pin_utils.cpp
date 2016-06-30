#include <stdint.h> // uint8_t
#include <stddef.h> // NULL ptr

#include "cpu.h" 
#include "pin_utils.h" // pin_t

/* Converts arduino pin id to pin_t structure */
pin_t id_to_pin(uint8_t id) {
    pin_t ret_val;

    switch (id) {
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
