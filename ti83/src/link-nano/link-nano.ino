
const int LED = 16; // 16 on pico

const int TIP_LED = 14;
const int RING_LED = 15;

const int TIP = 2; //arduino port where the tip of the stereo jack plug is connected
const int RING = 3; //arduino port where the middle part of the stereo jack plug is connected

const unsigned long TXTIMEOUT = 50; //ms
const unsigned long RXTIMEOUT = 5; //ms


void syncTip() {
  bool tip = digitalRead( TIP );
  digitalWrite( TIP_LED, tip );
}

void sendByte(uint8_t byte) {
  unsigned long currentTime;
  
  for (int i = 0; i < 8; ++i) {
    bool bit = byte & 0x01;
    byte >>= 1;

    //poll both lines until they are both high, which means we're ready to send a bit
    currentTime = millis();
    while ( digitalRead(TIP) == LOW || digitalRead(RING) == LOW ) {
      digitalWrite( TIP_LED, digitalRead( TIP ) );
      digitalWrite( RING_LED, digitalRead( RING ) );
      if (millis() - currentTime > TXTIMEOUT) {
        return;
      }
    }

    int ourLine;
    int ourLed;
    int oppositeLine;
    int oppositeLed;
    if ( bit ) {
      ourLine = RING;
      ourLed = RING_LED;
      oppositeLine = TIP;
      oppositeLed = TIP_LED;
    } else {
      ourLine = TIP;
      ourLed = TIP_LED;
      oppositeLine = RING;
      oppositeLed = RING_LED;
    }

    //send a bit by pulling appropriate line low
    pinMode(ourLine, OUTPUT);
    digitalWrite( ourLine, LOW );
    digitalWrite( ourLed, LOW );

    //wait for opposite line to become low
    currentTime = millis();
    while (digitalRead(oppositeLine) == HIGH) {
      if (millis() - currentTime > TXTIMEOUT) {
        pinMode(ourLine, INPUT_PULLUP);
        return;
      }
    }
    digitalWrite( oppositeLed, LOW );

    //release our line
    pinMode(ourLine, INPUT_PULLUP);

    //wait for opposite line to become high
    currentTime = millis();
    while (digitalRead(oppositeLine) == LOW) {
      if (millis() - currentTime > TXTIMEOUT) {
        return;
      }
    }
    digitalWrite( oppositeLed, HIGH );
  }
}

bool getByte(uint8_t& byte) {
  unsigned long currentTime;
  uint8_t result = 0;

  for (int i = 0; i < 8; ++i) {
    //poll both lines until one of them becomes low
    currentTime = millis();
    while (digitalRead(TIP) == HIGH && digitalRead(RING) == HIGH) {
      digitalWrite( TIP_LED, digitalRead( TIP ) );
      digitalWrite( RING_LED, digitalRead( RING ) );
      if (millis() - currentTime > RXTIMEOUT) {
        return false;
      }
    }

    int ourLine;
    int ourLed;
    int oppositeLine;
    int oppositeLed;
    bool bit = (digitalRead(RING) == LOW);
    result >>= 1;
    if ( bit ) {
      ourLine = TIP;
      ourLed = TIP_LED;
      oppositeLine = RING;
      oppositeLed = RING_LED;
      result |= 0x80; //bits are always transmitted LSb first (least significant bit)
    } else {
      ourLine = RING;
      ourLed = RING_LED;
      oppositeLine = TIP;
      oppositeLed = TIP_LED;
    }

    //acknowledge a bit by pulling appropriate line low
    pinMode(ourLine, OUTPUT);
    digitalWrite( ourLine, LOW );
    digitalWrite( ourLed, LOW );

    //wait for opposite line to become high
    currentTime = millis();
    while ( digitalRead( oppositeLine) == LOW ) {
      if (millis() - currentTime > RXTIMEOUT) {
        pinMode( ourLine, INPUT_PULLUP );
        return false;
      }
    }
    digitalWrite( oppositeLed, HIGH );

    //release our line
    pinMode(ourLine, INPUT_PULLUP);
  }

  byte = result;
  return true;
}

void testpins() {
    // 2, 4, 5 12-19 21-23 25-26 32-39
    // oled 4, 15, 16
    // btn-0 0, power detect 13, vext 21, led1 25 crystal 32, 33
    // tx/rx 1, 3
    // flash 6-11
}

void setup() {
  //turn off the built-in LED
  //pinMode(LED_BUILTIN, OUTPUT);
  //digitalWrite(LED_BUILTIN, LOW);
  
  //Serial.setFIFOSize( 256 );
  Serial.begin( 9600 );

  delay( 2000 );
  pinMode( LED, OUTPUT );
  digitalWrite( LED, 1 );
  delay( 2000 );
  digitalWrite( LED, 0 );

  pinMode( TIP_LED, OUTPUT );
  pinMode( RING_LED, OUTPUT );

  digitalWrite( TIP_LED, 1 );
  digitalWrite( RING_LED, 1 );

  delay( 2000 );

  //configure both lines to be in a high-impedance state and enable pull-up resistors
  pinMode(TIP, INPUT_PULLUP);
  pinMode(RING, INPUT_PULLUP);
  
}

unsigned long activeLast = 0;

int serialBufferIndex = 0;
const int serialBufferSize = 256;
int serialBuffer[ serialBufferSize ];

void loop() {
  
  if ( millis() - activeLast > 20 ) {
    digitalWrite( LED, 0 );
  }

  //forward incoming data from PC to the calculator
  while ( Serial.available() > 0 ) {
    activeLast = millis();
    digitalWrite( LED, 1 );
    sendByte( Serial.read() );
    /*
    if ( serialBufferIndex < serialBufferSize - 1 ) {
      serialBuffer[ serialBufferIndex++ ] = Serial.read();
    }
    */
  }

  /*
  for ( int i = 0; i < serialBufferIndex; i++ ) {
    sendByte( serialBuffer[ i ] );
  }
  serialBufferIndex = 0;
  */

  //forward incoming data from calculator to the PC
  uint8_t byteFromCalc;
  if ( getByte( byteFromCalc ) ) {
    activeLast = millis();
    digitalWrite( LED, 1 );
    Serial.write( byteFromCalc );
    //Serial.print( String( byteFromCalc ) + " " );
  }

}
