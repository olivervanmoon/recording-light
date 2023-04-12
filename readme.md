READ ME

It all started when Spencer's mom gave us what seemed like a simple present—a "RECORDING" light for the DazeyChain Records studio. The light, in fact, was quite simple. Just a strip of LEDs rated for 12 volts.

MIDI and CONTROL SURFACE PROTOCOLS

Can use either of two control surface protocols:
- Mackie Control:
	- For use with Logic Pro X and Ableton Live (or others).
	- DAW sends recording on and off messages on a single MIDI note:
		- Channel 1, Note B6 aka note number 95. Full velocity (127 aka 0x7F) for on, zero velocity (0 aka 0x00) for off. (This data can be used to configure the "Recording Light" type of control surface in Logic Pro X, although Mackie Control also works.)
		- Full MIDI messages (in hex):
			ON:					90 5F 7F
			OFF:				90 5F 00
- M-Audio Keyboard:
	- For use with Pro Tools (not sure if anyone else needs this).
	- DAW sends record enable and playing messages on two different MIDI notes. When Pro Tools is both record-enabled and playing, it is recording:
		- Full MIDI messages (in hex):
			PLAY ON: 			B0 75 7F
			PLAY OFF:			B0 75 00
			RECORD ENABLE ON:	B0 76 7F
			RECORD ENABLE OFF:	B0 76 00

ARDUINO FIRMWARE

The Arduino UNO is not a class-compliant MIDI device by default. The chip (atmega16u2)that interfaces from the USB port to the serial connection on the main microcontroller must be reflashed to have the Arduino show up as a MIDI device when plugged in via USB.