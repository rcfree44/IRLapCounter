# IRLapCounter
Infrared Lap Counter for Radio-Controlled Cars

IRTag is the car Infrared transmitter, sending the car ID. 
It's realized with a mino Arduino Pro Mini board 3.3V@8MHz and few electronics (resistor + IR led)

IRBase is the Lap Counter controller-box that receive the IR Tags and manage the race (lap/time limits, statistics, etc.)
It's realized with a standard Arduino Uno 5V@16MHz and more electronics (IR receivers, wires, buttons, LED matrix, LCD display, etc.)
The Base can communicates and be controller externally trough wired (usb) or wireless connection (Bluetooth / Wifi).

The hardware schematics will be soon published.

Copyright 2014-2015 Team RCFree44