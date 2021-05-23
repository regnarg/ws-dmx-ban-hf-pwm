all: ws-dmx-ban-hf-pwm.ihx

ws-dmx-ban-hf-pwm.ihx: main.c
	sdcc --std-c2x --opt-code-speed $< -o $@

flash: ws-dmx-ban-hf-pwm.ihx
	sudo stcgal  -p /dev/ttyUSB0  -o reset_pin_enabled=False -o clock_source=external ws-dmx-ban-hf-pwm.ihx -D

.PHONY: flash
