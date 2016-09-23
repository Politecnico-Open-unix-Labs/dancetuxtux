#ifndef SETTINGS_H
#define SETTINGS_H

#define START_THRESHOLD   0
#define DISCHARGE_TIME    10 // in us

// Enable it if you want timing discharge made with timers
// NOTE: this will take 64 more bytes in RAM
#define USE_DISCHARGE_TIMERS // undef to disable this operation
                             // Timer used is TIMER 3, do not use for something else
                             // Timer is set to use DISCHARGE_TIME

#ifdef USE_DISCHARGE_TIMERS // If using timers
#   define MAX_PORT_NUM 4 // This is the max number of ports at the same time
                          // The code will handle inefficently each more pin waiting for discharge
#endif

// If you don't know what next parameters are leave them as they are
#define SAMPLES_NUM 32 // Number of samples to take before choosing whether the button is pressed or not
#define LOW_THRESHOLD  0.90 // must be in [0.00-1.00], choosing parameter
#define HIGH_THRESHOLD 0.10 // must be in [0.00-1.00], as above

// Next two must be floating point in [0.0 - 1.0]
#define PRESS_CONDITION    0.80 // Higher will press a key harder
#define RELEASE_CONDITION  0.80 // Lower will release a key harder

// Coarse sensibility adjust
// Set next two to zero to disable the effect
#define PRESS_THRESHOLD 3 // increase in sensor threshold before keypress
#define RELEASE_THRESHOLD 0 // same as above for keyrelease (sensor threshold decrease)

#define HYSTERESIS_A 0 // Time to wait after a keyrelease and before sending the succesive
#define HYSTERESIS_B 0 // Time to wait before keyrelease

// Auto reset settings, they shouldn't be changed
// Grayzone is the zone between PRESS and RELEASE condition.
#define MAX_TIME_IN_GRAYZONE ((uint32_t)1E9) // uinsigned int between 0 and 2^32 -1
                        // After that time in grayzone sensibility is reset and re probed
#define MAX_PROBE_STEPS ((uint32_t)250) // sometimes probe gets stuck (don't know why).
                        // This forces exit after given number of steps


#endif // SETTINGS_H
