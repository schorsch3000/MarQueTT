# MarQueeTT

## A MAX7221 LED-Matrix Scrolling Marquee controlled via MQTT

### Configuration
Copy `local_config.dist.h` to `local_config.h` and configure to your needs.
`local_config.h` is ignored by git your secrets are safe there.


### MQTT Topics

#### Subscribed by Marquee

- ledMatrix/text: A UTF-8 coded text, max. 4096 bytes long.
- ledMatrix/intensity: 0 = lowest, 15 = highest. Default: 1.
- ledMatrix/delay: 1 = fastest, 1000 = slowest scrolling. Default: 25, 0 for static text.
- ledMatrix/blink: 0 = no blinking; 1 = fastest, 1000 = slowest blinking. Default: 0
- ledMatrix/enable: 0 = display off, 1 = display on. Default: 1

#### Published by Marquee

- ledMatrix/status: startup, reconnect, repeat, offline

### Wiring

Connect your display's X pin to your controller's Y pin:

- Data IN to MOSI
- CLK to SCK
- CS to any digial port except MISO or SS 

See https://github.com/bartoszbielawski/LEDMatrixDriver#pin-selection for more information


  
