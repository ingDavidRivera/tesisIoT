#include <SPI.h>
#include <MFRC522.h>
#include <LoRa.h>


MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance.

MFRC522::MIFARE_Key key;
#define RST_PIN        12           // Configurable, see typical pin layout above
#define SS_PIN          2           // Configurable, see typical pin layout above
#define IRQ_PIN         3           // Configurable, depends on hardware
#define SCK 15
#define MISO 14
#define MOSI 16
#define SS 8
#define RST 4
#define DIO0 7

volatile bool bNewInt = false;
byte regVal = 0x7F;
void activateRec(MFRC522 mfrc522);
void clearInt(MFRC522 mfrc522);
volatile bool doRead=false;
volatile int incomingPacketSize;
float tem,hum;
char tem_1[8]={"\0"},hum_1[8]={"\0"};   
char *node_id = "<4567>";  //From LG01 via web Local Channel settings on MQTT.Please refer <> dataformat in here. 
uint8_t datasend[36];
unsigned int count = 1; 
unsigned long new_time,old_time=0;

/**
 * Initialize.
 */
void setup() {
  LoRa.setPins(SS, RST, DIO0);
  Serial.begin(115200); // Initialize serial communications with the PC
  while (!Serial);      // Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4)
 //
  SPI.begin();          // Init SPI bus
  mfrc522.PCD_Init(); // Init MFRC522 card
  Serial.print(F("Ver: 0x"));
  byte readReg = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
  Serial.println(readReg, HEX);
  pinMode(IRQ_PIN, INPUT_PULLUP);
  regVal = 0xA0; //rx irq
  mfrc522.PCD_WriteRegister(mfrc522.ComIEnReg, regVal);
  bNewInt = false; //interrupt flag
  attachInterrupt(digitalPinToInterrupt(IRQ_PIN), readCard, FALLING);

  do { //clear a spourious interrupt at start
    ;
  } while (!bNewInt);
  bNewInt = false;
  //
  // LoRa
      Serial.println(F("Start MQTT Example"));
      if (!LoRa.begin(915000000))   //868000000 is frequency
      { 
          Serial.println("Starting LoRa failed!");
          while (1);
      }
      // Setup Spreading Factor (6 ~ 12)
      LoRa.setSpreadingFactor(7);
      
      // Setup BandWidth, option: 7800,10400,15600,20800,31250,41700,62500,125000,250000,500000
      //Lower BandWidth for longer distance.
      LoRa.setSignalBandwidth(125000);
      
      // Setup Coding Rate:5(4/5),6(4/6),7(4/7),8(4/8) 
      LoRa.setCodingRate4(5);
      LoRa.setSyncWord(52); 
      Serial.println("LoRa init succeeded.");      
      LoRa.onReceive(onReceive);   
      LoRa.receive();     


  // LoRa
  Serial.println(F("End setup"));
}

void SendData()
{
     LoRa.beginPacket();
     LoRa.print((char *)datasend);
     LoRa.endPacket();
     Serial.println("Packet Sent");
}    



/**
 * Main loop.
 */
void loop() {
  if (bNewInt) { //new read interrupt
    Serial.print(F("Interrupt. "));
    mfrc522.PICC_ReadCardSerial(); //read the tag data
    // Show some details of the PICC (that is: the tag/card)
    Serial.print(F("Card UID:"));
    dump_byte_array(mfrc522.uid.uidByte, mfrc522.uid.size);
    Serial.println();

    clearInt(mfrc522);
    mfrc522.PICC_HaltA();
    bNewInt = false;
  }

  activateRec(mfrc522);
  delay(100);
} //loop()

void dump_byte_array(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
  }
}

void readCard() {
  bNewInt = true;
}

/*
 * The function sending to the MFRC522 the needed commands to activate the reception
 */
void activateRec(MFRC522 mfrc522) {
  mfrc522.PCD_WriteRegister(mfrc522.FIFODataReg, mfrc522.PICC_CMD_REQA);
  mfrc522.PCD_WriteRegister(mfrc522.CommandReg, mfrc522.PCD_Transceive);
  mfrc522.PCD_WriteRegister(mfrc522.BitFramingReg, 0x87);
}

void clearInt(MFRC522 mfrc522) {
  mfrc522.PCD_WriteRegister(mfrc522.ComIrqReg, 0x7F);
}

void onReceive(int packetSize) {
  doRead = true;
  incomingPacketSize = packetSize; 
}
