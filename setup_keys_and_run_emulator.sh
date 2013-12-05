#!/bin/bash

#setup device name of Arduino to auto-detect, else us something like /dev/ttyACM0
devicename=-a

#button layout: 0=coin reject, 1=1 player, 2=2 players, 3=red, 4=black
./mameduino $devicename -s 1 1
./mameduino $devicename -s 2 2
./mameduino $devicename -s 3 p
./mameduino $devicename -l 3 ESC
./mameduino $devicename -l 4 PIN_RESET
#coin layout: 0=50ct, 1=1euro, 2=2euro
./mameduino $devicename -c 0 5
./mameduino $devicename -c 1 5 5
./mameduino $devicename -c 2 5 5 5 5
#turn coin rejection off
./mameduino $devicename -r off
wait

#run emulator
retroarch -L /bla/libretro/mame078_libretro.so --config /bla/configs/all/retroarch.cfg --appendconfig /bla/configs/mame/retroarch.cfg $1

#turn coin rejection on again
./mameduino $devicename -r on
#button layout: 0=coin reject, 1=1 player, 2=2 players, 3=red, 4=black
./mameduino $devicename -s 1 CLEAR
./mameduino $devicename -s 2 CLEAR
./mameduino $devicename -s 3 CLEAR
./mameduino $devicename -s 4 CLEAR
./mameduino $devicename -l 1 CLEAR
./mameduino $devicename -l 2 CLEAR
./mameduino $devicename -l 3 CLEAR
./mameduino $devicename -l 4 PIN_RESET
#coin layout: 0=50ct, 1=1euro, 2=2euro
./mameduino $devicename -c 0 CLEAR
./mameduino $devicename -c 1 CLEAR
./mameduino $devicename -c 2 CLEAR
wait
