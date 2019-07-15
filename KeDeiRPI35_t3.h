// https://github.com/PaulStoffregen/KEDEIRPI35_t3
// http://forum.pjrc.com/threads/26305-Highly-optimized-ILI9488-(320x240-TFT-color-display)-library

/***************************************************
  This is our library for the Adafruit  ILI9488 Breakout and Shield
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

// <SoftEgg>

//Additional graphics routines by Tim Trzepacz, SoftEgg LLC added December 2015
//(And then accidentally deleted and rewritten March 2016. Oops!)
//Gradient support
//----------------
//		fillRectVGradient	- fills area with vertical gradient
//		fillRectHGradient	- fills area with horizontal gradient
//		fillScreenVGradient - fills screen with vertical gradient
// 	fillScreenHGradient - fills screen with horizontal gradient

//Additional Color Support
//------------------------
//		color565toRGB		- converts 565 format 16 bit color to RGB
//		color565toRGB14		- converts 16 bit 565 format color to 14 bit RGB (2 bits clear for math and sign)
//		RGB14tocolor565		- converts 14 bit RGB back to 16 bit 565 format color

//Low Memory Bitmap Support
//-------------------------
// writeRect8BPP - 	write 8 bit per pixel paletted bitmap
// writeRect4BPP - 	write 4 bit per pixel paletted bitmap
// writeRect2BPP - 	write 2 bit per pixel paletted bitmap
// writeRect1BPP - 	write 1 bit per pixel paletted bitmap

//String Pixel Length support
//---------------------------
//		strPixelLen			- gets pixel length of given ASCII string

// <\SoftEgg>

#ifndef _KEDEIRPI35_t3H_
#define _KEDEIRPI35_t3H_

#ifdef __cplusplus
#include "Arduino.h"
#endif

#if defined(__AVR__)
#error "Sorry, KEDEIRPI35_t3 does not work with Teensy 2.0 or Teensy++ 2.0.  Use Adafruit_ILI9488."
#endif

// Allow us to enable or disable capabilities, particully Frame Buffer and Clipping for speed and size
#ifndef DISABLE_KEDEIRPI35_FRAMEBUFFER
#if defined(__MK64FX512__) || defined(__MK66FX1M0__)
#define ENABLE_KEDEIRPI35_FRAMEBUFFER
#elif defined(__IMXRT1052__) || defined(__IMXRT1062__)
#define ENABLE_KEDEIRPI35_FRAMEBUFFER
#endif
#endif

#define KEDEIRPI35_TFTWIDTH  320
#define KEDEIRPI35_TFTHEIGHT 480

#define KEDEIRPI35_NOP     0x00
#define KEDEIRPI35_SWRESET 0x01
#define KEDEIRPI35_RDDID   0x04
#define KEDEIRPI35_RDDST   0x09

#define KEDEIRPI35_SLPIN   0x10
#define KEDEIRPI35_SLPOUT  0x11
#define KEDEIRPI35_PTLON   0x12
#define KEDEIRPI35_NORON   0x13

#define KEDEIRPI35_RDMODE  0x0A
#define KEDEIRPI35_RDMADCTL  0x0B
#define KEDEIRPI35_RDPIXFMT  0x0C
#define KEDEIRPI35_RDIMGFMT  0x0D
#define KEDEIRPI35_RDSELFDIAG  0x0F

#define KEDEIRPI35_INVOFF  0x20
#define KEDEIRPI35_INVON   0x21
#define KEDEIRPI35_GAMMASET 0x26
#define KEDEIRPI35_DISPOFF 0x28
#define KEDEIRPI35_DISPON  0x29

#define KEDEIRPI35_CASET   0x2A   //Column Address Set 
#define KEDEIRPI35_PASET   0x2B   //Page Address Set 
#define KEDEIRPI35_RAMWR   0x2C   //Memory Write 
#define KEDEIRPI35_RAMRD   0x2E   //Memory Read

#define KEDEIRPI35_PTLAR    0x30
#define KEDEIRPI35_MADCTL   0x36
#define KEDEIRPI35_VSCRSADD 0x37
#define KEDEIRPI35_PIXFMT   0x3A

#define KEDEIRPI35_FRMCTR1 0xB1
#define KEDEIRPI35_FRMCTR2 0xB2
#define KEDEIRPI35_FRMCTR3 0xB3
#define KEDEIRPI35_INVCTR  0xB4
#define KEDEIRPI35_DFUNCTR 0xB6

#define KEDEIRPI35_PWCTR1  0xC0
#define KEDEIRPI35_PWCTR2  0xC1
#define KEDEIRPI35_PWCTR3  0xC2
#define KEDEIRPI35_PWCTR4  0xC3
#define KEDEIRPI35_PWCTR5  0xC4
#define KEDEIRPI35_VMCTR1  0xC5
#define KEDEIRPI35_VMCTR2  0xC7

#define KEDEIRPI35_RDID1   0xDA
#define KEDEIRPI35_RDID2   0xDB
#define KEDEIRPI35_RDID3   0xDC
#define KEDEIRPI35_RDID4   0xDD

#define KEDEIRPI35_GMCTRP1 0xE0
#define KEDEIRPI35_GMCTRN1 0xE1

// define aliases for commonly used commands. 
#define SET_COLUMN_ADDRESS_WINDOW  0x2A
#define SET_ROW_ADDRESS_WINDOW     0x2B
#define BEGIN_PIXEL_DATA           0x2C
#define BEGIN_READ_DATA            0x2E
#define READ_MEMORY_CONTINUE       0x3E
/*
#define KEDEIRPI35_PWCTR6  0xFC

*/

// Color definitions
#ifdef RGB_COLORS
#define KEDEIRPI35_BLACK       0x0000      /*   0,   0,   0 */
#define KEDEIRPI35_NAVY        0x000F      /*   0,   0, 128 */
#define KEDEIRPI35_DARKGREEN   0x03E0      /*   0, 128,   0 */
#define KEDEIRPI35_DARKCYAN    0x03EF      /*   0, 128, 128 */
#define KEDEIRPI35_MAROON      0x7800      /* 128,   0,   0 */
#define KEDEIRPI35_PURPLE      0x780F      /* 128,   0, 128 */
#define KEDEIRPI35_OLIVE       0x7BE0      /* 128, 128,   0 */
#define KEDEIRPI35_LIGHTGREY   0xC618      /* 192, 192, 192 */
#define KEDEIRPI35_DARKGREY    0x7BEF      /* 128, 128, 128 */
#define KEDEIRPI35_BLUE        0x001F      /*   0,   0, 255 */
#define KEDEIRPI35_GREEN       0x07E0      /*   0, 255,   0 */
#define KEDEIRPI35_CYAN        0x07FF      /*   0, 255, 255 */
#define KEDEIRPI35_RED         0xF800      /* 255,   0,   0 */
#define KEDEIRPI35_MAGENTA     0xF81F      /* 255,   0, 255 */
#define KEDEIRPI35_YELLOW      0xFFE0      /* 255, 255,   0 */
#define KEDEIRPI35_WHITE       0xFFFF      /* 255, 255, 255 */
#define KEDEIRPI35_ORANGE      0xFD20      /* 255, 165,   0 */
#define KEDEIRPI35_GREENYELLOW 0xAFE5      /* 173, 255,  47 */
#define KEDEIRPI35_PINK        0xF81F
#define CL(_r,_g,_b) ((((_r)&0xF8)<<8)|(((_g)&0xFC)<<3)|((_b)>>3))
#else
// Looks like we are BGR format
#define KEDEIRPI35_BLACK       0x0000    /*   0,   0,   0, RGB:0x0000 */
#define KEDEIRPI35_NAVY        0x7800    /*   0,   0, 120, RGB:0x000f */
#define KEDEIRPI35_DARKGREEN   0x03e0    /*   0, 124,   0, RGB:0x03e0 */
#define KEDEIRPI35_DARKCYAN    0x7be0    /*   0, 124, 120, RGB:0x03ef */
#define KEDEIRPI35_MAROON      0x000f    /* 120,   0,   0, RGB:0x7800 */
#define KEDEIRPI35_PURPLE      0x780f    /* 120,   0, 120, RGB:0x780f */
#define KEDEIRPI35_OLIVE       0x03ef    /* 120, 124,   0, RGB:0x7be0 */
#define KEDEIRPI35_LIGHTGREY   0xc618    /* 192, 192, 192, RGB:0xc618 */
#define KEDEIRPI35_DARKGREY    0x7bef    /* 120, 124, 120, RGB:0x7bef */
#define KEDEIRPI35_BLUE        0xf800    /*   0,   0, 248, RGB:0x001f */
#define KEDEIRPI35_GREEN       0x07e0    /*   0, 252,   0, RGB:0x07e0 */
#define KEDEIRPI35_CYAN        0xffe0    /*   0, 252, 248, RGB:0x07ff */
#define KEDEIRPI35_RED         0x001f    /* 248,   0,   0, RGB:0xf800 */
#define KEDEIRPI35_MAGENTA     0xf81f    /* 248,   0, 248, RGB:0xf81f */
#define KEDEIRPI35_YELLOW      0x07ff    /* 248, 252,   0, RGB:0xffe0 */
#define KEDEIRPI35_WHITE       0xffff    /* 248, 252, 248, RGB:0xffff */
#define KEDEIRPI35_ORANGE      0x053f    /* 248, 164,   0, RGB:0xfd20 */
#define KEDEIRPI35_GREENYELLOW 0x2ff5    /* 168, 252,  40, RGB:0xafe5 */
#define KEDEIRPI35_PINK        0xf81f    /* 248,   0, 248, RGB:0xf81f */
#define CL(_r,_g,_b) ((((_b)&0xF8)<<8)|(((_g)&0xFC)<<3)|((_r)>>3))
#endif


#define sint16_t int16_t


// Documentation on the KEDEIRPI35_t3 font data format:
// https://forum.pjrc.com/threads/54316-KEDEIRPI35_t-font-structure-format

typedef struct {
	const unsigned char *index;
	const unsigned char *unicode;
	const unsigned char *data;
	unsigned char version;
	unsigned char reserved;
	unsigned char index1_first;
	unsigned char index1_last;
	unsigned char index2_first;
	unsigned char index2_last;
	unsigned char bits_index;
	unsigned char bits_width;
	unsigned char bits_height;
	unsigned char bits_xoffset;
	unsigned char bits_yoffset;
	unsigned char bits_delta;
	unsigned char line_space;
	unsigned char cap_height;
} KEDEIRPI35_t3_font_t;


//These enumerate the text plotting alignment (reference datum point)
#define TL_DATUM 0 // Top left (default)
#define TC_DATUM 1 // Top centre
#define TR_DATUM 2 // Top right
#define ML_DATUM 3 // Middle left
#define CL_DATUM 3 // Centre left, same as above
#define MC_DATUM 4 // Middle centre
#define CC_DATUM 4 // Centre centre, same as above
#define MR_DATUM 5 // Middle right
#define CR_DATUM 5 // Centre right, same as above
#define BL_DATUM 6 // Bottom left
#define BC_DATUM 7 // Bottom centre
#define BR_DATUM 8 // Bottom right
//#define L_BASELINE  9 // Left character baseline (Line the 'A' character would sit on)
//#define C_BASELINE 10 // Centre character baseline
//#define R_BASELINE 11 // Right character baseline

#ifdef __cplusplus
// At all other speeds, ILI9241_KINETISK__pspi->beginTransaction() will use the fastest available clock
#include <SPI.h>
#if defined(__MK64FX512__) || defined(__MK66FX1M0__)
#if F_BUS >= 64000000
#define KEDEIRPI35_SPICLOCK 64000000
#define KEDEIRPI35_SPICLOCK_READ 4000000
#else
#define KEDEIRPI35_SPICLOCK 30000000
#define KEDEIRPI35_SPICLOCK_READ 2000000
#endif
#elif defined(__IMXRT1052__) || defined(__IMXRT1062__)  // Teensy 4.x
#define KEDEIRPI35_SPICLOCK 30000000
//#define KEDEIRPI35_SPICLOCK 72000000
#define KEDEIRPI35_SPICLOCK_READ 4000000
#else
#define KEDEIRPI35_SPICLOCK 30000000
#define KEDEIRPI35_SPICLOCK_READ 2000000
#endif

class KEDEIRPI35_t3 : public Print
{
  public:
	KEDEIRPI35_t3(SPIClass *SPIWire, uint8_t _CS, uint8_t _TOUCHCS, uint8_t _MOSI=11, uint8_t _SCLK=13, uint8_t _MISO=12);
	void begin(void);
  	void sleep(bool enable);		
	void pushColor(uint16_t color);
	void fillScreen(uint16_t color);
	void drawPixel(int16_t x, int16_t y, uint16_t color);
	void drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color);
	void drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color);
	void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
			
	void fillRectHGradient(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color1, uint16_t color2);
	void fillRectVGradient(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color1, uint16_t color2);
	void fillScreenVGradient(uint16_t color1, uint16_t color2);
	void fillScreenHGradient(uint16_t color1, uint16_t color2);

	void setRotation(uint8_t r);
	void setScroll(uint16_t offset);
	void invertDisplay(boolean i);
	void setAddrWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
	// Pass 8-bit (each) R,G,B, get back 16-bit packed color
#ifdef RGB_COLORS
	static uint16_t color565(uint8_t r, uint8_t g, uint8_t b) {
		return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
	}

	//color565toRGB		- converts 565 format 16 bit color to RGB
	static void color565toRGB(uint16_t color, uint8_t &r, uint8_t &g, uint8_t &b) {
		r = (color>>8)&0x00F8;
		g = (color>>3)&0x00FC;
		b = (color<<3)&0x00F8;
	}
	
	//color565toRGB14		- converts 16 bit 565 format color to 14 bit RGB (2 bits clear for math and sign)
	//returns 00rrrrr000000000,00gggggg00000000,00bbbbb000000000
	//thus not overloading sign, and allowing up to double for additions for fixed point delta
	static void color565toRGB14(uint16_t color, int16_t &r, int16_t &g, int16_t &b) {
		r = (color>>2)&0x3E00;
		g = (color<<3)&0x3F00;
		b = (color<<9)&0x3E00;
	}
	
	//RGB14tocolor565		- converts 14 bit RGB back to 16 bit 565 format color
	static uint16_t RGB14tocolor565(int16_t r, int16_t g, int16_t b)
	{
		return (((r & 0x3E00) << 2) | ((g & 0x3F00) >>3) | ((b & 0x3E00) >> 9));
	}
#else
	static uint16_t color565(uint8_t r, uint8_t g, uint8_t b) {
		return ((b & 0xF8) << 8) | ((g & 0xFC) << 3) | (r >> 3);
	}

	//color565toRGB		- converts 565 format 16 bit color to RGB
	static void color565toRGB(uint16_t color, uint8_t &r, uint8_t &g, uint8_t &b) {
		b = (color>>8)&0x00F8;
		g = (color>>3)&0x00FC;
		r = (color<<3)&0x00F8;
	}
	
	//color565toRGB14		- converts 16 bit 565 format color to 14 bit RGB (2 bits clear for math and sign)
	//returns 00rrrrr000000000,00gggggg00000000,00bbbbb000000000
	//thus not overloading sign, and allowing up to double for additions for fixed point delta
	static void color565toRGB14(uint16_t color, int16_t &r, int16_t &g, int16_t &b) {
		b = (color>>2)&0x3E00;
		g = (color<<3)&0x3F00;
		r = (color<<9)&0x3E00;
	}
	
	//RGB14tocolor565		- converts 14 bit RGB back to 16 bit 565 format color
	static uint16_t RGB14tocolor565(int16_t r, int16_t g, int16_t b)
	{
		return (((b & 0x3E00) << 2) | ((g & 0x3F00) >>3) | ((r & 0x3E00) >> 9));
	}
#endif	
	//uint8_t readdata(void);
	uint8_t readcommand8(uint8_t reg, uint8_t index = 0);

	// Added functions to read pixel data...
	uint16_t readPixel(int16_t x, int16_t y);
	void readRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t *pcolors);
	void writeRect(int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t *pcolors);

	// writeRect8BPP - 	write 8 bit per pixel paletted bitmap
	//					bitmap data in array at pixels, one byte per pixel
	//					color palette data in array at palette
	void writeRect8BPP(int16_t x, int16_t y, int16_t w, int16_t h, const uint8_t *pixels, const uint16_t * palette );

	// writeRect4BPP - 	write 4 bit per pixel paletted bitmap
	//					bitmap data in array at pixels, 4 bits per pixel
	//					color palette data in array at palette
	//					width must be at least 2 pixels
	void writeRect4BPP(int16_t x, int16_t y, int16_t w, int16_t h, const uint8_t *pixels, const uint16_t * palette );
	
	// writeRect2BPP - 	write 2 bit per pixel paletted bitmap
	//					bitmap data in array at pixels, 4 bits per pixel
	//					color palette data in array at palette
	//					width must be at least 4 pixels
	void writeRect2BPP(int16_t x, int16_t y, int16_t w, int16_t h, const uint8_t *pixels, const uint16_t * palette );
	
	// writeRect1BPP - 	write 1 bit per pixel paletted bitmap
	//					bitmap data in array at pixels, 4 bits per pixel
	//					color palette data in array at palette
	//					width must be at least 8 pixels
	void writeRect1BPP(int16_t x, int16_t y, int16_t w, int16_t h, const uint8_t *pixels, const uint16_t * palette );

		// writeRectNBPP - 	write N(1, 2, 4, 8) bit per pixel paletted bitmap
	//					bitmap data in array at pixels
	//  Currently writeRect1BPP, writeRect2BPP, writeRect4BPP use this to do all of the work. 
	// 
	void writeRectNBPP(int16_t x, int16_t y, int16_t w, int16_t h,  uint8_t bits_per_pixel, 
		const uint8_t *pixels, const uint16_t * palette );
	
	// from Adafruit_GFX.h
	void drawCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color);
	void drawCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t cornername, uint16_t color);
	void fillCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color);
	void fillCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t cornername, int16_t delta, uint16_t color);
	void drawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color);
	void fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color);
	void drawRoundRect(int16_t x0, int16_t y0, int16_t w, int16_t h, int16_t radius, uint16_t color);
	void fillRoundRect(int16_t x0, int16_t y0, int16_t w, int16_t h, int16_t radius, uint16_t color);
	void drawBitmap(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, uint16_t color);
	void drawChar(int16_t x, int16_t y, unsigned char c, uint16_t color, uint16_t bg, uint8_t size);
	void setCursor(int16_t x, int16_t y);
    void getCursor(int16_t *x, int16_t *y);
	void setTextColor(uint16_t c);
	void setTextColor(uint16_t c, uint16_t bg);
	void setTextSize(uint8_t s);
	uint8_t getTextSize();
	void setTextWrap(boolean w);
	boolean getTextWrap();
	virtual size_t write(uint8_t);
	int16_t width(void)  { return _width; }
	int16_t height(void) { return _height; }
	uint8_t getRotation(void);
	void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
	void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
	int16_t getCursorX(void) const { return cursor_x; }
	int16_t getCursorY(void) const { return cursor_y; }
	void setFont(const KEDEIRPI35_t3_font_t &f) { font = &f; }
	void setFontAdafruit(void) { font = NULL; }
	void drawFontChar(unsigned int c);
	int16_t strPixelLen(char * str);
	void write16BitColor(uint16_t color, bool last_pixel=false);
	void write16BitColor(uint16_t color, uint16_t count, bool last_pixel);
	
	// setOrigin sets an offset in display pixels where drawing to (0,0) will appear
	// for example: setOrigin(10,10); drawPixel(5,5); will cause a pixel to be drawn at hardware pixel (15,15)
	void setOrigin(int16_t x = 0, int16_t y = 0) { 
		_originx = x; _originy = y; 
		//if (Serial) Serial.printf("Set Origin %d %d\n", x, y);
		updateDisplayClip();
	}
	void getOrigin(int16_t* x, int16_t* y) { *x = _originx; *y = _originy; }

	// setClipRect() sets a clipping rectangle (relative to any set origin) for drawing to be limited to.
	// Drawing is also restricted to the bounds of the display

	void setClipRect(int16_t x1, int16_t y1, int16_t w, int16_t h) 
		{ _clipx1 = x1; _clipy1 = y1; _clipx2 = x1+w; _clipy2 = y1+h; 
			//if (Serial) Serial.printf("Set clip Rect %d %d %d %d\n", x1, y1, w, h);
			updateDisplayClip();
		}
	void setClipRect() {
			 _clipx1 = 0; _clipy1 = 0; _clipx2 = _width; _clipy2 = _height; 
			//if (Serial) Serial.printf("clear clip Rect\n");
			 updateDisplayClip(); 
		}
	
	
	// added support for drawing strings/numbers/floats with centering
	// modified from tft_ili9488_ESP github library
	// Handle numbers
	int16_t  drawNumber(long long_num,int poX, int poY);
	int16_t  drawFloat(float floatNumber,int decimal,int poX, int poY);   
	// Handle char arrays
	int16_t drawString(const String& string, int poX, int poY);
	int16_t drawString1(char string[], int16_t len, int poX, int poY);

	void setTextDatum(uint8_t datum);
	
	// added support for scrolling text area
	// https://github.com/vitormhenrique/KEDEIRPI35_t3
	// Discussion regarding this optimized version:
    //http://forum.pjrc.com/threads/26305-Highly-optimized-ILI9488-%28320x240-TFT-color-display%29-library
	//	
	void setScrollTextArea(int16_t x, int16_t y, int16_t w, int16_t h);
	void setScrollBackgroundColor(uint16_t color);
	void enableScroll(void);
	void disableScroll(void);
	void scrollTextArea(uint8_t scrollSize);
	void resetScrollBackgroundColor(uint16_t color);
	
	// added support to use optional Frame buffer
	void	setFrameBuffer(uint8_t *frame_buffer);
#ifdef ENABLE_KEDEIRPI35_FRAMEBUFFER
	uint8_t *getFrameBuffer() {return _pfbtft;}
	uint16_t *getPallet() {return _pallet; }
	void	colorsArePalletIndex(boolean b) {_colors_are_index = b;}
	boolean	colorsArePalletIndex() {return _colors_are_index;}
	inline uint8_t mapColorToPalletIndex(uint16_t color) 
		{
			if (_pallet && _colors_are_index) return (uint8_t)color;
			return doActualConvertColorToIndex(color);		
		}
#else
	uint8_t *getFrameBuffer() {return nullptr;}
	uint16_t *getPallet() {return nullptr; }
	void	colorsArePalletIndex(boolean b) {;}
	boolean	colorsArePalletIndex() {return true;}
	inline uint16_t mapColorToPalletIndex(uint16_t color) { return color; }
#endif	
	uint8_t useFrameBuffer(boolean b);		// use the frame buffer?  First call will allocate
	void	freeFrameBuffer(void);			// explicit call to release the buffer
	void	updateScreen(void);				// call to say update the screen now. 

	// Support for user to set Pallet. 
	void	setPallet(uint16_t *pal, uint16_t count);	// <= 256
	
	// probably not called directly... 
	uint8_t doActualConvertColorToIndex(uint16_t color);  

	//=============================================================================
	// DMA - Async support
	//=============================================================================
	// We are not going to bother with doing ASYNC this display driver
	// As we have to muck with CS and Touch CS for every 3/4 byte transfer
	// We re leav
	// no DMA support yet
	bool updateScreenAsync(bool update_cont) {
		updateScreen();
		return true;
	}	// call to say update the screen optinoally turn into continuous mode. 

	void waitUpdateAsyncComplete(void) {};
	void endUpdateAsync() {};			 // Turn of the continueous mode fla
	void dumpDMASettings() {};

	uint32_t frameCount() {return 0; }
	boolean	asyncUpdateActive(void)  {return false;}

	//3D Rendering Engine
	// State variables for controlling masked and overdrawn rendering
	uint8_t  mask_flag = 0;
	uint8_t  do_masking = 0;  
	uint8_t  do_overdraw = 0;
	uint16_t background_color = 0;
	uint16_t foreground_color = KEDEIRPI35_WHITE;
	  
	// Functions for controlling masked and overdrawn rendering
	void     overdraw_on();
	void     overdraw_off();
	void     masking_on();
	void     masking_off();
	void     flip_mask();

 protected:
    SPIClass                *spi_port;
	SPIClass::SPI_Hardware_t *_spi_hardware;
    //uint32_t                _spi_port_memorymap = 0;
#if defined(KINETISK)
 	KINETISK_SPI_t *_pkinetisk_spi;
 	uint8_t			_fifo_size;
#elif defined(__IMXRT1052__) || defined(__IMXRT1062__)  // Teensy 4.x
	//inline IMXRT_LPSPI_t & port() { return (*(IMXRT_LPSPI_t *)0x403A0000); }
	IMXRT_LPSPI_t *_pimxrt_spi;
#elif defined(KINETISL)
 	KINETISL_SPI_t *_pkinetisl_spi;
#endif
	int16_t _width, _height; // Display w/h as modified by current rotation
	int16_t  cursor_x, cursor_y;
	
	int16_t  _clipx1, _clipy1, _clipx2, _clipy2;
	int16_t  _originx, _originy;
	int16_t  _displayclipx1, _displayclipy1, _displayclipx2, _displayclipy2;
	bool _invisible = false; 
	bool _standard = true; // no bounding rectangle or origin set. 

	inline void updateDisplayClip() {
		_displayclipx1 = max(0,min(_clipx1+_originx,width()));
		_displayclipx2 = max(0,min(_clipx2+_originx,width()));

		_displayclipy1 = max(0,min(_clipy1+_originy,height()));
		_displayclipy2 = max(0,min(_clipy2+_originy,height()));
		_invisible = (_displayclipx1 == _displayclipx2 || _displayclipy1 == _displayclipy2);
		_standard =  (_displayclipx1 == 0) && (_displayclipx2 == _width) && (_displayclipy1 == 0) && (_displayclipy2 == _height);
		if (Serial) {
			//Serial.printf("UDC (%d %d)-(%d %d) %d %d\n", _displayclipx1, _displayclipy1, _displayclipx2, 
			//	_displayclipy2, _invisible, _standard);

		}
	}
	
	uint16_t textcolor, textbgcolor, scrollbgcolor;
	uint8_t textsize, rotation,textdatum;
	boolean wrap; // If set, 'wrap' text at right edge of display
	const KEDEIRPI35_t3_font_t *font;

	uint32_t padX;
	int16_t scroll_x, scroll_y, scroll_width, scroll_height;
	boolean scrollEnable,isWritingScrollArea; // If set, 'wrap' text at right edge of display

  	uint8_t _cs, _touchcs;
	uint8_t pcs_data, pcs_command;
	uint8_t _miso, _mosi, _sclk;
	// add support to allow only one hardware CS (used for dc)
#if defined(__IMXRT1052__) || defined(__IMXRT1062__)  // Teensy 4.x
    uint32_t _cspinmask;
    volatile uint32_t *_csport;
    uint32_t _spi_tcr_current;
    uint32_t _touchcspinmask;
    uint8_t _pending_rx_count;
    volatile uint32_t *_touchcsport;

#elif defined(KINETISK) || defined(KINETISL)
    uint8_t _cspinmask;
    volatile uint8_t *_csport;
    uint8_t _touchcspinmask;
    volatile uint8_t *_touchcsport;

#endif

#ifdef ENABLE_KEDEIRPI35_FRAMEBUFFER
    // Add support for optional frame buffer
    uint8_t		*_pfbtft;						// Optional Frame buffer 
    uint8_t		_use_fbtft;						// Are we in frame buffer mode?
    uint8_t		*_we_allocated_buffer;			// We allocated the buffer; 

    uint16_t	*_pallet;						// Support for user to set Pallet. 
    uint16_t	_pallet_size;					// How big is the pallet
    uint16_t	_pallet_count;					// how many items are in it...
    boolean		_colors_are_index;				// are the values passed in index or color?

#endif


//----------------------------------------------------------------------
// Processor Specific stuff
#if defined(__IMXRT1052__) || defined(__IMXRT1062__)  // Teensy 4.x
// T4
	uint32_t _tcr_save = 0;
	void DIRECT_WRITE_LOW(volatile uint32_t * base, uint32_t mask)  __attribute__((always_inline)) {
		*(base+34) = mask;
	}
	void DIRECT_WRITE_HIGH(volatile uint32_t * base, uint32_t mask)  __attribute__((always_inline)) {
		*(base+33) = mask;
	}

	void beginSPITransaction(uint32_t clock = KEDEIRPI35_SPICLOCK)  __attribute__((always_inline)) {
		spi_port->beginTransaction(SPISettings(clock, MSBFIRST, SPI_MODE0));
		 _tcr_save = _pimxrt_spi->TCR;
	    _pimxrt_spi->TCR = (_pimxrt_spi->TCR & 0xfffff000) | LPSPI_TCR_FRAMESZ(31);  // turn on 32 bit mode
	}
	void endSPITransaction()  __attribute__((always_inline)) {
		spi_port->endTransaction();
	}
	void spi_transmit32(uint32_t data32)
	{
	  DIRECT_WRITE_LOW(_csport, _cspinmask);
	  if ((_pimxrt_spi->TCR & 0xfff) != 31) {
	    _pimxrt_spi->TCR = (_pimxrt_spi->TCR & 0xfffff000) | LPSPI_TCR_FRAMESZ(31);  // turn on 32 bit mode
	  }
	  _pimxrt_spi->TDR = data32;    // output 32 bit data.
	  while ((_pimxrt_spi->RSR & LPSPI_RSR_RXEMPTY)) ;  // wait while the RSR fifo is empty...
	  data32 = _pimxrt_spi->RDR;
	  DIRECT_WRITE_HIGH(_csport, _cspinmask);
	  DIRECT_WRITE_LOW(_touchcsport, _touchcspinmask);
	  delayNanoseconds(5);
	  DIRECT_WRITE_HIGH(_touchcsport, _touchcspinmask);
	  return;
	}

	void spi_transmit24(uint32_t data32)
	{
	  DIRECT_WRITE_LOW(_csport, _cspinmask);
	  if ((_pimxrt_spi->TCR & 0xfff) != 23) {
	    _pimxrt_spi->TCR = (_pimxrt_spi->TCR & 0xfffff000) | LPSPI_TCR_FRAMESZ(23);  // turn on 24 bit mode
	  }
	  _pimxrt_spi->TDR = data32;    // output 32 bit data.
	  while ((_pimxrt_spi->RSR & LPSPI_RSR_RXEMPTY)) ;  // wait while the RSR fifo is empty...
	  data32 = _pimxrt_spi->RDR;
	  DIRECT_WRITE_HIGH(_csport, _cspinmask);
	  DIRECT_WRITE_LOW(_touchcsport, _touchcspinmask);
	  delayNanoseconds(5);
	  DIRECT_WRITE_HIGH(_touchcsport, _touchcspinmask);
	  return;
	}


#elif defined(KINETISK)
// T3.x	
	void DIRECT_WRITE_LOW(volatile uint8_t * base, uint8_t mask)  __attribute__((always_inline)) {
		*base = 0;
	}
	void DIRECT_WRITE_HIGH(volatile uint8_t * base, uint8_t mask)  __attribute__((always_inline)) {
		*base = 1;
	}

	void beginSPITransaction(uint32_t clock = KEDEIRPI35_SPICLOCK)  __attribute__((always_inline)) {
		spi_port->beginTransaction(SPISettings(clock, MSBFIRST, SPI_MODE0));
	}
	void endSPITransaction()  __attribute__((always_inline)) {
		spi_port->endTransaction();
	}

	void delayNanoseconds(uint8_t cnt) {
		while (cnt--) {

		}
	}

	void spi_transmit32(uint32_t data32)
	{
		DIRECT_WRITE_LOW(_csport, _cspinmask);
		uint8_t sr;
		// Push out the two value
 	  	_pkinetisk_spi->PUSHR = (data32 >>16)      |  SPI_PUSHR_CTAS(1) | SPI_PUSHR_CONT;
		do {
			sr = _pkinetisk_spi->SR;
		} while ((uint16_t)(sr & (15 << 12)) > ((uint16_t)(_fifo_size-1) << 12));

		_pkinetisk_spi->PUSHR = (data32 & 0xffff) |  SPI_PUSHR_CTAS(1);
			sr = _pkinetisk_spi->SR;

		uint8_t reads_remaining = 2;
		uint32_t tmp __attribute__((unused));
		while (reads_remaining) {
			sr = _pkinetisk_spi->SR;
			if (sr & 0xF0) {
				tmp = _pkinetisk_spi->POPR;  // drain RX FIFO
				reads_remaining--;
			}
		}
		DIRECT_WRITE_HIGH(_csport, _cspinmask);
		DIRECT_WRITE_LOW(_touchcsport, _touchcspinmask);
		delayNanoseconds(5);
		DIRECT_WRITE_HIGH(_touchcsport, _touchcspinmask);
	}

	void spi_transmit24(uint32_t data32)
	{
		DIRECT_WRITE_LOW(_csport, _cspinmask);
		uint8_t sr;
		// Push out the two value first one 8 bits
 	  	_pkinetisk_spi->PUSHR = (data32 >>16)      |  SPI_PUSHR_CTAS(0) | SPI_PUSHR_CONT;
		do {
			sr = _pkinetisk_spi->SR;
		} while ((uint16_t)(sr & (15 << 12)) > ((uint16_t)(_fifo_size-1) << 12));

		_pkinetisk_spi->PUSHR = (data32 & 0xffff) |  SPI_PUSHR_CTAS(1);
			sr = _pkinetisk_spi->SR;

		uint8_t reads_remaining = 2;
		uint32_t tmp __attribute__((unused));
		while (reads_remaining) {
			sr = _pkinetisk_spi->SR;
			if (sr & 0xF0) {
				tmp = _pkinetisk_spi->POPR;  // drain RX FIFO
				reads_remaining--;
			}
		}
		DIRECT_WRITE_HIGH(_csport, _cspinmask);
		DIRECT_WRITE_LOW(_touchcsport, _touchcspinmask);
		delayNanoseconds(5);
		DIRECT_WRITE_HIGH(_touchcsport, _touchcspinmask);
		return;
	}
#elif defined(KINETISL)
	void DIRECT_WRITE_LOW(volatile uint8_t * base, uint8_t mask)  __attribute__((always_inline)) {
		*base &= ~mask;
	}
	void DIRECT_WRITE_HIGH(volatile uint8_t * base, uint8_t mask)  __attribute__((always_inline)) {
		*base |= mask;
	}

	void beginSPITransaction(uint32_t clock = KEDEIRPI35_SPICLOCK)  __attribute__((always_inline)) {
		spi_port->beginTransaction(SPISettings(clock, MSBFIRST, SPI_MODE0));
	}
	void endSPITransaction()  __attribute__((always_inline)) {
		spi_port->endTransaction();
	}

	void delayNanoseconds(uint8_t cnt) {
		while (cnt--) {

		}
	}

	void spi_transmit32(uint32_t data32)
	{
		DIRECT_WRITE_LOW(_csport, _cspinmask);

		// For TLC - real simple and dumb
		spi_port->transfer16(data32 >>16);
		spi_port->transfer16(data32 & 0xffff);
		DIRECT_WRITE_HIGH(_csport, _cspinmask);
		DIRECT_WRITE_LOW(_touchcsport, _touchcspinmask);
		delayNanoseconds(5);
		DIRECT_WRITE_HIGH(_touchcsport, _touchcspinmask);
	}

	void spi_transmit24(uint32_t data32)
	{
		DIRECT_WRITE_LOW(_csport, _cspinmask);

		// For TLC - real simple and dumb
		spi_port->transfer(data32 >>16);
		spi_port->transfer16(data32 & 0xffff);
		DIRECT_WRITE_HIGH(_csport, _cspinmask);
		DIRECT_WRITE_LOW(_touchcsport, _touchcspinmask);
		delayNanoseconds(5);
		DIRECT_WRITE_HIGH(_touchcsport, _touchcspinmask);

	}

#endif

	// Other helper functions.
	// Simple helper functions
	inline void lcd_cmd(uint8_t cmd) {spi_transmit32(0x00110000L | cmd);} 
	inline void lcd_data(uint8_t dat) {spi_transmit32(0x00150000L | dat);}
	inline void lcd_color(uint16_t col){spi_transmit32(0x00150000L | col);}
	inline void lcd_color(uint16_t col, uint16_t count){
		uint32_t dat = 0x00150000L | col;
		while (count--) spi_transmit32(dat);
	}
	// 18bit color mode
	inline void lcd_colorRGB(uint8_t r, uint8_t g, uint8_t b) {
  		uint16_t col = ((r << 8) & 0xF800) | ((g << 3) & 0x07E0) | ((b >> 3) & 0x001F);
		spi_transmit24(0x150000L | col);
	}
	
	void setAddr(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
	void HLine(int16_t x, int16_t y, int16_t w, uint16_t color);
	void VLine(int16_t x, int16_t y, int16_t h, uint16_t color);
	void Pixel(int16_t x, int16_t y, uint16_t color);


	void drawFontBits(bool opaque, uint32_t bits, uint32_t numbits, int32_t x, int32_t y, uint32_t repeat);
};

#ifndef swap
#define swap(a, b) { typeof(a) t = a; a = b; b = t; }
#endif

// To avoid conflict when also using Adafruit_GFX or any Adafruit library
// which depends on Adafruit_GFX, #include the Adafruit library *BEFORE*
// you #include KeDeiRPI35_t3.h.
#ifndef _ADAFRUIT_GFX_H
class Adafruit_GFX_Button {
public:
	Adafruit_GFX_Button(void) { _gfx = NULL; }
	void initButton(KEDEIRPI35_t3 *gfx, int16_t x, int16_t y,
		uint8_t w, uint8_t h,
		uint16_t outline, uint16_t fill, uint16_t textcolor,
		const char *label, uint8_t textsize);
	void drawButton(bool inverted = false);
	bool contains(int16_t x, int16_t y);
	void press(boolean p) {
		laststate = currstate;
		currstate = p;
	}
	bool isPressed() { return currstate; }
	bool justPressed() { return (currstate && !laststate); }
	bool justReleased() { return (!currstate && laststate); }
private:
	KEDEIRPI35_t3 *_gfx;
	int16_t _x, _y;
	uint16_t _w, _h;
	uint8_t _textsize;
	uint16_t _outlinecolor, _fillcolor, _textcolor;
	char _label[10];
	boolean currstate, laststate;
};
#endif

#endif // __cplusplus


#endif
