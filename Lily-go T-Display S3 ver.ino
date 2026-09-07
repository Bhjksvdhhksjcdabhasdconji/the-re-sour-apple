#include <NimBLEDevice.h>
#include <TFT_eSPI.h>

// Pin Definitions
#define PIN_LCD_POWER_ON 15
#define PIN_LCD_BL       38
#define BUTTON_PIN       0   // Top onboard button (Boot button)

TFT_eSPI tft = TFT_eSPI();
NimBLEAdvertising *pAdvertising;

uint32_t packetCount = 0;
bool isBroadcasting = true;
bool lastButtonState = HIGH;

// Timer Tracking Variables
unsigned long startTime = 0;
unsigned long lastTimerUpdate = 0;

NimBLEAdvertisementData getOAdvertisementData() {
  NimBLEAdvertisementData randomAdvertisementData;
  uint8_t packet[17];
  uint8_t i = 0;

  packet[i++] = 16;                                 
  packet[i++] = 0xFF;                               
  packet[i++] = 0x4C;                               
  packet[i++] = 0x00;                               
  packet[i++] = 0x0F;                               
  packet[i++] = 0x05;                               
  packet[i++] = 0xC1;                               
  
  const uint8_t types[] = { 0x27, 0x09, 0x02, 0x1e, 0x2b, 0x2d, 0x2f, 0x01, 0x06, 0x20, 0xc0 };
  packet[i++] = types[rand() % sizeof(types)];       
  
  esp_fill_random(&packet[i], 3);                  
  i += 3;   
  
  packet[i++] = 0x00;                               
  packet[i++] = 0x00;                               
  packet[i++] = 0x10;                               
  
  esp_fill_random(&packet[i], 3);

  randomAdvertisementData.addData(packet, 17);
  return randomAdvertisementData;
}

void updateStatusDisplay() {
  tft.setTextSize(2);
  if (isBroadcasting) {
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.drawString("Status: BROADCASTING", 10, 45);
  } else {
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.drawString("Status: PAUSED      ", 10, 45);
  }
}

void updateTimerDisplay() {
  unsigned long elapsedSeconds = (millis() - startTime) / 1000;
  
  uint32_t hours = elapsedSeconds / 3600;
  uint32_t minutes = (elapsedSeconds % 3600) / 60;
  uint32_t seconds = elapsedSeconds % 60;

  char timeBuffer[12];
  snprintf(timeBuffer, sizeof(timeBuffer), "%02u:%02u:%02u", hours, minutes, seconds);

  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setTextSize(2);
  tft.drawString("Time: " + String(timeBuffer), 10, 70);
}

void setup() {
  // Setup button with internal pullup
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // Power on T-Display-S3 LCD
  pinMode(PIN_LCD_POWER_ON, OUTPUT);
  digitalWrite(PIN_LCD_POWER_ON, HIGH);

  pinMode(PIN_LCD_BL, OUTPUT);
  digitalWrite(PIN_LCD_BL, HIGH);

  delay(100); 

  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  
  // Header UI with Version Tag
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setTextSize(2);
  tft.drawString("SOUR APPLE BLE", 10, 10);
  
  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  tft.setTextSize(1);
  tft.drawString("v1.3.0", 260, 15);

  tft.drawFastHLine(0, 32, 320, TFT_WHITE);

  updateStatusDisplay();
  
  startTime = millis();
  updateTimerDisplay();

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.drawString("Power: +9 dBm", 10, 95);
  tft.drawString("Packets Sent:", 10, 120);

  // Initialize NimBLE
  NimBLEDevice::init("");
  NimBLEDevice::setPower(9); 

  NimBLEServer *pServer = NimBLEDevice::createServer();
  pAdvertising = pServer->getAdvertising();
}

void loop() {
  // 1. Update Timer
  if (millis() - lastTimerUpdate >= 1000) {
    lastTimerUpdate = millis();
    updateTimerDisplay();
  }

  // 2. Check Button Press (Toggle Pause/Resume)
  bool currentButtonState = digitalRead(BUTTON_PIN);
  if (lastButtonState == HIGH && currentButtonState == LOW) {
    isBroadcasting = !isBroadcasting;
    
    if (!isBroadcasting) {
      pAdvertising->stop();
    }
    
    updateStatusDisplay();
    delay(200); 
  }
  lastButtonState = currentButtonState;

  // 3. Broadcast Loop
  if (isBroadcasting) {
    pAdvertising->stop();
    
    NimBLEAdvertisementData advertisementData = getOAdvertisementData();
    pAdvertising->setAdvertisementData(advertisementData);
    
    pAdvertising->start();
    delay(20);
    pAdvertising->stop();

    packetCount++;
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.setTextSize(3);
    tft.drawString(String(packetCount), 10, 142);
  }

  delay(40);
}
