/*
  Debounce

*/
int buttonState     = HIGH;   // the current reading from the input pin
int lastButtonState = HIGH;   // the previous reading from the input pin

// the following variables are unsigned longs because the time, measured in
// milliseconds, will quickly become a bigger number than can be stored in an int.
unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long debounceDelay = 50;    // the debounce time; increase if the output flickers


int readButtonState() {
  // read the state of the switch into a local variable:
  int reading = digitalRead(buttonPin);

  // check to see if you just pressed the button
  // (i.e. the input went from LOW to HIGH), and you've waited long enough
  // since the last press to ignore any noise:

  // If the switch changed, due to noise or pressing:
  if (reading != lastButtonState) {
    // reset the debouncing timer
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    // whatever the reading is at, it's been there for longer than the debounce
    // delay, so take it as the actual current state:
  
    // if the button state has changed:
    if (reading != buttonState) {
      buttonState = reading;
      Serial.printf(" Key: %s\n", buttonState ? "HIGH" :"LOW");
    }
  }
  // save the reading. Next time through the loop, it'll be the lastButtonState:
  lastButtonState = reading;
  return buttonState;
}



void buttonControl()
{
  static bool pressed=false;
  int button = readButtonState() ;
  if ( (!pressed) && button == LOW ){
    pressed = true;
    requestDailyPlan(0);
  } 
  if ( button == HIGH ) pressed = false;
  
}
