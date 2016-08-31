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

#include "cpu.h" // All the needed to work with the microcontroller
#include "config.h" // Necessary configuration file
#include "capacitive.h" // To read capacitive pins
#include "circular_buffer.h" // all circular_buffer features
#include "USB.h" // Usb communication

// DEBUG PURPOSE
#include <stdio.h>
int ascii_to_usb(const uint8_t ch) {
    if (ch >= 'A' && ch <= 'Z') return ch - 'A' + 0x04; // Uppercase
    if (ch >= 'a' && ch <= 'z') return ch - 'a' + 0x04; // Lowercase
    if (ch >= '0' && ch <= '9') { if (ch == '0') return 0x27; else return ch - '1' + 0x1e;} // Digit
    if (ch == ' ') return 0x2c;
    return 0x00;
}

void __USB_send_string(const uint8_t * msg) {
    uint8_t i = 0;
    while (msg[i]) {
        __USB_send_keypress(ascii_to_usb(msg[i]));
        __USB_send_keyrelease(ascii_to_usb(msg[i]));
        i++;
    }
}

// Arrow codes
#define KEY_UP_ARROW      82
#define KEY_DOWN_ARROW    81
#define KEY_LEFT_ARROW    80
#define KEY_RIGHT_ARROW   79

// Lenght of a buffer containing SAMPLES_NUM samples
#define BUF_LEN CIRCULAR_BUFFER_BUF_LEN(SAMPLES_NUM)
// TODO: put everything together in a single structure, one for eahc buffer
uint8_t low_buffer_data[BUF_LEN];  // Actual buffer where data is stored
uint8_t high_buffer_data[BUF_LEN]; // Actual buffer where data is stored
circular_buffer_t low_buffer, high_buffer; // Buffer wrapper
uint8_t high_threeshold, low_threeshold; // between 0 and 255
uint8_t pin = 8; // TODO: correct initialization of this
uint8_t to_probe = 1; // Single bit TODO: use bitfield
uint8_t pressed = 0; // Single bit TODO: use bitfield
uint32_t hysteresis = 0; // Avoid double press
uint32_t gray_zone = 0; // TODO: initialize this in probe function; Time in spent gray zone

// TODO: pass buffer structure
#define BUFFER_NONE 0x00
#define BUFFER_HIGH 0x01
#define BUFFER_LOW  0x02
#define BUFFER_BOTH (BUFFER_LOW | BUFFER_HIGH)
void fill_buffer(uint8_t wich_buffer) {
    uint8_t i;

    for (i = 0; i < SAMPLES_NUM; i++) { // performs enough readings to fill the buffer
        if (wich_buffer & BUFFER_LOW) {
            set_threshold(low_threeshold); // Low threeshold must read as 1
            if (check_port(pin)) circular_buffer_push(low_buffer, 1);
            else                 circular_buffer_push(low_buffer, 0);
        }

        if (wich_buffer & BUFFER_HIGH) {
            set_threshold(high_threeshold); // Low threeshold must read as 0
            if (check_port(pin)) circular_buffer_push(high_buffer, 1);
            else                 circular_buffer_push(high_buffer, 0);
        }
    }
}

void set_press_release_threeshold() {
    uint8_t overflow; // In case of overflow sets min/max
    overflow = low_threeshold;
    low_threeshold -= RELEASE_THREESHOLD;
    if (low_threeshold > overflow) // Underflow (below 0)
        low_threeshold = 0;
    overflow = high_threeshold;
    high_threeshold += PRESS_THREESHOLD;
    if (high_threeshold < overflow) // Overflowed
        high_threeshold = UCHAR_MAX;
}

// +--- NB ---------------------------------------------+
// |   increasing threeshold leads to more 0 readings   |
// |   decreasing threeshold leads to more 1 readings   |
// +----------------------------------------------------+

// TODO: pass buffer structures
void adjust_interval() { // Moves threeshold up and down
    uint8_t high_done, low_done; // counters
    uint8_t high_dir, low_dir; // true if need to increase the threeshold

    // Preliminary fills the buffer
    fill_buffer(BUFFER_BOTH);
    if (circular_buffer_sum(low_buffer) <= SAMPLES_NUM*LOW_THREESHOLD) // must sum high
        low_dir = 0; // Should decrease threeshold to get an higher sum (more 1s)
    else
        low_dir = 1; // Should increase threeshold

    if (circular_buffer_sum(high_buffer) >= SAMPLES_NUM*HIGH_THREESHOLD) // must sum low
        high_dir = 1; // Should increase threeshold to get a lower sum (more 0s)
    else
        high_dir = 0; // Should decrease threeshold

    // Now phisically move the threesholds according to preliminary readings
    high_done = low_done = 0;
    while (!(low_done && high_done)) {
        if (!low_done) {
            // if probing threesholds increase
            if (low_dir == 1) low_threeshold++;
            if (low_dir == 0) low_threeshold--;
        }
        if (!high_done) {
            // if probing threesholds decrease
            if (high_dir == 1) high_threeshold++;
            if (high_dir == 0) high_threeshold--;
        }

        // Now collect some samples from the sensor
        fill_buffer((!low_done)*BUFFER_LOW | (!high_done)*BUFFER_HIGH);

        if (!low_done) {
            if (low_dir == 1) { // increasing low threeshold
                if (circular_buffer_sum(low_buffer) <= SAMPLES_NUM*LOW_THREESHOLD) { // must sum high
                    low_threeshold--; // low threeshold was too high, last stored was good
                    low_done = 1; // do not probe low anymore
                }
            } else if (low_dir == 0) { // decreasing low threeshold
                if (circular_buffer_sum(low_buffer) > SAMPLES_NUM*LOW_THREESHOLD) // must sum high
                    low_done = 1; // satisfied, do not probe low anymore
            }
        } // end if low_done

        if (!high_done) {
            if (high_dir == 0) { // decreasing low threeshold
                if (circular_buffer_sum(high_buffer) >= SAMPLES_NUM*HIGH_THREESHOLD) { // must sum low
                    high_threeshold++; // high threeshold was too low
                    high_done = 1; // do not probe high anymore
                }
            } else if (low_dir == 1) { // increasing low threeshold
                if (circular_buffer_sum(high_buffer) < SAMPLES_NUM*HIGH_THREESHOLD) // must sum low
                    high_done = 1; // do not probe high anymore
            }
        } // end if high_done

    } // end while
} // end function

// TODO: pass capacitive pin structure
void probe(void) { // Probes threeshold
    uint8_t lowest_high, highest_low; // highest and lowest reached by each threeshold
    uint8_t swap; // temporany
    uint32_t times;

    // Sets a General starting point, you can change it if you think performance will benefit
    // However this settings should not change the final output
    lowest_high = UCHAR_MAX; // Maximum
    highest_low = 0; // Minum
    high_threeshold = UCHAR_MAX*3/4;
    low_threeshold = UCHAR_MAX*1/4;

    // The first part of the algorithm is intented to give only an approximation onf the final result
    times = 0;
    while ((high_threeshold > (low_threeshold + 1)) && (times < MAX_PROBE_STEPS)) {
        times++;
        set_threshold(low_threeshold); // Low threeshold must read as 1
        if (check_port(pin)) { // Read correct value, tries increasing
            highest_low = low_threeshold;
            low_threeshold += (high_threeshold - low_threeshold) / 2;
        } else { // Read wrong value, tries decreasing
            low_threeshold = (low_threeshold + highest_low) / 2; // average, rounded down
        }

        set_threshold(high_threeshold); // High threeshold must read as 0
        if (check_port(pin)) { // Read wrong value, tries decreasing
            high_threeshold = (high_threeshold + lowest_high + 1) / 2; // average, rounded up
        } else { // Read correct value, tries increasing
            lowest_high = high_threeshold;
            high_threeshold += (high_threeshold - low_threeshold) / 2;
        }

        if (high_threeshold <= low_threeshold) {
            swap = (low_threeshold - high_threeshold + 1) / 2; // high, roundup
            low_threeshold  = (low_threeshold - high_threeshold - 1) / 2; // low, rounddown
            high_threeshold = swap; // because first operation changes data the second uses
        }
    }

    adjust_interval(); // Now fine adjusting
    set_press_release_threeshold(); // Now sets some sensibility
    to_probe = 0;
}

// TODO: pass capacitive pin structure
// Here is done all the Inttelligent work. This function checks the history buffers
// to say wether the key was pressed or not.
int capacitive_pressed() {
    uint8_t temp;

    if (to_probe) {
        probe(); // Probe function will automatically fill buffer
        to_probe = 0;
        gray_zone = 0; // no more in grayzone
    }

    fill_buffer(BUFFER_BOTH); // Fills buffer of readings

    temp = circular_buffer_sum(low_buffer);
    if (temp <= SAMPLES_NUM*(0.10)) { // Key release, sends keyrelease and probes again
        hysteresis++;
        if (hysteresis > HYSTERESIS) { // Actually send keyrelease
            pressed = 0;
            to_probe = 1;
            gray_zone = 0;
            hysteresis = 0;
        }
    } else if (temp <= SAMPLES_NUM*LOW_THREESHOLD) {
        gray_zone++; // it is not a keypress, but it is near to a keypress
        if (gray_zone >= MAX_TIME_IN_GRAYZONE) // Spent too many time in grayzone, request re-probe without sending keypress
            to_probe = 1;
    } else {
        gray_zone = 0;
    }

    temp = circular_buffer_sum(high_buffer);
    if (temp >= SAMPLES_NUM*(0.90)) { // Key press, send kaypress and probes again sensibility
        pressed = 1;
        hysteresis = 0;
        to_probe = 1;
        gray_zone = 0;
    } else if (temp >= SAMPLES_NUM*HIGH_THREESHOLD) { // gray_zone
        gray_zone++; // it is not a keypress, but it is near to a keypress
        if (gray_zone >= MAX_TIME_IN_GRAYZONE) // Spent too many time in grayzone, request re-probe without sending keypress
            to_probe = 1;
    } else { // Everything normal
        gray_zone = 0;
    }

    return pressed;
}

int main(void) {
    __USB_init();
    inint_inputs(inputs, inputs_len); // Inits capacitive inputs
    circular_buffer_init(low_buffer , low_buffer_data , SAMPLES_NUM);
    circular_buffer_init(high_buffer, high_buffer_data, SAMPLES_NUM);
    probe(); // chooses correct threeshold

    DDRC |= _BV(7); // output
    _MemoryBarrier(); // Forces executing r/w ops in order

    while (1) {
        if (capacitive_pressed()) {
            __USB_send_keypress(KEY_UP_ARROW); // Requires about 2ms to complete
            PORTC |= _BV(7); // Switchs Arduino Led on
        } else {
            __USB_send_keyrelease(KEY_UP_ARROW); // Requires abotu 2ms to complete
            PORTC &= ~(_BV(7)); // Switches Arduino Led off
        }

        _delay_ms(10); // TODO: use timer interrupts instead
    }
    return 0;
}
