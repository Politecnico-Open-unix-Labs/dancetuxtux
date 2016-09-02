#ifndef SETTINGS_H
#define SETTINGS_H

#define START_THRESHOLD   0
#define DISCHARGE_TIME    10 // in us

// If you don't know what next parameters are leave them as they are
#define SAMPLES_NUM 64 // Number of samples to take before choosing whether the button is pressed or not
#define LOW_THREESHOLD  0.90 // must be in [0.00-1.00], choosing parameter
#define HIGH_THREESHOLD 0.10 // must be in [0.00-1.00], as above

// Set next two to zero to disable the effect
#define PRESS_THREESHOLD 1 // increase in sensor threeshold before keypress
#define RELEASE_THREESHOLD 0 // same as above for keyrelease (sensor threeshold decrease)
#define HYSTERESIS 10 // Time to wait before keyrelease
#define MAX_TIME_IN_GRAYZONE ((uint32_t)1E9) // uinsigned int between 0 and 2^32 -1
                        // After that time in grayzone sensibility is reset
#define MAX_PROBE_STEPS ((uint32_t)250) // sometimes probe gets stuck (don't know why).
                        // This forces exit after given number of steps


#endif // SETTINGS_H
