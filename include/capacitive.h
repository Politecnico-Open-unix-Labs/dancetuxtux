#ifndef CAPACITIVE_H
#define CAPACITIVE_H

#include <stdint.h> // uint8_t

void inint_inputs(void); // Sets some useful bitmask
void discharge_ports(void); // Must call after each reading
unsigned char check_port(uint8_t in); // Capacitive read function

#endif // CAPACITIVE_H defined
