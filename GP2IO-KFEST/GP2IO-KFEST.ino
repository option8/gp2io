/*
                            =========
  Pushbutton 3 (GS) / NC 9 | *     * | 8  Ground
  Gm Ctrl 1 (Stick-1 Y) 10 | *     * | 7  Gm Ctrl 2 (Stick-2 X)
  Gm Ctrl 3 (Stick-2 Y) 11 | *     * | 6  Gm Ctrl 0 (Stick-1 X)
         Annunciator 3  12 | *     * | 5  /$C040 Strobe
         Annunciator 2  13 | *     * | 4  Pushbutton 2 / A7 / $c063 /SHIFT
         Annunciator 1  14 | *     * | 3  Pushbutton 1 / A8 / $c062
         Annunciator 0  15 | *     * | 2  Pushbutton 0 / A10 / $c061
         No Connection  16 | *     * | 1  +5V
                            ===| |===
                                ^


  SETAN0 =   $C058 ;Set annunciator-0 output to 0
  CLRAN0 =   $C059 ;Set annunciator-0 output to 1

  SETAN1 =   $C05A ;Set annunciator-1 output to 0
  CLRAN1 =   $C05B ;Set annunciator-1 output to 1


  SETAN2 =   $C05C ;Set annunciator-2 output to 0
  CLRAN2 =   $C05D ;Set annunciator-2 output to 1

  SETAN3 =   $C05E ;Set annunciator-3 output to 0
  CLRAN3 =   $C05F ;Set annunciator-3 output to 1


*/

// I2C

#define SDA_PORT PORTB
#define SDA_PIN 5
#define SCL_PORT PORTB
#define SCL_PIN 6

#define I2C_TIMEOUT 100
#define I2C_NOINTERRUPT 0
#define I2C_SLOWMODE 1
#define FAC 1
#define I2C_CPUFREQ (F_CPU/FAC)

#include <SoftI2CMaster.h>
#include <avr/io.h>

// Adafruit I2C LED backpacks
#define HT16K33_BLINK_CMD 0x80
#define HT16K33_BLINK_DISPLAYON 0x01
#define HT16K33_BLINK_OFF 0
#define HT16K33_CMD_BRIGHTNESS 0xE0

uint8_t i2c_addr = 0xe0;//0x70;

// UART/RS232

#define HWSERIAL Serial1

int counter = 0;
int delayLoop = 0;
bool sendingBit = 0;
bool FlipFlop = 0;

volatile long currentMicros = 0;
volatile long lastMicros = 0;
volatile int receivedBit;
volatile int delayMillis;
volatile long TIMEOUTCLOCK;
long TIMESINCELASTBYTE;

char receivedByte;
byte returnByte;
int changeCount = 0;
int bitCount = 0;

int byteArray[8];

//char stack[256];
char inputBuffer[256];
byte outputBuffer[256];
//byte USBSERIALBuffer[256];
//byte HWSERIALBuffer[256];
//int HWSERIALBufferLength = 0;
byte bufferLength[1];

/*byte I2CBuffer[8] = {
  B00000000,
  B00000000,
  B10101110,
  B01101010,
  B10101110
  };*/

byte I2CBuffer[8] = {
  B01111010, /* bit 7--0 */
  B10111000,
  B00000000,
  B10111000,
  B10111101,
  B10101110,
  B01101010,
  B10101110
};
/*
           1
          ---
       6 | 7 | 2
          ---
       5 | 4 | 3
          --- . 0

  colon is I2CBuffer[2], FF on 00 off

*/
int LED_ON = 1;

int messageLength;
int stackPointer;
int inputPointer;
int outputPointer;
unsigned long baud = 9600;
const int reset_pin = 4;


byte ESPByte;

const int LED_PIN = 11;

int RGB_RED = 9;
int RGB_GREEN = 10;
int RGB_BLUE = 4;

int ANN0_PIN = 5;
int ANN1_PIN = 6;
int ANN2_PIN = 0;
int ANN3_PIN = 1;

char PB0_PIN = 13;
char PB1_PIN = 12;
char PB2_PIN = 21;


int IO_MODE;
int NUM_MODES = 9;

int functionLength = 0;
char currentFunction;
int functionArray[] =
{ B00000001,   //0 $01 RGB LED, 3 bytes
  B00000010,    //1 $02 RGB LED, 1 byte (white intensity)
  B00000100,    //2 $04 WRITE TO INTERNAL BUFFER, 1 byte (message length), message (0-255 bytes).
  B00001000,    //3 $08 WRITE TO SERIAL, 1 byte (message length), message (0-255 bytes).
  B00010000,    //4 $10 WRITE TO I2C bus 8x8 LED Matrix, 7-segment, etc, 1 byte (message length), message (0-255 bytes).
  B00100000,    //5 $20 SPI (not yet implemented), READPIN, retreive analogRead from pin N
  B01000000,    //6 $40 QUERY BUFFER, 0 bytes. SEND 1 byte response with buffer length
  B10000000,    //7 $80 SEND BUFFER CONTENT, 1 byte (buffer to send). Ready to receive, expecting 0-255 bytes.
  B00000000
};   // $00 DEBUG mode. writes all subsequent bytes received to Serial Out. Sends one byte at a time from buffer to Apple on LATCH.
//DEBUG is active until the AVR is reset or input timeout is reached.

void setup() {
  //LED MATRIX / 7 segment display

  if (!i2c_init()) {
    Serial.println("I2C error. SDA or SCL are low");
  }
  if (!i2c_start(i2c_addr) ) {
    Serial.println("I2C device not found");
  }

  i2c_write(0x21);  // turn on oscillator
  i2c_stop();

  // set blinkrate 0
  i2c_start(i2c_addr);
  i2c_write(HT16K33_BLINK_CMD | HT16K33_BLINK_DISPLAYON | (0 << 1));
  i2c_stop();

  // set brightness to 15/15
  i2c_start(i2c_addr);
  i2c_write(HT16K33_CMD_BRIGHTNESS | 15);
  i2c_stop();

  writeI2C();  // "Good"/"ok"


  pinMode(ANN2_PIN, INPUT); // annunciator 2
  pinMode(ANN3_PIN, INPUT); // annunciator 3


  pinMode(ANN0_PIN, INPUT_PULLUP);
  pinMode(ANN1_PIN, INPUT_PULLUP);

  pinMode(RGB_RED, OUTPUT);
  pinMode(RGB_GREEN, OUTPUT);
  pinMode(RGB_BLUE, OUTPUT);


  pinMode(PB1_PIN, OUTPUT); // PB1 / solid apple
  pinMode(PB0_PIN, OUTPUT); // PB0 / open apple

  digitalWrite(PB0_PIN, LOW); // RTS
  digitalWrite(PB1_PIN, LOW); // Bits out
  digitalWrite(PB2_PIN, HIGH); // SHIFT is active LOW

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);


  Serial.begin(baud);
  // to ESP, other serial devices
  HWSERIAL.begin(baud);

  enablePinInterrupt(ANN2_PIN); // Apple ready to receive
  attachInterrupt(0, APPLERTS, RISING); // ANNUNCIATOR 0, Apple sending byte
  attachInterrupt(1, RECEIVINGBITS, CHANGE); // ANNUNCIATOR 1, Apple sending bits

  setRGBOneShot(0, 0, 200); // blue = ready
}

void loop()
{

  TIMESINCELASTBYTE = millis() - TIMEOUTCLOCK;
  if ( TIMESINCELASTBYTE > 5000 && IO_MODE > -1) {
    IOMODERESET(TIMESINCELASTBYTE, "timeout");
  }

  int incomingByte;

  if (Serial.available() > 0) {
    incomingByte = Serial.read();
    HWSERIAL.write(incomingByte);
  }


  if (HWSERIAL.available() > 0) {
    incomingByte = HWSERIAL.read();

    bufferLength[0]++; // should be fast enough to get one byte at a time...

    digitalWrite(PB0_PIN, HIGH); // RTS == UART has bytes to send

    outputBuffer[bufferLength[0]] = incomingByte;
    Serial.write(outputBuffer[bufferLength[0]]);

  }


} //loop

void IOMODERESET(long resetTime, String reason) {
  Serial.print("B");
  Serial.println(bufferLength[0]);

  if ( TIMESINCELASTBYTE > 5000) {    // double check there's actually been a timeout...
    functionLength = 0;
    IO_MODE = -1;
    //
    Serial.print("MODE RESET ");
    Serial.println(reason);
    // Serial.println(TIMESINCELASTBYTE);
    setRGBOneShot(0, 0, 200);

    digitalWrite(PB0_PIN, LOW); // RTS off
    digitalWrite(PB1_PIN, LOW); // Bits out
    digitalWrite(PB2_PIN, HIGH); // SHIFT is active LOW
    digitalWrite(LED_PIN, LOW);

    // Serial.write(HWSERIALBuffer, 256);

  }
}




void enablePinInterrupt(byte pin)
{
  *digitalPinToPCMSK(pin) |= bit (digitalPinToPCMSKbit(pin));  // enable pin
  PCIFR  |= bit (digitalPinToPCICRbit(pin)); // clear any outstanding interrupt
  PCICR  |= bit (digitalPinToPCICRbit(pin)); // enable interrupt for the group
}

ISR (PCINT0_vect) // handle pin change interrupt
{

  //APPLECTS == ANN2 pin rising

  if (digitalRead(ANN2_PIN) == HIGH) { // ready for queued byte, annunciator 2 HIGH

    if (IO_MODE == 6) { // sending one-byte buffer length.
      sendByte(bufferLength[0], 1);
      functionLength = 0;
      IO_MODE = -1;

      //      Serial.println(bufferLength[0]);
      //      Serial.println(outputPointer);

    } else if (outputPointer < bufferLength[0]) { //  will always return something when queried, for debugging.
      //      Serial.print(outputPointer);
      //      Serial.println(outputBuffer[outputPointer]);

      sendByte(outputBuffer[outputPointer], 0);
      outputPointer++;

      Serial.print(outputPointer);
      Serial.print(" == ");
      Serial.println(bufferLength[0]);

      if (outputPointer == bufferLength[0]) { // zero padding?

        //  Serial.println("BUFFER RESET");
        sendByte(0, 1);
        outputPointer = 0;
        bufferLength[0] = 0;

      }
    }
    /*   if (outputPointer == bufferLength[0]) { // done sending. RTS LOW
         digitalWrite(PB0_PIN, LOW); // RTS LOW == no more buffer to send
       }
    */

    digitalWrite(LED_PIN, HIGH);   // set the LED on

  } else {
    digitalWrite(LED_PIN, LOW);   // set the LED off

  }

}

void sendByte (byte byteToSend, bool oneShot) {


  if (oneShot) {
    outputPointer = 0;
  } else {
    //    outputPointer++;
  }

  delay(100); // ~7ms
  digitalWrite(PB1_PIN, HIGH);

  for (int i = 0; i < 8; i++) { // send 8 bits, 9 transitions

    int bitToSend = bitRead(byteToSend, 7 - i);

    if (bitToSend == 1) {
      delay(80);  // ~13ms
    }

    delay(20); // ~7ms

    if ( i % 2 ) {
      digitalWrite(PB1_PIN, HIGH);
    } else {
      digitalWrite(PB1_PIN, LOW);
    }


  }
  delay(100); // ~7ms
  digitalWrite(PB1_PIN, HIGH);
  delay(100); // ~7ms
  digitalWrite(PB1_PIN, LOW);

}


void APPLERTS() {
  // signal to start receiving bits from Apple II
  bitCount = 0;
  changeCount = 0;
  returnByte = B00000000;

  // Serial.print("RECEIVING. IO MODE: ");
  // Serial.println(IO_MODE);
  //  Serial.print("  FUNCTION LENGTH: ");
  //  Serial.println(functionLength);

}

void RECEIVINGBITS()
{
  // ignore short "reset" transitions
  currentMicros = micros();


  if (changeCount % 2 == 1) {



    //     Serial.println(currentMicros - lastMicros);
    //    Serial.print( " = " );
    if ((currentMicros - lastMicros) > 70) {

      receivedBit = 1;
      //digitalWrite(A0,HIGH);
      //digitalWrite(A1,LOW);
    } else {

      receivedBit = 0;
      // digitalWrite(A1,HIGH);
      // digitalWrite(A0,LOW);
    }
    //    Serial.println(receivedBit);

    byteArray[7 - bitCount] = receivedBit;

    //Serial.print(receivedByte);
    /*    if (receivedBit == 0) {
          returnByte &= ~(0 << (7 - bitCount));
        } else {
          returnByte |= (1 << (7 - bitCount));
        }
    */
    /*
       starts as 00000000

       1 or 0 gets added, byte shifted left
    */

    //returnByte = returnByte | receivedBit; // 00000000 to 00000001
    //returnByte = returnByte << 1;  // 00000001 to 00000010
    bitCount++;


  }
  changeCount++;



  if (bitCount == 8) { // got a BYTE
    //   Serial.println(receivedBit);
    receivedByte = arrayToByte(byteArray, 8);
    //   Serial.print("HEX ");
    //        Serial.println( String(arrayToByte(byteArray,8),HEX) );
    //    Serial.print("DEC ");
    //Serial.println(".");
    digitalWrite(LED_PIN, LOW);
    //  setRGBOneShot(0, 200, 0);
    PROCESSBYTE( byte(receivedByte) );
    TIMEOUTCLOCK = millis();
  }


  lastMicros = currentMicros;


}


void PROCESSBYTE(byte receivedByte) {
  //  char longstring[80] = "AT+CIPSTART=\"TCP\",\"www.option8llc.com\",80\n\r";
  //  int rd, wr;
  String connectString;
  int readPin; // pin to read for readpin mode
  byte returnByte; //
  // Serial.print("P");
  // Serial.println(receivedByte);
  //setRGBOneShot(0, 200, 200);

  //  TIMEOUTCLOCK = millis();

  /*
     if no IO_MODE, wait on a byte from functionArray to start IO.

    case IO_MODE
      each mode sets number of bytes required, sets byte countdown "functionLength"
        subsequent bytes in processbyte fall through to io_mode

      functionLength = 0, done with IO function, waiting on next switch byte

  */


  if (functionLength == 0) { // waiting on next function
    // interpret bytes as mode switch, values

    for (int i = 0; i < NUM_MODES; i++) {

      //      Serial.println(functionArray[i]);

      if (receivedByte == functionArray[i]) {
        IO_MODE = i;
        Serial.print("F");
        Serial.println(i);
        TIMEOUTCLOCK = millis();

        break;
      } else {
        IO_MODE = -1;
      }
    }

    //    Serial.println(IO_MODE);

    switch (IO_MODE) {
      case 0: // RGB PIXEL
        functionLength = 3;
        //       Serial.println("RGB LED - three more bytes");

        break;

      case 1: // RGB PIXEL, white
        functionLength = 1;
        //         Serial.println("White LED MODE - 1 more byte");

        break;

      case 2: // $04 WRITE TO STACK, 1 byte (message length), message (0-255 bytes)
        functionLength = -1;
        stackPointer = 0;

        break;

      case 3: // $08 WRITE TO UART SERIAL, 1 byte (message length), message (0-255 bytes)


        /* long string
           read available write size wr
           move wr bytes from longstring to writebuffer
           write wr bytes to HWSERIAL
           wait X


        */


        // connectString = "AT+CWLAP\n\r";

        // connectString.getBytes(HWSERIALBuffer, 16);//sizeof(connectString));

        // char HWSERIALBuffer[messageLength];

        // connectString.toCharArray(HWSERIALBuffer, messageLength);

        // Serial.print("Content length= ");
        // Serial.println(messageLength);


        //bufferToHWSERIAL(sizeof(connectString));

        //        functionLength = -1;
        //        stackPointer = 0;
        //   Serial.println("ESP8266");
        //   HWSERIAL.println("AT+CWLAP");
        //HWSERIAL.availableForWrite();



        //HWSERIAL.write("AT+CIPSTART=\"TCP\",\"www.option8llc.com\",80\n\r");

        //HWSERIAL.println("AT+CIPSEND=72");
        //       HWSERIAL.println("GET /KFEST.TXT HTTP/1.1");
        //      HWSERIAL.println("Host: option8llc.com");
        //      HWSERIAL.println("Connection: close");



        IOMODERESET(TIMESINCELASTBYTE, "serial demo"); // ONE-SHOT
        break;

      case 4: // 0x10 write pixels to 8x8 matrix display
        functionLength = -1;
        // wait on N bytes for matrix/7seg display
        // first byte sets message length

        break;
      case 5: // 0x20 read pinstate of pin determined by first byte
        functionLength = 1;
        Serial.print("READPIN ");

        break;
      case 6: // QUERY BUFFER - send 1 byte, buffer length
        functionLength = 1;
        Serial.print("RTS ");

        //Serial.print("SENDING BUFFER LENGTH = ");
        // Serial.println(bufferLength[0]);
        //messageLength = 80; // how to get the last byte/length of the buffer ???

        //outputBuffer = char("this is a test.");
        //outputPointer = 0;
        //sendByte(outputBuffer[0]);

        //bufferLength[0] = HWSERIALBufferLength;


        //sendByte(bufferLength[0], 1);

        break;

      case 7:  // SEND BUFFER - send the bytes from the stack
        functionLength = 1;
        Serial.print("RTS ");
        break;

      case 8: // DEBUG MODE
        functionLength = -1;
        Serial.println("DEBUG MODE.");
        break;

      default:
        Serial.print("BAD MODE.");
        Serial.println(receivedByte);
        break;
    }

  } else { // waiting on n bytes

    //    Serial.println(functionLength);

    switch (IO_MODE) {
      case 0: // RGB LED
        setRGB(receivedByte, functionLength);
        functionLength --;
        break;

      case 1:// RGB LED WHITE
        setRGB(receivedByte, 0);
        functionLength = 0;
        break;

      case 2:// read bytes into local buffer
        //Serial.print(receivedByte);

        if (functionLength == -1) { // first byte == message length
          functionLength = receivedByte;
          Serial.print("I");
          Serial.println(functionLength);
        } else {
          inputBuffer[stackPointer] = receivedByte;
          stackPointer++;
          // Serial.println(stackPointer);

          if (stackPointer == functionLength) { // last byte

            for (int i = 0; i < stackPointer; i++) {
              HWSERIAL.write(inputBuffer[i]);
            }
            HWSERIAL.write('\r');
            HWSERIAL.write('\n');

            IOMODERESET(TIMESINCELASTBYTE, "readbytes"); // SEEMS TO GET MISSED?

            IO_MODE = -1;
            functionLength = 0;

          }


        }

        break;


      case 3:// read bytes and send to UART (e.g. ESP8266)
        // handled in interrupts
        break;


      case 4: // read and buffer 8 bytes to I2C buffer for matrix/7segment display
        if (functionLength == -1) { // first byte == message length
          functionLength = receivedByte;
          Serial.print("I2C ");
          Serial.println(functionLength);
        } else {
          I2CBuffer[functionLength - 1] = receivedByte;
          Serial.println(receivedByte);

          functionLength --;
          if (functionLength == 0) {
            writeI2C();
            Serial.print("I2C DONE");
          }

        }


        break;

      case 5:
        readPin = int(receivedByte);
        Serial.print("AnalogRead pin ");
        Serial.print(readPin);
        pinMode(readPin, INPUT);
        Serial.print(" = ");
        Serial.println(analogRead(readPin));

        returnByte = map(analogRead(readPin), 0, 1023, 0, 255);

        // now put that value into the buffer at 0 and signal ready to send ***
        inputBuffer[0] = returnByte;
        bufferLength[0] = 1;

        IOMODERESET(TIMESINCELASTBYTE, "readpin"); // SEEMS TO GET MISSED?

        IO_MODE = -1;
        functionLength = 0;
        break;


      case 7: // send inputbuffer contents to apple

        // *** send only N bytes, zero padded

        /*
           get length of expected message = L
           move L bytes from inputbuffer to outputbuffer
           set outputpointer = 0, wait for latch
        */

        // first byte = expected message length
        functionLength = receivedByte;
        Serial.print("SENDING ");
        //    Serial.print(functionLength);
        Serial.println(functionLength);

        //    Serial.println("SENT");
        outputPointer = 0; // buffer sent, start fresh.
        functionLength = 0;
        inputPointer = 0; // buffer sent, start fresh.
        bufferLength[0] = 0; // buffer sent, start fresh.
        break;

      case 8: // DEBUG MODE
        functionLength = -1;
        //Serial.print(receivedByte);
        Serial.print(char(receivedByte));
        break;

      default:
        Serial.print("NO MODE FOR BYTE. ");
        Serial.println(receivedByte);
        break;
    }

  }

}

void setRGB(byte inByte, int bytePosition) {

  switch (bytePosition) {
    case 3:
      analogWrite(RGB_RED, 255 - inByte);
      break;

    case 2:
      analogWrite(RGB_GREEN, 255 - inByte);
      break;

    case 1:
      analogWrite(RGB_BLUE, 255 - inByte);
      break;

    default:
      analogWrite(RGB_RED, 255 - inByte);
      analogWrite(RGB_GREEN, 255 - inByte);
      analogWrite(RGB_BLUE, 255 - inByte);
  }

  // Serial.println(inByte);

}


void setRGBOneShot(int REDInt, int GREENInt, int BLUEInt) {
  analogWrite(RGB_RED, 255 - REDInt);
  analogWrite(RGB_GREEN, 255 - GREENInt);
  analogWrite(RGB_BLUE, 255 - BLUEInt);
}






byte arrayToByte(int arr[], int len) {
  // Convert -1 to 0 and pack the array into a byte
  int i;
  byte result = 0;

  for (i = 0; i < len; i++) {
    if (arr[i] == 0) {
      result &= ~(0 << i);
    } else {
      result |= (1 << i);
    }
  }
  return result;
}

void drawPixel(int16_t x, int16_t y, uint16_t color) {

  //  if ((y < 0) || (y >= 8)) return;
  //  if ((x < 0) || (x >= 8)) return;

  // wrap around the x
  x += 7; // 0,0 => 7,0 // 7,0 => 14,0 // 1,0 => 8,0
  x %= 8; // 7,0 => 7,0 // 14,0 => 6,0 // 8,0 => 0,0

  if (color) {
    I2CBuffer[y] |= 1 << x; // 0,0,1 => 10000000 // 1,0,1 => 00000001
  } else {
    I2CBuffer[y] &= ~(1 << x);
  }

  Serial.print(x);
  Serial.println(y);

  Serial.println(I2CBuffer[y], BIN);

}

void writeI2C(void) {
  // writes bytes out to HT16K33 controller via I2C.
  //Needs a general purpose I2C version that will work with more devices

  i2c_start(i2c_addr);
  i2c_write(0x00); // start address $00

  for (uint8_t i = 0; i < 8; i++) {
    // rotate the byte right by 1. "carry" bit gets rolled around
    I2CBuffer[i] = ((I2CBuffer[i] & 0x01) ? 0x80 : 0x00) | (I2CBuffer[i] >> 1);
    // seriously? ROR.

    i2c_write(I2CBuffer[i]); // write the bits to turn on/off pixels
    i2c_write(0xff); // control/bitmask?
  }
  i2c_stop();
}

void ClearI2C(void) {
  for (uint8_t i = 0; i < 8; i++) {
    I2CBuffer[i] = 0;
  }
  writeI2C();
}

