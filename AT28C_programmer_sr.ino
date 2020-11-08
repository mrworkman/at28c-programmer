

// Shift-register control.
#define SR_SER   A0
#define SR_OE    A1
#define SR_RCLK  A2
#define SR_SRCLK A3

// ROM control
#define ROM_WE 11
#define ROM_OE 10

#define MAX_ADDR 0x4000 // 32KB

int numDataPins = 8;
int dataPins[] = { 2, 3, 4, 5, 6, 7, 8, 9 };

// Source the ROM data from disk.
const PROGMEM
#include "D:\\Users\\sworkman\\Documents\\Electronics\\rom\\rom.h"

void setup() {

  // Setup shift-register pins.
  pinMode(SR_SER,   OUTPUT);
  pinMode(SR_RCLK,  OUTPUT);
  pinMode(SR_SRCLK, OUTPUT);
  pinMode(SR_OE,    OUTPUT);

  // Shift-register output is ENABLED, by default.
  digitalWrite(SR_OE, LOW);

  // ROM "enable" pins are OUTPUT only.
  pinMode(ROM_WE, OUTPUT);
  pinMode(ROM_OE, OUTPUT);

  // Write (ROM_WE) is DISABLED, by default.
  digitalWrite(ROM_WE, HIGH);

  // Output (ROM_OE) is ENABLED, by default.
  digitalWrite(ROM_OE, LOW);

  // Data pins are INPUT, by default.
  for (int i = 0; i < numDataPins; i++) {
    pinMode(dataPins[i], INPUT);
  }  

  // TODO: Revisit baud.
  Serial.begin(230400);

//  Serial.println("Erasing...");
//  erase(0, MAX_ADDR);

  if (checkRom() == true) {
    Serial.println("Programming...");
    Serial.print("ROM Size: ");
    Serial.print(rom_len);
    Serial.println(" bytes.");
  
    for (int i = 0; i < rom_len; i++) {
      if ((i % 1024) == 0) {
        Serial.println(i);
      }
      writeAddress(i, pgm_read_byte(&rom[i]));
    }
  
    Serial.println("Done.");

    checkRom();
  }


//  dumpMem();
}

int checkRom() {
  bool diffFound = false;

  Serial.print("Checking ROM... ");
  
  for (int i = 0; i < rom_len; i++) {
    if (readAddress(i) != pgm_read_byte(&rom[i])) {
      diffFound = true;
      break;
    }
  }

  if (diffFound == true) {
    Serial.println("ROM is Different!");
  } else {
    Serial.println("ROM is the Same.");
  }

  return diffFound;
}

void erase(unsigned int startAddress, unsigned int count) {
  for (unsigned int i = 0; i < count; i++) {
    if ((i % 1024) == 0) {
      Serial.println(i);
    }
    
    writeAddress(startAddress + i, 0xff);
  }
}

void setAddress(unsigned int address) {
  // Bounds checking.
  if (address >= MAX_ADDR) {
    Serial.println("ERROR> Illegal address!");
    return -1;
  }

  // Send the address out onto the shift registers.
  digitalWrite(SR_RCLK, LOW);
  shiftOut(SR_SER, SR_SRCLK, MSBFIRST, address & 0xff);
  shiftOut(SR_SER, SR_SRCLK, MSBFIRST, address >> 8);
  digitalWrite(SR_RCLK, HIGH);

}

unsigned char readAddress(unsigned int address) {
  unsigned char data = 0;
  
  setAddress(address);

  // Read the byte at the given address
  for (int i = 0; i < numDataPins; i++) {
    int bit = digitalRead(dataPins[i]);
    data |= bit << i;
  }

  return data;
}

unsigned char writeAddress(unsigned int address, unsigned char data) {

  // Set our data pins to OUTPUT.
  for (int i = 0; i < numDataPins; i++) {
    pinMode(dataPins[i], OUTPUT);
  }

  // Disable output.
  digitalWrite(ROM_OE, HIGH);

  // Set the address we want to write to.
  setAddress(address);

  // Set the data we want to write.
  for (int i = 0; i < numDataPins; i++) {
    digitalWrite(dataPins[i], ((data >> i) & 1));
  }

  // Pulse ROM_WE. LOW to HIGH.
  digitalWrite(ROM_WE, LOW);
//  delay(10); 
  digitalWrite(ROM_WE, HIGH);

  // Set data pins back to INPUT.
  for (int i = 0; i < numDataPins; i++) {
    pinMode(dataPins[i], INPUT);
  }

  // Enable output.
  digitalWrite(ROM_OE, LOW);

  waitForWriteCycleCompletion();

  return readAddress(address);
}

// Polls data pin 6 waiting for the chip's write cycle to finish.
void waitForWriteCycleCompletion() {
  pinMode(dataPins[6], INPUT);

  int prev = -1;
  
  for (;;) {
    // We need to toggle ROM_OE to get the status to update.
    digitalWrite(ROM_OE, HIGH);
    // TODO: Sleep?
    digitalWrite(ROM_OE, LOW);

    int val = digitalRead(dataPins[6]);

    // Break when the pin stops flapping.
    if (val == prev) {
      break;
    }
    
    prev = val;
  }
}

char buf[100];

void dumpMem() {
  Serial.println();
  Serial.println("DUMP");
  Serial.println("=====");

  Serial.println("ADDR    BYTES                                           DATA");

  for (int row = 0; row < (MAX_ADDR / 16); row++) {
    if (row == 0) {
      Serial.print("0x0000  ");
    } else {
      sprintf(buf, "%#0.4x  ", (row * 16));
      Serial.print(buf);
    }
    
    for (int col = 0; col < 16; col++) {
      int offset = (row * 16) + col;
      
      sprintf(buf, "%0.2x ", readAddress(offset));
      Serial.print(buf);
    }

    Serial.print("................");
    Serial.println();
  }
}

void loop() { }
