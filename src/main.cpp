#define F_CPU 16000000 // Cpu frequency
#ifndef __AVR_ATmega32U4__
#    define __AVR_ATmega32U4__  // microcontroller
#endif

#include <avr/io.h>
#include <util/delay.h>
#include <avr/cpufunc.h>

int main() {
    DDRC = _BV(7);
    _MemoryBarrier(); // Forces executing r/w ops in order
    while (1) {
        PORTC |= _BV(7);
        _delay_ms(1000);
        PORTC &= ~(_BV(7));
        _delay_ms(1000);
    }
}
