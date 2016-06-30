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

Ordered by importance

* Tests & bugfix
* Flexible Timer time measurment
* Sensibility Adjust
* Debouncing
* Add a description to this readme
* Makefile for all platforms
* Enrich Troubleshooting section
