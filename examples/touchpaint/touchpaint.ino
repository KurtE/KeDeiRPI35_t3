/***************************************************
  This is our touchscreen painting example for the Adafruit ILI9341 Shield
  ----> http://www.adafruit.com/products/1651

  Check out the links above for our tutorials and wiring diagrams
  These displays use SPI to communicate, 4 or 5 pins are required to
  interface (RST is optional)
  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.
  MIT license, all text above must be included in any redistribution
 ****************************************************/


#include <SPI.h>
#include <Wire.h>      // this is needed even tho we aren't using it
#include <KeDeiRPI35_t3.h>
#include <XPT2046_Touchscreen.h>

// This is calibration data for the raw touch data to the screen coordinates
#define TS_MINX 150
#define TS_MINY 130
#define TS_MAXX 3800
#define TS_MAXY 4000

// The STMPE610 uses hardware SPI on the shield, and #8
#define STMPE_CS 9
XPT2046_Touchscreen ts = XPT2046_Touchscreen(STMPE_CS);

// The display also uses hardware SPI, plus #9 & #10
#define TFT_CS 10
KEDEIRPI35_t3 tft = KEDEIRPI35_t3(&SPI, TFT_CS, STMPE_CS);

// Size of the color selection boxes and the paintbrush size
#define BOXSIZE 40
#define PENRADIUS 3
int oldcolor, currentcolor;

void setup(void) {
 
  while(!Serial && millis() < 2000) ;
  Serial.begin(9600);
  Serial.println(F("Touch Paint!"));
  
  pinMode(STMPE_CS, OUTPUT);
  digitalWrite(STMPE_CS, HIGH);
  pinMode(TFT_CS, OUTPUT);
  digitalWrite(TFT_CS, HIGH);

  // Start touch first 
  if (!ts.begin()) {
    Serial.println("Couldn't start touchscreen controller");
    while (1);
  }

  tft.begin();

  Serial.println("Touchscreen started");
  
  tft.fillScreen(KEDEIRPI35_BLACK);
  
  // make the color selection boxes
  tft.fillRect(0, 0, BOXSIZE, BOXSIZE, KEDEIRPI35_RED);
  tft.fillRect(BOXSIZE, 0, BOXSIZE, BOXSIZE, KEDEIRPI35_YELLOW);
  tft.fillRect(BOXSIZE*2, 0, BOXSIZE, BOXSIZE, KEDEIRPI35_GREEN);
  tft.fillRect(BOXSIZE*3, 0, BOXSIZE, BOXSIZE, KEDEIRPI35_CYAN);
  tft.fillRect(BOXSIZE*4, 0, BOXSIZE, BOXSIZE, KEDEIRPI35_BLUE);
  tft.fillRect(BOXSIZE*5, 0, BOXSIZE, BOXSIZE, KEDEIRPI35_MAGENTA);
 
  // select the current color 'red'
  tft.drawRect(0, 0, BOXSIZE, BOXSIZE, KEDEIRPI35_WHITE);
  currentcolor = KEDEIRPI35_RED;
}


void loop()
{
  // See if there's any  touch data for us
  if (ts.bufferEmpty()) {
    return;
  }
  /*
  // You can also wait for a touch
  if (! ts.touched()) {
    return;
  }
  */

  // Retrieve a point  
  TS_Point p = ts.getPoint();
  
 /*
  Serial.print("X = "); Serial.print(p.x);
  Serial.print("\tY = "); Serial.print(p.y);
  Serial.print("\tPressure = "); Serial.println(p.z);  
 */
 
  // Scale from ~0->4000 to tft.width using the calibration #'s
  p.x = map(p.x, TS_MINX, TS_MAXX, 0, tft.width());
  p.y = map(p.y, TS_MINY, TS_MAXY, 0, tft.height());

  /*
  Serial.print("("); Serial.print(p.x);
  Serial.print(", "); Serial.print(p.y);
  Serial.println(")");
  */

  if (p.y < BOXSIZE) {
     oldcolor = currentcolor;

     if (p.x < BOXSIZE) { 
       currentcolor = KEDEIRPI35_RED; 
       tft.drawRect(0, 0, BOXSIZE, BOXSIZE, KEDEIRPI35_WHITE);
     } else if (p.x < BOXSIZE*2) {
       currentcolor = KEDEIRPI35_YELLOW;
       tft.drawRect(BOXSIZE, 0, BOXSIZE, BOXSIZE, KEDEIRPI35_WHITE);
     } else if (p.x < BOXSIZE*3) {
       currentcolor = KEDEIRPI35_GREEN;
       tft.drawRect(BOXSIZE*2, 0, BOXSIZE, BOXSIZE, KEDEIRPI35_WHITE);
     } else if (p.x < BOXSIZE*4) {
       currentcolor = KEDEIRPI35_CYAN;
       tft.drawRect(BOXSIZE*3, 0, BOXSIZE, BOXSIZE, KEDEIRPI35_WHITE);
     } else if (p.x < BOXSIZE*5) {
       currentcolor = KEDEIRPI35_BLUE;
       tft.drawRect(BOXSIZE*4, 0, BOXSIZE, BOXSIZE, KEDEIRPI35_WHITE);
     } else if (p.x < BOXSIZE*6) {
       currentcolor = KEDEIRPI35_MAGENTA;
       tft.drawRect(BOXSIZE*5, 0, BOXSIZE, BOXSIZE, KEDEIRPI35_WHITE);
     }

     if (oldcolor != currentcolor) {
        if (oldcolor == KEDEIRPI35_RED) 
          tft.fillRect(0, 0, BOXSIZE, BOXSIZE, KEDEIRPI35_RED);
        if (oldcolor == KEDEIRPI35_YELLOW) 
          tft.fillRect(BOXSIZE, 0, BOXSIZE, BOXSIZE, KEDEIRPI35_YELLOW);
        if (oldcolor == KEDEIRPI35_GREEN) 
          tft.fillRect(BOXSIZE*2, 0, BOXSIZE, BOXSIZE, KEDEIRPI35_GREEN);
        if (oldcolor == KEDEIRPI35_CYAN) 
          tft.fillRect(BOXSIZE*3, 0, BOXSIZE, BOXSIZE, KEDEIRPI35_CYAN);
        if (oldcolor == KEDEIRPI35_BLUE) 
          tft.fillRect(BOXSIZE*4, 0, BOXSIZE, BOXSIZE, KEDEIRPI35_BLUE);
        if (oldcolor == KEDEIRPI35_MAGENTA) 
          tft.fillRect(BOXSIZE*5, 0, BOXSIZE, BOXSIZE, KEDEIRPI35_MAGENTA);
     }
  }
  if (((p.y-PENRADIUS) > BOXSIZE) && ((p.y+PENRADIUS) < tft.height())) {
    tft.fillCircle(p.x, p.y, PENRADIUS, currentcolor);
  }
}
