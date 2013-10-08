void setup() {
  // initialize serial communication at 9600 bits per second:
  Serial.begin(9600);
  Keyboard.begin();
  //setup buttons
  pinMode(2, INPUT_PULLUP); //Button 1
  pinMode(3, INPUT_PULLUP); //Button 2
  pinMode(4, INPUT_PULLUP); //Button 3
  pinMode(5, INPUT_PULLUP); //Button 4
  //setup mainboard connection
  pinMode(6, OUTPUT); //Reset pin
  digitalWrite(6, LOW); //active high pulse
  pinMode(7, OUTPUT); //Power on pin
  digitalWrite(7, LOW); //active high pulse
  //setup coin detector
  pinMode(8, INPUT_PULLUP); //Coin 1
  pinMode(9, INPUT_PULLUP); //Coin 2
  pinMode(10, INPUT_PULLUP); //Coin 3
  pinMode(11, INPUT_PULLUP); //Reject button
  pinMode(12, OUTPUT); //Accept all coins
  digitalWrite(12, LOW);
}

void loop() {
  // read the input pin:
  int buttonState2 = digitalRead(2);
  int buttonState3 = digitalRead(3);
  int buttonState4 = digitalRead(4);
  int buttonState5 = digitalRead(5);
  int buttonState8 = digitalRead(8);
  int buttonState9 = digitalRead(9);
  int buttonState10 = digitalRead(10);
  int buttonState11 = digitalRead(11);
  // print out the state of the buttons:
  Serial.print(buttonState2);
  Serial.print(buttonState3);
  Serial.print(buttonState4);
  Serial.print(buttonState5);
  Serial.print(buttonState8);
  Serial.print(buttonState9);
  Serial.print(buttonState10);
  Serial.print(buttonState11);
  Serial.print('\n');
  delay(25);        // delay in between reads for stability
  if(buttonState2 == LOW) {
    Keyboard.write('A');
    digitalWrite(6, HIGH); //push reset
  }
  else {
    digitalWrite(6, LOW);
  }
  if(buttonState3 == LOW) {
    Keyboard.write('B');
    digitalWrite(7, HIGH); //push power
  }
  else {
    digitalWrite(7, LOW);
  }
  if(buttonState4 == LOW) {
    Keyboard.write('C');
  }
  if(buttonState5 == LOW) {
    Keyboard.write('D');
  }
  if(buttonState8 == LOW) {
    Keyboard.write('1');
  }
  if(buttonState9 == LOW) {
    Keyboard.write('2');
  }
  if(buttonState10 == LOW) {
    Keyboard.write('3');
  }
}
