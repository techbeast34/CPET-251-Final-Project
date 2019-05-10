# CPET-251-Final-Project
My final project for CPET-251 Lab

This is code for a standalone MST (Magnetic Secure Transmission) device, designed for an ATmega328P.

The actual code for encoding the tracks for transmission was written by Samy Kamkar in his project "magspoof"
(https://github.com/samyk/magspoof), this was modified to work with an ATmega328P, due to the original being
written for an ATtiny85. It was also modified to use a transistor instead of an H-bridge.

My code allows for local storage and the retrieval of 4 3-track magnetic stripe cards on the ATmega328P's 1K of EEPROM through a serial download mode.
It also accomodates a 16x2 LCD for output and 2 push-buttons for inputs, so any card can be selected. When the device is
idle, the MCU goes to sleep. On startup, the default card is the first card so this device can run headless.
