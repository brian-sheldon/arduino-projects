

const int tip = 12;
const int ring = 13;

void waithigh( int pin ) {
  unsigned long start = micros();
  int count = 0;
  while( count < 20000 && digitalRead( pin ) != 1 ) {
    count++;
  }
  //Serial.println( micros() - start );
}

void waitlow( int pin ) {
  unsigned long start = micros();
  int count = 0;
  while ( digitalRead( pin ) != 0 ) {
    count++;
  }
  //Serial.println( micros() - start );
}

void tiplow() {
  //Serial.println( "tip ..." );
  pinMode( tip, OUTPUT );
  digitalWrite( tip, 0 );
  waitlow( ring );
  pinMode( tip, INPUT );
  waithigh( ring );
  delay( 100 );
}

void ringlow() {
  //Serial.println( "ring ..." );
  pinMode( ring, OUTPUT );
  digitalWrite( ring, 0 );
  waitlow( tip );
  pinMode( ring, INPUT );
  waithigh( tip );
  delay( 100 );
}

void sync() {
}

void send( int value ) {
  for ( int i = 7; i >= 0; i-- ) {
    int bit = ( value >> i ) & 1;
    if ( bit == 0 ) {
      tiplow();
    } else {
      ringlow();
    }
  }
}

bool sendByte( uint8_t byte ) {
  unsigned long txtimeout = 20;
  unsigned long start;
  for ( int digit = 0; digit < 8; digit++ ) {
    int bit = ( byte >> digit ) & 1;
    start = millis();
    int pin0;
    int pin1;
    if ( bit == 0 ) {
      pin0 = tip;
      pin1 = ring;
    } else {
      pin0 = ring;
      pin1 = tip;
    }
    pinMode( pin0, OUTPUT );
    digitalWrite( pin0, 0 );
    start = millis();
    while ( digitalRead( pin1 ) == 1 ) {
      if ( millis() - start > txtimeout ) {
        pinMode( pin0, INPUT );
        return false;
      }
    }
    pinMode( pin0, INPUT );
    start = millis();
    while ( digitalRead( pin1 ) == 0 ) {
      if ( millis() - start > txtimeout ) {
        return false;
      }
    }
  }
  return true;
}

void sendPacket() {
  uint8_t packet[] = { 0x73, 0xa2, 0x0d, 0x00, 0x09, 0x00, 0x00, 0x42, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4b, 0x00 };
  for ( int i = 0; i < sizeof( packet ); i++ ) {
    uint8_t byte = packet[i];
    Serial.print( byte );
    Serial.print( " " );
    sendByte( byte );
  }
  Serial.println();
}

bool getByte( uint8_t &byte ) {
  unsigned long rcvTimeout = 20;
  unsigned long start;
  int value = 0;
  for ( int digit = 0; digit < 8; digit++ ) {
    start = millis();
    while ( digitalRead( tip ) == 1 && digitalRead( ring ) == 1 ) {
      if ( millis() - start > rcvTimeout ) {
        return false;
      }
    }
    int pin0;
    int pin1;
    if ( digitalRead( tip ) == 0 ) {
      pin0 = tip;
      pin1 = ring;
    } else {
      pin0 = ring;
      pin1 = tip;
      value += 1 << digit;
    }
    pinMode( pin1, OUTPUT );
    digitalWrite( pin1, 0 );
    start = millis();
    while ( digitalRead( pin0 ) == 0 ) {
      if ( millis() - start > rcvTimeout ) {
        pinMode( pin1, INPUT );
        return false;
      }
    }
    pinMode( pin1, INPUT );
  }
  byte = value;
  return true;
}

void getPacket() {
  char buffer[5];
  int count = 0;
  uint16_t len = 0;
  uint16_t sum = 0;
  uint16_t checksum = 0;
  uint8_t value;
  while ( getByte( value ) != false ) {
    count++;
    if ( count == 3 ) {
      len = value;
    }
    if ( count == 4 ) {
      len += value * 0x100;
    }
    if ( count > 4 ) {
      if ( count < len + 5 ) {
        sum += value;
      }
      if ( count == len + 5 ) {
        checksum = value;
      }
      if ( count == len + 6 ) {
        checksum += value * 0x100;
      }
    }
    sprintf( buffer, "%02x", value );
    Serial.print( buffer );
    Serial.print( " " );
  }
  if ( count > 0 ) {
    Serial.println();
    Serial.print( "Len: " );
    Serial.println( len );
    Serial.print( "sum: " );
    Serial.println( sum );
    Serial.print( "checksum: " );
    Serial.println( checksum );
  }
}


void setup() {
  Serial.begin( 115000 );
  pinMode( tip, INPUT );
  pinMode( ring, INPUT );
  delay( 5000 );
  Serial.println();
  Serial.println();
  Serial.println();
  Serial.println();
  Serial.println();
  Serial.println( "Starting ..." );
  delay( 2000 );
  //sendPacket();
  //getPacket();
}

void loop() {
  //delay( 2000 );
  //send( 241 );
  
  //getPacket();

  while ( Serial.available() > 0 ) {
    Serial.write( Serial.read() );
    //sendByte( Serial.read() );
  }
  
  uint8_t rcvByte;
  if ( getByte( rcvByte ) ) {
    Serial.write( rcvByte );
  }

}
