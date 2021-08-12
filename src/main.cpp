#include <Arduino.h>
#include "./QueueList.h"
//can you do the CANCAN!
/*
CUMMINS/BOSCH CAN bus diagnostic tool with mcp2515 CAN module
NiRen module from China with 8Mhz clock.
Arduino Uno or Mega enbedded microcontroller demo board.
Interrupt will be used in this demo.
A how to guide for understanding the mcp2515. This example does not use a CAN or mcp2515 library.

 Circuit:

 UNO shared pins      Mega 2560 R3 shared pins
 MOSI: pin 11         MOSI: pin 51
 MISO: pin 12         MISO: pin 50
 SCK: pin 13          SCK: pin 52
 Vcc:                 Vcc:
 GND:                 GND:
 SS: pin 10           SS: pin 53        // SS not wired

 CAN Module 1 to Arduino
 CS: pin 6                CS: pin 6
 INT: pin 2               INT: pin 2    //INT0
*/

/*Cummins Can bus messages





I need some help decoding these messages.  I’m not sure about the timing command it requests 25*BTDC at idle as soon as

it is started when cold then comes down after a long warm up to about 15*. That seems very high.

 Could it really be going that high?



// 24 Valve Cummins CAN bus messages. How to decode them ?
// Arduino Uno and MCP2515 CAN module (no name) from Asia.

Message format
byte 0  1  4 5 6  7  8  9  10 11 12
     22 40 8 4 30 98 23 96 6  11 13

byte 0 PID maybe?
 * ends in 2 when messages come from the VP44
 * ends in 0 when messages come from the ECU
 * ends in 7 when messages come from the Quadzilla ??

byte 1
 * 0x40 for messages from the VP44
 * 0x00 for messages from the ECU
 * 98 and 5A appear for PID 67
 * FA and 5A appear for PID C7

byte 4
 * data length

bytes 5-12
 * data


22 40 8 4 30 98 23 96 6 11 13       P >> E 74158432 diff 25256 idle val sw acc pedal up   timing error  counter 2 RPM range 3 Fuel temp F 46
A2 40 8 AE FF 0 0 B8 2 10 3       P >> E 74158904 diff 472 Offset? -82 Volts 13.9
20 0 8 12 1 0 0 3 6 2B D        E >> P 74159476 diff 572 cylinder 2 Fuel percent 6.7 Timing deg 15.4 RPM 842
A0 0 8 F8 2 0 0 16 1 42 1       E >> P 74159868 diff 392 ACK
22 40 8 4 30 9D 43 96 6 11 13       P >> E 74182240 diff 22372 idle val sw acc pedal up   timing error  counter 3 RPM range 3 Fuel temp F 46
A2 40 8 AE FF 0 0 B0 2 C 3       P >> E 74182720 diff 480 Offset? -82 Volts 13.8
20 0 8 1B 1 0 0 FA 5 21 D       E >> P 74183292 diff 572 cylinder 4 Fuel percent 6.9 Timing deg 15.3 RPM 840
A0 0 8 F8 2 0 0 16 1 42 1       E >> P 74183688 diff 396 ACK
Bytes 2 and 3 are for 29 bit CAN IDs and are not shown. Byte 4 is the data length. Long numbers and 'diff' are timestamps
I took manual control of the VP's timing solenoid that causes the timing lock bit to set (P0216)

<<pseudo code>>

         //Not sure about this compute fuel temp in Inj Pump >> message ID 0x22 DEC 34
      fuelTemp = (dataBuff[y][12] << 8) | dataBuff[y][11];
      fuelTemp = (fuelTemp / 100) - 40;
      fuelTemp = ((fuelTemp * 9) / 5) + 32;
      Serial.print(" Fuel temp F ");
      Serial.print(fuelTemp);



          // compute timing advance >> message ID 0x20 DEC 32
      Timing = (dataBuff[y][10] << 8) | dataBuff[y][9]; //convert from little endian. 100 bits per degree.
      Timing = (float)((Timing) / 100);      //
      Serial.print(" Timing deg ");
      Serial.print(Timing, 1);



      //Fuel compute >> ID 0x20
      FuelPCT = (dataBuff[y][6] << 8) | dataBuff[y][5];
      FuelPCT = (float)(FuelPCT*100) / 4096;    // 4095 is max fuel allowed by pump
      Serial.print(" Fuel percent ");
      Serial.print(FuelPCT, 1);


    //second message from IP (0xA2 DEC 162) has other stuff in it.
    //looks like a +/- offset or +/- error.
    // hangs around 0x0001 - 0xFFFE
    if(dataBuff[y][0] == 0xA2)
             {
          offSet = (dataBuff[y][6] << 8) | dataBuff[y][5];
          Serial.print(" Offset? ");
          Serial.print(offSet);


        // This byte changes from 0 to 1 after a delay. Seems to check if the timing advance mechanism is
        // able to match the commanded timing value
    //0x22 message ID
      (dataBuff[y][6] == 0x00) ? Serial.print(" timing locked ") : Serial.print(" timing error ");



      //compute RPM from ID 20xx bytes 11 and 12
      RPM = (dataBuff[y][12] << 8) | dataBuff[y][11];     //convert from little endian
      RPM = RPM/4 ;                           //divide by 4*/
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27,20,4);  // set the LCD address to 0x3F for a 16 chars and 2 line display

// mcp2515 uses SPI, so include the SPI library
#include <SPI.h>
//#define SPI2_NSS_PIN PB12   //SPI_2 Chip Select pin is PB12. You can change it to the STM32 pin you want.
//SPIClass SPI_2(2); //Create an instance of the SPI Class called SPI_2 that uses the 2nd SPI Port

// using non volatile memory too
#include <EEPROM.h>

struct CanMessage {
    byte id;
    byte unknown;
    byte unknown2;
    byte length;
    byte data[8];
    unsigned count;
};

// unknown pids, but fills up the memory and FAST...
volatile QueueList<CanMessage> canMessages;
CanMessage newMessage{};

// Known PID's
volatile CanMessage C75BFFMessage{};
volatile CanMessage C778FFMessage{};
volatile CanMessage C7980Message{};
volatile CanMessage C7FADFMessage{};
volatile CanMessage C7FAE4Message{};
volatile CanMessage C7FAEEMessage{};
volatile CanMessage C7FAEFMessage{};
volatile CanMessage C7FAF0Message{};
volatile CanMessage C7FAF1Message{};
volatile CanMessage C7FAF2Message{};
volatile CanMessage C7FAF5Message{};
volatile CanMessage C7FAF6Message{};
volatile CanMessage C7FAF7Message{};
volatile CanMessage C7FBE0Message{};
volatile CanMessage x20Message{};
volatile CanMessage x22Message{};
volatile CanMessage xA0Message{};
volatile CanMessage xA2Message{};
volatile CanMessage x675A0Message{0x67, 0x5A, 0x0, 0xF8, {0x3, 0xF7, 0xFE, 0x0, 0x0, 0x0, 0x0, 0x0}, 0};
volatile CanMessage x67984Message{0x67, 0x98, 0x4, 0x8, {0xFE, 0x7D, 0x7D, 0xC0, 0x1, 0xFF, 0xFF, 0xFF}, 0};
volatile CanMessage x67983Message{0x67, 0x98, 0x3, 0x8, {0xF1, 0x0, 0x0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, 0};

const int CANint1 = 2;  //CAN1 INT pin to Arduino pin 2
const int ss = 53;        //pin 10 for UNO or pin 53 for mega

volatile byte data[8][16], dataBuff[8][16];
volatile byte prpm9, prpm10;
volatile byte CylinderTDC;
volatile byte fireBuff[8];

const int FiringOrd[] {0, 1, 5, 3, 6, 2, 4};

int x, y;
int CS = 6;   //chip select CAN1
int CMP = 3;  //cam pos sensor
int EEwrite, APPS, APPSi;
volatile unsigned long timeStamp, timeStampM[8];
unsigned long ID, diffT;
unsigned long lastSample, lastPrint, PRINTinterval = 1000;
unsigned long RPM, FuelLongM, FuelLongR, TimingLongM, TimingLongR;
double fuelTemp;
double oilPressure = 0;
int waterTemp = 0;
unsigned long throttlePercentage = 0;
float ait;

byte printID;
byte DLC = 0x08;  //data length code
byte RTR = 0;   //RTR bit 0 or 1
volatile byte m1, intStat;
char incomingChar;
String instring, lettersIn, inNum;
int numbersIn;
int fuel, timing;
int all = 1;
int saved;
int printed;
int counter1;
int killCyl;
int threeCyl;

void writeRegister(int CS, byte thisRegister, byte thisValue) {
    // take the chip select low to select the device:
    digitalWrite(CS, LOW);
    SPI.transfer(0x02);         //write register command (02)
    SPI.transfer(thisRegister); //Send register address
    SPI.transfer(thisValue);  //Send value to record into register
    // take the chip select high to de-select:
    digitalWrite(CS, HIGH);
}

byte readRegister(int CS, byte thisRegister) {
    byte result = 0;
    digitalWrite(CS, LOW);
    SPI.transfer(0x03);                 //read register command (03)
    SPI.transfer(thisRegister);
    result = SPI.transfer(0x00);        //shift out dummy byte (00) so "thisRegister" gets clocked into result
    digitalWrite(CS, HIGH);
    return result;
}

void modFuel(byte a, byte b, byte c ){
    FuelLongR = (b << 8) | a;    //little endian to big
    FuelLongM = FuelLongR;

    if(FuelLongR > 0x300 && fuel > 0){
        FuelLongM = FuelLongR + (FuelLongR/8);
    }
    if(FuelLongR > 0x400 && fuel > 0 && RPM > 1400){
        FuelLongM = FuelLongR + (FuelLongR/4);
    }
    if(FuelLongR > 0xA00 && fuel > 1 && RPM > 1700){
        FuelLongM = FuelLongR + (FuelLongR/2);
    }
    if(fuel > 2){
        //FuelLongM = FuelLongR + (FuelLongR/2);
        FuelLongM = APPS*2;
        if(APPS > 200){
            FuelLongM = APPS*3;
        }
    }

    if(FiringOrd[c] == killCyl)
    {
        FuelLongM = 0x0000;
    }

    if(threeCyl && RPM < 1050)
    {
        FuelLongM = FuelLongR + 500;
        if(FiringOrd[c] == 1 || FiringOrd[c] == 3 || FiringOrd[c] == 2)
        {
            FuelLongM = 0x0000;
        }
    }

    FuelLongM = constrain(FuelLongM, 0x0000, 0x0FFD);
    data[m1][5] = FuelLongM & 0x00FF;  //big endian back to little endian
    data[m1][6] = FuelLongM >> 8;

}//modFuel done

void modTiming(byte a, byte b){
    TimingLongR = (b << 8) | a;
    TimingLongM = TimingLongR;

    if(timing == 1){                   //reduce timing 2 degrees
        TimingLongM = TimingLongR - 256;
    }
    if(timing == 2){                   //add 2 degrees
        TimingLongM = TimingLongR + 256;
    }
    if(timing == 0){                   //stock
        TimingLongM = TimingLongR;
    }
    if(timing == 4){                   //static timimng 15 degrees like P7100 pump
        TimingLongM = 1920;
    }
    if(RPM > 2200 && timing == 3){   //add 2 degrees above 2200 rpm
        TimingLongM = TimingLongR + 256;
    }
    TimingLongM = constrain(TimingLongM, 0x0000, 0x0FFD);
    data[m1][9] = TimingLongM & 0x00FF;
    data[m1][10] = TimingLongM >> 8;

}

void ISR_trig0(){
//  Serial.println("ISR_trig0");
    //RECEIVE interrupt CAN 1, buffers up to 4 messages, 16 bytes per message, only 13 bytes are used, m1 is the message pointer

    digitalWrite(6, LOW);
    SPI.transfer(0x90);   //read buffer 0 Command. also clears the CAN module interrupt flag
    for(int i = 0; i < 13; i++){
        data[m1][i] = SPI.transfer(0x00);
    }

    if (data[m1][0] == 0xC7 && data[m1][1] == 0x78 && data[m1][2] == 0xFF) {
        C778FFMessage.id = data[m1][0];
        C778FFMessage.unknown = data[m1][1];
        C778FFMessage.length = data[m1][4];
        C778FFMessage.data[0] = data[m1][5];
        C778FFMessage.data[1] = data[m1][6];
        C778FFMessage.data[2] = data[m1][7];
        C778FFMessage.data[3] = data[m1][8];
        C778FFMessage.data[4] = data[m1][9];
        C778FFMessage.data[5] = data[m1][10];
        C778FFMessage.data[6] = data[m1][11];
        C778FFMessage.data[7] = data[m1][12];
        C778FFMessage.count++;
    } else if (data[m1][0] == 0xC7 && data[m1][1] == 0x5B && data[m1][2] == 0xFF) {
        C75BFFMessage.id = data[m1][0];
        C75BFFMessage.unknown = data[m1][1];
        C75BFFMessage.length = data[m1][4];
        C75BFFMessage.data[0] = data[m1][5];
        C75BFFMessage.data[1] = data[m1][6];
        C75BFFMessage.data[2] = data[m1][7];
        C75BFFMessage.data[3] = data[m1][8];
        C75BFFMessage.data[4] = data[m1][9];
        C75BFFMessage.data[5] = data[m1][10];
        C75BFFMessage.data[6] = data[m1][11];
        C75BFFMessage.data[7] = data[m1][12];
        C75BFFMessage.count++;
    } else if (data[m1][0] == 0xC7 && data[m1][1] == 0xFA && data[m1][2] == 0xDF) {
        C7FADFMessage.id = data[m1][0];
        C7FADFMessage.unknown = data[m1][1];
        C7FADFMessage.length = data[m1][4];
        C7FADFMessage.data[0] = data[m1][5];
        C7FADFMessage.data[1] = data[m1][6];
        C7FADFMessage.data[2] = data[m1][7];
        C7FADFMessage.data[3] = data[m1][8];
        C7FADFMessage.data[4] = data[m1][9];
        C7FADFMessage.data[5] = data[m1][10];
        C7FADFMessage.data[6] = data[m1][11];
        C7FADFMessage.data[7] = data[m1][12];
        C7FADFMessage.count++;
    } else if (data[m1][0] == 0xC7 && data[m1][1] == 0xFA && data[m1][2] == 0xF5) {
        C7FAF5Message.id = data[m1][0];
        C7FAF5Message.unknown = data[m1][1];
        C7FAF5Message.length = data[m1][4];
        C7FAF5Message.data[0] = data[m1][5];
        C7FAF5Message.data[1] = data[m1][6];
        C7FAF5Message.data[2] = data[m1][7];
        C7FAF5Message.data[3] = data[m1][8];
        C7FAF5Message.data[4] = data[m1][9];
        C7FAF5Message.data[5] = data[m1][10];
        C7FAF5Message.data[6] = data[m1][11];
        C7FAF5Message.data[7] = data[m1][12];
        C7FAF5Message.count++;
    } else if (data[m1][0] == 0xC7 && data[m1][1] == 0xFA && data[m1][2] == 0xF2) {
        C7FAF2Message.id = data[m1][0];
        C7FAF2Message.unknown = data[m1][1];
        C7FAF2Message.length = data[m1][4];
        C7FAF2Message.data[0] = data[m1][5];
        C7FAF2Message.data[1] = data[m1][6];
        C7FAF2Message.data[2] = data[m1][7];
        C7FAF2Message.data[3] = data[m1][8];
        C7FAF2Message.data[4] = data[m1][9];
        C7FAF2Message.data[5] = data[m1][10];
        C7FAF2Message.data[6] = data[m1][11];
        C7FAF2Message.data[7] = data[m1][12];
        C7FAF2Message.count++;
    } else if (data[m1][0] == 0xC7 && data[m1][1] == 0xFA && data[m1][2] == 0xF1) {
        C7FAF1Message.id = data[m1][0];
        C7FAF1Message.unknown = data[m1][1];
        C7FAF1Message.length = data[m1][4];
        C7FAF1Message.data[0] = data[m1][5];
        C7FAF1Message.data[1] = data[m1][6];
        C7FAF1Message.data[2] = data[m1][7];
        C7FAF1Message.data[3] = data[m1][8];
        C7FAF1Message.data[4] = data[m1][9];
        C7FAF1Message.data[5] = data[m1][10];
        C7FAF1Message.data[6] = data[m1][11];
        C7FAF1Message.data[7] = data[m1][12];
        C7FAF1Message.count++;
    } else if (data[m1][0] == 0xC7 && data[m1][1] == 0xFA && data[m1][2] == 0xEE) {
        C7FAEEMessage.id = data[m1][0];
        C7FAEEMessage.unknown = data[m1][1];
        C7FAEEMessage.length = data[m1][4];
        C7FAEEMessage.data[0] = data[m1][5];
        C7FAEEMessage.data[1] = data[m1][6];
        C7FAEEMessage.data[2] = data[m1][7];
        C7FAEEMessage.data[3] = data[m1][8];
        C7FAEEMessage.data[4] = data[m1][9];
        C7FAEEMessage.data[5] = data[m1][10];
        C7FAEEMessage.data[6] = data[m1][11];
        C7FAEEMessage.data[7] = data[m1][12];
        C7FAEEMessage.count++;
    } else if (data[m1][0] == 0xC7 && data[m1][1] == 0xFA && data[m1][2] == 0xEF) {
        C7FAEFMessage.id = data[m1][0];
        C7FAEFMessage.unknown = data[m1][1];
        C7FAEFMessage.length = data[m1][4];
        C7FAEFMessage.data[0] = data[m1][5];
        C7FAEFMessage.data[1] = data[m1][6];
        C7FAEFMessage.data[2] = data[m1][7];
        C7FAEFMessage.data[3] = data[m1][8];
        C7FAEFMessage.data[4] = data[m1][9];
        C7FAEFMessage.data[5] = data[m1][10];
        C7FAEFMessage.data[6] = data[m1][11];
        C7FAEFMessage.data[7] = data[m1][12];
        C7FAEFMessage.count++;
    } else if (data[m1][0] == 0xC7 && data[m1][1] == 0xFA && data[m1][2] == 0xE4) {
        C7FAE4Message.id = data[m1][0];
        C7FAE4Message.unknown = data[m1][1];
        C7FAE4Message.length = data[m1][4];
        C7FAE4Message.data[0] = data[m1][5];
        C7FAE4Message.data[1] = data[m1][6];
        C7FAE4Message.data[2] = data[m1][7];
        C7FAE4Message.data[3] = data[m1][8];
        C7FAE4Message.data[4] = data[m1][9];
        C7FAE4Message.data[5] = data[m1][10];
        C7FAE4Message.data[6] = data[m1][11];
        C7FAE4Message.data[7] = data[m1][12];
        C7FAE4Message.count++;
    } else if (data[m1][0] == 0xC7 && data[m1][1] == 0x98 && data[m1][2] == 0x0) {
        C7980Message.id = data[m1][0];
        C7980Message.unknown = data[m1][1];
        C7980Message.length = data[m1][4];
        C7980Message.data[0] = data[m1][5];
        C7980Message.data[1] = data[m1][6];
        C7980Message.data[2] = data[m1][7];
        C7980Message.data[3] = data[m1][8];
        C7980Message.data[4] = data[m1][9];
        C7980Message.data[5] = data[m1][10];
        C7980Message.data[6] = data[m1][11];
        C7980Message.data[7] = data[m1][12];
        C7980Message.count++;
    } else if (data[m1][0] == 0xC7 && data[m1][1] == 0xFA && data[m1][2] == 0xF0) {
        C7FAF0Message.id = data[m1][0];
        C7FAF0Message.unknown = data[m1][1];
        C7FAF0Message.length = data[m1][4];
        C7FAF0Message.data[0] = data[m1][5];
        C7FAF0Message.data[1] = data[m1][6];
        C7FAF0Message.data[2] = data[m1][7];
        C7FAF0Message.data[3] = data[m1][8];
        C7FAF0Message.data[4] = data[m1][9];
        C7FAF0Message.data[5] = data[m1][10];
        C7FAF0Message.data[6] = data[m1][11];
        C7FAF0Message.data[7] = data[m1][12];
        C7FAF0Message.count++;
    } else if (data[m1][0] == 0xC7 && data[m1][1] == 0xFA && data[m1][2] == 0xF6) {
        C7FAF6Message.id = data[m1][0];
        C7FAF6Message.unknown = data[m1][1];
        C7FAF6Message.length = data[m1][4];
        C7FAF6Message.data[0] = data[m1][5];
        C7FAF6Message.data[1] = data[m1][6];
        C7FAF6Message.data[2] = data[m1][7];
        C7FAF6Message.data[3] = data[m1][8];
        C7FAF6Message.data[4] = data[m1][9];
        C7FAF6Message.data[5] = data[m1][10];
        C7FAF6Message.data[6] = data[m1][11];
        C7FAF6Message.data[7] = data[m1][12];
        C7FAF6Message.count++;
    } else if (data[m1][0] == 0xC7 && data[m1][1] == 0xFA && data[m1][2] == 0xF7) {
        C7FAF7Message.id = data[m1][0];
        C7FAF7Message.unknown = data[m1][1];
        C7FAF7Message.length = data[m1][4];
        C7FAF7Message.data[0] = data[m1][5];
        C7FAF7Message.data[1] = data[m1][6];
        C7FAF7Message.data[2] = data[m1][7];
        C7FAF7Message.data[3] = data[m1][8];
        C7FAF7Message.data[4] = data[m1][9];
        C7FAF7Message.data[5] = data[m1][10];
        C7FAF7Message.data[6] = data[m1][11];
        C7FAF7Message.data[7] = data[m1][12];
        C7FAF7Message.count++;
    } else if (data[m1][0] == 0xC7 && data[m1][1] == 0xFB && data[m1][2] == 0xE0) {
        C7FBE0Message.id = data[m1][0];
        C7FBE0Message.unknown = data[m1][1];
        C7FBE0Message.length = data[m1][4];
        C7FBE0Message.data[0] = data[m1][5];
        C7FBE0Message.data[1] = data[m1][6];
        C7FBE0Message.data[2] = data[m1][7];
        C7FBE0Message.data[3] = data[m1][8];
        C7FBE0Message.data[4] = data[m1][9];
        C7FBE0Message.data[5] = data[m1][10];
        C7FBE0Message.data[6] = data[m1][11];
        C7FBE0Message.data[7] = data[m1][12];
        C7FBE0Message.count++;
    } else if (data[m1][0] == 0x67 && data[m1][1] == 0x5A && data[m1][2] == 0x0) {
        x675A0Message.data[0] = data[m1][5];
        x675A0Message.data[1] = data[m1][6];
        x675A0Message.data[2] = data[m1][7];
        x675A0Message.data[3] = data[m1][8];
        x675A0Message.data[4] = data[m1][9];
        x675A0Message.data[5] = data[m1][10];
        x675A0Message.data[6] = data[m1][11];
        x675A0Message.data[7] = data[m1][12];
        x675A0Message.count++;
    }  else if (data[m1][0] == 0x67 && data[m1][1] == 0x98 && data[m1][2] == 0x3) {
        x67983Message.data[0] = data[m1][5];
        x67983Message.data[1] = data[m1][6];
        x67983Message.data[2] = data[m1][7];
        x67983Message.data[3] = data[m1][8];
        x67983Message.data[4] = data[m1][9];
        x67983Message.data[5] = data[m1][10];
        x67983Message.data[6] = data[m1][11];
        x67983Message.data[7] = data[m1][12];
        x67983Message.count++;
    }  else if (data[m1][0] == 0x67 && data[m1][1] == 0x98 && data[m1][2] == 0x4) {
        x67984Message.data[0] = data[m1][5];
        x67984Message.data[1] = data[m1][6];
        x67984Message.data[2] = data[m1][7];
        x67984Message.data[3] = data[m1][8];
        x67984Message.data[4] = data[m1][9];
        x67984Message.data[5] = data[m1][10];
        x67984Message.data[6] = data[m1][11];
        x67984Message.data[7] = data[m1][12];
        x67984Message.count++;
    } else if (data[m1][0] == 0xA0) {
        xA0Message.id = data[m1][0];
        xA0Message.unknown = data[m1][1];
        xA0Message.length = data[m1][4];
        xA0Message.data[0] = data[m1][5];
        xA0Message.data[1] = data[m1][6];
        xA0Message.data[2] = data[m1][7];
        xA0Message.data[3] = data[m1][8];
        xA0Message.data[4] = data[m1][9];
        xA0Message.data[5] = data[m1][10];
        xA0Message.data[6] = data[m1][11];
        xA0Message.data[7] = data[m1][12];
        xA0Message.count++;
    } else if (data[m1][0] == 0xA2) {
        xA2Message.id = data[m1][0];
        xA2Message.unknown = data[m1][1];
        xA2Message.length = data[m1][4];
        xA2Message.data[0] = data[m1][5];
        xA2Message.data[1] = data[m1][6];
        xA2Message.data[2] = data[m1][7];
        xA2Message.data[3] = data[m1][8];
        xA2Message.data[4] = data[m1][9];
        xA2Message.data[5] = data[m1][10];
        xA2Message.data[6] = data[m1][11];
        xA2Message.data[7] = data[m1][12];
        xA2Message.count++;
    } else if (data[m1][0] == 0x22) {
        x22Message.id = data[m1][0];
        x22Message.unknown = data[m1][1];
        x22Message.length = data[m1][4];
        x22Message.data[0] = data[m1][5];
        x22Message.data[1] = data[m1][6];
        x22Message.data[2] = data[m1][7];
        x22Message.data[3] = data[m1][8];
        x22Message.data[4] = data[m1][9];
        x22Message.data[5] = data[m1][10];
        x22Message.data[6] = data[m1][11];
        x22Message.data[7] = data[m1][12];
        x22Message.count++;
    } else if (data[m1][0] == 0x20) {
        x20Message.id = data[m1][0];
        x20Message.unknown = data[m1][1];
        x20Message.length = data[m1][4];
        x20Message.data[0] = data[m1][5];
        x20Message.data[1] = data[m1][6];
        x20Message.data[2] = data[m1][7];
        x20Message.data[3] = data[m1][8];
        x20Message.data[4] = data[m1][9];
        x20Message.data[5] = data[m1][10];
        x20Message.data[6] = data[m1][11];
        x20Message.data[7] = data[m1][12];
        x20Message.count++;
    } else { // we only want unknown PID's in the databuff
        for(int i = 0; i < 13; i++) {
            if (!saved) { dataBuff[m1][i] = data[m1][i]; }

            switch (i) {
                case 0:
                    newMessage.id = data[m1][i];
                    break;
                case 1:
                    newMessage.unknown = data[m1][i];
                    break;
                case 4:
                    newMessage.length = data[m1][i];
                    break;
                case 5:
                case 6:
                case 7:
                case 8:
                case 9:
                case 10:
                case 11:
                case 12:
                    newMessage.data[i - 5] = data[m1][i];
                    break;
                default:
                    break;
            }
        }
        // messages can fill the memory up and FAST...
        canMessages.push(newMessage);
    }
//    canMessages.push(newMessage);
    digitalWrite(6, HIGH);

    //modify fuel bytes if CAN message has ID of 0x20

    if(data[m1][0] == 0x20)
    {
        CylinderTDC ++;
        if(CylinderTDC > 6){CylinderTDC = 1;}

        if(!saved){fireBuff[m1] = FiringOrd[CylinderTDC];}

        modFuel(data[m1][5], data[m1][6], CylinderTDC);    //bytes 5 and 6 have the fuel value. Bytes 2 and 3 are for extended IDs 29 bit.
        //This bus only uses 11 bit standard IDs so bytes 2 and 3 are not printed.


        //modify timing bytes if CAN message has ID of 0x20
        modTiming(data[m1][9], data[m1][10]); //bytes 9 and 10 are timing value


        //Load Transmit buffer 0
        while((readRegister(6, 0x30)&0x08)){  //first wait for any pending tx, checks success or fail
        }

        digitalWrite(6,LOW);
        SPI.transfer(0x40);           //load tx buffer 0 CMD
        for(int i = 0; i< 13; i++){
            SPI.transfer(data[m1][i]); //copy each byte that came in and substitute the modified bytes
        }
        digitalWrite(6,HIGH);

        //send the transmit buffer, all bytes
        //RTS tx buffer 0
        digitalWrite(6, LOW);
        SPI.transfer(0x81);   //RTS command
        digitalWrite(6, HIGH);

    }

    //let the main routine know an interrupt occured and update the message pointer.
    intStat = 1;
    if(!saved)
    {
        timeStampM[m1] = micros();
    }
    if(m1 == 7 && !saved)
    {
        // printed = 0;
        saved = 1;
    }
    m1 = (m1+1)%8;
    if(printed && m1 == 0)
    {
        saved = 0;
        printed = 0;
    }
    digitalWrite(4, LOW);
}// ISR done



void dtcErase()
{
    for(int i = 0; i < 65; i++){
        EEPROM.write(i,0);
    }
    Serial.println(" DTC ERRORS erased ");
}
void printDTC()
{
    //print raw can bytes in message groups NOTE: includes extended ID bytes which are probably not valid for Cummins
    for(int i = 0; i < 65; i++){
        if(!(i % 13) && i != 0){Serial.println();} //start a new line after printing 13 bytes
        Serial.print((EEPROM.read(i)),HEX);
        Serial.print(" ");

    }
    Serial.println();
}

int onlyPrint16 = 0;
void printout(){
    int offSet;
    byte counter;
    float Timing;
    float FuelPCT;
    float volts;

    while (!canMessages.isEmpty() && onlyPrint16++ < 16) {
        CanMessage currentMessage = canMessages.pop();
        Serial.print("New message: ");
        Serial.print(currentMessage.id, HEX);
        Serial.print(" ");
        Serial.print(currentMessage.unknown, HEX);
        Serial.print(" ");
        Serial.print(currentMessage.length, HEX);

        for (unsigned char i : currentMessage.data) {
            Serial.print(" ");
            Serial.print(i, HEX);
        }
        Serial.println();
    }
    Serial.println();
    onlyPrint16 = 0;

    // start xA0Message
    Serial.print("xA0Message ");
    Serial.print(xA0Message.id, HEX);
    Serial.print(" ");
    Serial.print(xA0Message.unknown, HEX);
    Serial.print(" ");
    Serial.print(xA0Message.unknown2, HEX);
    Serial.print(" ");
    Serial.print(xA0Message.length);
    Serial.print(" ");
    Serial.print(xA0Message.data[0]);
    Serial.print(" ");
    Serial.print(xA0Message.data[1]);
    Serial.print(" ");
    Serial.print(xA0Message.data[2]);
    Serial.print(" ");
    Serial.print(xA0Message.data[3]); // always 255
    Serial.print(" ");
    Serial.print(xA0Message.data[4]); // always 255
    Serial.print(" ");
    Serial.print(xA0Message.data[5]); // always 255
    Serial.print(" ");
    Serial.print(xA0Message.data[6]); // always 255
    Serial.print(" ");
    Serial.print(xA0Message.data[7]); // always 255
    Serial.print(" ");
    Serial.print(xA0Message.count);
    Serial.print("    ACK   ");
    Serial.println();
    // done xA0Message

    // start C778FFMessage
    Serial.print("C778FFMessage ");
    Serial.print(C778FFMessage.id, HEX);
    Serial.print(" ");
    Serial.print(C778FFMessage.unknown, HEX);
    Serial.print(" ");
    Serial.print(C778FFMessage.unknown2, HEX);
    Serial.print(" ");
    Serial.print(C778FFMessage.length);
    Serial.print(" ");
    Serial.print(C778FFMessage.data[0]);
    Serial.print(" ");
    Serial.print(C778FFMessage.data[1]);
    Serial.print(" ");
    Serial.print(C778FFMessage.data[2]);
    Serial.print(" ");
    Serial.print(C778FFMessage.data[3]); // always 255
    Serial.print(" ");
    Serial.print(C778FFMessage.data[4]); // always 255
    Serial.print(" ");
    Serial.print(C778FFMessage.data[5]); // always 255
    Serial.print(" ");
    Serial.print(C778FFMessage.data[6]); // always 255
    Serial.print(" ");
    Serial.print(C778FFMessage.data[7]); // always 255
    Serial.print(" ");
    Serial.print(C778FFMessage.count);
    Serial.println();
    // done C778FFMessage

    // start C7FADFMessage
    Serial.print("C7FADFMessage ");
    Serial.print(C7FADFMessage.id, HEX);
    Serial.print(" ");
    Serial.print(C7FADFMessage.unknown, HEX);
    Serial.print(" ");
    Serial.print(C7FADFMessage.unknown2, HEX);
    Serial.print(" ");
    Serial.print(C7FADFMessage.length);
    Serial.print(" ");
    Serial.print(C7FADFMessage.data[0]);
    Serial.print(" ");
    Serial.print(C7FADFMessage.data[1]);
    Serial.print(" ");
    Serial.print(C7FADFMessage.data[2]);
    Serial.print(" ");
    Serial.print(C7FADFMessage.data[3]); // always 255
    Serial.print(" ");
    Serial.print(C7FADFMessage.data[4]); // always 255
    Serial.print(" ");
    Serial.print(C7FADFMessage.data[5]); // always 255
    Serial.print(" ");
    Serial.print(C7FADFMessage.data[6]); // always 255
    Serial.print(" ");
    Serial.print(C7FADFMessage.data[7]); // always 255
    Serial.print(" ");
    Serial.print(C7FADFMessage.count);
    Serial.println();
    // done C7FADFMessage

    // start C75BFFMessage
    Serial.print("C75BFFMessage ");
    Serial.print(C75BFFMessage.id, HEX);
    Serial.print(" ");
    Serial.print(C75BFFMessage.unknown, HEX);
    Serial.print(" ");
    Serial.print(C75BFFMessage.unknown2, HEX);
    Serial.print(" ");
    Serial.print(C75BFFMessage.length);
    Serial.print(" ");
    Serial.print(C75BFFMessage.data[0]);
    Serial.print(" ");
    Serial.print(C75BFFMessage.data[1]);
    Serial.print(" ");
    Serial.print(C75BFFMessage.data[2]);
    Serial.print(" ");
    Serial.print(C75BFFMessage.data[3]); // always 255
    Serial.print(" ");
    Serial.print(C75BFFMessage.data[4]); // always 255
    Serial.print(" ");
    Serial.print(C75BFFMessage.data[5]); // always 255
    Serial.print(" ");
    Serial.print(C75BFFMessage.data[6]); // always 255
    Serial.print(" ");
    Serial.print(C75BFFMessage.data[7]); // always 255
    Serial.print(" ");
    Serial.print(C75BFFMessage.count);
    Serial.println();
    // done C75BFFMessage
    
    // start C7980Message
    Serial.print("C7980Message ");
    Serial.print(C7980Message.id, HEX);
    Serial.print(" ");
    Serial.print(C7980Message.unknown, HEX);
    Serial.print(" ");
    Serial.print(C7980Message.unknown2, HEX);
    Serial.print(" ");
    Serial.print(C7980Message.length);
    Serial.print(" ");
    Serial.print(C7980Message.data[0]);
    Serial.print(" ");
    Serial.print(C7980Message.data[1]);
    Serial.print(" ");
    Serial.print(C7980Message.data[2]);
    Serial.print(" ");
    Serial.print(C7980Message.data[3]); // always 255
    Serial.print(" ");
    Serial.print(C7980Message.data[4]); // always 255
    Serial.print(" ");
    Serial.print(C7980Message.data[5]); // always 255
    Serial.print(" ");
    Serial.print(C7980Message.data[6]); // always 255
    Serial.print(" ");
    Serial.print(C7980Message.data[7]); // always 255
    Serial.print(" ");
    Serial.print(C7980Message.count);
    Serial.println();
    // done C7980Message

    // start C7FAE4Message
    Serial.print("C7FAE4Message ");
    Serial.print(C7FAE4Message.id, HEX);
    Serial.print(" ");
    Serial.print(C7FAE4Message.unknown, HEX);
    Serial.print(" ");
    Serial.print(C7FAE4Message.unknown2, HEX);
    Serial.print(" ");
    Serial.print(C7FAE4Message.length);
    Serial.print(" ");
    Serial.print(C7FAE4Message.data[0]);
    Serial.print(" ");
    Serial.print(C7FAE4Message.data[1]);
    Serial.print(" ");
    Serial.print(C7FAE4Message.data[2]);
    Serial.print(" ");
    Serial.print(C7FAE4Message.data[3]); // always 255
    Serial.print(" ");
    Serial.print(C7FAE4Message.data[4]); // always 255
    Serial.print(" ");
    Serial.print(C7FAE4Message.data[5]); // always 255
    Serial.print(" ");
    Serial.print(C7FAE4Message.data[6]); // always 255
    Serial.print(" ");
    Serial.print(C7FAE4Message.data[7]); // always 255
    Serial.print(" ");
    Serial.print(C7FAE4Message.count);
    Serial.println();
    // done C7FAE4Message

    // start C7FAEEMessage
    Serial.print("C7FAEEMessage ");
    Serial.print(C7FAEEMessage.id, HEX);
    Serial.print(" ");
    Serial.print(C7FAEEMessage.unknown, HEX);
    Serial.print(" ");
    Serial.print(C7FAEEMessage.unknown2, HEX);
    Serial.print(" ");
    Serial.print(C7FAEEMessage.length);
    Serial.print(" ");
    Serial.print(C7FAEEMessage.data[0]);
    Serial.print(" ");
    Serial.print(C7FAEEMessage.data[1]);
    Serial.print(" ");
    Serial.print(C7FAEEMessage.data[2]);
    Serial.print(" ");
    Serial.print(C7FAEEMessage.data[3]); // always 255
    Serial.print(" ");
    Serial.print(C7FAEEMessage.data[4]); // always 255
    Serial.print(" ");
    Serial.print(C7FAEEMessage.data[5]); // always 255
    Serial.print(" ");
    Serial.print(C7FAEEMessage.data[6]); // always 255
    Serial.print(" ");
    Serial.print(C7FAEEMessage.data[7]); // always 255
    Serial.print(" ");
    Serial.print(C7FAEEMessage.count);
    Serial.println();
    // done C7FAEEMessage

    // start C7FAEFMessage
    Serial.print("C7FAEFMessage ");
    Serial.print(C7FAEFMessage.id, HEX);
    Serial.print(" ");
    Serial.print(C7FAEFMessage.unknown, HEX);
    Serial.print(" ");
    Serial.print(C7FAEFMessage.unknown2, HEX);
    Serial.print(" ");
    Serial.print(C7FAEFMessage.length);
    Serial.print(" ");
    Serial.print(C7FAEFMessage.data[0]);
    Serial.print(" ");
    Serial.print(C7FAEFMessage.data[1]);
    Serial.print(" ");
    Serial.print(C7FAEFMessage.data[2]);
    Serial.print(" ");
    Serial.print(C7FAEFMessage.data[3]); // changes with throttle
    Serial.print(" ");
    Serial.print(C7FAEFMessage.data[4]); // always 255
    Serial.print(" ");
    Serial.print(C7FAEFMessage.data[5]); // always 255
    Serial.print(" ");
    Serial.print(C7FAEFMessage.data[6]); // always 0
    Serial.print(" ");
    Serial.print(C7FAEFMessage.data[7]); // always 255
    Serial.print(" Count: ");
    Serial.print(C7FAEFMessage.count);
    Serial.print(" Guess? ");
    Serial.print(C7FAEFMessage.data[3] * 4 / 6.895); // OIL PRESSURE!!!
    oilPressure = C7FAEFMessage.data[3] * 4 / 6.895;
    Serial.println();
    // done C7FAEFMessage

    // start C7FAF1Message
    Serial.print("C7FAF1Message ");
    Serial.print(C7FAF1Message.id, HEX);
    Serial.print(" ");
    Serial.print(C7FAF1Message.unknown, HEX);
    Serial.print(" ");
    Serial.print(C7FAF1Message.unknown2, HEX);
    Serial.print(" ");
    Serial.print(C7FAF1Message.length);
    Serial.print(" ");
    Serial.print(C7FAF1Message.data[0]);
    Serial.print(" ");
    Serial.print(C7FAF1Message.data[1]);
    Serial.print(" ");
    Serial.print(C7FAF1Message.data[2]);
    Serial.print(" ");
    Serial.print(C7FAF1Message.data[3]); // always 255
    Serial.print(" ");
    Serial.print(C7FAF1Message.data[4]); // always 255
    Serial.print(" ");
    Serial.print(C7FAF1Message.data[5]); // always 255
    Serial.print(" ");
    Serial.print(C7FAF1Message.data[6]); // always 255
    Serial.print(" ");
    Serial.print(C7FAF1Message.data[7]); // always 255
    Serial.print(" ");
    Serial.print(C7FAF1Message.count);
    Serial.println();
    // done C7FAF1Message

    // start C7FAF2Message
    Serial.print("C7FAF2Message ");
    Serial.print(C7FAF2Message.id, HEX);
    Serial.print(" ");
    Serial.print(C7FAF2Message.unknown, HEX);
    Serial.print(" ");
    Serial.print(C7FAF2Message.unknown2, HEX);
    Serial.print(" ");
    Serial.print(C7FAF2Message.length);
    Serial.print(" ");
    Serial.print(C7FAF2Message.data[0]);
    Serial.print(" ");
    Serial.print(C7FAF2Message.data[1]);
    Serial.print(" ");
    Serial.print(C7FAF2Message.data[2]);
    Serial.print(" ");
    Serial.print(C7FAF2Message.data[3]); // always 255
    Serial.print(" ");
    Serial.print(C7FAF2Message.data[4]); // always 255
    Serial.print(" ");
    Serial.print(C7FAF2Message.data[5]); // always 255
    Serial.print(" ");
    Serial.print(C7FAF2Message.data[6]); // always 255
    Serial.print(" ");
    Serial.print(C7FAF2Message.data[7]); // always 255
    Serial.print(" ");
    Serial.print(C7FAF2Message.count);
    Serial.println();
    // done C7FAF2Message

    // start C7FAF5Message
    Serial.print("C7FAF5Message ");
    Serial.print(C7FAF5Message.id, HEX);
    Serial.print(" ");
    Serial.print(C7FAF5Message.unknown, HEX);
    Serial.print(" ");
    Serial.print(C7FAF5Message.unknown2, HEX);
    Serial.print(" ");
    Serial.print(C7FAF5Message.length);
    Serial.print(" ");
    Serial.print(C7FAF5Message.data[0]);
    Serial.print(" ");
    Serial.print(C7FAF5Message.data[1]);
    Serial.print(" ");
    Serial.print(C7FAF5Message.data[2]);
    Serial.print(" ");
    Serial.print(C7FAF5Message.data[3]); // always 255
    Serial.print(" ");
    Serial.print(C7FAF5Message.data[4]); // always 255
    Serial.print(" ");
    Serial.print(C7FAF5Message.data[5]); // always 255
    Serial.print(" ");
    Serial.print(C7FAF5Message.data[6]); // always 255
    Serial.print(" ");
    Serial.print(C7FAF5Message.data[7]); // always 255
    Serial.print(" ");
    Serial.print(C7FAF5Message.count);
    Serial.println();
    // done C7FAF5Message

    // start x675A0Message
    Serial.print("x675A0Message ");
    Serial.print(x675A0Message.id, HEX);
    Serial.print(" ");
    Serial.print(x675A0Message.unknown, HEX);
    Serial.print(" ");
    Serial.print(x675A0Message.unknown2, HEX);
    Serial.print(" ");
    Serial.print(x675A0Message.length);
    Serial.print(" ");
    Serial.print(x675A0Message.data[0]);
    Serial.print(" ");
    Serial.print(x675A0Message.data[1]);
    Serial.print(" ");
    Serial.print(x675A0Message.data[2]);
    Serial.print(" ");
    Serial.print(x675A0Message.data[3]); // always 255
    Serial.print(" ");
    Serial.print(x675A0Message.data[4]); // always 255
    Serial.print(" ");
    Serial.print(x675A0Message.data[5]); // always 255
    Serial.print(" ");
    Serial.print(x675A0Message.data[6]); // always 255
    Serial.print(" ");
    Serial.print(x675A0Message.data[7]); // always 255
    Serial.print(" ");
    Serial.print(x675A0Message.count);
    Serial.println();
    // done x675A0Message

    // start x67983Message
    Serial.print("x67983Message ");
    Serial.print(x67983Message.id, HEX);
    Serial.print(" ");
    Serial.print(x67983Message.unknown, HEX);
    Serial.print(" ");
    Serial.print(x67983Message.unknown2, HEX);
    Serial.print(" ");
    Serial.print(x67983Message.length);
    Serial.print(" ");
    Serial.print(x67983Message.data[0]);
    Serial.print(" ");
    Serial.print(x67983Message.data[1]);
    Serial.print(" ");
    Serial.print(x67983Message.data[2]);
    Serial.print(" ");
    Serial.print(x67983Message.data[3]); // always 255
    Serial.print(" ");
    Serial.print(x67983Message.data[4]); // always 255
    Serial.print(" ");
    Serial.print(x67983Message.data[5]); // always 255
    Serial.print(" ");
    Serial.print(x67983Message.data[6]); // always 255
    Serial.print(" ");
    Serial.print(x67983Message.data[7]); // always 255
    Serial.print(" ");
    Serial.print(x67983Message.count);
    Serial.println();
    // done x67983Message

    // start x67984Message
    Serial.print("x67984Message ");
    Serial.print(x67984Message.id, HEX);
    Serial.print(" ");
    Serial.print(x67984Message.unknown, HEX);
    Serial.print(" ");
    Serial.print(x67984Message.unknown2, HEX);
    Serial.print(" ");
    Serial.print(x67984Message.length);
    Serial.print(" ");
    Serial.print(x67984Message.data[0]);
    Serial.print(" ");
    Serial.print(x67984Message.data[1]);
    Serial.print(" ");
    Serial.print(x67984Message.data[2]);
    Serial.print(" ");
    Serial.print(x67984Message.data[3]); // rpm
    Serial.print(" ");
    Serial.print(x67984Message.data[4]); // rpm
    Serial.print(" ");
    Serial.print(x67984Message.data[5]); // always 255
    Serial.print(" ");
    Serial.print(x67984Message.data[6]); // always 255
    Serial.print(" ");
    Serial.print(x67984Message.data[7]); // always 255
    Serial.print(" ");
    Serial.print(x67984Message.count);
    Serial.print(" RPM: ");
    Serial.print(((x67984Message.data[4] << 8) | x67984Message.data[3]) / 8); // rpm again :(
    Serial.println();
    // done x67984Message

    // start x67983Message
    Serial.print("C7FAF0Message ");
    Serial.print(C7FAF0Message.id, HEX);
    Serial.print(" ");
    Serial.print(C7FAF0Message.unknown, HEX);
    Serial.print(" ");
    Serial.print(C7FAF0Message.unknown2, HEX);
    Serial.print(" ");
    Serial.print(C7FAF0Message.length);
    Serial.print(" ");
    Serial.print(C7FAF0Message.data[0]);
    Serial.print(" ");
    Serial.print(C7FAF0Message.data[1]);
    Serial.print(" ");
    Serial.print(C7FAF0Message.data[2]);
    Serial.print(" ");
    Serial.print(C7FAF0Message.data[3]);
    Serial.print(" ");
    Serial.print(C7FAF0Message.data[4]);
    Serial.print(" ");
    Serial.print(C7FAF0Message.data[5]);
    Serial.print(" ");
    Serial.print(C7FAF0Message.data[6]);
    Serial.print(" ");
    Serial.print(C7FAF0Message.data[7]);
    Serial.print(" ");
    Serial.print(C7FAF0Message.count);
    Serial.println();
    // done C7FAF0Message

    // start C7FBE0Message
    Serial.print("C7FBE0Message ");
    Serial.print(C7FBE0Message.id, HEX);
    Serial.print(" ");
    Serial.print(C7FBE0Message.unknown, HEX);
    Serial.print(" ");
    Serial.print(C7FBE0Message.unknown2, HEX);
    Serial.print(" ");
    Serial.print(C7FBE0Message.length);
    Serial.print(" ");
    Serial.print(C7FBE0Message.data[0]);
    Serial.print(" ");
    Serial.print(C7FBE0Message.data[1]);
    Serial.print(" ");
    Serial.print(C7FBE0Message.data[2]);
    Serial.print(" ");
    Serial.print(C7FBE0Message.data[3]);
    Serial.print(" ");
    Serial.print(C7FBE0Message.data[4]);
    Serial.print(" ");
    Serial.print(C7FBE0Message.data[5]);
    Serial.print(" ");
    Serial.print(C7FBE0Message.data[6]);
    Serial.print(" ");
    Serial.print(C7FBE0Message.data[7]);
    Serial.print(" ");
    Serial.print(C7FBE0Message.count);
    // Confidence in PID, 1%
    // Confidence in Math, 0%
    throttlePercentage = (C7FBE0Message.data[1] * 256) + C7FBE0Message.data[0];
    throttlePercentage /= 256;
    throttlePercentage -= 24;
//    throttlePercentage *= (100/255);
    throttlePercentage /= 3;

    // Confidence in PID, 0%
    // Confidence in Math, 0%
    ait = C7FBE0Message.data[2];
    ait -= 40;
//        ait /= 256;
//        ait -= 273.15;
//        ait = ait * 9 / 5 + 32;
    Serial.println();
    // end C7FBE0Message

    //parse the 0x22 message
    Serial.print("x22Message ");
    Serial.print(x22Message.id, HEX);
    Serial.print(" ");
    Serial.print(x22Message.unknown, HEX);
    Serial.print(" ");
    Serial.print(x22Message.unknown2, HEX);
    Serial.print(" ");
    Serial.print(x22Message.length);
    Serial.print(" ");
    Serial.print(x22Message.data[0]);
    Serial.print(" ");
    Serial.print(x22Message.data[1]);
    Serial.print(" ");
    Serial.print(x22Message.data[2]);
    Serial.print(" ");
    Serial.print(x22Message.data[3]);
    Serial.print(" ");
    Serial.print(x22Message.data[4]);
    Serial.print(" ");
    Serial.print(x22Message.data[5]);
    Serial.print(" ");
    Serial.print(x22Message.data[6]);
    Serial.print(" ");
    Serial.print(x22Message.data[7]);
    Serial.print(" ");
    Serial.print(x22Message.count);
    if(x22Message.data[0] == 0x0C)
    {
        Serial.print(" idle val sw crank - NR "); //Idle validation switch in TPS goes to IP for fault protection
    }
    if(x22Message.data[0] == 0x04)
    {
        Serial.print(" idle val sw acc pedal up  ");
    }
    if(x22Message.data[0] == 0x00)
    {
        Serial.print(" idle val sw acc pedal pressed ");
    }

    // This byte changes from 0 to 1 after a delay. Seems to check if the timing advance mechanism is
    // able to match the commanded timing value
    ((x22Message.data[1] & 0x01) == 0x00) ? Serial.print(" timing locked ") : Serial.print(" timing error ");

    //cylinder counter is high nibble of byte 8. Lower nibble changes from 0 to 3 when running
    counter = ((x22Message.data[3] >> 4) / 2) + 1; //decode; counts by even numbers
    if(!(x22Message.data[3] & 0x03))
    {
        counter = 0;    //not running
    }
    Serial.print(" counter ");
    Serial.print(counter);

    Serial.print(" RPM range ");
    Serial.print(x22Message.data[3] & 0x0F);

    //bytes 4 and 5 are RPM again

    //Not sure about this compute fuel temp in Inj Pump
    // Confidence this is the correct PID, 100%
    // Confidence in the math, 100%
    // raw  F as reported by my quadzilla
    // 4984 100
    // 4989 101
    // 4999 102
    // 5006 103
    //
    //
    // 5034 106
    // 5041 107
    // 5051 108
    // 5065 109
    fuelTemp = (x22Message.data[7] << 8) | x22Message.data[6]; // Raw
    Serial.print(" Fuel temp F Raw: ");
    Serial.print(x22Message.data[7]);
    fuelTemp = fuelTemp / 16; // Kelvin
    fuelTemp = fuelTemp - 273.15; // Celsius
    fuelTemp = ((fuelTemp * 9) / 5) + 32; // Fahrenheit
    Serial.print(" F: ");
    Serial.print(fuelTemp);

    Serial.println();
    // 0x22 done

    // 0xA2 start
    Serial.print("xA2Message ");
    Serial.print(xA2Message.id, HEX);
    Serial.print(" ");
    Serial.print(xA2Message.unknown, HEX);
    Serial.print(" ");
    Serial.print(xA2Message.unknown2, HEX);
    Serial.print(" ");
    Serial.print(xA2Message.length);
    Serial.print(" ");
    Serial.print(xA2Message.data[0]);
    Serial.print(" ");
    Serial.print(xA2Message.data[1]);
    Serial.print(" ");
    Serial.print(xA2Message.data[2]);
    Serial.print(" ");
    Serial.print(xA2Message.data[3]);
    Serial.print(" ");
    Serial.print(xA2Message.data[4]);
    Serial.print(" ");
    Serial.print(xA2Message.data[5]);
    Serial.print(" ");
    Serial.print(xA2Message.data[6]);
    Serial.print(" ");
    Serial.print(xA2Message.data[7]);
    Serial.print(" ");
    Serial.print(xA2Message.count);
    //second message from IP has other stuff in it.
    offSet = (xA2Message.data[1] << 8) | xA2Message.data[0];
    Serial.print(" Offset? ");
    Serial.print(offSet);

    // 8 and 7 maybe oil pressure?
    Serial.print(" oilPressure? Raw8: ");
    Serial.print(xA2Message.data[3]);
    Serial.print(" Raw7: ");
    Serial.print(xA2Message.data[2]);
    // Lets try the fuel pressure obd2 math, pid 0A
//    oilPressure = (xA2Message.data[3] << 8 ) | (xA2Message.data[2]);
//    oilPressure /= 3;
//    Serial.print(" Guess: ");
//    Serial.print(oilPressure);

    // not sure if these are volts are not
    // Maybe AIT? Not climbing as AIT does
    Serial.print(" volts? Raw10: ");
    Serial.print(xA2Message.data[5]);
    Serial.print(" Raw9: ");
    Serial.print(xA2Message.data[4]);
    volts = (xA2Message.data[5] << 4) | xA2Message.data[4];
//            volts = volts / 6.7; // 6.7 seems close to water temp NOPE
//            volts = ((volts * 9) / 5) + 32;
    volts = volts / 35;
//            ait = volts;
    Serial.print(" Volts ");
    Serial.print(volts,1);

    // 11 and 12 seem to be AIT?
    offSet = (xA2Message.data[7] << 8) | xA2Message.data[6];
    offSet = offSet / 16;
    offSet = offSet - 273.15;
    offSet = ((offSet * 9) / 5) + 32;
//            offSet = offSet / 100;
//            offSet = ((offSet * 9) / 5) + 32;
    Serial.print(" AIT? ");
    Serial.print(offSet);

    //TODO Other stuff to figure out
    Serial.println();
    //0xA2 done

    // 0x20 start
    Serial.print("x20Message ");
    Serial.print(x20Message.id, HEX);
    Serial.print(" ");
    Serial.print(x20Message.unknown, HEX);
    Serial.print(" ");
    Serial.print(x20Message.unknown2, HEX);
    Serial.print(" ");
    Serial.print(x20Message.length);
    Serial.print(" ");
    Serial.print(x20Message.data[0]);
    Serial.print(" ");
    Serial.print(x20Message.data[1]);
    Serial.print(" ");
    Serial.print(x20Message.data[2]);
    Serial.print(" ");
    Serial.print(x20Message.data[3]);
    Serial.print(" ");
    Serial.print(x20Message.data[4]);
    Serial.print(" ");
    Serial.print(x20Message.data[5]);
    Serial.print(" ");
    Serial.print(x20Message.data[6]);
    Serial.print(" ");
    Serial.print(x20Message.data[7]);
    Serial.print(" ");
    Serial.print(x20Message.count);
    //this is the reply from the ECM to the IP fuel / timing request message
    // Fuel command for the next cylinder in firing order.
    if(counter)
    {
        Serial.print(" cylinder ");
        Serial.print(fireBuff[y]);
    }

    //Fuel compute
    FuelPCT = (x20Message.data[1] << 8) | x20Message.data[0];
    FuelPCT = (float)(FuelPCT*100) / 4096;    // 4095 is max fuel allowed by pump
    Serial.print(" Fuel percent ");
    Serial.print(FuelPCT, 1);

    // compute timing advance
    Timing = (x20Message.data[5] << 8) | x20Message.data[4]; //convert from little endian. 128 bits per degree.
    Timing = (float)((Timing) / 128);      //
    Serial.print(" Timing deg ");
    Serial.print(Timing, 1);

    //compute RPM from ID 20xx bytes 11 and 12
    RPM = (x20Message.data[7] << 8) | x20Message.data[6];     //convert from little endian
    RPM = RPM/4 ;                           //divide by 4
    Serial.print(" RPM ");
    Serial.print(RPM);
    if(RPM > 1050){threeCyl = 0;}  //disable 3 cyclinder mode if rev'd
    Serial.println();
    //0x20 done

    // unknown PIDs
//    for(y = 0; y < 8; y++){
//        for(int i = 0; i < (dataBuff[y][4]&0x0F)+5; i++){ //data[y][4] lower nibble is the DLC (data length)
//            //do not print bytes 2 and 3 if STID frame
////            if(i > 3 || i < 2){
//                if((dataBuff[y][0] == printID) || (all == 1))
//                {
//                    Serial.print(dataBuff[y][i], HEX);
//                    Serial.print(" ");
//                }
////            }
//        }
//        Serial.print(" ");
//        Serial.print("\t");
        //determine message direction. Pump messages have 0x40 in byte 1.
        // (dataBuff[y][1] == 0x40) ? Serial.print(" P >> E ") : Serial.print(" E >> P ");
        //  Serial.print(timeStampM[y]);
        //  Serial.print(" diff ");
        //wrap around buffer 8 deep
        //  (y != 0) ? Serial.print(timeStampM[y] - timeStampM[y-1]) : Serial.print(timeStampM[7] - timeStampM[0]);

//        if(dataBuff[y][0] == 0x67) {
//            Serial.print(" THIS'N! 1: " + (String)dataBuff[y][1] + " 2: "+ (String)dataBuff[y][2] + " 3: "+ (String)dataBuff[y][3] + " 4: "+ (String)dataBuff[y][4] + " 5: "+ (String)dataBuff[y][5] + " 6: "+ (String)dataBuff[y][6] + " 7: "+ (String)dataBuff[y][7] + " 8: "+ (String)dataBuff[y][8] + " 9: "+ (String)dataBuff[y][9] + " 10: "+ (String)dataBuff[y][10] + " 11: "+ (String)dataBuff[y][11] + " 12: "+ (String)dataBuff[y][12]);
//// 3 4 5 6
//            waterTemp = dataBuff[y][4];
//            oilPressure = dataBuff[y][3];
//        }

//        if(dataBuff[y][0] == 0xC7 && dataBuff[y][1] == 0xFB) {
//            // Confidence in PID, 99%
//            // Confidence in Math, 20%
//            oilPressure = (dataBuff[y][6] * 256) + dataBuff[y][5];
//            oilPressure /= 256;
//
//            // Confidence in PID, 0%
//            // Confidence in Math, 0%
////            ait = (float)(dataBuff[y][12] * 256) + (float)dataBuff[y][11];
//                ait = dataBuff[y][7];
//                ait -= 40;
////            ait /= 256;
////            ait -= 273.15;
////            ait = ait * 9 / 5 + 32;
//        }

//        if(dataBuff[y][0] == 0x67 && dataBuff[y][1] == 0x98 && dataBuff[y][4] == 0x8 && dataBuff[y][5] == 0xF0 && dataBuff[y][9] != 0xFF) {
            // 6 and 7 always seem to be the same bit, maybe this has meaning?
            // didn't show up on startup until I pressed the throttle :thinking:
            // also seems to stop when the throttle is pressed and show back up when you take your foot off
//            waterTemp = (dataBuff[y][9] << 4) | (dataBuff[y][8]);
//            waterTemp = waterTemp / 1000;
//            waterTemp = ((waterTemp * 9) / 5) + 32;
//        }

        //This message from the ECM is mostly static
//        if(dataBuff[y][0] == 0xA0)
//        {
//            Serial.print(" ACK ");
//        }
//        Serial.println();
//    }
    Serial.println();
    // end unknowns

}   //print out done

void printOut2()
{
    Serial.print(FuelLongM);
    Serial.print(" ");
    Serial.print(TimingLongM);
    Serial.print(" ");
    Serial.print(APPS);
    Serial.print(" ");
    Serial.println();
    //Print the error detection registers (CAN module)
    Serial.print(" T E C ");
    Serial.print(readRegister(CS, 0x1C),HEX);
    Serial.print(" R E C ");
    Serial.print(readRegister(CS, 0x1D),HEX);
    Serial.print(" Error Flags ");
    Serial.println(readRegister(CS, 0x2D),HEX);
}

void help()
{
    Serial.println();
    Serial.println(F(" print [ID] <ENTER> prints messages with first byte matching ID number. ID is DEC e.g: 32 "));
    Serial.println(F(" print all <ENTER> prints all messages "));
    Serial.println(F(" dtc <ENTER> displays stored Diagnostic Trouble Codes "));
    Serial.println(F(" erase <ENTER> erases stored Diagnostic Trouble Codes "));
    Serial.println(F(" fuel [level] <ENTER> fueling level 0 thru 3; 0 is stock "));
    Serial.println(F(" timing [level] <ENTER> timing level 0 thru 3; 0 is stock "));
    Serial.println(F(" kill [1 thru 6] <ENTER> Stop fuel to cylinder number; kill 0 is normal "));
    //Serial.println(F(" trim <ENTER> 1 thru 6 cylinder # then 128 +/- 10 adjust individual cylinder injection duration. "));
    //Serial.println(F(" values under 128 subtract fuel. e.g.: trim 2 123 <ENTER> sets cylinder 2 to negative 5 % "));
    //Serial.println(F(" values over 128 add fuel. e.g.: trim 5 130 <ENTER> sets cylinder 5 to positive 2 % "));
    //Serial.println(F(" save <ENTER> saves the fuel trim values to non volatile memory "));
    Serial.println();
}   //end of help

void serialREC()
{
    //process any incoming bytes from the serial port.
    // read the incoming characters
    //separate numbers from Alpha
    //build number or alpha string from incoming stream
    //end when newline <ENTER> is received


    while (Serial.available() > 0)
    {
        incomingChar = Serial.read();
        if (isDigit(incomingChar))
        {
            inNum += incomingChar;      //concatonate this is a string not interger
        }
        if (isAlpha(incomingChar))
        {
            instring += incomingChar;
        }
        if (incomingChar == '?') {    //help prompt
            help();
        }
        if (incomingChar == '\n')     //<enter> end of input
        {
            lettersIn = instring;
            numbersIn = (inNum.toInt());    //now string is converted to interger
            instring = "";
            inNum = "";
            //  Serial.print(lettersIn);
            //  Serial.print(" ");
            //  Serial.println(numbersIn);
        }
    } //end of while serial

    if(lettersIn == "printall"){
        lettersIn = "";
        all = 1;
        Serial.println("all ");
    }
    if(lettersIn == "print"){
        lettersIn = "";
        all = 0;
        printID = numbersIn;
        Serial.println(printID);
    }
    if(lettersIn == "dtc"){
        lettersIn = "";
        printDTC();
    }


    if (lettersIn == "erase") {
        lettersIn = "";
        instring = "erase Dtc ";        //sub in this string so we know what the next question is answering
        Serial.println(F(" Are you sure? y or n <ENTER> "));
    }
    if (lettersIn == "erase Dtc y") { // y or n gets appended
        lettersIn = "";
        dtcErase();
    }
    if (lettersIn == "erase Dtc n") {
        lettersIn = "";
        Serial.println(" NOT erased ");
    }


    if (lettersIn == "trim") {
//    fuelBalance();
    }
    if (lettersIn == "save") {
//    saveFuelBalance();
    }
    if (lettersIn == "fuel") {
        lettersIn = "";
        fuel = numbersIn;
        EEPROM.write(0xFE, fuel); //save for next time
        Serial.print("fuel ");
        Serial.println(fuel);
    }

    if (lettersIn == "timing") {
        lettersIn = "";
        timing = numbersIn;
        EEPROM.write(0xFD, timing);// save for next time
        Serial.print("timing ");
        Serial.println(timing);
    }
    if (lettersIn == "kill") {
        lettersIn = "";
        killCyl = numbersIn;
        if (killCyl < 0 || killCyl > 6) {
            killCyl = 0;
        }
        Serial.print(" Cylinder off ");
        Serial.println(killCyl);
        Serial.println();
    }
    if (lettersIn == "three"){
        lettersIn ="";
        Serial.println(" 3 cylinder mode. type three again to toggle ");
        threeCyl = !threeCyl;
    }

}   //end of serial rec

void setup(){

    Serial.begin(115200);

    // Init the lcd
    lcd.init();
    lcd.clear();
    lcd.backlight();      // Make sure backlight is on

    // start the SPI library:
    SPI.begin();
    //default settings, but shown here for learning example.
    SPI.setBitOrder(MSBFIRST);
    SPI.setDataMode(SPI_MODE0);
    SPI.setClockDivider(SPI_CLOCK_DIV2);

    // initalize the  intterupts, ss, and chip select pins:

    pinMode(ss, OUTPUT);    //Arduino is master
    pinMode(CANint1, INPUT);  //INT0
    pinMode(CMP, INPUT);      //cam posistion sensor input
    pinMode(CS, OUTPUT);
    pinMode(4, OUTPUT);   //debug pin

    // take the chip select low to select the device
    digitalWrite(6, LOW);
    SPI.transfer(0xC0); //reset CAN1 command & enter config mode
    // take the chip select high to de-select
    digitalWrite(6, HIGH);

    // give the mcp2515 time to reset
    delay(10);

    //do this once for CAN1
    //set baud rate to 8MHZ osc 250Kb/s
    writeRegister(CS, 0x2A, 0x40);  //cnf1
    writeRegister(CS, 0x29, 0xF1);  //cnf2
    writeRegister(CS, 0x28, 0x85);  //cnf3

    //recieve buff 0 apply mask and filters SID or EXID
    writeRegister(CS, 0x60, 0x60);  // 60 RXB0CTRL rec all. ignore filters

    //set mask to filter all bits
    // writeRegister(CS, 0x20,0xFF);
    //writeRegister(CS, 0x21,0xD0);

    //set filters
    // writeRegister(CS, 0x00, 0x00);
    // writeRegister(6, 0x01, 0x40);
    //writeRegister(7, 0x01, 0x20);

    //SOF signal output to mcp2515 CLCKout pin 3 for O'scope trigger
    writeRegister(CS, 0x0F, 0x04);  //canctrl ******normal mode***** //one shot mode trans disabled

    //*** optional: un-comment below for loopback mode
    //CAN change from config mode to loop back mode / one shot mode enabled
    // writeRegister(CS, 0x0F, 0x4C);  //canctrl *******loopback***

    if(readRegister(CS, 0x0E)== 0 && readRegister(CS, 0x0F) == 04){ //mode status to check if init happened correctly
        Serial.println("CAN module init Normal ");
    }
    else{
        Serial.println("CAN module init FAILED !! ");
    }
    //get last user fuel and timing values from non volatile mem.
    fuel = EEPROM.read(0xFE);
    timing = EEPROM.read(0xFD);
    // if invalid set to default 'stock'
    if(fuel > 3 || timing > 3){
        fuel = 0;
        timing = 0;
    }
    Serial.print("Fuel level = ");
    Serial.print(fuel);
    Serial.print(" Timing level = ");
    Serial.println(timing);

    //DTC / ERRORS are stored in no volatile mem
    if(EEPROM.read(0x00) > 0){
        Serial.println("Stored DTC errors are present.");
    }
    Serial.println("Send Arduino a '?' for a menu of commands.");

    //CAN 1 module enable REC buffer 0 interrupt
    writeRegister(CS, 0x2B, 0x01);
    attachInterrupt(digitalPinToInterrupt(19), ISR_trig0, FALLING);
//    attachInterrupt(1, ISR_trig1, FALLING);

} // setup done

void ISR_trig1()
{
    Serial.println("ISR_trig1");
    CylinderTDC = 1;    //rest the cylinder counter to 1 on Cam position sensor falling edge

}

void updateLcd() {
    char buffer [21];
    // row 1
    lcd.setCursor(0,0);
    dtostrf(throttlePercentage, 5, 0, buffer);
    lcd.print(buffer);
    dtostrf(oilPressure, 5, 0, buffer);
    lcd.print(buffer);
//    dtostrf((C7FAF6Message.data[2] << 8) | C7FAF6Message.data[1], 5, 0, buffer);
    lcd.print(buffer);

    // row 2
    lcd.setCursor(0,1);
    dtostrf(C7FAF7Message.data[6], 5, 0, buffer);
    lcd.print(buffer);
    dtostrf(C7FAF7Message.data[7], 5, 0, buffer);
    lcd.print(buffer);
    float testytest = (C7FBE0Message.data[3] << 8) | C7FBE0Message.data[2];
    testytest /= 16;
    dtostrf(testytest, 5, 0, buffer);
    lcd.print(buffer);

    // row 3
    lcd.setCursor(0,2);
    dtostrf(C7FBE0Message.data[0], 5, 0, buffer);
    lcd.print(buffer);
    dtostrf((C7FBE0Message.data[1] - 24) * 100 / 256, 5, 0, buffer);
    lcd.print(buffer);
    dtostrf(C7FBE0Message.data[2], 5, 0, buffer);
    lcd.print(buffer);
    dtostrf(C7FBE0Message.data[3], 5, 0, buffer);
    lcd.print(buffer);


    // row 4
    lcd.setCursor(0,3);
    dtostrf(C7FBE0Message.data[4], 5, 0, buffer);
    lcd.print(buffer);
    dtostrf(C7FBE0Message.data[5], 5, 0, buffer);
    lcd.print(buffer);
    dtostrf(C7FBE0Message.data[6], 5, 0, buffer);
    lcd.print(buffer);
    dtostrf(C7FBE0Message.data[7], 5, 0, buffer);
    lcd.print(buffer);
}

void loop() {
    updateLcd();
    //check for ERROR messages and save up to 4
    //observed error message frames have first ID byte set to E0 or E2. Could be others though
    // so just catch anything greater than A2 (typical ECM ACK message)
    if(data[y][0] > 0xA2 && EEwrite < 4){
        for(int i = 0; i < 13; i++){
            EEPROM.write((y*13) + i, data[y][i]);
        }
        EEwrite++;
    }

    // sample APPS on chanel A0 and intergrate 'smooth'
    if(micros()-lastSample > 500){
        APPS = APPS + ((analogRead(0)- APPS)/4);
    }
// Serial.println("print out?");
    //print out
    if(millis() - lastPrint > PRINTinterval){
        lastPrint = millis();
        //print any messages from CAN 1

        if(saved == 1 && printed == 0)
        {
            printout();
            printed = 1;
        }
    } //end of print interval

    //check serial port
    serialREC();

}//loop done