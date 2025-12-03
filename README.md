# xmas2025

This is the code and schematics/PCB design for the 2025 Christmas ornament.

The ornament runs on an ATTINY85 microcontroller running the Micronucleus bootloader.  The code runs on the Arduino platform using the ATTinyCore.

## The top of the ornament looks like a USB?
It is! You can reprogram the ornament through the usb.  Download the ATTinyCore for Arduino, set your board to "ATTiny85 (Micronucleus/Digispark)".  When uploading new code, insert the ESB end of the ornament into your USB port, component-side up - it's not quite the same right size, so it may take some wiggling and careful holding.  You will likely need to use `Zadig` to install the `libusbK` driver.
