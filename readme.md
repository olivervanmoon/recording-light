# Development notes (read me?) #

It all started when Spencer's mom gave us what seemed like a simple present, a recording light for the DazeyChain Records studio. It was just a strip of red LEDs in a good-looking black box with the word "RECORDING" etched into a glass pannel. No power supply, no controller, no instructions. I knew what I had to do.

## Goals and constraints ##

- When the studio computer is actively recording, the recording light is on (on the other side of the door so people know not to ruin the vocal take).
- Must work with all the DAWs used at the studio (Logic Pro, Pro Tools, and Ableton Live).
- Should be easy to set up and foolproof, not requiring any additional software to run on the studio computer.
- Use parts I already have (an Arduino Uno).

## Solution ##

An Arduino Uno is set up as a class-compliant MIDI device (easier said than done) which appears as a control surface in any DAW (easier said than done). The Arduino controls a MOSFET, which switches power to the light on and off (pretty easy).

# ARDUINO USB FIRMWARE #

The Arduino Uno is not a class-compliant MIDI device by default. The chip that interfaces from the USB port to the serial connection on the main microcontroller (ATmega16U2) must be reflashed to have the Arduino show up as a MIDI device when plugged in via USB. This presents a problem during development: when the Arduino is in "MIDI mode", it is no longer visible to the Arduino IDE on a serial port and cannot be programmed over USB. Therefore, it was necessary to switch back and forth. Luckily, smart people have figured out how to do this in a way that makes it *almost* not a pain in the ass.

During development, I used the [mocoLUFA](https://github.com/kuwatay/mocolufa) firmware which allows switching between MIDI and serial modes by connecting certain pins while power cycling the Arduino.

To reflash the ATmega16U2 chip, I used the `dfu-programmer` command line tool, installed via Homebrew (but it can also be installed from [here](https://github.com/dfu-programmer/dfu-programmer)). The steps are as follows.
1. While the Arduino is connected to the computer via USB, connect the two closest pins to the USB port on the Arduino. It should look like this:


.  .  .
|
.  .  . 

2.
	a. Run `sudo dfu-programmer atmega16u2 erase`
	b. `sudo dfu-programmer atmega16u2 flash [PATH TO HEX FILE HERE]` (use `dualMoco.hex` or `USBMidiKliK_dual_uno.hex`)
	c. `sudo dfu-programmer atmega16u2 reset`

3. Disconnect the pins.

The `lsusb` command line tool can be used to see if and how the device appears.

Now that the USB chip has new firmware, when the Arduino is power cycled (disconnected and reconnected to USB), it will show up as a MIDI device by default. To put the Arduino back in serial mode for programming, connect other pins that look like this

.  .  .

._ .  . 

while power cycling. Then the Arduino will be available for programming in the Arduino IDE.

Later, after finishing the Arduino program, I discovered the [USBMidiKliK](https://github.com/TheKikGen/USBMidiKliK) firmware which allowed me to customize the name of the MIDI device as it appears to DAWs for greater clarify and foolproofness. I'm not sure if this firmware works the same as mocoLUFA as far as using the pins to change modes, but the mode can be changed via System Exclusive (SysEx) MIDI messages. One SysEx message in particular (hex: F0 77 77 77 08 F7) puts the Arduino into a serial communication mode which allows settings to be changed via a serial terminal like the one in the Arduino IDE. I created the binary SysEx file (.syx) from the hexadecimal text representation using the `xxd` command line tool. I used SysEx Librarian for macOS to "play" the SysEx command to the Arduino in MIDI mode. Then I was able to name my device so that it appears "OvM Recording Light" in DAWs.


# MIDI and CONTROL SURFACE PROTOCOLS #

There are several great libraries for Arduino for creating control surfaces and dealing with MIDI messages, but I ended up writing my own code to parse bytes from the serial buffer. It was quite frustrating to find out that Pro Tools offers a very limited number of options for interfacing with control surfaces, and these options do not seem to overlap well with Logic Pro and Ableton Live. To make the Arduino work with all the DAWs I required, I needed to implement two different protocols of receiving a recording message via MIDI:
- "Mackie Control":
	- For use with Logic Pro X and Ableton Live (or others).
	- DAW sends recording on and off messages on a single MIDI note:
		- Channel 1, Note B6 aka note number 95. Full velocity (127 aka 0x7F) for on, zero velocity (0 aka 0x00) for off. (This data can be used to configure the "Recording Light" type of control surface in Logic Pro X, although Mackie Control also works.)
	 message | hex command 
	--- | ---
	 ON | 90 5F 7F 
	 OFF| 90 5F 00 
- "M-Audio Keyboard":
	- For use with Pro Tools (not sure if anyone else needs this).
	- DAW sends record enable and playing messages on two different MIDI notes. When Pro Tools is both record-enabled and playing, it is recording:
	| message | hex command |
	|---|---|
	| PLAY ON |	B0 75 7F |
	| PLAY OFF | B0 75 00 |
	| RECORD ENABLE ON | B0 76 7F |
	| RECORD ENABLE OFF | B0 76 00 |

