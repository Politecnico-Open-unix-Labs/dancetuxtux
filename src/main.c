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
/*
    If you enjoy this work and you would like to support this project,
    please, consider donating me some bitcoins
          ------ 1NFZcpPRPchSfWAHCBqXrKAzXKrYVHL9ca ------
    Any amount, also a single satoshi, is always well accepted!
    You will get fantastic updates!
*/

#include <stdint.h> // uint8_t
#include <limits.h>

#include <avr/interrupt.h> // ISR macro
#include "cpu.h" // All the needed to work with the microcontroller
#include "capacitive.h" // To read capacitive pins
#include "circular_buffer.h" // all circular_buffer features
#include "timer_utils.h" // Everything we need to work with timers
#include "USB.h" // Usb communication

// USB Arrow codes
#define KEY_UP_ARROW      82
#define KEY_DOWN_ARROW    81
#define KEY_LEFT_ARROW    80
#define KEY_RIGHT_ARROW   79

#include <avr/interrupt.h> // ISR macro
#include <avr/power.h> // Powersaving functions
#include <avr/sleep.h> // CPU Sleep modes (powersave)

#include "config.h" // Necessary configuration file (needs function definition)

#ifdef USE_ARDUINO_LED
#  define ARDUINO_LED_INIT() DDRC |= _BV(PORTC7)
#  define ARDUINO_LED_ON()   PORTC |= _BV(PORTC7)
#  define ARDUINO_LED_OFF()  PORTC &= ~(_BV(PORTC7))
    static uint8_t switchon_led = 0; // counter
#  define ONE_MORE_SWITCH() switchon_led++
#  define ONE_LESS_SWITCH() switchon_led--
#  define AT_LEAST_ONE_SWITCH() (switchon_led == 0)
#else // Defines the same macros, without any arg
#  define ARDUINO_LED_INIT()
#  define ARDUINO_LED_ON()
#  define ARDUINO_LED_OFF()
#  define ONE_MORE_SWITCH()
#  define ONE_LESS_SWITCH()
#  define AT_LEAST_ONE_SWITCH() 0 // Always false
#endif // USE_ARDUINO_LED

// ============================================
// ===   Workaround for Handler functions   ===
// ============================================

#define CREATE_PRESS_HANDLER(name)                                              \
        void handler_ ## name ## _keypress(void) {                              \
            /* Atual send key, Requires about 2ms to complete */                \
            __USB_send_keypress(KEY_ ## name ## _ARROW);                        \
            ARDUINO_LED_ON(); /* Switchs Arduino Led on */                      \
            ONE_MORE_SWITCH();                                                  \
        }                                                                       \
        void handler_ ## name ## _keypress(void)

#define CREATE_RELEASE_HANDLER(name)                                            \
        void handler_ ## name ## _keyrelease(void) {                            \
            /* Atual send key, Requires about 2ms to complete */                \
            __USB_send_keyrelease(KEY_ ## name ## _ARROW);                      \
            ONE_LESS_SWITCH(); /* Switches Arduino Led off */                   \
            if (AT_LEAST_ONE_SWITCH())                                          \
                ARDUINO_LED_OFF(); /* Switches Arduino Led off */               \
        }                                                                       \
        void handler_ ## name ## _keyrelease(void)

// All of the next macros will crerate the needed functions
CREATE_PRESS_HANDLER(UP);
CREATE_PRESS_HANDLER(DOWN);
CREATE_PRESS_HANDLER(LEFT);
CREATE_PRESS_HANDLER(RIGHT);

CREATE_RELEASE_HANDLER(UP);
CREATE_RELEASE_HANDLER(DOWN);
CREATE_RELEASE_HANDLER(LEFT);
CREATE_RELEASE_HANDLER(RIGHT);

// =============================
// ===   Free RAM in bytes   ===
// =============================

#include <alloca.h>
static inline int get_free_ram(void) {
    extern int __heap_start, *__brkval;
    uint8_t * v = alloca(sizeof(uint8_t)); // Allocates a sinlge byte on the top of the stack
    return (int)v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}

// Global timer status
volatile uint8_t has_timer_ticked = 0;
volatile uint8_t is_executing = 0;

ISR(TIMER1_COMPA_vect) {
    if (is_executing == 0) // Timer ticks only if execution has finished
        has_timer_ticked = 1;
    // else handle the error (increase delay)
}

__attribute__((noreturn)) // This function never ends
void fatal_error(void) {
    // Makes arduino led blink fast, forever.
    ARDUINO_LED_INIT();
    while (1) {
        ARDUINO_LED_ON();
        _delay_ms(100);
        ARDUINO_LED_OFF();
        _delay_ms(100);
    }
    __builtin_unreachable();
}

__attribute__((always_inline)) static inline // Forces function inlining
void main_loop(capacitive_sensor_t * sensors) {
    uint8_t stat, i; // status of sensors, counter
    handler_t handler; // Temporany handler function pointer
    
    _MemoryBarrier();
    is_executing = 1; // Now execution starts
    _MemoryBarrier(); // Forces order
    
    stat = capacitive_sensor_pressed(sensors, inputs_len); // Input len is a global
    for (i = 0; i < inputs_len; i++) {
        if (stat & _BV(i)) // pressed the i-th key
#ifdef USE_PROGMEM
            handler = pgm_read_ptr_near(keypress_handlers + i); // Retreive from progmem
#else
            handler = keypress_handlers[i]; // Retreive from normal array
#endif
        else
#ifdef USE_PROGMEM
            handler = pgm_read_ptr_near(keyrelease_handlers + i);
#else
            handler = keyrelease_handlers[i];
#endif

        if (handler != NULL) // If event handler is configuered
            handler(); // Executes the event handler
    }
    
    _MemoryBarrier(); // Forces order
    is_executing = 0; // Main loop execution finished
    _MemoryBarrier();
}

int main(void) {
    uint16_t timer_comparator = 64; // Increase to increase time
    capacitive_sensor_t sensors[inputs_len]; // One for each input

    cli();
    power_all_disable(); // Disables every device (saving power)

    if (get_free_ram() <= 2048) // If static variables uses too mutch memory
        fatal_error(); // Making the execution going on might have unfunny consequences  

    // Now does all the initializations
    __USB_power_enable(); // Switches on USB
    __USB_init(); // Automatically do all the needed to init usb
#ifdef USE_PROGMEM
    capacitive_sensor_inits_P(sensors, inputs, inputs_len); // Calls the PROGMEM version
#else
    capacitive_sensor_inits(sensors, inputs, inputs_len); // Calls the npn-PROGMEM version
#endif

    ARDUINO_LED_INIT(); // sets Arduino LED as output
    _MemoryBarrier(); // Forces executing r/w ops in order
    
    cli();

    // Now configures and starts the timer
    timer_enable(TIMER_ID_1); // enables the timer, this way it can start when done
    timer_init(TIMER_ID_1, // Timer settings
               TIMER_SOURCE_CLK_1024, // Sets cpu clock / 1024 tick frequency
               TIMER_MODE_CTC_OCR, // When reaches Compare A resets
               OUT_MODE_NORMAL_A | OUT_MODE_NORMAL_B); // Not used output
    timer_init_interrupt(TIMER_ID_1, TIMER_INTERRUPT_MODE_OCIA); // This enables interrupt, on compare A
    timer1_compare(TIMER_COMP_A, &timer_comparator); // sets compare A
    // Comparator is 64, Clock prescaler is 1024
    // 64*1024 = 65536 clock cycles between two consecutive calls. At 16Mhz it is about 4ms

    timer_start(TIMER_ID_1); // Now starts the timer!

    if (!is_timer_running(TIMER_ID_1)) { // It should never happen
        __USB_power_disable();
        timer_disable(TIMER_ID_1);
        fatal_error();
    }

    sei(); // Re-enables interrupts
    
    _MemoryBarrier();
    has_timer_ticked = 0;
    while (1) {
        // Main loop, where capacitive is done!
        main_loop(sensors); // N.B. sensors is a pointer

        // Interrupts should now be enabled, but for security re-enables again
        sei(); // Enables interrupts. We need interrupts to handle timers
        
        // Now buisy wait until the timer ticks
        while (has_timer_ticked == 0) {
            set_sleep_mode(SLEEP_MODE_IDLE); // Idle mode does not disable any interrupt
            sleep_enable(); // Enables the selected sleep mode
            sleep_mode(); // Control blocks here until an interrupt is executed
            sleep_disable(); // Control resumes here. It is important it is immediately after sleep_mode
            _MemoryBarrier();
            __builtin_avr_delay_cycles(64); // Delays some cycles (you may change it), this way calls the ISR
        }
        _MemoryBarrier(); // Forces next operation to be executed after
        has_timer_ticked = 0; // Resets
    }

    __builtin_unreachable(); // Control should never exit main
}

