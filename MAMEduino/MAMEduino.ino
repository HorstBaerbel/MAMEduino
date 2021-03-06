//This is the MAMEduino Arduino Button/Coin receptor interface
//found at http://github.com/HorstBaerbel/MAMEduino
//Make sure the version of the Arduino code matches that of the PC code, or strange things might happen
//In case of bugs/question, please file an issue at the github page or
//drop me an email at: bim.overbohm@googlemail.com

#define PROGRAM_VERSION_STRING "MAMEduino 0.9.9.3"

//----- LEDs (you simply can leave this out if you don't want LEDs) ------------------------------------------------------------
#include <FastLED.h>

//pin for WS2812 LED strip connection
#define PIN_LED_STRIP 1
//array holding LED color data
#define NUM_LEDS 8
CRGB leds[NUM_LEDS];

//define indices of LED types
#define LED_INTERIOR_START 0
#define LED_INTERIOR_END (LED_INTERIOR_START + 5)
#define LED_COIN_RECEPTOR_START (LED_INTERIOR_END + 1)
#define LED_COIN_RECEPTOR_END (LED_COIN_RECEPTOR_START + 1)
//current interior LED color
CRGB interiourColor = CRGB(0, 0, 0);

void ledShowRejectCoin(bool reject)
{
    const CRGB newColor = reject ? CRGB(255, 0, 0) : CRGB(0, 255, 0);
    for (uint8_t i = LED_COIN_RECEPTOR_START; i <= LED_COIN_RECEPTOR_END; ++i) {
        leds[i] = newColor;
    }
    FastLED.show();
}

void ledCycleInteriorColor()
{
    const uint8_t hue = (uint8_t)(millis() >> 7);
    const CRGB newColor = CHSV(hue, 255, 255);
    if (interiourColor != newColor) {
        for (uint8_t i = LED_INTERIOR_START; i <= LED_INTERIOR_END; ++i) {
            leds[i] = newColor;
        }
        interiourColor = newColor;
        FastLED.show();
    }
}

//----- power/reset ------------------------------------------------------------------------

//pin for mainboard power switch
#define PIN_POWER 7
#define CHAR_PIN_POWER 255
//pin for mainboard reset switch
#define PIN_RESET 6
#define CHAR_PIN_RESET (CHAR_PIN_POWER-1)

//this is where the first hardware function starts
#define CHAR_HARDWARE_FUNCTION CHAR_PIN_RESET

//----- keyboard ------------------------------------------------------------------------------------
#include <Keyboard.h>

//pin for redirecting keyboard codes to serial port instead of keyboard
#define PIN_KEY_TO_SERIAL 0

//max number of keys that can be defined
#define KEYS_NUMBER_OF 6

//How long to press a simulated key
#define KEYS_PRESS_DELAY 100
//How long to wait between simulated key presses
#define KEYS_PRESS_NEXT_DELAY 200

void keyboardSendString(const byte keys[KEYS_NUMBER_OF])
{
  int i = 0;
  //SAFETY BELT: check for keyboard to COM redirection
  if (digitalRead(PIN_KEY_TO_SERIAL) == LOW) {
    //on. send to serial port
    while ((i < KEYS_NUMBER_OF) && (keys[i] != 0)) {
      Serial.write(keys[i]);
      i++;
    }
    Serial.write('\n');
  }
  else {
    //off. send as keystrokes
    while ((i < KEYS_NUMBER_OF) && (keys[i] != 0)) {
      //send ALL keys using press/release, because otherwise wrong key codes are sent
      /*if (keys[i] < 128) {
        Keyboard.write(keys[i]);
        delay(KEYS_PRESS_NEXT_DELAY);
      }
      else*/ if (keys[i] < CHAR_HARDWARE_FUNCTION) {
        Keyboard.press(keys[i]);
        delay(KEYS_PRESS_DELAY);
        Keyboard.releaseAll();
        delay(KEYS_PRESS_NEXT_DELAY);
      }
      else {
        //hardware function. yeah, I could have used a map.
        if (keys[i] == CHAR_PIN_RESET) {
          //activate reset pin
          digitalWrite(PIN_RESET, HIGH);
          delay(KEYS_PRESS_DELAY);
          digitalWrite(PIN_RESET, LOW);
        }
        else if (keys[i] == CHAR_PIN_POWER) {
          //activate power pin
          digitalWrite(PIN_POWER, HIGH);
          delay(KEYS_PRESS_DELAY);
          digitalWrite(PIN_POWER, LOW);
        }
      }
      i++;
    }
  }
}

//----- buttons ----------------------------------------------------------------------------

#define BUTTONS_NUMBER_OF 5

#define REJECT_BUTTON 0
#define EXIT_RESET 1
#define PAUSE_CHEAT 2
#define PLAYER_1 3
#define PLAYER_2 4

//pin numbers for buttons
const byte PIN_BUTTON[BUTTONS_NUMBER_OF] = {11, 2, 3, 4, 5};

//characters sent when a button is pressed. the numbers >= 250 are currently reserved for hardware functions
//atm 254=RESET and 255=POWER pin are supported
byte buttonShortPressedString[BUTTONS_NUMBER_OF][KEYS_NUMBER_OF] = {{0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0}, {CHAR_PIN_POWER, 0, 0, 0, 0, 0}};
byte buttonLongPressedString[BUTTONS_NUMBER_OF][KEYS_NUMBER_OF] = {{0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0}};

//button press durations in ms
#define RELEASE_DURATION 50
#define SHORT_PRESS_DURATION 50
#define LONG_PRESS_DURATION 4000

//button state variables. PIN_REJECT_BUTTON, PIN_EXIT_RESET, PIN_PAUSE_CHEAT, PIN_PLAYER_1, PIN_PLAYER_2
byte lastButtonPin[BUTTONS_NUMBER_OF] = {LOW, LOW, LOW, LOW, LOW};
long int lastButtonStart[BUTTONS_NUMBER_OF] = {0, 0, 0, 0, 0};
byte currentButtonState[BUTTONS_NUMBER_OF] = {0, 0, 0, 0, 0}; //0 = released, 1 = short pressed, 2 = long pressed
boolean buttonWasPressedShort[BUTTONS_NUMBER_OF] = {false, false, false, false, false};
boolean buttonWasPressedLong[BUTTONS_NUMBER_OF] = {false, false, false, false, false};

void buttonsReadState()
{
  //the state must be button signal high, hold high SHORT_PRESS_DURATION, signal low, hold low RELEASE_DURATION -> valid short press
  //the state must be button signal high, hold high LONG_PRESS_DURATION, signal low, hold low RELEASE_DURATION -> valid long press
  int i = 0;
  for (; i < BUTTONS_NUMBER_OF; i++) {
    //check what state the pin is in
    const byte state = digitalRead(PIN_BUTTON[i]);
    //check if the state is the same as before
    if (state == lastButtonPin[i]) {
      //state is the same. check what it is
      if (state == LOW) {
        //button still pressed
        if (currentButtonState[i] == 0) {
          //button was released before. check if enough time has passed to get a short/long press
          if ((millis() - lastButtonStart[i]) >= SHORT_PRESS_DURATION) {
            //Serial.print("Short press button #"); Serial.println(i);
            currentButtonState[i] = 1;
          }
        }
        if (currentButtonState[i] == 1) {
          //button was pressed before. check if enough time has passed to get a long press
          if ((millis() - lastButtonStart[i]) >= LONG_PRESS_DURATION) {
            //Serial.print("Long press button #"); Serial.println(i);
            currentButtonState[i] = 2;
          }
        }
      }
      else {
        //button still unpressed
        if (currentButtonState[i] == 1) {
          //button was short pressed and released. was the release long enough
          if ((millis() - lastButtonStart[i]) >= RELEASE_DURATION) {
            //Serial.print("Short release button #"); Serial.println(i);
            buttonWasPressedShort[i] = true;
            buttonWasPressedLong[i] = false;
            currentButtonState[i] = 0;
          }
        }
        else if (currentButtonState[i] == 2) {
          //button was pressed and released. was the release long enough
          if ((millis() - lastButtonStart[i]) >= RELEASE_DURATION) {
            //Serial.print("Long release button #"); Serial.println(i);
            buttonWasPressedShort[i] = false;
            buttonWasPressedLong[i] = true;
            currentButtonState[i] = 0;
          }
        }
      }
    }
    else {
      //state changed. store time
      lastButtonStart[i] = millis();
      lastButtonPin[i] = state;
      //Serial.print(i); Serial.print(" = "); Serial.println(state);
    }
  }
}

void buttonsDoCommands()
{
  int i = 0;
  for (; i < BUTTONS_NUMBER_OF; i++) {
    if (buttonWasPressedLong[i]) {
      keyboardSendString(buttonLongPressedString[i]);
      buttonWasPressedLong[i] = false;
      buttonWasPressedShort[i] = false;
    }
    else if (buttonWasPressedShort[i]) {
      keyboardSendString(buttonShortPressedString[i]);
      buttonWasPressedLong[i] = false;
      buttonWasPressedShort[i] = false;
    }
  }
}

//----- coin interface -----------------------------------------------------------------------------

#define COINS_NUMBER_OF 3

#define COIN_0 0
#define COIN_1 1
#define COIN_2 2

//pin numbers for coins
const byte PIN_COIN[COINS_NUMBER_OF] = {8, 9, 10};
//pin for the signal to reject all coins
#define PIN_REJECT_COINS 12

//characters sent when a coin is inserted. Coin 1, 2, 3
byte coinInsertedString[COINS_NUMBER_OF][KEYS_NUMBER_OF] = {{0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0}};

//minumum coin signal duration
#define COIN_INSERT_DURATION 70

//coin slot state variables. Coin 1, 2, 3
byte lastCoinPin[COINS_NUMBER_OF] = {LOW, LOW, LOW};
long int lastCoinStart[COINS_NUMBER_OF] = {0, 0, 0};
byte currentCoinState[COINS_NUMBER_OF] = {0, 0, 0}; //0 = released, 1 = inserted
boolean coinWasInserted[COINS_NUMBER_OF] = {false, false, false};

void coinsReadState()
{
  //the state must be insert signal on, hold on COIN_INSERT_DURATION, signal off, hold off COIN_INSERT_DURATION -> valid insertion
  int i = 0;
  for (; i < COINS_NUMBER_OF; i++) {
    //check what state the pin is in
    const byte state = digitalRead(PIN_COIN[i]);
    //check if the state is the same as before
    if (state == lastCoinPin[i]) {
      //state is the same. check what it is
      if (state == LOW) {
        //coin insert signal still ON
        if (currentCoinState[i] == 0) {
          //coin insert signal was off before. check if enough time has passed to get a proper insert signal
          if ((millis() - lastCoinStart[i]) >= COIN_INSERT_DURATION) {
            currentCoinState[i] = 1;
          }
        }
      }
      else {
        //coin insert signal still OFF
        if (currentCoinState[i] == 1) {
          //coin insert signal was on and now is off. was the off time long enough?
          if ((millis() - lastCoinStart[i]) >= COIN_INSERT_DURATION) {
            coinWasInserted[i] = true;
            currentCoinState[i] = 0;
          }
        }
      }
    }
    else {
      //state changed. store time
      lastCoinStart[i] = millis();
      lastCoinPin[i] = state;
    }
  }
}

void coinsDoCommands()
{
  int i = 0;
  for (; i < COINS_NUMBER_OF; i++) {
    if (coinWasInserted[i]) {
      keyboardSendString(coinInsertedString[i]);
      coinWasInserted[i] = false;
    }
  }
}

//----- serial commands ----------------------------------------------------------------------------

#define COMMAND_UNKNOWN 0
#define COMMAND_SET_COIN_REJECT 'R' //set coin rejection to off (0b) or on (> 0b)
#define COMMAND_SET_BUTTON_SHORT 'S' //set keys sent on short button press. followed by 1 byte button number and KEYS_NUMBER_OF bytes of key codes. unused codes must be 0!
#define COMMAND_SET_BUTTON_LONG 'L' //set keys sent on short button press. followed by 1 byte button number and KEYS_NUMBER_OF bytes of key codes. unused codes must be 0!
#define COMMAND_SET_COIN 'C' //set keys sent on coin insertion. followed by 1 byte coin number and KEYS_NUMBER_OF bytes of key codes. unused codes must be 0!
#define COMMAND_DUMP_CONFIG 'D' //dump version information and all configured data to serial port after waiting for a short while. for debug purposes.
#define COMMAND_CHECK_VERSION '?' //send version string to serial port. used by the PC side to find MAMEduino serial port.
#define COMMAND_OK "OK\n" //Response sent when a command is detected.
#define COMMAND_NOK "NK\n" //Response sent when the command or its arguments are not ok.
#define COMMAND_TERMINATOR 10 //Terminate lines with LF aka '\n'

void clearKeys(byte array[KEYS_NUMBER_OF])
{
  for (int i = 0; i < KEYS_NUMBER_OF; i++) {
    array[i] = 0;
  }
}

void serialReadKeys(byte array[KEYS_NUMBER_OF])
{
  //clear all buttons first
  clearKeys(array);
  int i = 0;
  int key;
  do {
    key = Serial.read();
    if (key != COMMAND_TERMINATOR) {
      array[i] = key;
      i++;
    }
    //Keyboard.print(" "); Keyboard.print(key);
  } while ((i < KEYS_NUMBER_OF) && (key != COMMAND_TERMINATOR));
}

void serialDumpConfig()
{
  Serial.println(PROGRAM_VERSION_STRING);
  for (int ib = 0; ib < BUTTONS_NUMBER_OF; ib++) {
    Serial.print("Button #");
    Serial.print(ib);
    Serial.print(" short: ");
    for (int ik = 0; ik < KEYS_NUMBER_OF; ik++) {
      Serial.print(buttonShortPressedString[ib][ik]); Serial.print(' ');
    }
    Serial.print("long: ");
    for (int ik = 0; ik < KEYS_NUMBER_OF; ik++) {
      Serial.print(buttonLongPressedString[ib][ik]); Serial.print(' ');
    }
    Serial.println();
  }
  for (int ic = 0; ic < COINS_NUMBER_OF; ic++) {
    Serial.print("Coin #");
    Serial.print(ic);
    Serial.print(": ");
    for (int ik = 0; ik < KEYS_NUMBER_OF; ik++) {
      Serial.print(coinInsertedString[ic][ik]); Serial.print(' ');
    }
    Serial.println();
  }
  Serial.print("Coin rejection is ");
  if (digitalRead(PIN_REJECT_COINS) == HIGH) {
    Serial.println("ON");
  }
  else {
    Serial.println("OFF");
  }
}

void serialReadCommand()
{
  static int serialCommand = COMMAND_UNKNOWN;
  static int serialBytesNeeded = 1;
  //check if command or data
  if (Serial.available() >= serialBytesNeeded && serialCommand == COMMAND_UNKNOWN) {
    //read command byte
    serialCommand = Serial.read();
    switch (serialCommand) {
      case COMMAND_SET_COIN_REJECT:
        //we expect another byte stating true or false here
        //Keyboard.print("Coin reject commmand");
        serialBytesNeeded = 2;
        break;
      case COMMAND_SET_BUTTON_SHORT:
      case COMMAND_SET_BUTTON_LONG:
      case COMMAND_SET_COIN:
        //we expect another byte with the button number here and then 1-6 key codes
        //Keyboard.print("Button/coin command");
        serialBytesNeeded = 3;
        break;
      case COMMAND_DUMP_CONFIG:
        //flush commands
        while (Serial.available() > 0) {
          Serial.read();
        }
        //wait for next command
        serialCommand = COMMAND_UNKNOWN;
        serialBytesNeeded = 1;
        //dump configuration
        serialDumpConfig();
        //send positive response
        Serial.print(COMMAND_OK);
        break;
      case COMMAND_CHECK_VERSION:
        //flush commands
        while (Serial.available() > 0) {
          Serial.read();
        }
        //wait for next command
        serialCommand = COMMAND_UNKNOWN;
        serialBytesNeeded = 1;
        //dump version string
        Serial.print(PROGRAM_VERSION_STRING);
        //send positive response
        Serial.print(COMMAND_OK);
        break;
      default:
        //unknown command. flush port
        //Keyboard.print("Bad command");
        while (Serial.available() > 0) {
          Serial.read();
        }
        //read until we find a valid command
        serialCommand = COMMAND_UNKNOWN;
        serialBytesNeeded = 1;
        //invalid command. send negative response
        Serial.write(COMMAND_NOK);
        return;
    }
  }
  if (Serial.available() >= serialBytesNeeded && serialCommand != COMMAND_UNKNOWN) {
    //read data depending on command
    switch (serialCommand) {
        case COMMAND_SET_COIN_REJECT:
          //we expect another byte stating true or false here
          if (Serial.read() > 0) {
            //Keyboard.print(" ON");
            digitalWrite(PIN_REJECT_COINS, HIGH);
            ledShowRejectCoin(true);
          }
          else {
            //Keyboard.print(" OFF");
            digitalWrite(PIN_REJECT_COINS, LOW);
            ledShowRejectCoin(false);
          }
	      //valid command. send positive response
	      Serial.write(COMMAND_OK);
          break;
        case COMMAND_SET_BUTTON_SHORT:
        case COMMAND_SET_BUTTON_LONG: {
          //we expect another byte with the button number here and then 5 key codes
          int button = Serial.read();
          if (button < 0) {
            button = 0;
          }
          else if (button > (BUTTONS_NUMBER_OF - 1)) {
            button = (BUTTONS_NUMBER_OF - 1);
          }
          //Keyboard.print(" "); Keyboard.print(button);
          if (COMMAND_SET_BUTTON_SHORT == serialCommand) {
            serialReadKeys(buttonShortPressedString[button]);
          }
          else if (COMMAND_SET_BUTTON_LONG == serialCommand) {
            serialReadKeys(buttonLongPressedString[button]);
          }
	  //valid command. send positive response
	  Serial.write(COMMAND_OK);
          break;
        }
        case COMMAND_SET_COIN: {
          //we expect another byte with the coin number here and then 5 key codes
          int coin = Serial.read();
          if (coin < 0) {
            coin = 0;
          }
          else if (coin > (COINS_NUMBER_OF - 1)) {
            coin = (COINS_NUMBER_OF - 1);
          }
          //Keyboard.print(" "); Keyboard.print(coin);
          serialReadKeys(coinInsertedString[coin]);
	  //valid command. send positive response
	  Serial.write(COMMAND_OK);
          break;
        }
    }
    //finished. flush port
    while (Serial.available() > 0) {
      Serial.read();
    }
    //wait for next command
    serialCommand = COMMAND_UNKNOWN;
    serialBytesNeeded = 1;
  }
}

//----- main ------------------------------------------------------------------------------------------

void setup()
{
  //setup mainboard I/O pins
  pinMode(PIN_POWER, OUTPUT);
  digitalWrite(PIN_POWER, LOW); //power off
  pinMode(PIN_RESET, OUTPUT);
  digitalWrite(PIN_RESET, LOW); //no reset

  //setup keyboard redirection pin
  pinMode(PIN_KEY_TO_SERIAL, INPUT);
  digitalWrite(PIN_KEY_TO_SERIAL, HIGH); //enable pullup

  //setup coin I/O pins
  pinMode(PIN_REJECT_COINS, OUTPUT);
  digitalWrite(PIN_REJECT_COINS, HIGH); //startup rejecting all coins
  int i = 0;
  for (; i < COINS_NUMBER_OF; i++) {
    pinMode(PIN_COIN[i], INPUT_PULLUP);
  }

  //setup button input pins
  i = 0;
  for (; i < BUTTONS_NUMBER_OF; i++) {
    pinMode(PIN_BUTTON[i], INPUT_PULLUP);
  }

  //setup LED strip
  FastLED.addLeds<WS2812, PIN_LED_STRIP, GRB>(leds, NUM_LEDS);
  FastLED.setCorrection(TypicalLEDStrip);
  FastLED.setDither(DISABLE_DITHER);
  //show coin receptor status with LEDs
  ledShowRejectCoin(true);
  FastLED.show();

  //open the serial port
  Serial.begin(38400);

  //initialize control over the keyboard
  Keyboard.begin();
}

void loop()
{
  //check for incoming commands via serial port
  serialReadCommand();
  //check buttons and issue commands
  buttonsReadState();
  buttonsDoCommands();
  //read coin states and issue commands
  coinsReadState();
  coinsDoCommands();
  //do color-cycling of interior color
  ledCycleInteriorColor();
}

