all: ws-dmx-ban-hf-pwm.ihx

ws-dmx-ban-hf-pwm.ihx: main.c
	sdcc --std-c2x --opt-code-speed $< -o $@

test.ihx: test.c
	sdcc --std-c2x --opt-code-speed $< -o $@

flash: ws-dmx-ban-hf-pwm.ihx
	killall rd.py &>/dev/null || true
	stcgal  -p /dev/ttyUSB0  -o reset_pin_enabled=False -o clock_source=external $<

flash_test: test.ihx
	killall rd.py &>/dev/null || true
	stcgal  -p /dev/ttyUSB0  -o reset_pin_enabled=False -o clock_source=external $<

.PHONY: flash
