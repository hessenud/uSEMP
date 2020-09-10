


void loopIO()
{
  buttonControl(); // Manual ctrl of relay
  loopTimeClk();

  digitalWrite(relayPin, relayState );
  digitalWrite(ledPin, ledState );
}


void setupIO()
{
    // initialize digital pin LED_BUILTIN as an output.
  pinMode(ledPin, OUTPUT);
  pinMode(relayPin, OUTPUT);

  pinMode(buttonPin, INPUT);
  digitalWrite(relayPin, relayState );
  digitalWrite(ledPin,   ledState );

}
