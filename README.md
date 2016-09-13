What's this?
------------
This is a controller for the videogame Stepmania. You need an Arduino-compatible
board to run this software. You may also need a board, see the instructions below
to build one yourself.

Unfortunately not all Arduino boards are supported at the moment. Only the Arduno
Leonardo is suitable for this software right now.
You need to connect your Arduino to your computer via standard USB cable, using the
defaul port on the Arduino. This controller will act as a keyboard. Stepmania works
well with keyboard inputs. There are only four inputs: Arrow Key Up, Down, Left and
Right.

NOTE: This board will act as a keyboard even with other programs, so if you leave
the board connected to your computer when other programs are running you may get
undesidered inputs (only the four arrow keys), and it might conflict with your
existing keyboard.

Building your board
-------------------
To build your board you need:
* An Arduno Leonardo
* Either Aluminium or Steel foils. You need eight of the same size, approximatively square
* Some foarm rubber, thickness = 3mm
* Wood, many wood
* Tools (glue, nails, hummer, skewdrivers...)

TODO: Complete this section

Connections: Connect each one of the lower foil to the ground, then all together to the
Arduino ground. Connect the Up key to pin number 8, Down to the 9, Left to the 10, and
Right to 11. You are done! Now you can the USB cable of your Arduino to the computer
and upload the software (see section below)


Compiling
---------

Before doing anything you need avr-gcc to compile and avrdude to install.
They are both provided with the arduino package from the repo of your OS,
or you can manually install only the packages `avr-gcc` and `avrdude`.
Follow the man pages to install both the packages. You need to include
the path of their binary executable in your $PATH variable.

To compile and upload on the board run `make` & `make install`

When uploading you'll be requested to press and release the reset button on your board.
When you get the request press and release the release button, then press any key on your computer.
Wait some time the upload finishes. When appears a green message you are done.
You can now enjoy your new stepmania controller!

N.B. Uploading may not work on all computer.

Troubleshooting:
----------------

Customize your settings.h file to increase/decrease sensibility.

TODO:
-----

Ordered by priority

1. <del>Timer Utils function, to interact easily with timers<del>
2. <del>Flexible Timer time measurment<del>
3. <del>Keypress/Keyrelease event dialing</del>
4. <del>All the USB stuff, this will re-use many of the Arduino code</del>
  * <del>Get a working USB communication</del>
  * <del>Send the correct key event on keypress/keyrelease</del>
  * <del>Remove dead/unuseful code</del>
5. Tests & bugfix (to configure timing proprerly before going on)
6. <del>Dynamic Sensibility Adjust</del>
7. <del>Debouncing, & Debouncing settings</del>
8. Add a description to this readme
9. <del>Smart discharge (i.e. using another timer)<del>
10. Makefile for all platforms
11. Enrich Troubleshooting section
12. Support for more boards (Arduino Uno, etc...)
