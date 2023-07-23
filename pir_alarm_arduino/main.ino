/*
 * PIR sensor tester
 */
 
int ledPin = 13;                // choose the pin for the LED
int pirPin = 7;               // choose the input pin (for PIR sensor)
int pirState = LOW;             // we start, assuming no motion detected
int val = 0;                    // variable for reading the pin status
 
int buzzerPin = 5;
int playing = 0;
int bellPin = 2;
int alarmPin = 6; //LOW for disable alarm
long lastmillis;

void setup() {
  pinMode(ledPin, OUTPUT);      // declare LED as output
  pinMode(pirPin, INPUT);     // declare sensor as input

  
  pinMode(buzzerPin, OUTPUT);
  digitalWrite(buzzerPin, HIGH);
  //configure pin2 as an input and enable the internal pull-up resistor
  pinMode(bellPin, INPUT_PULLUP); // bel
  pinMode(alarmPin, INPUT_PULLUP); // 
  lastmillis=millis();
  
  Serial.begin(9600);
}
 
void loop(){
  if(playing==1)return;
  
  int bell = digitalRead(bellPin);
  int alarm = digitalRead(alarmPin);
  //Serial.println(alarm==LOW?"LOW":"HIGH");
  if(bell== LOW)
  {
    playSound();
    return;
  }
  if(alarm == LOW)return; //disable alarm
  
  val = digitalRead(pirPin);  // read input value
  if (val == HIGH) {            // check if the input is HIGH
    //digitalWrite(ledPin, HIGH);  // turn LED ON
    if (pirState == LOW) {
      // we have just turned on
      Serial.println("Motion detected!");
      playAlarm();
      // We only want to print on the output change, not state
      pirState = HIGH;
    }
  } else {
    //digitalWrite(ledPin, LOW); // turn LED OFF
    if (pirState == HIGH){
      // we have just turned of
      Serial.println("Motion ended!");
      // We only want to print on the output change, not state
      pirState = LOW;
    }
  }
}

void playSound()
{
  playing=1;
  digitalWrite(buzzerPin, LOW);
  digitalWrite(ledPin, HIGH);
  delay(2000);
  digitalWrite(ledPin, LOW);
  digitalWrite(buzzerPin, HIGH);  
  playing=0;
}

void playAlarm()
{
  playing=1;
  digitalWrite(buzzerPin, LOW);
  digitalWrite(ledPin, HIGH);
  delay(200);
  digitalWrite(ledPin, LOW);
  digitalWrite(buzzerPin, HIGH); 
  delay(200);  
  digitalWrite(buzzerPin, LOW);
  digitalWrite(ledPin, HIGH);
  delay(200); 
  digitalWrite(ledPin, LOW);
  digitalWrite(buzzerPin, HIGH); 
  delay(200);
  digitalWrite(buzzerPin, LOW);
  digitalWrite(ledPin, HIGH);
  delay(200);
  digitalWrite(ledPin, LOW);
  digitalWrite(buzzerPin, HIGH); 
  playing=0;
}
