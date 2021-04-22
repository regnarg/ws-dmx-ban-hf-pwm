# Alternative firmware for "WS-DMX-BAN" cheap Chinese LED dimmers with high-frequency PWM

## What, why

![photo of the dimmer](doc/dmx.jpg)

You can buy very cheap Chinese LED strip dimmers from [AliExpress][], controlled over
an RS-485 bus using the [DMX512 protocol][DMX512]. Unfortunately, they operate at
a PWM frequency of circa 200 Hz, which can cause eye strain, headaches and other
unpleasant effects.

They are powered by an STC11Fxx or similar CPU by STC MCU (depending on
revision and number of channels; the 3- and 4-channel versions I saw use
STC11L04E) based on the Intel 8051 architecture.

The CPU itself runs at 24 MHz, so I was quite convinced that it could generate
higher frequencies. The basic trick is to not use interrupts and generate PWM
by directly toggling GPIO pins in a tight loop. This way, I was able to achieve
reliable 32 kHz PWM output with 64 brightness steps:

![generated PWM signal as caught by a logic analyzer](doc/pwm.png)

## Status

  * PWM generation seems complete and stable.
  * We need to implement a communication protocol. I currently prefer some
    simplified variation on Modbus that is interoperable with other Modbus
    devices on the same RS485 bus. Implementing DMX should be also feasible.
    Currently there is a trivial protocol that allows you to set uniform brightness
    on all channels by sending a digit 0 to 9 over the serial line (9600 8N1).
  * Currently it was tested on a 3-channel `WS-DMX-3CH BAN-V2` with a STC11L04E
    CPU. It seems that the 4 channel version uses the same CPU but I don't know
    which GPIO pin controls the last channel.

## Build and install

To build it, you need the [sdcc][] compiler (packaged in many distros). Then
just run `make`.

To flash the CPU, you can use an unpopulated programming header one the board:

![programming header location on the board](doc/proghdr.jpg)

Just solder a pin header or some wires to it.

You can connect it to a 3.3V USB-UART converter (RX-TX, TX-RX, GND-GND). Do not
connect VCC and instead power the board from a 12V power supply, as you would
during normal use.

Before flashing, you must remove the RS485 decoder chip from its socket because
it will also try to drive the CPU's RX pin, which would interfere with the
programming communication.

You can then flash it using [stcgal][]:

    stcgal ws-dmx-ban-hw-pwm.ihx

After running the command, power cycle your board (disconnect and reconnect 12V
power supply).

*Warning: this permanently erases the original firmware. There is no way to back
it up or restore it. This may brick your device.* (But in my opinion, it is pretty
useless with the original firmware so well worth the risk.)

After flashing, it should light up all channels at 50%. You can send it commands
over the UART that you used for programming.

When satisfied, you can disconnect the programming UART, put back the RS485
receiver chip and you should be able to send the same commands over RS485.


## Related projects and links

  * [arneboe/ws-dmx-ban-alt-firmware][] -- uses interrupts to achieve ~ 400 Hz,
    was an inspiration for this project
  * [STC10/11 datasheet][dsheet]

[arneboe/ws-dmx-ban-alt-firmware]: https://github.com/arneboe/ws-dmx-ban-alt-firmware/
[DMX512]: https://en.wikipedia.org/wiki/DMX512
[AliExpress]: https://www.aliexpress.com/wholesale?trafficChannel=main&d=y&CatId=0&SearchText=DMX512+decoder+4ch&ltype=wholesale&SortType=price_asc&groupsort=1&page=1
[dsheet]: https://cdn.datasheetspdf.com/pdf-down/S/T/C/STC10F04-STCTechnology.pdf
[sdcc]: http://sdcc.sourceforge.net/
[stcgal]: https://github.com/grigorig/stcgal/
