#include "cpu.h" // All the needed to work with the microcontroller
#include "config.h" // Necessary configuration file
#include "capacitive.h" // To read capacitive pins

int main() {
    DDRC = _BV(7);
    _MemoryBarrier(); // Forces executing r/w ops in order

    inint_inputs();
    while (1) {
        if (check_port(8))
            PORTC |= _BV(7);
        else
            PORTC &= ~(_BV(7));
        discharge_ports();
    }
    return 0;
}
