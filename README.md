# espwake - A reliable WoL alternative

## Overview
espwake is a self contained remote wakeup solution for ATX compilant PCs. espwake was designed
to be significantly more reliable than wake-on-lan, requiring no support in BIOS and network card,
instead relying only on well respected ATX power standards and standard power up - power down sequences.
The hardware hooks up to the USB 2.0 header (which it only uses to draw power from the +5VSB line) and
to the ATX front panel connector, more specifically to the power button and power led pins. The hardware
can then sense the power on state of the computer using the power led pins (drawing virtually no current,
so regular leds should work normally), and turn the PC on/off using simulated button presses. The hardware
does not impede normal functions of the power switch or power led. Using a regular cable which branches the pins
into two should allow for parallel operation of these two.

## Usage
espwake uses a custom UDP protocol running on port 9999.

### Request
The request packet looks something like this:
```
 __________________________________________ ________________
|         .         |                      ~               |
| VERSION | COMMAND | PASSWORD (16B)       ~ RESERVED(23B) |
|_________._________|______________________~_______________|
  0 1 2 3 4 5 6 7 8   0 1 2 3 4 5 6 7 8 ...
```
The entire packet must be 40B long. The last 23 bytes are reserved for future use and should be set to 0.

The version denotes the protocol major version. It is assumed that different protocol major version
will be incompatible with each other, and thus the firmware will reject anything that does not have
the 4 LSb of the first byte set to its own major version (currently: 1).

The 4 most significant bits in the first byte represent the command. Currently,
the following commands are supported:

| 8 7 6 5 | Meaning                       |
|---------|-------------------------------|
| 0 0 0 0 | Report the PC power status    |
| 0 0 0 1 | Start the PC (short press)    |
| 0 0 1 0 | Shutdown the PC (short press) |
| 0 0 1 1 | Force power off (long press)  |

By default, every command except status report requires the password to match the stored one.

### Response
The reply packet is exactly 4 bytes long and looks like this:
```
 ______________________________________________________
|         .         |                   |              |
| VERSION | STATUS  | DEVSTATUS         | RESERVED(2B) |
|_________._________|___________________|______________|
  0 1 2 3 4 5 6 7 8   0 1 2 3 4 5 6 7 8   0 1 2 3 ...
```

The version field in response has the same meaning as the version field in request packets.

The status field (4 most significant bits of first byte) represents the server's reaction
to the accompanying request, and should be one of the following values:

| 8 7 6 5 | Meaning                       |
|---------|-------------------------------|
| 0 0 0 0 | OK                            |
| 0 0 0 1 | Unauthorized (wrong password) |
| 0 0 1 0 | Incompatible protocols        |
| 0 0 1 1 | Unknown command               |
| 1 1 1 1 | Unknown error                 |

The device status in the next byte represents the state of the connected computed,
and can contain the following values:

|  Value  | Meaning       |
|---------|---------------|
|    0    | Powered down  |
|    1    | Shutting down |
|    2    | Powering up   |
|    3    | Running       |
