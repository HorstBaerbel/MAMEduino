//----- keyboard ------------------------------------------------------------------------------------

//pin for redirecting keyboard codes to serial port instead of keyboard
#define PIN_KEY_TO_SERIAL 13

//max number of keys that can be defined
#define KEYS_NUMBER_OF 5

//How long to wait between simulated key presses
#define KEYS_PRESS_NEXT_DELAY 300

void keyboardSendString(byte keys[5])
{
  byte i = 0;
  //check for keyboard to COM redirection
  if (digitalRead(PIN_KEY_TO_SERIAL) == HIGH) {
    //on. send to serial port
    while (i < KEYS_NUMBER_OF && keys[i] != 0) {
      Serial.write(keys[i]);
      i++;
    }
    Serial.write('\n');
  }
  else {
    //off. send as keystrokes
    while (i < KEYS_NUMBER_OF && keys[i] != 0) {
      Keyboard.write(keys[i]);
      i++;
      if (i < KEYS_NUMBER_OF && keys[i] != 0) {
        //there is a next key. insert delay.
        delay(KEYS_PRESS_NEXT_DELAY);
      }
    }
  }
}

//----- power/reset ------------------------------------------------------------------------

//pin for mainboard reset switch
#define PIN_RESET 6
//pin for mainboard power switch
#define PIN_POWER 7

//----- buttons ----------------------------------------------------------------------------

#define BUTTONS_NUMBER_OF 5

#define REJECT_BUTTON 0
#define EXIT_RESET 1
#define PAUSE_CHEAT 2
#define PLAYER_1 3
#define PLAYER_2 4

//pin numbers for buttons
static const byte PIN_BUTTON[BUTTONS_NUMBER_OF] = {11, 2, 3, 4, 5};

//characters sent when a button is pressed
byte buttonShortPressedString[BUTTONS_NUMBER_OF][KEYS_NUMBER_OF] = {{0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}};
byte buttonLongPressedString[BUTTONS_NUMBER_OF][KEYS_NUMBER_OF] = {{0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}};

//button press durations in ms
#define RELEASE_DURATION 100
#define SHORT_PRESS_DURATION 200
#define LONG_PRESS_DURATION 5000

//button state variables. PIN_REJECT_BUTTON, PIN_EXIT_RESET, PIN_PAUSE_CHEAT, PIN_PLAYER_1, PIN_PLAYER_2
byte lastButtonPin[BUTTONS_NUMBER_OF] = {LOW, LOW, LOW, LOW, LOW};
int lastButtonStart[BUTTONS_NUMBER_OF] = {0, 0, 0, 0, 0};
byte currentButtonState[BUTTONS_NUMBER_OF] = {0, 0, 0, 0, 0}; //0 = released, 1 = short pressed, 2 = long pressed
boolean buttonWasPressedShort[BUTTONS_NUMBER_OF] = {false, false, false, false, false};
boolean buttonWasPressedLong[BUTTONS_NUMBER_OF] = {false, false, false, false, false};

void buttonsReadState()
{
  //the state must be button signal high, hold high SHORT_PRESS_DURATION, signal low, hold low RELEASE_DURATION -> valid short press
  //the state must be button signal high, hold high LONG_PRESS_DURATION, signal low, hold low RELEASE_DURATION -> valid long press
  for (byte i = 0; i < BUTTONS_NUMBER_OF; i++) {
    //check what state the pin is in
    byte state = digitalRead(PIN_BUTTON[i]);
    //check if the state is the same as before
    if (state == lastButtonPin[i]) {
      //state is the same. check what it is
      if (state == LOW) {
        //button still pressed
        if (currentButtonState[i] == 0) {
          //button was released before. check if enough time has passed to get a short/long press
          if ((millis() - lastButtonStart[i]) >= SHORT_PRESS_DURATION) {
            currentButtonState[i] = 1;
          }
        }
        if (currentButtonState[i] == 1) {
          //button was pressed before. check if enough time has passed to get a long press
          if ((millis() - lastButtonStart[i]) >= LONG_PRESS_DURATION) {
            currentButtonState[i] = 2;
          }
        }
      }
      else {
        //button still unpressed
        if (currentButtonState[i] == 1) {
          //button was short pressed and released. was the release long enough
          if ((millis() - lastButtonStart[i]) >= RELEASE_DURATION) {
            buttonWasPressedShort[i] = true;
            currentButtonState[i] = 0;
          }
        }
        else if (currentButtonState[i] == 2) {
          //button was pressed and released. was the release long enough
          if ((millis() - lastButtonStart[i]) >= RELEASE_DURATION) {
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
    }
  }
}

void buttonsDoCommands()
{
  for (byte i = 0; i < BUTTONS_NUMBER_OF; i++) {
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
static const byte PIN_COIN[COINS_NUMBER_OF] = {8, 9, 10};
//pin for the signal to reject all coins
#define PIN_REJECT_COINS 12

//characters sent when a coin is inserted. Coin 1, 2, 3
byte coinInsertedString[COINS_NUMBER_OF][KEYS_NUMBER_OF] = {{0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}};

//minumum coin signal duration
#define COIN_INSERT_DURATION 70

//coin slot state variables. Coin 1, 2, 3
byte lastCoinPin[COINS_NUMBER_OF] = {LOW, LOW, LOW};
int lastCoinStart[COINS_NUMBER_OF] = {0, 0, 0};
byte currentCoinState[COINS_NUMBER_OF] = {0, 0, 0}; //0 = released, 1 = inserted
boolean coinWasInserted[COINS_NUMBER_OF] = {false, false, false};

void coinsReadState()
{
  //the state must be insert signal on, hold on COIN_INSERT_DURATION, signal off, hold off COIN_INSERT_DURATION -> valid insertion
  for (byte i = 0; i < COINS_NUMBER_OF; i++) {
    //check what state the pin is in
    byte state = digitalRead(PIN_COIN[i]);
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
  for (byte i = 0; i < COINS_NUMBER_OF; i++) {
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
#define COMMAND_OK "OK" //Response sent when a command is detected.
#define COMMAND_NOK "NOK" //Response sent when the command or its arguments are not ok.

static byte serialCommand = COMMAND_UNKNOWN;
static byte serialBytesNeeded = 1;

void serialReadCommand()
{
  if (Serial.available() >= serialBytesNeeded) {
    //check if command or data
    if (serialCommand == COMMAND_UNKNOWN) {
      //read command byte
      byte serialCommand = Serial.read();
      switch (serialCommand) {
        case COMMAND_SET_COIN_REJECT:
          //we expect another byte stating true or false here
          serialBytesNeeded = 1;
          break;
        case COMMAND_SET_BUTTON_SHORT:
        case COMMAND_SET_BUTTON_LONG:
        case COMMAND_SET_COIN:
          //we expect another byte with the button number here and then 5 key codes
          serialBytesNeeded = KEYS_NUMBER_OF + 1;
          break;
        default:
          //unknown command. read until we find a valid command
          serialCommand = COMMAND_UNKNOWN;
          serialBytesNeeded = 1;
		  //invalid command. send negative response
		  Serial.write(COMMAND_NOK);
      }
    }
    if (serialCommand != COMMAND_UNKNOWN) {
      //read data depending on command
      switch (serialCommand) {
          case COMMAND_SET_COIN_REJECT:
            //we expect another byte stating true or false here
            if (Serial.read() > 0) {
              digitalWrite(PIN_REJECT_COINS, HIGH);
            }
            else {
              digitalWrite(PIN_REJECT_COINS, LOW);
            }
			//valid command. send response
			Serial.write(COMMAND_OK);
            break;
          case COMMAND_SET_BUTTON_SHORT:
          case COMMAND_SET_BUTTON_LONG: {
            //we expect another byte with the button number here and then 5 key codes
            byte button = Serial.read();
            if (button < 0) {
              button = 0;
            }
            else if (button > (BUTTONS_NUMBER_OF - 1)) {
              button = (BUTTONS_NUMBER_OF - 1);
            }
            for (byte i = 0; i < KEYS_NUMBER_OF; i++) {
              if (COMMAND_SET_BUTTON_SHORT == serialCommand) {
                buttonShortPressedString[button][i] = Serial.read();
              }
              else if (COMMAND_SET_BUTTON_LONG == serialCommand) {
                buttonLongPressedString[button][i] = Serial.read();
              }
            }
			//valid command. send response
			Serial.write(COMMAND_OK);
            break;
          }
          case COMMAND_SET_COIN: {
            //we expect another byte with the coin number here and then 5 key codes
            byte coin = Serial.read();
            if (coin < 0) {
              coin = 0;
            }
            else if (coin > (COINS_NUMBER_OF - 1)) {
              coin = (COINS_NUMBER_OF - 1);
            }
            for (byte i = 0; i < KEYS_NUMBER_OF; i++) {
              coinInsertedString[coin][i] = Serial.read();
            }
			//valid command. send response
			Serial.write(COMMAND_OK);
            break;
          }
      }
      //finished. wait for next command
      serialCommand = COMMAND_UNKNOWN;
      serialBytesNeeded = 1;
    }
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

  //setup coin I/O pins
  pinMode(PIN_REJECT_COINS, OUTPUT);
  digitalWrite(PIN_REJECT_COINS, HIGH); //startup rejecting all coins
  for (byte i = 0; i < COINS_NUMBER_OF; i++) {
    pinMode(PIN_COIN[i], INPUT_PULLUP);
  }

  //setup button input pins
  for (byte i = 0; i < BUTTONS_NUMBER_OF; i++) {
    pinMode(PIN_BUTTON[i], INPUT_PULLUP);
  }

  //open the serial port
  Serial.begin(9600);

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
}

