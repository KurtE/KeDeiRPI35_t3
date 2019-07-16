#include <KeDeiRPI35_t3.h>
#define TFT_CS 10
#define TOUCH_CS 9
KEDEIRPI35_t3 tft = KEDEIRPI35_t3(&SPI, TFT_CS, TOUCH_CS);

float p = 3.1415926;


void setup(void) {
  while (!Serial && millis() < 4000) ;
  Serial.begin(9600);
  Serial.print("hello!");
  tft.begin();
}

uint8_t rotation = 0;

void drawTestScreen() {
  tft.fillScreen(KEDEIRPI35_RED);
  tft.fillRect(tft.width() / 2 - 32, 20, 64, tft.height() - 40, KEDEIRPI35_GREEN);
  tft.fillRect(tft.width() / 4 - 16, tft.height() / 4 - 16, 32, 32, KEDEIRPI35_BLUE);
  tft.fillRect(tft.width() * 3 / 4 - 16, tft.height() * 3 / 4 - 16, 32, 32, KEDEIRPI35_WHITE);
  tft.fillRect(tft.width() * 3 / 4 - 16, tft.height() / 4 - 16, 32, 32, KEDEIRPI35_BLACK);
  tft.fillRect(0, 0, 8, 8, KEDEIRPI35_BLACK);
  tft.fillRect(tft.width() - 8, tft.height() - 8, 8, 8, KEDEIRPI35_WHITE);
  tft.drawPixel(0, 0, KEDEIRPI35_YELLOW);
  tft.drawPixel(1, 1, KEDEIRPI35_RED);
  tft.drawPixel(2, 2, KEDEIRPI35_BLUE);
  tft.setCursor(0, tft.height() / 2 - 4);
  tft.printf("R:%d W:%d H:%d", rotation, tft.width(), tft.height());
  tft.drawPixel(tft.width() - 1, tft.height() - 1, KEDEIRPI35_YELLOW);
  tft.drawPixel(tft.width() - 2, tft.height() - 2, KEDEIRPI35_RED);
  tft.drawPixel(tft.width() - 3, tft.height() - 3, KEDEIRPI35_BLUE);
}

void playWithMADCTL() {
  Serial.println("Try changing one bit of MADCTL to see if it fixes problem");
  Serial.printf("Current value for Rotation %d = %x\n", rotation,
                KEDEIRPI35_t3::MADCTLRotionValues[rotation]);
  for (uint8_t mask = 0x01; mask != 0; mask <<= 1) {
    uint8_t madctl_value = KEDEIRPI35_t3::MADCTLRotionValues[rotation] ^ mask;
    Serial.printf("Try madctl: %x\n", madctl_value);
    tft.sendCommand(KEDEIRPI35_MADCTL, &madctl_value, 1);
    drawTestScreen();
    Serial.println("Hit any key to continue");
    int ch; 
    while ((ch = Serial.read()) == -1) ;
    while (Serial.read() != -1) ;
    if (ch >= ' ') break;
  }

}

void loop() {
  tft.setRotation(rotation);
  Serial.printf("Set Rotation: %d (%x) width: %d height: %d\n",
                rotation, KEDEIRPI35_t3::MADCTLRotionValues[rotation], tft.width(), tft.height());
  elapsedMillis timer;
  drawTestScreen();
  // large block of text
  //delay(2500);
  Serial.println("Hit any key to continue");
  uint8_t loffset = 0;
  for (;;) {
    while (!Serial.available()) ;
    char ch = Serial.read();
    while (Serial.read() != -1) ;
    if (ch == '.') {
      tft.drawRect(loffset, loffset, tft.width() - 2 * loffset, tft.height() - 2 * loffset, KEDEIRPI35_GREEN);
      loffset++;
    } else if (ch == 'm') {
      playWithMADCTL();
    } else {
      break;
    }
  }
  rotation = (rotation + 1) & 0x3;
}
