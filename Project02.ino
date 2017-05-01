/*
 * --------------------------------------------------------------------------------------------------------------------
 * Example sketch/program showing how to read new NUID from a PICC to serial.
 * --------------------------------------------------------------------------------------------------------------------
 * This is a MFRC522 library example; for further details and other examples see: https://github.com/miguelbalboa/rfid
 * 
 * Example sketch/program showing how to the read data from a PICC (that is: a RFID Tag or Card) using a MFRC522 based RFID
 * Reader on the Arduino SPI interface.
 * 
 * When the Arduino and the MFRC522 module are connected (see the pin layout below), load this sketch into Arduino IDE
 * then verify/compile and upload it. To see the output: use Tools, Serial Monitor of the IDE (hit Ctrl+Shft+M). When
 * you present a PICC (that is: a RFID Tag or Card) at reading distance of the MFRC522 Reader/PCD, the serial output
 * will show the type, and the NUID if a new card has been detected. Note: you may see "Timeout in communication" messages
 * when removing the PICC from reading distance too early.
 * 
 * @license Released into the public domain.
 * 
 * Typical pin layout used:
 * -----------------------------------------------------------------------------------------
 *             MFRC522      Arduino       Arduino   Arduino    Arduino          Arduino
 *             Reader/PCD   Uno/101       Mega      Nano v3    Leonardo/Micro   Pro Micro
 * Signal      Pin          Pin           Pin       Pin        Pin              Pin
 * -----------------------------------------------------------------------------------------
 * RST/Reset   RST          9             5         D9         RESET/ICSP-5     RST
 * SPI SS      SDA(SS)      10            53        D10        10               10
 * SPI MOSI    MOSI         11 / ICSP-4   51        D11        ICSP-4           16
 * SPI MISO    MISO         12 / ICSP-1   50        D12        ICSP-1           14
 * SPI SCK     SCK          13 / ICSP-3   52        D13        ICSP-3           15
 */

#include <TimerOnes.h>
#include <SPI.h>
#include <MFRC522.h>
#include <EEPROM.h>

#define SS_PIN 10
#define RST_PIN 9
#define approveLED 5
#define magnet 4
#define doorAccess 2
#define remoteAccess 3
#define rfidbuzzer 7

MFRC522 rfid(SS_PIN, RST_PIN); // Instance of the class

MFRC522::MIFARE_Key key; 

// Init array that will store new NUID 
byte nuidPICC[4];
int const MAX_CARD = 30;
unsigned long cards[30];
int numberOfCards;
bool interruptFlag = 0;
int address = 0;

void EEPROMWritelong(int address, long value);
unsigned long EEPROMReadlong(int address);
void doorOpen();
void buzzerAccept();
void buzzerReject();
void callback();
void registercallback();
unsigned long printDec(byte *buffer, byte bufferSize);

void setup() { 
  Serial.begin(9600);
  SPI.begin(); // Init SPI bus
  rfid.PCD_Init(); // Init MFRC522 
  Timer1.initialize(8000000); 
  Timer1.attachInterrupt(callback);
  Timer1.stop();
  
  pinMode(approveLED, OUTPUT);
  pinMode(magnet, OUTPUT);
  pinMode(rfidbuzzer, OUTPUT);
  pinMode(doorAccess, INPUT_PULLUP);
  pinMode(remoteAccess, INPUT_PULLUP);

  digitalWrite(magnet, LOW);
  digitalWrite(rfidbuzzer, HIGH);
  
  for (int x = 0; x < 30; x++){
    cards[x] = EEPROMReadlong(address);
    if(cards[x] == -1){
      numberOfCards = x;
      break;  
    }
    address += 4;
  }
  for(int x = 0; x< numberOfCards; x++){
    Serial.println(cards[x]);
  }  
}

void loop() {
  if(rfid.PICC_IsNewCardPresent()){
    if(!rfid.PICC_ReadCardSerial())
    return;
    for (byte i = 0; i < 4; i++) {
      nuidPICC[i] = rfid.uid.uidByte[i];
    }
    unsigned long code = printDec(rfid.uid.uidByte, rfid.uid.size);
    Serial.print("The NUID tag is: ");
    Serial.println(code);
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
    for(int x = 0; x < numberOfCards; x++){
      if(code == cards[x]){
        digitalWrite(rfidbuzzer, LOW);
        Serial.println("Access Granted");
        doorOpen();
        delay(200);
        digitalWrite(rfidbuzzer, HIGH);
        break;
      }
    }
  }
  if(!(digitalRead(doorAccess))){
    digitalWrite(rfidbuzzer, LOW);
    doorOpen();
    delay(200);
    digitalWrite(rfidbuzzer, HIGH);
  }
  delay(100);
}

/**
 * Helper routine to dump a byte array as dec values to Serial.
 */
unsigned long printDec(byte *buffer, byte bufferSize) {
  String code;
  for (byte i = 0; i < bufferSize; i++) {
    code += buffer[i];
  }
  if(code.length()>9){
    code = code.substring(0,9);
  }
  return code.toInt();
}

void EEPROMWritelong(int address, long value)
{
  byte four = (value & 0xFF);
  byte three = ((value >> 8) & 0xFF);
  byte two = ((value >> 16) & 0xFF);
  byte one = ((value >> 24) & 0xFF);

  EEPROM.write(address, four);
  EEPROM.write(address + 1, three);
  EEPROM.write(address + 2, two);
  EEPROM.write(address + 3, one);  
}

unsigned long EEPROMReadlong(int address)
{
  long four = EEPROM.read(address);
  long three = EEPROM.read(address + 1);
  long two = EEPROM.read(address + 2);
  long one = EEPROM.read(address + 3);
  return ((four << 0) & 0xFF) + ((three << 8) & 0xFFFF) + ((two << 16) & 0xFFFFFF) + ((one << 24) & 0xFFFFFFFF);
}
 
void doorOpen(){
  digitalWrite(magnet, HIGH);
  digitalWrite(approveLED, HIGH);
  Serial.println("doorOpen");
  Timer1.attachInterrupt(callback);
  Timer1.resume();
}

void callback(){
  digitalWrite(magnet, LOW);
  digitalWrite(approveLED, LOW);
  Serial.println("doorClosing");
  Timer1.detachInterrupt();
  Timer1.stop();
  delay(100);
}

void registercallback(){
  interruptFlag = 1;
  Timer1.detachInterrupt();
  Timer1.stop();
  delay(100);
}

