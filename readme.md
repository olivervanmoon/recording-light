# DAW setup instructions
1. Plug it in.
2. Add the recording light MIDI device as a control surface in your DAW.

	| DAW		| control surface type/protocol 	|
	| --- 		| ---		|
	| Pro Tools	| HUI |
	| Logic Pro 	| Mackie Control |
	| Ableton Live 	| Mackie Control |

**Pro Tools note:** HUI is available at **Setup → Peripherals → MIDI Controllers → HUI**. Older Pro Tools installations (pre-2023.x) can use "M-Audio Keyboard" instead, which was the original workaround before HUI support was added to this firmware.

# Development notes #

It all started when Spencer's mom gave us what seemed like a simple present, a recording light for the DazeyChain Records studio. It was just a strip of red LEDs in a good-looking black box with the word "RECORDING" etched into a glass pannel. No power supply, no controller, no instructions. I knew what I had to do.

## Goals and constraints ##

- When the studio computer is actively recording, the recording light is on (on the other side of the door so people know not to ruin the vocal take).
- Must work with all the DAWs used at the studio (Logic Pro, Pro Tools, and Ableton Live).
- Should be easy to set up and foolproof, not requiring any additional software to run on the studio computer.
- Use parts I already have (an Arduino Uno).

## Solution ##

An Arduino Uno is set up as a class-compliant MIDI device (easier said than done) which appears as a control surface in any DAW (easier said than done). The Arduino controls a MOSFET, which switches power to the light on and off (pretty easy).

## ARDUINO USB FIRMWARE ##

The Arduino Uno is not a class-compliant MIDI device by default. The chip that interfaces from the USB port to the serial connection on the main microcontroller (ATmega16U2) must be reflashed to have the Arduino show up as a MIDI device when plugged in via USB. This presents a problem during development: when the Arduino is in "MIDI mode", it is no longer visible to the Arduino IDE on a serial port and cannot be programmed over USB. Therefore, it was necessary to switch back and forth. Luckily, smart people have figured out how to do this in a way that makes it *almost* not a pain in the ass.

During development, I used the [mocoLUFA](https://github.com/kuwatay/mocolufa) firmware which allows switching between MIDI and serial modes by connecting certain pins while power cycling the Arduino.

To reflash the ATmega16U2 chip, I used the `dfu-programmer` command line tool, installed via Homebrew (but it can also be installed from [here](https://github.com/dfu-programmer/dfu-programmer)). The steps are as follows.
1. While the Arduino is connected to the computer via USB, connect the two closest pins to the USB port on the Arduino. It should look like this:

```
.  .  .
|
.  .  . 
```

2.
	1. Run `sudo dfu-programmer atmega16u2 erase`
	2. Run `sudo dfu-programmer atmega16u2 flash [PATH TO HEX FILE]` (use [`dualMoco.hex`](./USB_MIDI/dualMoco.hex) or [`USBMidiKliK_dual_uno.hex`](./USB_MIDI/USBMidiKliK_dual_uno.hex)
	3. Run `sudo dfu-programmer atmega16u2 reset`

3. Disconnect the pins.

The `lsusb` command line tool can be used to see if and how the device appears.

Now that the USB chip has new firmware, when the Arduino is power cycled (disconnected and reconnected to USB), it will show up as a MIDI device by default. To put the Arduino back in serial mode for programming, connect other pins that look like this
```
.  .  .

._ .  . 
```

while power cycling. Then the Arduino will be available for programming in the Arduino IDE.

Later, after finishing the Arduino program, I discovered the [USBMidiKliK](https://github.com/TheKikGen/USBMidiKliK) firmware which allowed me to customize the name of the MIDI device as it appears to DAWs for greater clarify and foolproofness. I'm not sure if this firmware works the same as mocoLUFA as far as using the pins to change modes, but the mode can be changed via System Exclusive (SysEx) MIDI messages. One SysEx message in particular (hex: F0 77 77 77 08 F7) puts the Arduino into a serial communication mode which allows settings to be changed via a serial terminal like the one in the Arduino IDE. I created the binary SysEx file (.syx) from the hexadecimal text representation using the `xxd` command line tool. I used SysEx Librarian for macOS to "play" the SysEx command to the Arduino in MIDI mode. Then I was able to name my device so that it appears "OvM Recording Light" in DAWs.


## MIDI and CONTROL SURFACE PROTOCOLS ##

There are several great libraries for Arduino for creating control surfaces and dealing with MIDI messages, but I ended up writing my own code to parse bytes from the serial buffer. The protocol logic lives in `protocols.h` and the Arduino-specific setup and loop is in `OvM Recording Light.ino`.

Three protocols are implemented. The device automatically handles whichever one the connected DAW happens to send — there are no byte collisions between them so they coexist safely.

### HUI (Pro Tools, modern) ###

Pro Tools uses Mackie's older HUI protocol rather than Mackie Control. HUI encodes transport button state as paired Control Change messages and requires the device to reply to a keepalive ping or Pro Tools reports it as disconnected.

**Keepalive (must reply within ~1 second):**

| message | hex |
| --- | --- |
| Ping from Pro Tools | `90 00 00` |
| Pong from device | `90 00 7F` |

**Recording state** is sent as a zone select followed by a port+state byte. The transport zone is `0x0E` and the Record LED port is `5`. Bit 6 of the port byte carries the on/off state.

| message | hex |
| --- | --- |
| Record ON | `B0 0C 0E` then `B0 2C 45` |
| Record OFF | `B0 0C 0E` then `B0 2C 05` |

Pro Tools also sends SysEx messages for display and LED updates (`F0 00 00 66 ... F7`). These are flushed from the buffer without being parsed.

### Mackie Control (Logic Pro X, Ableton Live) ###

Mackie Control is straightforward. The DAW sends recording on and off messages on a single MIDI note: Channel 1, Note B6 (note number 95, `0x5F`). Full velocity (`0x7F`) for on, zero velocity (`0x00`) for off.

| message | hex |
| --- | --- |
| ON | `90 5F 7F` |
| OFF | `90 5F 00` |

### M-Audio Keyboard (legacy Pro Tools, pre-2023.x) ###

This was the original workaround before HUI was implemented here. Pro Tools 2023.x removed the "M-Audio Keyboard" control surface option, but older installations can still use this. Shout out [this person](https://github.com/dupontgu/pro_tools_iot_sync) and [this video](https://www.youtube.com/watch?v=q4VlN0nZlpw) for the tip.

The DAW sends play and record-enable on two separate CCs. When both are active simultaneously, Pro Tools is recording.

| message | hex |
| --- | --- |
| PLAY ON | `B0 75 7F` |
| PLAY OFF | `B0 75 00` |
| RECORD ENABLE ON | `B0 76 7F` |
| RECORD ENABLE OFF | `B0 76 00` |

## Testing ##

Protocol logic can be tested on a desktop machine (no Arduino hardware needed) using a mocked `Serial` object:

```
g++ -std=c++11 -I tests tests/test_protocols.cpp -o tests/run_tests && ./tests/run_tests
```

This runs 16 test functions (18 assertions) covering HUI keepalive, recording on/off, zone/port decoding, Mackie Control, M-Audio Keyboard, and protocol isolation.

## Other notes and ideas ##
- Before deciding to deal with the MIDI data myself, I was experimenting with the [Control Surface](https://github.com/tttapa/Control-Surface) library for Arduino which is incredibly powerful.
- HUI absolutely sucks. I tried to make it work, gave up, found the M-Audio Keyboard workaround, and years later Pro Tools removed M-Audio Keyboard and forced me to implement HUI properly anyway.
- I'm excited to do some recording!
