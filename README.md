Pi Pico RP2040 NES joystick(s) to PC/Nintendo Switch adapter
===

Allows to connect two NES joysticks to PC and Nintendo Switch using a single Raspberry Pi Pico RP2040/RP2350 board. 

Board pin out

```
GPIO0 -> DATA joystick 1
GPIO1 -> LATCH (both)
GPIO2 -> CLK (both)
GPIO3 -> DATA joystick 2 (optional)
```

Connector pinout
```                         
                              +--------< DATA                
                              | +------> LATCH            +------------------ GND
                              | | +----> CLK              | 
                              | | |                       |             +---< DATA
        .-                    | | |                       |             |
 GND -- |O\              +-------------+               +-------------------+
 CLK <- |OO\ -- +5V       \ O O O O O /                 \ O O O O O O O O /
LATC <- |OO|               \ O O O O /                   \ O O O O O O O /
DATA -> |OO|                +-------+                     +-------------+
        '--'                 |   |                         |     |     |
                             |   +------ GND               |     |     +---- +5V
                             +---------- +5V               |     +---------: LATCH
                                                           +---------------> CLK
     NES port           9 pin famiclone port               15 pin famiclone port     
```

Connections from console side of the port. 


Programmed using [arduino-pico](https://github.com/earlephilhower/arduino-pico).

To compile choose Tools -> USB Stack -> "Adafruit TinyUSB"