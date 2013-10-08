MAMEduino
========

Use an Arduino Leonardo to connect physical (arcade-style) buttons and/or a coin receptor to a PC. The Arduino then sends a configurable number of keypresses to the PC when buttons are pressed or coins are inserted, and can also startup/shutdown or reset the PC with the appropriate pins connected. The keypresses sent can be configured via small console program which currently works on Linux only (tested on Ubuntu 13.04). 
A Fritzing layout for the connection can be found in [MAMEduino.fzz](https://github.com/HorstBaerbel/MAMEduino/blob/master/MAMEduino.fzz) and the necessary Arduino source code in [MAMEduino/MAMEduino.ino](https://github.com/HorstBaerbel/MAMEduino/blob/master/MAMEduino/MAMEduino.ino).

License
========

[BSD-2-Clause](http://opensource.org/licenses/BSD-2-Clause), see [LICENSE.md](https://github.com/HorstBaerbel/MAMEduino/blob/master/LICENSE.md).

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

**Valid commands:**
- -r "on"|"off" Set coin rejection to on or off.
- -s BUTTON# KEY ... Set keyboard keys to send when button is SHORT-pressed (~0.3s).
- -l BUTTON# KEY ... Set keyboard keys to send when button is LONG-pressed (~5s).
- -c COIN# KEY ... Set keyboard keys to send when coin is inserted.
- -v Be verbose.

**Currently valid buttons: 0-4.**  
**Currently valid coins: 0-2.**  
**Up to 5 keys are currently supported. Special keys are supported by their names:**  
  LCTRL, LSHIFT, LALT, LGUI, RCTRL, RSHIFT, RALT, RGUI, UP, DOWN, LEFT, RIGHT, BACKSPACE, TAB, RETURN, ESC, INSERT, DELETE, PAGEUP, PAGEDOWN, HOME, END, F1-F12

**Examples:** 
Turn coin rejection on: ```mameduino /dev/ttyUSB0 -r on```  
Set keys to send when button 0 is pressed: ```mameduino /dev/ttyS0 -s 0 UP UP LEFT```  
Set keys to send when coin 2 is inserted: ```mameduino /dev/ttyS0 -c 2 b l a r g```  

I found a bug or have suggestion
========

The best way to report a bug or suggest something is to post an issue on GitHub. Try to make it simple, but descriptive and add ALL the information needed to REPRODUCE the bug. **"Does not work" is not enough!** If you can not compile, please state your system, compiler version, etc! You can also contact me via email if you want to.
