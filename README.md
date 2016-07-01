Compiling
---------

Before doing anything you need avr-gcc to compile and avrdude to install.
They are both provided with the arduino package from the repo of your OS,
or you can manually install only the packages `avr-gcc` and `avrdude`.
Follow the man pages to install both the packages. You need to include
the path of their binary executable in your $PATH variable.

To compile and upload on the board run `make` & `make install`

When uploading you have to press and release the reset button on your board

N.B. Uploading may not work on all computer.

Troubleshooting:
----------------

Customize your settings.h file to increase/decrease sensibility.

TODO:
-----

Ordered by priority

1. Timer Utils function, to interact easily with timers
2. Flexible Timer time measurment
3. Keypress/Keyrelease event dialing
4. All the USB stuff, this will re-use many of the Arduino code
..* Get a working USB communication
..* Send the correct key event on keypress/keyrelease
..* Remove dead/unuseful code
5. Tests & bugfix (to configure timing proprerly before going on)
6. Dynamic Sensibility Adjust
7. Debouncing, & Debouncing settings
8. Add a description to this readme
9. Smart discharge (i.e. using another timer)
10. Makefile for all platforms
11. Enrich Troubleshooting section
