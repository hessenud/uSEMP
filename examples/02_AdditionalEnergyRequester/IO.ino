


void loopIO()
{
  buttonControl(); // Manual ctrl of relay
  loopTimeClk();

  digitalWrite(ledPin, ledState );
}


void setupIO()
{
    // initialize digital pin LED_BUILTIN as an output.
  pinMode(ledPin, OUTPUT);

  pinMode(buttonPin, INPUT);
  digitalWrite(ledPin,   ledState );

}
