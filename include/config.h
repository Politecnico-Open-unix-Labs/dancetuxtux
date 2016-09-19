#ifndef CONFIG_H
#define CONFIG_H

#define USE_ARDUINO_LED // Each keypress will light the Arduino led
#define USE_PROGMEM // Store globals inside executable flash rather than RAM
#define SAMPLES_PER_SECOND 244 // Number of times per second a keypress is checked

#define INPUT_PIN_UP    8
#define INPUT_PIN_DOWN  9
#define INPUT_PIN_LEFT  10
#define INPUT_PIN_RIGHT 11

// Do not change the name of the following variables
// You should not include this file outside of main.cpp

#ifdef USE_PROGMEM
#    include <avr/pgmspace.h> // PROGMEM stuff
#else
#    ifdef PROGMEM
#        undef PROGMEM
#    endif
#    define PROGMEM // Defines progmem to do nothing!
#endif

// Array of input pins
const uint8_t inputs[] PROGMEM = {INPUT_PIN_UP, INPUT_PIN_DOWN, INPUT_PIN_LEFT, INPUT_PIN_RIGHT};
const uint8_t inputs_len PROGMEM = sizeof(inputs)/sizeof(*inputs); // number of inputs

// Must delcare functions before using
void handler_UP_keypress(void);       // All of those functions
void handler_DOWN_keypress(void);     // Must be defined inside the main program
void handler_LEFT_keypress(void);
void handler_RIGHT_keypress(void);
void handler_UP_keyrelease(void);
void handler_DOWN_keyrelease(void);
void handler_LEFT_keyrelease(void);
void handler_RIGHT_keyrelease(void);

// Function Handlers. Each time an event is detected on the pin the correspunding handler is called
// N.B. Order is important! Different order will associate different handlers to different pins
// You can set some handler to NULL if you don't want any function call for that event
typedef void (*handler_t)(void); // Pointer to function rquiring no args and retuirning void
const handler_t keypress_handlers[] PROGMEM = {&handler_UP_keypress  , &handler_DOWN_keypress ,
                                               &handler_LEFT_keypress, &handler_RIGHT_keypress, };
const handler_t keyrelease_handlers[] PROGMEM = {&handler_UP_keyrelease  , &handler_DOWN_keyrelease ,
                                                 &handler_LEFT_keyrelease, &handler_RIGHT_keyrelease, };

#ifndef USE_PROGMEM
#    undef PROGMEM
#endif

#endif // CONFIG_H defined
