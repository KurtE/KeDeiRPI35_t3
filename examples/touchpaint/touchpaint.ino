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

// Updated by KurtE
// This version completely hacked for use with the KeDei RPI display
// that uses XPT2046 and the like. 


#include <SPI.h>
#include <Wire.h>      // this is needed even tho we aren't using it
#include <KeDeiRPI35_t3.h>
#include <XPT2046_Touchscreen.h>

// This is calibration data for the raw touch data to the screen coordinates
#define TS_MINX 175
#define TS_MAXX 3900

#define TS_MINY 250
#define TS_MAXY 3800

// The STMPE610 uses hardware SPI on the shield, and #8
#define STMPE_CS 9
XPT2046_Touchscreen ts = XPT2046_Touchscreen(STMPE_CS);

// The display also uses hardware SPI, plus #9 & #10
#define TFT_CS 10
KEDEIRPI35_t3 tft = KEDEIRPI35_t3(&SPI, TFT_CS, STMPE_CS);

uint8_t debug_touch = 0; 

// Size of the color selection boxes and the paintbrush size
#define BOXSIZE 40
#define PENRADIUS 3

// Define which colors we wish to use
uint16_t colors[] = {
    KEDEIRPI35_RED, KEDEIRPI35_YELLOW, KEDEIRPI35_GREEN, KEDEIRPI35_CYAN, 
    KEDEIRPI35_BLUE, KEDEIRPI35_MAGENTA, KEDEIRPI35_PINK, KEDEIRPI35_ORANGE, 
    KEDEIRPI35_WHITE, KEDEIRPI35_LIGHTGREY, KEDEIRPI35_DARKGREY, KEDEIRPI35_BLACK};
char * color_names[] = {
    "RED", "YELLOW", "GREEN", "CYAN", 
    "BLUE", "MAGENTA", "PINK","ORANGE", 
    "WHITE", "LIGHTGREY", "DARKGREY", "BLACK"}; 
#define CNT_COLORS (sizeof(colors) / sizeof(colors[0]))

uint8_t  currentcolor_index;
uint16_t currentcolor;

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
  Serial.println("Entering anything in Serial Monitor will toggle touch screen debug output");
  
  tft.fillScreen(KEDEIRPI35_BLACK);
  
  // make the color selection boxes
  uint16_t x = 0;
  for (uint8_t i = 0; i < CNT_COLORS; i++) {
    tft.fillRect(x, 0, BOXSIZE, BOXSIZE, colors[i]);
    x += BOXSIZE;
  }

  // select the current color 'red'
  tft.drawRect(0, 0, BOXSIZE, BOXSIZE, KEDEIRPI35_WHITE);
  currentcolor = colors[0];
  currentcolor_index = 0;
}


void loop()
{
  // See if the user entered anything
  if (Serial.available()) {
    while (Serial.read() != -1) ; // remove all data.
    if (debug_touch) {
      debug_touch = 0;
      Serial.println("*** Debug turned off ***");
    } else {
      debug_touch = 1;
      Serial.println("### Debug turned on ###");      
    }

  }
  // See if there's any  touch data for us
  if (ts.bufferEmpty()) {
    return;
  }

  // Retrieve a point  
  TS_Point p = ts.getPoint();
  
  if (debug_touch) {
    Serial.print("X = "); Serial.print(p.x);
    Serial.print("\tY = "); Serial.print(p.y);
    Serial.print("\tPressure = "); Serial.print(p.z);  

  }
 
  // Scale from ~0->4000 to tft.width using the calibration #'s
  p.x = map(p.x, TS_MAXX, TS_MINX, 0, tft.width());  // X's reversed
  p.y = map(p.y, TS_MINY, TS_MAXY, 0, tft.height());

  if (debug_touch) {
    Serial.print(" ("); Serial.print(p.x);
    Serial.print(", "); Serial.print(p.y);
    Serial.println(")");
  } 
  
  if (p.y < BOXSIZE) {
    uint8_t newcolor_index = p.x / BOXSIZE;
    if ((newcolor_index != currentcolor_index) && (newcolor_index < CNT_COLORS)) {
      Serial.print("New color: ");
      Serial.println(color_names[newcolor_index]);

      // redraw the old color box full
      tft.fillRect(currentcolor_index*BOXSIZE, 0, BOXSIZE, BOXSIZE, colors[currentcolor_index]);

      // Put outline around new color handle white.
      currentcolor_index = newcolor_index;
      currentcolor = colors[currentcolor_index];
      tft.drawRect(currentcolor_index*BOXSIZE, 0, BOXSIZE, BOXSIZE, (currentcolor != KEDEIRPI35_WHITE)? KEDEIRPI35_WHITE : KEDEIRPI35_RED );

    }
  }
  if (((p.y-PENRADIUS) > BOXSIZE) && ((p.y+PENRADIUS) < tft.height())) {
    tft.fillCircle(p.x, p.y, PENRADIUS, currentcolor);
  }
}
