all: ws-dmx-ban-hw-pwm.ihx

ws-dmx-ban-hw-pwm.ihx: main.c
	sdcc --std-c2x --opt-code-speed $< -o $@
