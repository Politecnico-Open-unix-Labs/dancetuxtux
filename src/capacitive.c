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

#include <stdint.h> // uint8_t, uint32_t
#include <string.h> // memset
#include <avr/pgmspace.h> // PROGMEM

#include "cpu.h"
#include "capacitive.h" // capacitive_pin_t definition
#include "circular_buffer.h" // this code makes heavy use of circular buffers
#include "capacitive_settings.h" // All the settings

#define BUFFER_NONE 0x00
#define BUFFER_HIGH 0x01
#define BUFFER_LOW  0x02
#define BUFFER_BOTH (BUFFER_LOW | BUFFER_HIGH)
static inline void fill_buffer(const capacitive_sensor_ptr_t sensors, const uint8_t * const wich_buffer, const uint8_t num) {
    uint8_t i, sensor_id; // counters
    uint8_t ckres; // result of check_port

    for (i = 0; i < SAMPLES_NUM; i++) { // performs enough readings to fill the buffer
        for (sensor_id = 0; sensor_id < num; sensor_id++) {
            if (wich_buffer[sensor_id] & BUFFER_LOW) {
                set_threshold(sensors[sensor_id].low_threshold); // Low threshold must read as 1
                ckres = check_port(sensors[sensor_id].pin);
                if (ckres) circular_buffer_push(sensors[sensor_id].low_buffer, 1);
                else       circular_buffer_push(sensors[sensor_id].low_buffer, 0);
            } // end if
        } // end for
        _MemoryBarrier(); // Forces keeping the order (jouning the loop is a bad thing, slows down the execution)
        for (sensor_id = 0; sensor_id < num; sensor_id++) {
            if (wich_buffer[sensor_id] & BUFFER_HIGH) {
                set_threshold(sensors[sensor_id].high_threshold); // Low threshold must read as 0
                ckres = check_port(sensors[sensor_id].pin);
                if (ckres) circular_buffer_push(sensors[sensor_id].high_buffer, 1);
                else circular_buffer_push(sensors[sensor_id].high_buffer, 0);
            } // end if
        } // end for
    } // end for
}

static inline void set_press_release_threshold(const capacitive_sensor_ptr_t sensors, const uint8_t * const to_probe, const uint8_t num) {
    uint8_t sensor_id;
    uint8_t overflow; // In case of overflow sets min/max

    // All of this can be done in parallel
    for (sensor_id = 0; sensor_id < num; sensor_id++) {
        if (to_probe[sensor_id] == 0) continue; // This has do not be touched
        overflow = sensors[sensor_id].low_threshold;
        sensors[sensor_id].low_threshold -= RELEASE_THRESHOLD;
        if (sensors[sensor_id].low_threshold > overflow) // Underflow (below 0)
            sensors[sensor_id].low_threshold = 0;
        overflow = sensors[sensor_id].high_threshold;
        sensors[sensor_id].high_threshold += PRESS_THRESHOLD;
        if (sensors[sensor_id].high_threshold < overflow) // Overflowed
            sensors[sensor_id].high_threshold = UCHAR_MAX;
    }
}

// +--- NB ---------------------------------------------+
// |   increasing threeshold leads to more 0 readings   |
// |   decreasing threeshold leads to more 1 readings   |
// +----------------------------------------------------+

// Sets the best threeshold
static void adjust_interval(const capacitive_sensor_ptr_t sensors, const uint8_t * const to_probe, const uint8_t num) {
    uint8_t sensor_id, at_least_one_ck; // counter through sensors array
    uint8_t high_done[num], low_done[num]; // counters
    uint8_t high_dir[num], low_dir[num]; // true if need to increase the threshold
    uint8_t buffer_to_fill[num];

    // Preliminary fills all the buffers
    memset(buffer_to_fill, BUFFER_BOTH, sizeof(buffer_to_fill)); // Relays on the fact buffer_to_fill is uint8_t
    memset(high_done, 0, sizeof(high_done));
    memset(low_done , 0, sizeof(low_done )); // Resets done buffers
    // Noe excludes what hasn't to be done
    for (sensor_id = 0; sensor_id < num; sensor_id++)
        if (to_probe[sensor_id] == 0) { // Not to do
            buffer_to_fill[sensor_id] = BUFFER_NONE; // Do not fill noone of the buffers
            high_done[sensor_id] = low_done[sensor_id] = 1; // All done!
        }

    fill_buffer(sensors, buffer_to_fill, num); // Parallel buffer filling

    for (sensor_id = 0; sensor_id < num; sensor_id++) { // Two operations in the loop can be executed sequentially
        if (  (!low_done[sensor_id]) &&
           (circular_buffer_sum(sensors[sensor_id].low_buffer) <= SAMPLES_NUM*LOW_THRESHOLD)   ) // must sum high
             low_dir[sensor_id] = 0; // Should decrease threshold to get an higher sum (more 1s)
        else low_dir[sensor_id] = 1; // Should increase threshold

        if (   (!high_done[sensor_id]) &&
           (circular_buffer_sum(sensors[sensor_id].high_buffer) >= SAMPLES_NUM*HIGH_THRESHOLD)   ) // must sum low
             high_dir[sensor_id] = 1; // Should increase threshold to get a lower sum (more 0s)
        else high_dir[sensor_id] = 0; // Should decrease threshold
    }

    // Now phisically move the thresholds according to preliminary readings
    while ( 1 ) { // repeats always until a break
        at_least_one_ck = 0;
        for (sensor_id = 0; sensor_id < num; sensor_id++) { // Everything in the loop is sequentializable
            if (!low_done[sensor_id]) {
                // if probing thresholds increase
                if (low_dir[sensor_id] == 1) { // Needs to increment, but first checks for maximum
                    if (sensors[sensor_id].low_threshold == UCHAR_MAX) {
                        low_done[sensor_id] = 1; // All done for this time
                        at_least_one_ck = UCHAR_MAX; // Forces re-execution of the while (see below)
                        break; // Repeats again the process
                    }
                    sensors[sensor_id].low_threshold++;
                }
                if (low_dir[sensor_id] == 0) { // Needs to decrement, but first checks for minimum
                    if (sensors[sensor_id].low_threshold == 0) { // "UCHAR_MIN"
                        low_done[sensor_id] = 1; // All done for this time
                        at_least_one_ck = UCHAR_MAX; // Forces re-execution of the while (see below)
                        break; // Repeats again the process
                    }
                    sensors[sensor_id].low_threshold--;
                }
                at_least_one_ck = 1; // This sensor has not done
            } else { // Removes filling low buffer
                buffer_to_fill[sensor_id] &= ~BUFFER_LOW; // Stops watching this buffer
            }
            if (!high_done[sensor_id]) {
                // if probing thresholds decrease
                if (high_dir[sensor_id] == 1) {
                    if (sensors[sensor_id].high_threshold == UCHAR_MAX) {
                        high_done[sensor_id] = 1; // All done for this time
                        at_least_one_ck = UCHAR_MAX; // Forces re-execution of the while (see below)
                        break; // Repeats again the process
                    }
                    sensors[sensor_id].high_threshold++;
                }
                if (high_dir[sensor_id] == 0) {
                    if (sensors[sensor_id].high_threshold == 0) { // "UCHAR_MIN"
                        high_done[sensor_id] = 1; // All done for this time
                        at_least_one_ck = UCHAR_MAX; // Forces re-execution of the while (see below)
                        break; // Repeats again the process
                    }
                    sensors[sensor_id].high_threshold--;
                }
                at_least_one_ck = 1;
            } else { // Removes filling high buffer
                buffer_to_fill[sensor_id] &= ~BUFFER_HIGH; // Stops watching this buffer
            }
        } // end for
        if (at_least_one_ck == 0) break; // everything done!
        if (at_least_one_ck == UCHAR_MAX) continue; // restarts loop

        // Now collect some samples from the sensor
        fill_buffer(sensors, buffer_to_fill, num); // Parallel buffer filling
                                                // sequentialize buffer fillin may result in a significant slowdown

        _MemoryBarrier();
        for (sensor_id = 0; sensor_id < num; sensor_id++) { // Now again, everything in the loop is sequentializable
            if (!low_done[sensor_id]) {
                if (low_dir[sensor_id] == 1) { // increasing low threshold
                    if (circular_buffer_sum(sensors[sensor_id].low_buffer) <= SAMPLES_NUM*LOW_THRESHOLD) { // must sum high
                        sensors[sensor_id].low_threshold--; // low threshold was too high, last stored was good
                        low_done[sensor_id] = 1; // do not probe low anymore
                    }
                } else if (low_dir[sensor_id] == 0) { // decreasing low threshold
                    if (circular_buffer_sum(sensors[sensor_id].low_buffer) > SAMPLES_NUM*LOW_THRESHOLD) // must sum high
                        low_done[sensor_id] = 1; // satisfied, do not probe low anymore
                }
            } // end if low_done
            if (!high_done[sensor_id]) {
                if (high_dir[sensor_id] == 0) { // decreasing low threshold
                    if (circular_buffer_sum(sensors[sensor_id].high_buffer) >= SAMPLES_NUM*HIGH_THRESHOLD) { // must sum low
                        sensors[sensor_id].high_threshold++; // high threshold was too low
                        high_done[sensor_id] = 1; // do not probe high anymore
                    }
                } else if (high_dir[sensor_id] == 1) { // increasing low threshold
                    if (circular_buffer_sum(sensors[sensor_id].high_buffer) < SAMPLES_NUM*HIGH_THRESHOLD) // must sum low
                        high_done[sensor_id] = 1; // do not probe high anymore
                }
            } // end if high_done
        } // end for
        _MemoryBarrier();
    } // end while
    return;
} // end function

static void probe(const capacitive_sensor_ptr_t sensors, const uint8_t * const to_probe, const uint8_t num) { // Probes threshold
    uint8_t sensor_id; // counter through sensors array
    uint8_t lowest_high[num], highest_low[num]; // highest and lowest reached by each threshold
    uint8_t done[num], all_done;
    uint32_t times; // iterator counter
    uint8_t ckres, swap; // result of check_port, temporany

    memset(done, 0, sizeof(done)); // No data has already been processed
    // Subtracts immediately what does not need to be probed
    for (sensor_id = 0; sensor_id < num; sensor_id++) {
        if (to_probe[sensor_id] == 0) // This has not to be done
            done[sensor_id] = 1; // Exclude him
    }

    // Sets a General starting point, you can change it if you think performance will benefit
    // However this settings should not change the final output
    for (sensor_id = 0; sensor_id < num; sensor_id++) {
        if (done[sensor_id] == 1) continue; // skips already done
        lowest_high[sensor_id] = UCHAR_MAX; // Maximum
        highest_low[sensor_id] = 0; // Minum
        sensors[sensor_id].high_threshold = UCHAR_MAX*3/4;
        sensors[sensor_id].low_threshold = UCHAR_MAX*1/4;
    }

    // The first part of the algorithm is intented to give only an approximation onf the final result
    times = 0; // no times in the loop
    while (times < MAX_PROBE_STEPS) { // Checks only the times condition, there is a second
        // Now sets done, a pin is done when (high_threshold > (low_threshold + 1)) is false
        for (sensor_id = 0; sensor_id < num; sensor_id++)
            if (sensors[sensor_id].high_threshold <= (sensors[sensor_id].low_threshold + 1))
                done[sensor_id] = 1; // This pin has done
        all_done = 1; // And now checks if all done
        for (sensor_id = 0; sensor_id < num; sensor_id++) // Now checks condition to exit cycle: all done
            if (done[sensor_id] == 0) { // At least one not done
                all_done = 0;
                break;
            }
        if (all_done) break; // Nothing else to do here!
        times++; // Cycle repeated one more time
        _MemoryBarrier();
        for (sensor_id = 0; sensor_id < num; sensor_id++) { // For each sensor
            if (done[sensor_id] == 1) continue; // skips already done
            set_threshold(sensors[sensor_id].low_threshold); // Low threshold must read as 1
            ckres = check_port(sensors[sensor_id].pin);
            if (ckres) { // Read correct value, tries increasing
                highest_low[sensor_id] = sensors[sensor_id].low_threshold;
                sensors[sensor_id].low_threshold += // Takes middle point between low and high
                  /* += */  (sensors[sensor_id].high_threshold - sensors[sensor_id].low_threshold) / 2;
            } else { // Read wrong value, tries decreasing
                sensors[sensor_id].low_threshold =
                 /* = */    (sensors[sensor_id].low_threshold + highest_low[sensor_id]) / 2; // average, rounded down
            } // end if
        } // end for
        _MemoryBarrier();
        for (sensor_id = 0; sensor_id < num; sensor_id++) { // Again, for each sensor
            if (done[sensor_id] == 1) continue; // skips already done
            set_threshold(sensors[sensor_id].high_threshold); // High threshold must read as 0
            ckres = check_port(sensors[sensor_id].pin);
            if (ckres) { // Read wrong value, tries decreasing
                sensors[sensor_id].high_threshold =
                  /* = */   (sensors[sensor_id].high_threshold + lowest_high[sensor_id] + 1) / 2; // average, rounded up
            } else { // Read correct value, tries increasing
                lowest_high[sensor_id] = sensors[sensor_id].high_threshold;
                sensors[sensor_id].high_threshold +=
                  /* += */  (sensors[sensor_id].high_threshold - sensors[sensor_id].low_threshold) / 2;
            } // end if
        } // end for
        _MemoryBarrier();
        for (sensor_id = 0; sensor_id < num; sensor_id++) { // And again, for each sensor
            if (done[sensor_id] == 1) continue; // skips already done
            if (sensors[sensor_id].high_threshold <= sensors[sensor_id].low_threshold) { // Should not happen
                swap = (sensors[sensor_id].low_threshold - sensors[sensor_id].high_threshold + 1) / 2; // high, roundup
                sensors[sensor_id].low_threshold =
                  /* = */   (sensors[sensor_id].low_threshold - sensors[sensor_id].high_threshold - 1) / 2; // low, rounddown
                sensors[sensor_id].high_threshold = swap; // because first operation changes data the second uses
            } // end if
        } // end for
        _MemoryBarrier();
    } // end while

    adjust_interval(sensors, to_probe, num); // Now fine adjusting
    set_press_release_threshold(sensors, to_probe, num); // Now sets some sensibility

    // For each done sets to_probe = 0
    for (sensor_id = 0; sensor_id < num; sensor_id++) {
        if (to_probe[sensor_id] != 0) // This has not to be done
            sensors[sensor_id].to_probe = 0;
    }
}

// Here is done all the Inttelligent work. This function checks the history buffers
// to say wether the key was pressed or not.
uint32_t capacitive_sensor_pressed(const capacitive_sensor_ptr_t sensors, const uint8_t num) {
    uint8_t sensor_id; // counter through sensors array
    uint8_t buffer_to_fill[num];
    uint8_t to_probe[num];
    uint32_t retval;
    circular_buffer_sum_t temp;

    memset(to_probe, 0, sizeof(to_probe));
    for (sensor_id = 0; sensor_id < num; sensor_id++) {
        if (sensors[sensor_id].to_probe) {
            to_probe[sensor_id] = 1; // informs to execute probing
            sensors[sensor_id].to_probe = 0;
            sensors[sensor_id].gray_zone = 0; // no more in grayzone
        }
    }
    probe(sensors, to_probe, num); // re-probes what needed

    memset(buffer_to_fill, BUFFER_BOTH, sizeof(buffer_to_fill));
    fill_buffer(sensors, buffer_to_fill, num); // Fills buffer of readings

    retval = 0; // now have to choose retval
    for (sensor_id = 0; sensor_id < num; sensor_id++) { // sequentially for each sensor
        temp = circular_buffer_sum(sensors[sensor_id].low_buffer);
        if (temp <= SAMPLES_NUM*(0.10)) { // Key release, sends keyrelease and probes again
            sensors[sensor_id].hysteresis++;
            if (sensors[sensor_id].hysteresis > HYSTERESIS) { // Actually send keyrelease
                sensors[sensor_id].pressed = 0;
                sensors[sensor_id].to_probe = 1;
                sensors[sensor_id].gray_zone = 0;
                sensors[sensor_id].hysteresis = 0;
            }
        } else if (temp <= SAMPLES_NUM*LOW_THRESHOLD) {
            sensors[sensor_id].gray_zone++; // it is not a keypress, but it is near to a keypress
            if (sensors[sensor_id].gray_zone >= MAX_TIME_IN_GRAYZONE) { // Spent too many time in grayzone, request re-probe without sending keypress
                sensors[sensor_id].to_probe = 1;
                sensors[sensor_id].pressed = 0; // Resets pressed status
            }
        } else {
            sensors[sensor_id].gray_zone = 0;
        }

        temp = circular_buffer_sum(sensors[sensor_id].high_buffer);
        if (temp >= SAMPLES_NUM*(0.90)) { // Key press, send kaypress and probes again sensibility
            sensors[sensor_id].pressed = 1;
            sensors[sensor_id].hysteresis = 0;
            sensors[sensor_id].to_probe = 1;
            sensors[sensor_id].gray_zone = 0;
        } else if (temp >= SAMPLES_NUM*HIGH_THRESHOLD) { // gray_zone
            sensors[sensor_id].gray_zone++; // it is not a keypress, but it is near to a keypress
            if (sensors[sensor_id].gray_zone >= MAX_TIME_IN_GRAYZONE) { // Spent too many time in grayzone, request re-probe without sending keypress
                sensors[sensor_id].to_probe = 1;
                sensors[sensor_id].pressed = 0; // Resets pressed status
            }
        } else { // Everything normal
            sensors[sensor_id].gray_zone = 0;
        }

        if (sensors[sensor_id].pressed == 1) // If after decision sensor has been pressed
            retval |= _BV(sensor_id); // Sets corresponding bit
        else
            retval &= ~(_BV(sensor_id)); // Sets corresponding bit
    } // end for

    return retval;
}

static void capacitive_sensor_init_no_probe(const capacitive_sensor_ptr_t sensors, const uint8_t pin_id) {
    circular_buffer_init(sensors->low_buffer , sensors->low_buffer_data , SAMPLES_NUM);
    circular_buffer_init(sensors->high_buffer, sensors->high_buffer_data, SAMPLES_NUM);
    sensors->hysteresis = 0; // default init
    sensors->gray_zone = 0; // default init
    sensors->to_probe = 1; // this will cleared during probe
    sensors->pressed = 0; // Button starts not pressed
    sensors->pin = pin_id; // Pins  to read. Make sure the pin is configured in low level configurations
    sensors->high_threshold = sensors->low_threshold = 0; // Should change when probing
}

void capacitive_sensor_init(const capacitive_sensor_ptr_t sensors, const uint8_t pin_id) {
    uint8_t to_probe = 1; /* pseudo-array */
    // This before everything
    inint_inputs(&pin_id, 1); // Inits capacitive inputs (lowlevel)
    capacitive_sensor_init(sensors, pin_id);
    probe(sensors, &to_probe, 1); // chooses correct threshold
}

void capacitive_sensor_inits(const capacitive_sensor_ptr_t sensors, const uint8_t * const pin_id, const uint8_t num) {
    uint8_t sensor_id;
    uint8_t to_probe[num];

    // This before everything
    inint_inputs(pin_id, num); // Inits capacitive inputs (lowlevel)

    for (sensor_id = 0; sensor_id < num; sensor_id++)
        capacitive_sensor_init_no_probe(sensors + sensor_id, pin_id[sensor_id]);
    memset(to_probe, 1, sizeof(to_probe)); // Every input has to be probed
    probe(sensors, to_probe, num); // chooses correct threshold
}

// Progmem version of the function
void capacitive_sensor_inits_P(const capacitive_sensor_ptr_t sensors, const uint8_t * const pin_id, const uint8_t num) {
    uint8_t sensor_id;
    uint8_t to_probe[num];

    // This before everything
    inint_inputs_P(pin_id, num); // Inits capacitive inputs (lowlevel)

    for (sensor_id = 0; sensor_id < num; sensor_id++)
        capacitive_sensor_init_no_probe(sensors + sensor_id, pgm_read_byte_near(pin_id + sensor_id));
    memset(to_probe, 1, sizeof(to_probe)); // Every input has to be probed
    probe(sensors, to_probe, num); // chooses correct threshold
}
