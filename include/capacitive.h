#ifndef CAPACITIVE_H
#define CAPACITIVE_H

#include <stdint.h> // uint8_t

/* Inits pin to use as capacitive sensor */
void inint_inputs(const uint8_t inputs[], const uint8_t inputs_len);

/* Should call after each round of capacitive reads (TODO: automatic process?) */
void discharge_ports(void);

/* Capacitive read function,
   returns 1 if there is contact, 0 if not, and -1 on errors */
unsigned char check_port(uint8_t in);
void set_sensibility(uint8_t new_sens);

#endif // CAPACITIVE_H defined
