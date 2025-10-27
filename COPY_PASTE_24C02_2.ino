/*
 * SKETCH EEPROM 24C02 COPIER
 * 
 * EEPROM 1 (Sumber) terhubung ke Hardware I2C (A4/A5).
 * EEPROM 2 (Tujuan) terhubung ke Software I2C (D4/D5).
 * Notifikasi menggunakan LED D13.
 */

#include <Wire.h> 
#include <SoftWire.h> 

// =========================================================
// I. KONFIGURASI PIN & I2C
// =========================================================

// Pin untuk Software I2C (EEPROM 2)
#define SOFT_SDA_PIN 4  // Pin D4
#define SOFT_SCL_PIN 5  // Pin D5
SoftWire WireAlt(SOFT_SDA_PIN, SOFT_SCL_PIN); 

// Pin Kontrol
const int PIN_BUTTON = 2;       // Tombol Start
const int PIN_LED = 13;         // LED Status (D13)

// Konfigurasi EEPROM
const int ADDR_EEPROM = 0x50;   
const int EEPROM_SIZE = 256;    

// Buffer Global (RAM Arduino)
byte dataBuffer[EEPROM_SIZE]; 

// State Machine untuk satu tombol
enum CopyState {
  WAIT_FOR_READ,
  DATA_READ_SUCCESS,
  COPY_COMPLETE
};
CopyState currentState = WAIT_FOR_READ;

// =========================================================
// II. FUNGSI I2C HARDWARE (EEPROM 1 - A4/A5)
// =========================================================

byte readEEPROM_HW(unsigned int memoryAddress) {
  byte data = 0xFF; 
  Wire.beginTransmission(ADDR_EEPROM);
  Wire.write((byte)memoryAddress); 
  Wire.endTransmission(false); 
  Wire.requestFrom(ADDR_EEPROM, 1);
  if (Wire.available()) {
    data = Wire.read();
  }
  return data;
}

void readEEPROMToBuffer() {
  
  // Visualisasi: LED berkedip cepat saat membaca
  for (unsigned int addr = 0; addr < EEPROM_SIZE; addr++) {
    dataBuffer[addr] = readEEPROM_HW(addr);
    // Kedipan sebagai indikator progress
    if (addr % 16 == 0) { 
      digitalWrite(PIN_LED, !digitalRead(PIN_LED));
    }
  }
  
  digitalWrite(PIN_LED, LOW);
  currentState = DATA_READ_SUCCESS;
}


// =========================================================
// III. FUNGSI I2C SOFTWARE (EEPROM 2 - D4/D5)
// =========================================================

void writeEEPROM_SW(unsigned int memoryAddress, byte data) {
  WireAlt.beginTransmission(ADDR_EEPROM); 
  WireAlt.write((byte)memoryAddress); 
  WireAlt.write(data);
  WireAlt.endTransmission(); 
  
  delay(5); 
}

void writeBufferToEEPROM() {

  // Visualisasi: LED menyala konstan saat menulis
  digitalWrite(PIN_LED, HIGH);

  for (unsigned int addr = 0; addr < EEPROM_SIZE; addr++) {
    writeEEPROM_SW(addr, dataBuffer[addr]);
  }
  
  digitalWrite(PIN_LED, LOW);
  currentState = COPY_COMPLETE;
}


// =========================================================
// IV. SETUP DAN LOOP
// =========================================================

void setup() {
  
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_BUTTON, INPUT_PULLUP); 

  // Inisialisasi kedua bus I2C
  Wire.begin();       // Hardware I2C (A4/A5)
  WireAlt.begin();    // Software I2C (D4/D5)
  
  // Indikasi sistem siap
  digitalWrite(PIN_LED, HIGH);
  delay(200);
  digitalWrite(PIN_LED, LOW);
}

void loop() {
  if (digitalRead(PIN_BUTTON) == LOW) {
    // Debounce
    delay(50); 
    if (digitalRead(PIN_BUTTON) == LOW) {
      // Tunggu tombol dilepas
      while(digitalRead(PIN_BUTTON) == LOW); 
      
      
      switch (currentState) {
        case WAIT_FOR_READ:
          // Tombol Tekan 1: READ
          readEEPROMToBuffer();
          // Indikasi siap untuk langkah berikutnya: LED kedip 2x
          blinkReady(); 
          break;
          
        case DATA_READ_SUCCESS:
          // Tombol Tekan 2: WRITE
          writeBufferToEEPROM();
          blinkSuccess(); // Indikasi sukses total
          currentState = WAIT_FOR_READ;
          break;
          
        case COPY_COMPLETE:
          // Tekan 3 atau lebih: Reset
          blinkReset(); // Indikasi reset
          currentState = WAIT_FOR_READ;
          break;
      }
    }
  }
}

// =========================================================
// V. FUNGSI NOTIFIKASI LED
// =========================================================

// Indikasi sukses total (5 kedipan lambat)
void blinkSuccess() {
  for (int i = 0; i < 5; i++) {
    digitalWrite(PIN_LED, HIGH);
    delay(150);
    digitalWrite(PIN_LED, LOW);
    delay(150);
  }
}

// Indikasi siap/ready (2 kedipan sedang)
void blinkReady() {
  for (int i = 0; i < 2; i++) {
    digitalWrite(PIN_LED, HIGH);
    delay(100);
    digitalWrite(PIN_LED, LOW);
    delay(100);
  }
}

// Indikasi reset (3 kedipan cepat)
void blinkReset() {
  for (int i = 0; i < 3; i++) {
    digitalWrite(PIN_LED, HIGH);
    delay(50);
    digitalWrite(PIN_LED, LOW);
    delay(50);
  }
}