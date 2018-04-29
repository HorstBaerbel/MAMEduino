MAMEduino
========

Use an Arduino Leonardo to connect physical (arcade-style) buttons and/or a coin receptor to a PC. The Arduino then sends a configurable number of keypresses to the PC when buttons are pressed or coins are inserted, and can also startup/shutdown or reset the PC with the appropriate pins connected. The keypresses sent can be configured via small console program which currently works on Linux only (tested on Ubuntu 18.04).  
A [Fritzing](http://fritzing.org/) layout for the connection can be found in [MAMEduino.fzz](MAMEduino.fzz) and the necessary Arduino source code in [MAMEduino/MAMEduino.ino](MAMEduino/MAMEduino.ino).  
![Fritzing circuit layout](Fritzing_circuit.png?raw=true)  
The coin receptor that was used is the [MoneyControls SR3 Type2](https://www.google.de/search?q=MoneyControls+SR3+Type2+datasheet). Other models will probably need adjustments to the Arduino source code or even the electronic interface/wiring.  

License
========

[BSD-2-Clause](http://opensource.org/licenses/BSD-2-Clause), see [LICENSE.md](LICENSE.md).

Building
========

**Use CMake:**
<pre>
cd MAMEduino
cmake .
make
</pre>

G++ 4.7 (for C++11) is needed to compile MAMEduino. For installing G++ 4.7 see [here](http://lektiondestages.blogspot.de/2013/05/installing-and-switching-gccg-versions.html).

Usage
========

```
mameduino <SERIAL_DEVICE> <COMMAND>
```  
The SERIAL_DEVICE should be something like /dev/ttyACM0, or you can use the option -a to auto-detect it.  

**Valid commands:**
- -r "on"|"off" Set coin rejection to on or off.
- -s BUTTON# KEY ... Set keyboard keys to send when button is SHORT-pressed (~0.1s).
- -l BUTTON# KEY ... Set keyboard keys to send when button is LONG-pressed (~4s).
- -c COIN# KEY ... Set keyboard keys to send when coin is inserted.
- -d Dump version and current configuration of Arduino program.
- -h/-?/--help Show help.

**Currently valid buttons: 0-4.**  
**Currently valid coins: 0-2.**  
**Up to 6 KEYs are currently supported. Special KEYs are supported by their names:**  
  LCTRL, LSHIFT, LALT, LGUI, RCTRL, RSHIFT, RALT, RGUI, UP, DOWN, LEFT, RIGHT, BACKSPACE, TAB, RETURN, ESC, INSERT, DELETE, PAGEUP, PAGEDOWN, HOME, END, F1-F12  
**Also the reset and power pin/button can be accessed:**  
  PIN_RESET, PIN_POWER (It makes no sense to send more than one "key press" for those...)  
**You can clear key bindings for a button/coin with the keyword:**  
  CLEAR  

**Examples:**  
Turn coin rejection on, auto-detect serial port: ```mameduino -a -r on```  
Set keys to send when button 0 is short-pressed: ```mameduino /dev/ttyS0 -s 0 UP LEFT```  
Pulse power pin when button 3 is long-pressed: ```mameduino /dev/ttyUSB0 -l 3 PIN_POWER```  
Remove key bindings when button 1 is long-pressed: ```mameduino /dev/ttyS0 -l 1 CLEAR```  
Set some keys to send when coin 2 is inserted: ```mameduino /dev/ttyS0 -c 2 b l a h r g```  
Dump current configuration from Arduino to stdout: ```mameduino /dev/ttyACM0 -d```  

An example batch file for starting up an emulator can be found [here](setup_keys_and_run_emulator.sh). Run it with the ROM name as a parameter.

FAQ
========
**Q:** How is this better than an old butchered USB-Keyboard?!  
**A:** Configurable, expandable, coin receptor, PC control, and then some...  

**Q:** I have problems because the Arduino keeps on sending commands to the keyboard and I can't stop it.  
**A:** Pull pin 0 LOW (connect to GND). This will re-route all keyboard commands to the serial port.  

**Q:** When I send commands via mameduino to the Arduino communication is not working properly.  
**A:** Make sure Arduino and PC are using the same MAMEduino version. Reset the Arduino and wait a few seconds for it to boot properly.  

**Q:** I can not access the serial port somehow...  
**A:** You might need to add your USERNAME to the dialout group: ```sudo usermod -a -G dialout USERNAME```.  

I found a bug or have a suggestion
========

The best way to report a bug or suggest something is to post an issue on GitHub. Try to make it simple, but descriptive and add ALL the information needed to REPRODUCE the bug. **"Does not work" is not enough!** If you can not compile, please state your system, compiler version, etc! You can also contact me via email if you want to.
