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
/*
    If you enjoy this work and you would like to support this project,
    please, consider donating me some bitcoins
          ------ 1NFZcpPRPchSfWAHCBqXrKAzXKrYVHL9ca ------
    Any amount, also a single satoshi, is always well accepted!
    You will get fantastic updates!
*/

#include <stdint.h> // uint8_t type
#include <string.h> // memcpy, memmmove
#include <stddef.h> // NULL pointer
#include <avr/pgmspace.h>

#include "cpu.h" // Almost all the needed to work with AVR controller
#include "capacitive.h" // function defined in this code
#include "pin_utils.h" // id_to_pin function
#include "capacitive_settings.h" // capacitive settings

// Note: inline will not work between multiple files unless LTO is enabled (compile with -flto)
static uint8_t portb_bitmask, portc_bitmask, portd_bitmask, portf_bitmask; // input bitmask (1 input, 0 no input)
static uint8_t portb_used_mask, portc_used_mask, // automagic discharge when necessary
               portd_used_mask, portf_used_mask;
static uint8_t _threshold;

static inline
void can_now_use_pin(pin_t pin) {
    if (pin.port == &PORTB)
        portb_used_mask &= (~pin.bitmask); // Clears only that bit
    else if (pin.port == &PORTC)
        portc_used_mask &= (~pin.bitmask); // Clears only that bit
    else if (pin.port == &PORTD)
        portd_used_mask &= (~pin.bitmask); // Clears only that bit
    else if (pin.port == &PORTF)
        portf_used_mask &= (~pin.bitmask); // Clears only that bit
}

#ifdef USE_DISCHARGE_TIMERS
#   include "timer_utils.h" // many timer functions
#   include <util/atomic.h> // Atomic functions (i.e. disabling interrupts)
#   include <avr/interrupt.h> // ISR macro

    static uint16_t waiting_ocra[MAX_PORT_NUM]; // Will hold Compare register for each port
    static pin_t waiting_pin_no[MAX_PORT_NUM - 1]; // Holds pin number
    static pin_t last_pin; // Holds last pin
    static uint8_t waiting = 0; // Number of pin waiting

    // TIMER ISR
    ISR(TIMER3_COMPA_vect) {
        uint16_t new_compare;

        timer_stop(TIMER_ID_3); // Stops the timer for a while
        if (waiting != 0) {
            new_compare = timer3_compare(TIMER_COMP_A, NULL); // Gers current compare
            new_compare -= waiting_ocra[0]; // USes the first aviable
            timer3_compare(TIMER_COMP_A, &new_compare); // Sets new_compare
            can_now_use_pin(last_pin);
            // Moves data
            last_pin = waiting_pin_no[0];
            waiting--; // One less data in the queue
            memmove(waiting_ocra, waiting_ocra + 1, sizeof(*waiting_ocra)*waiting);
            memmove(waiting_pin_no, waiting_pin_no + 1, sizeof(*waiting_pin_no)*waiting);
            timer_start(TIMER_ID_3); // Restarts the timer
        } else {
            // No more waiting pins, stops the timer
            // (timer is stopped, do not restart!)
            can_now_use_pin(last_pin);
        }
    }

#endif // USE_DISCHARGE_TMERS not defined

/* check pin low level function */
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

    set_threshold(START_THRESHOLD);
    discharge_ports(); // complete the setup by making ports readable
}

// PROGMEM version of the initialization function
void inint_inputs_P(const uint8_t inputs[], const uint8_t inputs_len)
{
    pin_t temp; // temporany, to switch pins
    uint8_t i; // counter

    // Automatically sets up input ports bitmask
    portb_bitmask = portc_bitmask = portd_bitmask = portf_bitmask = 0; // sets all to zero
    for (i = 0; i < inputs_len; i++) { // for each input
        temp = id_to_pin(pgm_read_byte_near(inputs + i));
        if (temp.port == &PORTB)
            portb_bitmask |= temp.bitmask;
        if (temp.port == &PORTC)
            portc_bitmask |= temp.bitmask;
        if (temp.port == &PORTD)
            portd_bitmask |= temp.bitmask;
        if (temp.port == &PORTF)
            portf_bitmask |= temp.bitmask;
    }

    set_threshold(START_THRESHOLD);
    discharge_ports(); // complete the setup by making ports readable
}

// threshold functions
void set_threshold(uint8_t new_sens) { _threshold = new_sens; }
uint8_t get_threshold(void) { return _threshold; }

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
    _MemoryBarrier();
}

uint8_t check_port(uint8_t in)
{
    pin_t pin; // comfortable pin structure
    uint8_t ret_val; // return value
    uint8_t old_SREG; // to store interrupt configuration
    uint8_t * tmp_bitmask, * tmp_used_bitmask;

#ifdef USE_DISCHARGE_TIMERS
    static const uint16_t timer_comparator = ((double)DISCHARGE_TIME/1E6 * F_CPU)/1024.0; // 1024 = prescaler!
    static uint8_t timer_initialized = 0;
    uint16_t last_timer_read;
    
    if (timer_initialized == 0) { // Executes this only one time
        // Inits timer 3
        timer_enable(TIMER_ID_3);
        timer_init(TIMER_ID_3, TIMER_SOURCE_CLK_1024, // Sets cpu clock / 1024 tick frequency
                   TIMER_MODE_CTC_OCR, // When reaches Compare A resets
                   OUT_MODE_NORMAL_A | OUT_MODE_NORMAL_B); // Not used output
        timer_init_interrupt(TIMER_ID_3, TIMER_INTERRUPT_MODE_OCIA); // This enables interrupt, on compare A
        // N.B. timer is now STOPPED
        timer_initialized = 1; // All done for now.
    }
#endif

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
            || (*tmp_used_bitmask & pin.bitmask)) // if port was used recently
        discharge_ports(); // Cannot read data after use without a prior reset
                           // so reset here, now can use the port

    // Now we are ready to actually read
    _MemoryBarrier();
    old_SREG = SREG; // Stores interrupt configuration
    SREG = 0; // disables interrupts for a while

    ret_val = check_pin(pin); // time critical section, readng

    SREG = old_SREG; // re-enable interrupts (if enabled)

    // marks the port as used
    if (tmp_used_bitmask != NULL)
        *tmp_used_bitmask |= pin.bitmask;

#ifdef USE_DISCHARGE_TIMERS
    // Now starts a timer to discharge the port
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        if (is_timer_running(TIMER_ID_3)) { // Someone else is waiting for a discharge
        // Writes in the first free block, wich is waiting id
            if (waiting < MAX_PORT_NUM) { // Everything OK
                uint16_t tmp_read = timer3_count(NULL);
                _MemoryBarrier();
                waiting_ocra[waiting] = tmp_read - last_timer_read; // Difference with last read
                last_timer_read = tmp_read; // Now this is last read
                waiting_pin_no[waiting] = pin; // Waiting pin
                waiting++;
            } else {
                // Cannot store that pin.
                // Simply do nothing, the discharge_ports function
                // will take care of this case automatically (but inefficently)
            }
        } else {
            waiting = 0; // One pin waiting (well, the 0 might be confusing)
            last_timer_read = 0;
            last_pin = pin; // Current pin
            timer3_compare(TIMER_COMP_A, &timer_comparator); // sets compare A
            _MemoryBarrier(); // Firs compare, then timer start
            timer_start(TIMER_ID_3); // Starts the timer and waits for the interrupt
        } // end if
    } // end atomic
#endif

    return !!ret_val; // binary return, or 1, or 0
}

// ====== ALL THE 'HARD WORK' IS DONE HERE ======
// ====== WARNING: DO NOT REMOVE CAP_DISCHARGE(); FUNCTION =====

// N.B. _MemoryBarrier function forces r/w to follow the order. It should not slow down execution
#define CAP_CHARGHE(_pin) do {                                                                                      \
    _MemoryBarrier();                                                                                               \
    *(_pin.ddr) &= ~(_pin.bitmask); /* Make the port an input (connect internal resistor) */                        \
    _MemoryBarrier();                                                                                               \
    *(_pin.port) |= _pin.bitmask; /* writes 1 on the pin (port is a pull-up). Capacitor charging starts now */      \
    _MemoryBarrier();                                                                                               \
} while (0)

/* Performs the reading */
#define CAP_READ(_pin) (*(_pin.pin) & _pin.bitmask) //  N.B Keep _pin on the local stack, this way gains in speed

/* discharge port, it is important to leave the pis low if you want to do multiple readings
   discharge port function should do the same thing on all the pins, but it is safer to do it just after reading */
#define CAP_DISCHARGHE(_pin) do {                                                                                  \
    _MemoryBarrier(); /* WARNING: Keep the order of the following two */                                            \
    *(_pin.port) &= ~(_pin.bitmask); /* wirtes 0 */                                                                 \
    _MemoryBarrier();                                                                                               \
    *(_pin.ddr) |= _pin.bitmask; /* port is now an output (disconnect internal resistor) */                         \
    _MemoryBarrier();                                                                                               \
} while (0)

// Actual implementation
#define CAP_HARD_WORK(pin, delay) do {                                                                              \
    uint8_t read, _count = _threshold / 3;                                                                          \
    pin_t temp;  /* Needet to keep a copy of param on local stack. This way gains a faster access */                \
    memcpy(&temp, &pin, sizeof(pin)); /* DO NOT REMOVE THIS, faster access is necessary for fast reading */         \
    CAP_CHARGHE(temp);                                                                                              \
    __builtin_avr_delay_cycles(delay);  /* Waits some nops */                                                       \
    _delay_loop_1(_count); /* waits the rest of the time. Each loop requires 3 instructions */                      \
    read = CAP_READ(temp); /* here local stack reduces asm overhead, i.e. overhead is only given by loops */        \
    CAP_DISCHARGHE(temp);                                                                                           \
    return !read;                                                                                                   \
} while (0)

#define CAP_HARD_WORK_NO_LOOP(pin, delay) do {                                                                      \
    uint8_t read;                                                                                                   \
    pin_t temp;  /* Needet to keep a copy of param on local stack. This way gains a faster access */                \
    memcpy(&temp, &pin, sizeof(pin)); /* DO NOT REMOVE THIS, faster access is necessary for fast reading */         \
    CAP_CHARGHE(pin);                                                                                               \
    __builtin_avr_delay_cycles(delay);  /* Waits some nops */                                                       \
    read = CAP_READ(temp); /* here local stack reduces asm overhead, i.e. overhead is only given by loops */        \
    CAP_DISCHARGHE(pin);                                                                                            \
    return !read;                                                                                                   \
} while (0)

// Do not inline the following function !!! They work because the compiler will not mix their code with the rest
__attribute__((noinline)) static uint8_t hard_work_0(pin_t pin) { CAP_HARD_WORK(pin, 0); }
__attribute__((noinline)) static uint8_t hard_work_1(pin_t pin) { CAP_HARD_WORK(pin, 1); }
__attribute__((noinline)) static uint8_t hard_work_2(pin_t pin) { CAP_HARD_WORK(pin, 2); }
__attribute__((noinline)) static uint8_t hard_work_no_loop_0(pin_t pin) { CAP_HARD_WORK_NO_LOOP(pin, 0); }
__attribute__((noinline)) static uint8_t hard_work_no_loop_1(pin_t pin) { CAP_HARD_WORK_NO_LOOP(pin, 1); }
__attribute__((noinline)) static uint8_t hard_work_no_loop_2(pin_t pin) { CAP_HARD_WORK_NO_LOOP(pin, 2); }

// Implements three times the function, each time with a different delay
static inline // this function may be inlined
uint8_t check_pin(pin_t pin) {
    uint8_t read;
    uint8_t _algorithm = _threshold % 3;

    if (_threshold < 3) { //tis implies (_threshold / 3) == 0, that will cause '_count' starts from 0.
        // when calling _delay_loop_1 with 0 parameter it will actually loops 256 times
        // So the solution is to not call the _delay_loop_1 when it will cause a bug
        if (_algorithm == 0) // Use algorithm 0
            read = hard_work_no_loop_0(pin);
        else if (_algorithm == 1) // Yses algorithm 1
            read = hard_work_no_loop_1(pin);
        else if (_algorithm == 2) // Yses algorithm 2
            read = hard_work_no_loop_2(pin);
    } else {
        if (_algorithm == 0) // Use algorithm 0
            read = hard_work_0(pin);
        else if (_algorithm == 1) // Yses algorithm 1
            read = hard_work_1(pin);
        else if (_algorithm == 2) // Yses algorithm 2
            read = hard_work_2(pin);
    }

    return read;
}
