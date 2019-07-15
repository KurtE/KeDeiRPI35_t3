// https://github.com/PaulStoffregen/KEDEIRPI35_t3
// http://forum.pjrc.com/threads/26305-Highly-optimized-ILI9488-(320x240-TFT-color-display)-library

/***************************************************
  This is our library for the Adafruit ILI9488 Breakout and Shield
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
// 		writeRect8BPP - 	write 8 bit per pixel paletted bitmap
// 		writeRect4BPP - 	write 4 bit per pixel paletted bitmap
// 		writeRect2BPP - 	write 2 bit per pixel paletted bitmap
// 		writeRect1BPP - 	write 1 bit per pixel paletted bitmap

//TODO: transparent bitmap writing routines for sprites

//String Pixel Length support 
//---------------------------
//		strPixelLen			- gets pixel length of given ASCII string

// <\SoftEgg>

#include "KeDeiRPI35_t3.h"
#include <SPI.h>

// Teensy 3.1 can only generate 30 MHz SPI when running at 120 MHz (overclock)
// At all other speeds, spi_port->beginTransaction() will use the fastest available clock

#define WIDTH  KEDEIRPI35_TFTWIDTH
#define HEIGHT KEDEIRPI35_TFTHEIGHT

#ifdef ENABLE_KEDEIRPI35_FRAMEBUFFER
#define CBALLOC (KEDEIRPI35_TFTHEIGHT*KEDEIRPI35_TFTWIDTH)
#define	COUNT_WORDS_WRITE  ((KEDEIRPI35_TFTHEIGHT*KEDEIRPI35_TFTWIDTH)/SCREEN_DMA_NUM_SETTINGS) // Note I know the divide will give whole number
#endif

//#define DEBUG_ASYNC_UPDATE
#if defined(__MK66FX1M0__) 
DMASetting 	KEDEIRPI35_t3::_dmasettings[3];
DMAChannel 	KEDEIRPI35_t3::_dmatx;
#elif defined(__MK64FX512__)
DMAChannel  KEDEIRPI35_t3::_dmatx;
//DMAChannel  KEDEIRPI35_t3::_dmarx;
//uint16_t 	KEDEIRPI35_t3::_dma_count_remaining;
//uint16_t	KEDEIRPI35_t3::_dma_write_size_words;
//volatile short _dma_dummy_rx;
#elif defined(__IMXRT1052__) || defined(__IMXRT1062__)  // Teensy 4.x
//#define DEBUG_ASYNC_LEDS	// Enable to use digitalWrites to Debug
#ifdef DEBUG_ASYNC_LEDS
#define DEBUG_PIN_1 2
#define DEBUG_PIN_2 3
#define DEBUG_PIN_3 4
#endif


DMASetting 	KEDEIRPI35_t3::_dmasettings[2];
DMAChannel 	KEDEIRPI35_t3::_dmatx;
#else
#endif	

#ifdef ENABLE_KEDEIRPI35_FRAMEBUFFER
KEDEIRPI35_t3 *KEDEIRPI35_t3::_dmaActiveDisplay = 0;
volatile uint8_t  	KEDEIRPI35_t3::_dma_state = 0;  // Use pointer to this as a way to get back to object...
volatile uint32_t	KEDEIRPI35_t3::_dma_frame_count = 0;	// Can return a frame count...
volatile uint32_t 	KEDEIRPI35_t3::_dma_pixel_index = 0;
volatile uint16_t	KEDEIRPI35_t3::_dma_sub_frame_count = 0;	// Can return a frame count...

#endif

// Constructor when using hardware SPI.  Faster, but must use SPI pins
// specific to each board type (e.g. 11,13 for Uno, 51,52 for Mega, etc.)
KEDEIRPI35_t3::KEDEIRPI35_t3(SPIClass *SPIWire, uint8_t cs, uint8_t touchcs, uint8_t mosi, uint8_t sclk, uint8_t miso)
{
	spi_port = SPIWire;

	_cs   = cs;
	_touchcs   = touchcs;
	_mosi = mosi;
	_sclk = sclk;
	_miso = miso;
	_width    = WIDTH;
	_height   = HEIGHT;
	rotation  = 0;
	cursor_y  = cursor_x    = 0;
	cursor_y  = cursor_x    = 0;
	textsize  = 1;
	textcolor = textbgcolor = 0xFFFF;
	wrap      = true;
	font      = NULL;
	// Added to see how much impact actually using non hardware CS pin might be
    _cspinmask = 0;
    _csport = NULL;
	
#ifdef ENABLE_KEDEIRPI35_FRAMEBUFFER
	_use_fbtft = false;
	_pfbtft = nullptr;
	_pallet = NULL ;	// 
	// Probably should check that it did not fail... 
	_pallet_size = 0;					// How big is the pallet
	_pallet_count = 0;					// how many items are in it...
#endif

 	uint32_t *pa = (uint32_t*)((void*)spi_port);
	_spi_hardware = (SPIClass::SPI_Hardware_t*)(void*)pa[1];
    #ifdef KINETISK
	_pkinetisk_spi = (KINETISK_SPI_t *)(void*)pa[0];
	_fifo_size = _spi_hardware->queue_size;		// remember the queue size
	#elif defined(__IMXRT1052__) || defined(__IMXRT1062__)
	_pimxrt_spi = (IMXRT_LPSPI_t *)(void*)pa[0];
	#endif
	setClipRect();
	setOrigin();
}

//=======================================================================
// Add optional support for using frame buffer to speed up complex outputs
//=======================================================================
	
	// Support for user to set Pallet. 
void KEDEIRPI35_t3::setPallet(uint16_t *pal, uint16_t count) {
#ifdef ENABLE_KEDEIRPI35_FRAMEBUFFER
	if (_pallet && _pallet_size) free(_pallet);

	_pallet = pal;
	_pallet_count = count;
	_pallet_size = 0;	// assume we can not set this internally 
#endif
}

#ifdef ENABLE_KEDEIRPI35_FRAMEBUFFER
uint8_t KEDEIRPI35_t3::doActualConvertColorToIndex(uint16_t color) {
	if (_pallet == nullptr) {
		// Need to allocate it... 
		_pallet = (uint16_t *)malloc(256*sizeof(uint16_t));	// 
		// Probably should check that it did not fail... 
    	_pallet_size = 256;					// How big is the pallet
    	_pallet_count = 0;					// how many items are in it...
	}
	
	if (_colors_are_index) return (uint8_t)color;

	for (uint8_t i = 0; i < _pallet_count; i++) {
		if (_pallet[i] == color) return i;
	}
	if (_pallet_count >=  _pallet_size) return 0;
	_pallet[_pallet_count] = color;
	return _pallet_count++;
}
	
#endif


void KEDEIRPI35_t3::setFrameBuffer(uint8_t *frame_buffer) 
{
	#ifdef ENABLE_KEDEIRPI35_FRAMEBUFFER
	_pfbtft = frame_buffer;
	if (_pfbtft != NULL) {
		// Frame buffer is color index only here...
		memset(_pfbtft, 0, KEDEIRPI35_TFTHEIGHT*KEDEIRPI35_TFTWIDTH);
	}

	#endif	
}

uint8_t KEDEIRPI35_t3::useFrameBuffer(boolean b)		// use the frame buffer?  First call will allocate
{
	#ifdef ENABLE_KEDEIRPI35_FRAMEBUFFER

	if (b) {
		// First see if we need to allocate buffer
		if (_pfbtft == NULL) {
			// Hack to start frame buffer on 32 byte boundary
			_we_allocated_buffer = (uint8_t *)malloc(CBALLOC+32);
			if (_we_allocated_buffer == NULL)
				return 0;	// failed 
			_pfbtft = (uint8_t*) (((uintptr_t)_we_allocated_buffer + 32) & ~ ((uintptr_t) (31)));
			memset(_pfbtft, 0, CBALLOC);	
		}
		_use_fbtft = 1;
	} else 
		_use_fbtft = 0;

	return _use_fbtft;	
	#else
	return 0;
	#endif
}

void KEDEIRPI35_t3::freeFrameBuffer(void)						// explicit call to release the buffer
{
	#ifdef ENABLE_KEDEIRPI35_FRAMEBUFFER
	if (_we_allocated_buffer) {
		free(_we_allocated_buffer);
		_pfbtft = NULL;
		_use_fbtft = 0;	// make sure the use is turned off
		_we_allocated_buffer = NULL;
	}
	#endif
}
void KEDEIRPI35_t3::updateScreen(void)					// call to say update the screen now.
{
	// Not sure if better here to check flag or check existence of buffer.
	// Will go by buffer as maybe can do interesting things?
	#ifdef ENABLE_KEDEIRPI35_FRAMEBUFFER
	if (_use_fbtft) {
		beginSPITransaction();
		if (_standard) {
			//Serial.printf("Update Screen Standard %x(%x)\n", *_pfbtft, _pallet[*_pfbtft]);
			// Doing full window. 
			setAddr(0, 0, _width-1, _height-1);
			lcd_cmd(KEDEIRPI35_RAMWR);

			// BUGBUG doing as one shot.  Not sure if should or not or do like
			// main code and break up into transactions...
			uint8_t *pfbtft_end = &_pfbtft[(KEDEIRPI35_TFTWIDTH*KEDEIRPI35_TFTHEIGHT)-1];	// setup 
			uint8_t *pftbft = _pfbtft;

			// Quick write out the data;
			while (pftbft < pfbtft_end) {
				lcd_color(_pallet[*pftbft++]);
			}
			lcd_color(_pallet[*pftbft]);
		} else {
			// setup just to output the clip rectangle area. 
			setAddr(_displayclipx1, _displayclipy1, _displayclipx2-1, _displayclipy2-1);
			lcd_cmd(KEDEIRPI35_RAMWR);

			// BUGBUG doing as one shot.  Not sure if should or not or do like
			// main code and break up into transactions...
			uint8_t * pfbPixel_row = &_pfbtft[ _displayclipy1*_width + _displayclipx1];
			for (uint16_t y = _displayclipy1; y < _displayclipy2; y++) {
				uint8_t * pfbPixel = pfbPixel_row;
				for (uint16_t x = _displayclipx1; x < (_displayclipx2-1); x++) {
					lcd_color(_pallet[*pfbPixel++]);
				}
				if (y < (_displayclipy2-1))
					lcd_color(_pallet[*pfbPixel]);
				else	
					lcd_color(_pallet[*pfbPixel]);
				pfbPixel_row += _width;	// setup for the next row. 
			}
		}
		endSPITransaction();
	}
	#endif
}			 




void KEDEIRPI35_t3::setAddrWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
	beginSPITransaction();
	setAddr(x0, y0, x1, y1);
	lcd_cmd(KEDEIRPI35_RAMWR);
	endSPITransaction();
}

#if 0
void KEDEIRPI35_t3::lcd_color(uint16_t color){
  // #if (__STM32F1__)
  //     uint8_t buff[4] = {
  //       (((color & 0xF800) >> 11)* 255) / 31,
  //       (((color & 0x07E0) >> 5) * 255) / 63,
  //       ((color & 0x001F)* 255) / 31
  //     };
  //     spi_port->dmaSend(buff, 3);
  // #else
#if defined(__IMXRT1052__) || defined(__IMXRT1062__)  // Teensy 4.x
  uint8_t r = (color & 0xF800) >> 11;
  uint8_t g = (color & 0x07E0) >> 5;
  uint8_t b = color & 0x001F;

  r = (r * 255) / 31;
  g = (g * 255) / 63;
  b = (b * 255) / 31;
  uint32_t color24 = ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
  if (last_pixel)  {
	maybeUpdateTCR(LPSPI_TCR_PCS(1) | LPSPI_TCR_FRAMESZ(23));
	_pimxrt_spi->TDR = color24;
	_pending_rx_count++;	//
	waitTransmitComplete();
  } else {
	maybeUpdateTCR(LPSPI_TCR_PCS(1) | LPSPI_TCR_FRAMESZ(23) | LPSPI_TCR_CONT);
	_pimxrt_spi->TDR = color24;
	_pending_rx_count++;	//
	waitFifoNotFull();
  }

#elif defined(KINETISK)
  uint8_t r = (color & 0xF800) >> 11;
  r = (r * 255) / 31;
  writedata8_cont(r);

  uint8_t g = (color & 0x07E0) >> 5;
  g = (g * 255) / 63;
  writedata8_cont(g);

  uint8_t b = color & 0x001F;
  b = (b * 255) / 31;
  if (last_pixel)  {
  	writedata8_last(b);
  } else {
  	writedata8_cont(b);
  }
#elif defined(KINETISL)
  uint8_t r = (color & 0xF800) >> 11;

  r = (r * 255) / 31;
  setDataMode();
  outputToSPI(r);
  uint8_t g = (color & 0x07E0) >> 5;
  g = (g * 255) / 63;
  outputToSPIAlready8Bits(g);
  uint8_t b = color & 0x001F;
  b = (b * 255) / 31;
  outputToSPIAlready8Bits(b);
  if (last_pixel) {
	waitTransmitComplete();
  } 

#endif
  // #endif
}


void KEDEIRPI35_t3::lcd_color(uint16_t color, uint16_t count){
#if defined(__IMXRT1052__) || defined(__IMXRT1062__)  // Teensy 4.x
  uint8_t r = (color & 0xF800) >> 11;
  uint8_t g = (color & 0x07E0) >> 5;
  uint8_t b = color & 0x001F;

  r = (r * 255) / 31;
  g = (g * 255) / 63;
  b = (b * 255) / 31;
  uint32_t color24 = ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
  while (count > 1) {
	maybeUpdateTCR(LPSPI_TCR_PCS(1) | LPSPI_TCR_FRAMESZ(23) | LPSPI_TCR_CONT);
	_pimxrt_spi->TDR = color24;
	_pending_rx_count++;	//
	waitFifoNotFull();
	count--;  	
  }

  if (last_pixel)  {
	maybeUpdateTCR(LPSPI_TCR_PCS(1) | LPSPI_TCR_FRAMESZ(23));
	_pimxrt_spi->TDR = color24;
	_pending_rx_count++;	//
	waitTransmitComplete();
  } else {
	maybeUpdateTCR(LPSPI_TCR_PCS(1) | LPSPI_TCR_FRAMESZ(23) | LPSPI_TCR_CONT);
	_pimxrt_spi->TDR = color24;
	_pending_rx_count++;	//
	waitFifoNotFull();
  }

#elif defined(KINETISK)
  if (count < 2) {
  	lcd_color(color);
  	return;
  }
  uint8_t r = (color & 0xF800) >> 11;
  r = (r * 255) / 31;
  writedata8_cont(r);

  uint8_t g = (color & 0x07E0) >> 5;
  g = (g * 255) / 63;
  writedata8_cont(g);

  uint8_t b = color & 0x001F;
  b = (b * 255) / 31;

  writedata8_cont(b);
 
  while (--count) {
	  writedata8_cont(r);
	  writedata8_cont(g);

	  if ((count == 1) && last_pixel)  {
	  	writedata8_last(b);
	  } else {
	  	writedata8_cont(b);
	  }
  }
#elif defined(KINETISL)
  uint8_t r = (color & 0xF800) >> 11;

  r = (r * 255) / 31;
  setDataMode();
  outputToSPI(r);
  uint8_t g = (color & 0x07E0) >> 5;
  g = (g * 255) / 63;
  outputToSPIAlready8Bits(g);
  uint8_t b = color & 0x001F;
  b = (b * 255) / 31;
  outputToSPIAlready8Bits(b);
  while (--count) {
	  outputToSPIAlready8Bits(r);
	  outputToSPIAlready8Bits(g);
	  outputToSPIAlready8Bits(b);
  }

  if (last_pixel) {
		waitTransmitComplete();
  } 

#endif
}
#endif


void KEDEIRPI35_t3::pushColor(uint16_t color)
{
	beginSPITransaction();
	//lcd_color(color);
	lcd_color(color);
	endSPITransaction();
}

void KEDEIRPI35_t3::drawPixel(int16_t x, int16_t y, uint16_t color) {
	x += _originx;
	y += _originy;
	if((x < _displayclipx1) ||(x >= _displayclipx2) || (y < _displayclipy1) || (y >= _displayclipy2)) return;

	#ifdef ENABLE_KEDEIRPI35_FRAMEBUFFER
	if (_use_fbtft) {
		_pfbtft[y*_width + x] = mapColorToPalletIndex(color);

	} else 
	#endif
	{
		beginSPITransaction();
		setAddr(x, y, x, y);
		lcd_cmd(KEDEIRPI35_RAMWR);
		lcd_color(color);
		endSPITransaction();
	}
}

void KEDEIRPI35_t3::drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color)
{
	x+=_originx;
	y+=_originy;
	// Rectangular clipping
	if((x < _displayclipx1) || (x >= _displayclipx2) || (y >= _displayclipy2)) return;
	if(y < _displayclipy1) { h = h - (_displayclipy1 - y); y = _displayclipy1;}
	if((y+h-1) >= _displayclipy2) h = _displayclipy2-y;
	if(h<1) return;

	#ifdef ENABLE_KEDEIRPI35_FRAMEBUFFER
	if (_use_fbtft) {
		uint8_t * pfbPixel = &_pfbtft[ y*_width + x];
		uint8_t color_index = mapColorToPalletIndex(color);
		while (h--) {
			*pfbPixel = color_index;
			pfbPixel += _width;
		}
	} else 
	#endif
	{
		beginSPITransaction();
		setAddr(x, y, x, y+h-1);
		lcd_cmd(KEDEIRPI35_RAMWR);
		lcd_color(color,h);
		endSPITransaction();
	}
}

void KEDEIRPI35_t3::drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color)
{
	x+=_originx;
	y+=_originy;

	// Rectangular clipping
	if((y < _displayclipy1) || (x >= _displayclipx2) || (y >= _displayclipy2)) return;
	if(x<_displayclipx1) { w = w - (_displayclipx1 - x); x = _displayclipx1; }
	if((x+w-1) >= _displayclipx2)  w = _displayclipx2-x;
	if (w<1) return;

	#ifdef ENABLE_KEDEIRPI35_FRAMEBUFFER
	if (_use_fbtft) {
		uint8_t color_index = mapColorToPalletIndex(color);
		uint8_t * pfbPixel = &_pfbtft[ y*_width + x];
		while (w--) {
			*pfbPixel++ = color_index;
		}
	} else 
	#endif
	{
		beginSPITransaction();
		setAddr(x, y, x+w-1, y);
		lcd_cmd(KEDEIRPI35_RAMWR);
		lcd_color(color, w);
		endSPITransaction();
	}
}

void KEDEIRPI35_t3::fillScreen(uint16_t color)
{
	fillRect(0, 0, _width, _height, color);
}

// fill a rectangle
void KEDEIRPI35_t3::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
	x+=_originx;
	y+=_originy;

	// Rectangular clipping (drawChar w/big text requires this)
	if((x >= _displayclipx2) || (y >= _displayclipy2)) return;
	if (((x+w) <= _displayclipx1) || ((y+h) <= _displayclipy1)) return;
	if(x < _displayclipx1) {	w -= (_displayclipx1-x); x = _displayclipx1; 	}
	if(y < _displayclipy1) {	h -= (_displayclipy1 - y); y = _displayclipy1; 	}
	if((x + w - 1) >= _displayclipx2)  w = _displayclipx2  - x;
	if((y + h - 1) >= _displayclipy2) h = _displayclipy2 - y;

	#ifdef ENABLE_KEDEIRPI35_FRAMEBUFFER
	if (_use_fbtft) {
		uint8_t color_index = mapColorToPalletIndex(color);
		//if (x==0 && y == 0) Serial.printf("fillrect %x %x %x\n", color, color_index, _pallet[color_index]);
		if (1 || (x&3) || (w&3)) {
			uint8_t * pfbPixel_row = &_pfbtft[ y*_width + x];
			for (;h>0; h--) {
				uint8_t * pfbPixel = pfbPixel_row;
				for (int i = 0 ;i < w; i++) {
					*pfbPixel++ = color_index;
				}
				pfbPixel_row += _width;
			}
		} else {
			// Horizontal is even number so try 32 bit writes instead
			uint32_t color32 = ((uint32_t)color_index << 24) | ((uint32_t)color_index << 16) | ((uint32_t)color_index << 8) | color_index;
			uint32_t * pfbPixel_row = (uint32_t *)((uint8_t*)&_pfbtft[ y*_width + x]);
			w = w/4;	// only iterate quarter the times
			for (;h>0; h--) {
				uint32_t * pfbPixel = pfbPixel_row;
				for (int i = 0 ;i < w; i++) {
					*pfbPixel++ = color32;
				}
				pfbPixel_row += (_width/4);
			}
		}
	} else 
	#endif
	{

		// TODO: this can result in a very long transaction time
		// should break this into multiple transactions, even though
		// it'll cost more overhead, so we don't stall other SPI libs
		beginSPITransaction();
		setAddr(x, y, x+w-1, y+h-1);
		lcd_cmd(KEDEIRPI35_RAMWR);
		for(y=h; y>0; y--) {
			lcd_color(color, w);
		}
		endSPITransaction();
	}
}

// fillRectVGradient	- fills area with vertical gradient
void KEDEIRPI35_t3::fillRectVGradient(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color1, uint16_t color2)
{
	x+=_originx;
	y+=_originy;

	// Rectangular clipping 
	if((x >= _displayclipx2) || (y >= _displayclipy2)) return;
	if(x < _displayclipx1) {	w -= (_displayclipx1-x); x = _displayclipx1; 	}
	if(y < _displayclipy1) {	h -= (_displayclipy1 - y); y = _displayclipy1; 	}
	if((x + w - 1) >= _displayclipx2)  w = _displayclipx2  - x;
	if((y + h - 1) >= _displayclipy2) h = _displayclipy2 - y;
	
	int16_t r1, g1, b1, r2, g2, b2, dr, dg, db, r, g, b;
	color565toRGB14(color1,r1,g1,b1);
	color565toRGB14(color2,r2,g2,b2);
	dr=(r2-r1)/h; dg=(g2-g1)/h; db=(b2-b1)/h;
	r=r1;g=g1;b=b1;	

	#ifdef ENABLE_KEDEIRPI35_FRAMEBUFFER
	if (_use_fbtft) {
		uint8_t * pfbPixel_row = &_pfbtft[ y*_width + x];
		for (;h>0; h--) {
			uint16_t color = RGB14tocolor565(r,g,b);
			uint8_t * pfbPixel = pfbPixel_row;
			for (int i = 0 ;i < w; i++) {
				*pfbPixel++ = mapColorToPalletIndex(color);
			}
			r+=dr;g+=dg; b+=db;
			pfbPixel_row += _width;
		}
	} else 
	#endif
	{		
		beginSPITransaction();
		setAddr(x, y, x+w-1, y+h-1);
		lcd_cmd(KEDEIRPI35_RAMWR);
		for(y=h; y>0; y--) {
			uint16_t color = RGB14tocolor565(r,g,b);

			for(x=w; x>1; x--) {
				lcd_color(color);
			}
			lcd_color(color);
			if (y > 1 && (y & 1)) {
				endSPITransaction();
				beginSPITransaction();
			}
			r+=dr;g+=dg; b+=db;
		}
		endSPITransaction();
	}
}

// fillRectHGradient	- fills area with horizontal gradient
void KEDEIRPI35_t3::fillRectHGradient(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color1, uint16_t color2)
{
	x+=_originx;
	y+=_originy;

	// Rectangular clipping 
	if((x >= _displayclipx2) || (y >= _displayclipy2)) return;
	if(x < _displayclipx1) {	w -= (_displayclipx1-x); x = _displayclipx1; 	}
	if(y < _displayclipy1) {	h -= (_displayclipy1 - y); y = _displayclipy1; 	}
	if((x + w - 1) >= _displayclipx2)  w = _displayclipx2  - x;
	if((y + h - 1) >= _displayclipy2) h = _displayclipy2 - y;
	
	int16_t r1, g1, b1, r2, g2, b2, dr, dg, db, r, g, b;
	uint16_t color;
	color565toRGB14(color1,r1,g1,b1);
	color565toRGB14(color2,r2,g2,b2);
	dr=(r2-r1)/w; dg=(g2-g1)/w; db=(b2-b1)/w;
	r=r1;g=g1;b=b1;	
	#ifdef ENABLE_KEDEIRPI35_FRAMEBUFFER
	if (_use_fbtft) {
		uint8_t * pfbPixel_row = &_pfbtft[ y*_width + x];
		for (;h>0; h--) {
			uint8_t * pfbPixel = pfbPixel_row;
			for (int i = 0 ;i < w; i++) {
				*pfbPixel++ = mapColorToPalletIndex(RGB14tocolor565(r,g,b));
				r+=dr;g+=dg; b+=db;
			}
			pfbPixel_row += _width;
			r=r1;g=g1;b=b1;
		}
	} else 
	#endif
	{
		beginSPITransaction();
		setAddr(x, y, x+w-1, y+h-1);
		lcd_cmd(KEDEIRPI35_RAMWR);
		for(y=h; y>0; y--) {
			for(x=w; x>1; x--) {
				color = RGB14tocolor565(r,g,b);
				lcd_color(color);
				r+=dr;g+=dg; b+=db;
			}
			color = RGB14tocolor565(r,g,b);
			lcd_color(color);
			if (y > 1 && (y & 1)) {
				endSPITransaction();
				beginSPITransaction();
			}
			r=r1;g=g1;b=b1;
		}
		endSPITransaction();
	}
}

// fillScreenVGradient - fills screen with vertical gradient
void KEDEIRPI35_t3::fillScreenVGradient(uint16_t color1, uint16_t color2)
{
	fillRectVGradient(0,0,_width,_height,color1,color2);
}

// fillScreenHGradient - fills screen with horizontal gradient
void KEDEIRPI35_t3::fillScreenHGradient(uint16_t color1, uint16_t color2)
{
	fillRectHGradient(0,0,_width,_height,color1,color2);
}

void KEDEIRPI35_t3::setRotation(uint8_t m)
{
	rotation = m % 4; // can't be higher than 3
	beginSPITransaction();
	lcd_cmd(KEDEIRPI35_MADCTL);
	switch (rotation) {
	case 0:
		lcd_data(0xEA); 
		_width  = KEDEIRPI35_TFTHEIGHT;
		_height = KEDEIRPI35_TFTWIDTH;
		break;
	case 1:
		lcd_data(0x4A);
		_width  = KEDEIRPI35_TFTWIDTH;
		_height = KEDEIRPI35_TFTHEIGHT;
		break;
	case 2:
		lcd_data(0x2A);
		_width  = KEDEIRPI35_TFTHEIGHT;
		_height = KEDEIRPI35_TFTWIDTH;
		break;
	case 3:
		lcd_data(0x8A);
		_width  = KEDEIRPI35_TFTWIDTH;
		_height = KEDEIRPI35_TFTHEIGHT;
		break;
	}
	endSPITransaction();
	setClipRect();
	setOrigin();
	cursor_x = 0;
	cursor_y = 0;
}

void KEDEIRPI35_t3::setScroll(uint16_t offset)
{
#ifdef LATER
	beginSPITransaction();
	writecommand_cont(KEDEIRPI35_VSCRSADD);
	writedata16_last(offset);
	endSPITransaction();
#endif	
}

void KEDEIRPI35_t3::invertDisplay(boolean i)
{
#ifdef LATER
	beginSPITransaction();
	writecommand_last(i ? KEDEIRPI35_INVON : KEDEIRPI35_INVOFF);
	endSPITransaction();
#endif
}










/*
uint8_t KEDEIRPI35_t3::readdata(void)
{
  uint8_t r;
       // Try to work directly with SPI registers...
       // First wait until output queue is empty
        uint16_t wTimeout = 0xffff;
        while (((_pkinetisk_spi->SR) & (15 << 12)) && (--wTimeout)) ; // wait until empty
        
//       	_pkinetisk_spi->MCR |= SPI_MCR_CLR_RXF; // discard any received data
//		_pkinetisk_spi->SR = SPI_SR_TCF;
        
        // Transfer a 0 out... 
        writedata8_cont(0);   
        
        // Now wait until completed. 
        wTimeout = 0xffff;
        while (((_pkinetisk_spi->SR) & (15 << 12)) && (--wTimeout)) ; // wait until empty
        r = _pkinetisk_spi->POPR;  // get the received byte... should check for it first...
    return r;
}
 */


uint8_t KEDEIRPI35_t3::readcommand8(uint8_t c, uint8_t index)
{
	return 0xff;
}



// Read Pixel at x,y and get back 16-bit packed color
#define READ_PIXEL_PUSH_BYTE 0x3f
uint16_t KEDEIRPI35_t3::readPixel(int16_t x, int16_t y)
{
	return 0;
}

// Now lets see if we can read in multiple pixels
void KEDEIRPI35_t3::readRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t *pcolors)
{

}

// Now lets see if we can writemultiple pixels
void KEDEIRPI35_t3::writeRect(int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t *pcolors)
{

	x+=_originx;
	y+=_originy;
	uint16_t x_clip_left = 0;  // How many entries at start of colors to skip at start of row
	uint16_t x_clip_right = 0;    // how many color entries to skip at end of row for clipping
	// Rectangular clipping 

	// See if the whole thing out of bounds...
	if((x >= _displayclipx2) || (y >= _displayclipy2)) return;
	if (((x+w) <= _displayclipx1) || ((y+h) <= _displayclipy1)) return;

	// In these cases you can not do simple clipping, as we need to synchronize the colors array with the
	// We can clip the height as when we get to the last visible we don't have to go any farther. 
	// also maybe starting y as we will advance the color array. 
 	if(y < _displayclipy1) {
 		int dy = (_displayclipy1 - y);
 		h -= dy; 
 		pcolors += (dy*w); // Advance color array to 
 		y = _displayclipy1; 	
 	}

	if((y + h - 1) >= _displayclipy2) h = _displayclipy2 - y;

	// For X see how many items in color array to skip at start of row and likewise end of row 
	if(x < _displayclipx1) {
		x_clip_left = _displayclipx1-x; 
		w -= x_clip_left; 
		x = _displayclipx1; 	
	}
	if((x + w - 1) >= _displayclipx2) {
		x_clip_right = w;
		w = _displayclipx2  - x;
		x_clip_right -= w; 
	} 

	#ifdef ENABLE_KEDEIRPI35_FRAMEBUFFER
	if (_use_fbtft) {
		uint8_t * pfbPixel_row = &_pfbtft[ y*_width + x];
		for (;h>0; h--) {
			uint8_t * pfbPixel = pfbPixel_row;
			pcolors += x_clip_left;
			for (int i = 0 ;i < w; i++) {
				*pfbPixel++ = mapColorToPalletIndex(*pcolors++);
			}
			pfbPixel_row += _width;
			pcolors += x_clip_right;

		}
		return;	
	}
	#endif

   	beginSPITransaction();
	setAddr(x, y, x+w-1, y+h-1);
	lcd_cmd(KEDEIRPI35_RAMWR);
	for(y=h; y>0; y--) {
		pcolors += x_clip_left;
		for(x=w; x>1; x--) {
			lcd_color(*pcolors++);
		}
		lcd_color(*pcolors++);
		pcolors += x_clip_right;
	}
	endSPITransaction();
}

// writeRect8BPP - 	write 8 bit per pixel paletted bitmap
//					bitmap data in array at pixels, one byte per pixel
//					color palette data in array at palette
void KEDEIRPI35_t3::writeRect8BPP(int16_t x, int16_t y, int16_t w, int16_t h, const uint8_t *pixels, const uint16_t * palette )
{
	//Serial.printf("\nWR8: %d %d %d %d %x\n", x, y, w, h, (uint32_t)pixels);
	x+=_originx;
	y+=_originy;

	uint16_t x_clip_left = 0;  // How many entries at start of colors to skip at start of row
	uint16_t x_clip_right = 0;    // how many color entries to skip at end of row for clipping
	// Rectangular clipping 

	// See if the whole thing out of bounds...
	if((x >= _displayclipx2) || (y >= _displayclipy2)) return;
	if (((x+w) <= _displayclipx1) || ((y+h) <= _displayclipy1)) return;

	// In these cases you can not do simple clipping, as we need to synchronize the colors array with the
	// We can clip the height as when we get to the last visible we don't have to go any farther. 
	// also maybe starting y as we will advance the color array. 
 	if(y < _displayclipy1) {
 		int dy = (_displayclipy1 - y);
 		h -= dy; 
 		pixels += (dy*w); // Advance color array to 
 		y = _displayclipy1; 	
 	}

	if((y + h - 1) >= _displayclipy2) h = _displayclipy2 - y;

	// For X see how many items in color array to skip at start of row and likewise end of row 
	if(x < _displayclipx1) {
		x_clip_left = _displayclipx1-x; 
		w -= x_clip_left; 
		x = _displayclipx1; 	
	}
	if((x + w - 1) >= _displayclipx2) {
		x_clip_right = w;
		w = _displayclipx2  - x;
		x_clip_right -= w; 
	} 
	//Serial.printf("WR8C: %d %d %d %d %x- %d %d\n", x, y, w, h, (uint32_t)pixels, x_clip_right, x_clip_left);
	#ifdef ENABLE_KEDEIRPI35_FRAMEBUFFER
	if (_use_fbtft) {
		uint8_t * pfbPixel_row = &_pfbtft[ y*_width + x];
		for (;h>0; h--) {
			pixels += x_clip_left;
			uint8_t * pfbPixel = pfbPixel_row;
			for (int i = 0 ;i < w; i++) {
				*pfbPixel++ = mapColorToPalletIndex(palette[*pixels++]);
			}
			pixels += x_clip_right;
			pfbPixel_row += _width;
		}
		return;	
	}
	#endif

	beginSPITransaction();
	setAddr(x, y, x+w-1, y+h-1);
	lcd_cmd(KEDEIRPI35_RAMWR);
	for(y=h; y>0; y--) {
		pixels += x_clip_left;
		//Serial.printf("%x: ", (uint32_t)pixels);
		for(x=w; x>1; x--) {
			//Serial.print(*pixels, DEC);
			lcd_color(palette[*pixels++]);
		}
		//Serial.println(*pixels, DEC);
		lcd_color(palette[*pixels++]);
		pixels += x_clip_right;
	}
	endSPITransaction();
}

// writeRect4BPP - 	write 4 bit per pixel paletted bitmap
//					bitmap data in array at pixels, 4 bits per pixel
//					color palette data in array at palette
//					width must be at least 2 pixels
void KEDEIRPI35_t3::writeRect4BPP(int16_t x, int16_t y, int16_t w, int16_t h, const uint8_t *pixels, const uint16_t * palette )
{
   	beginSPITransaction();
	setAddr(x, y, x+w-1, y+h-1);
	lcd_cmd(KEDEIRPI35_RAMWR);
	for(y=h; y>0; y--) {
		for(x=w; x>2; x-=2) {
			lcd_color(palette[((*pixels)>>4)&0xF]);
			lcd_color(palette[(*pixels++)&0xF]);
		}
		lcd_color(palette[((*pixels)>>4)&0xF]);
		lcd_color(palette[(*pixels++)&0xF]);
	}
	endSPITransaction();
}

// writeRect2BPP - 	write 2 bit per pixel paletted bitmap
//					bitmap data in array at pixels, 4 bits per pixel
//					color palette data in array at palette
//					width must be at least 4 pixels
void KEDEIRPI35_t3::writeRect2BPP(int16_t x, int16_t y, int16_t w, int16_t h, const uint8_t *pixels, const uint16_t * palette )
{
   	beginSPITransaction();
	setAddr(x, y, x+w-1, y+h-1);
	lcd_cmd(KEDEIRPI35_RAMWR);
	for(y=h; y>0; y--) {
		for(x=w; x>4; x-=4) {
			//unrolled loop might be faster?
			lcd_color(palette[((*pixels)>>6)&0x3]);
			lcd_color(palette[((*pixels)>>4)&0x3]);
			lcd_color(palette[((*pixels)>>2)&0x3]);
			lcd_color(palette[(*pixels++)&0x3]);
		}
		lcd_color(palette[((*pixels)>>6)&0x3]);
		lcd_color(palette[((*pixels)>>4)&0x3]);
		lcd_color(palette[((*pixels)>>2)&0x3]);
		lcd_color(palette[(*pixels++)&0x3]);
	}
	endSPITransaction();
}

// writeRect1BPP - 	write 1 bit per pixel paletted bitmap
//					bitmap data in array at pixels, 4 bits per pixel
//					color palette data in array at palette
//					width must be at least 8 pixels
void KEDEIRPI35_t3::writeRect1BPP(int16_t x, int16_t y, int16_t w, int16_t h, const uint8_t *pixels, const uint16_t * palette )
{
   	beginSPITransaction();
	setAddr(x, y, x+w-1, y+h-1);
	lcd_cmd(KEDEIRPI35_RAMWR);
	for(y=h; y>0; y--) {
		for(x=w; x>8; x-=8) {
			//unrolled loop might be faster?
			lcd_color(palette[((*pixels)>>7)&0x1]);
			lcd_color(palette[((*pixels)>>6)&0x1]);
			lcd_color(palette[((*pixels)>>5)&0x1]);
			lcd_color(palette[((*pixels)>>4)&0x1]);
			lcd_color(palette[((*pixels)>>3)&0x1]);
			lcd_color(palette[((*pixels)>>2)&0x1]);
			lcd_color(palette[((*pixels)>>1)&0x1]);
			lcd_color(palette[(*pixels++)&0x1]);
		}
		lcd_color(palette[((*pixels)>>7)&0x1]);
		lcd_color(palette[((*pixels)>>6)&0x1]);
		lcd_color(palette[((*pixels)>>5)&0x1]);
		lcd_color(palette[((*pixels)>>4)&0x1]);
		lcd_color(palette[((*pixels)>>3)&0x1]);
		lcd_color(palette[((*pixels)>>2)&0x1]);
		lcd_color(palette[((*pixels)>>1)&0x1]);
		lcd_color(palette[(*pixels++)&0x1]);
	}
	endSPITransaction();
}

///============================================================================
// writeRectNBPP - 	write N(1, 2, 4, 8) bit per pixel paletted bitmap
//					bitmap data in array at pixels
//  Currently writeRect1BPP, writeRect2BPP, writeRect4BPP use this to do all of the work. 
void KEDEIRPI35_t3::writeRectNBPP(int16_t x, int16_t y, int16_t w, int16_t h,  uint8_t bits_per_pixel, 
		const uint8_t *pixels, const uint16_t * palette )
{
	//Serial.printf("\nWR8: %d %d %d %d %x\n", x, y, w, h, (uint32_t)pixels);
	x+=_originx;
	y+=_originy;
	uint8_t pixels_per_byte = 8/ bits_per_pixel;
	uint16_t count_of_bytes_per_row = (w + pixels_per_byte -1)/pixels_per_byte;		// Round up to handle non multiples
	uint8_t row_shift_init = 8 - bits_per_pixel;				// We shift down 6 bits by default 
	uint8_t pixel_bit_mask = (1 << bits_per_pixel) - 1; 		// get mask to use below
	// Rectangular clipping 

	// See if the whole thing out of bounds...
	if((x >= _displayclipx2) || (y >= _displayclipy2)) return;
	if (((x+w) <= _displayclipx1) || ((y+h) <= _displayclipy1)) return;

	// In these cases you can not do simple clipping, as we need to synchronize the colors array with the
	// We can clip the height as when we get to the last visible we don't have to go any farther. 
	// also maybe starting y as we will advance the color array. 
	// Again assume multiple of 8 for width
 	if(y < _displayclipy1) {
 		int dy = (_displayclipy1 - y);
 		h -= dy; 
 		pixels += dy * count_of_bytes_per_row;
 		y = _displayclipy1; 	
 	}

	if((y + h - 1) >= _displayclipy2) h = _displayclipy2 - y;

	// For X see how many items in color array to skip at start of row and likewise end of row 
	if(x < _displayclipx1) {
		uint16_t x_clip_left = _displayclipx1-x; 
		w -= x_clip_left; 
		x = _displayclipx1; 
		// Now lets update pixels to the rigth offset and mask
		uint8_t x_clip_left_bytes_incr = x_clip_left / pixels_per_byte;
		pixels += x_clip_left_bytes_incr;
		row_shift_init = 8 - (x_clip_left - (x_clip_left_bytes_incr * pixels_per_byte) + 1) * bits_per_pixel; 	
	}

	if((x + w - 1) >= _displayclipx2) {
		w = _displayclipx2  - x;
	} 

	const uint8_t * pixels_row_start = pixels;  // remember our starting position offset into row

	#ifdef ENABLE_KEDEIRPI35_FRAMEBUFFER
	if (_use_fbtft) {
		uint8_t * pfbPixel_row = &_pfbtft[ y*_width + x];
		for (;h>0; h--) {
			uint8_t * pfbPixel = pfbPixel_row;
			pixels = pixels_row_start;				// setup for this row
			uint8_t pixel_shift = row_shift_init;			// Setup mask

			for (int i = 0 ; i < w; i++) {
				*pfbPixel++ = mapColorToPalletIndex(palette[((*pixels)>>pixel_shift) & pixel_bit_mask]);
				if (!pixel_shift) {
					pixel_shift = 8 - bits_per_pixel;	//setup next mask
					pixels++;
				} else {
					pixel_shift -= bits_per_pixel;
				}
			}
			pfbPixel_row += _width;
			pixels_row_start += count_of_bytes_per_row;
		}
		return;	

	}
	#endif

	beginSPITransaction();
	setAddr(x, y, x+w-1, y+h-1);
	lcd_cmd(KEDEIRPI35_RAMWR);
	for (;h>0; h--) {
		pixels = pixels_row_start;				// setup for this row
		uint8_t pixel_shift = row_shift_init;			// Setup mask

		for (int i = 0 ;i < w; i++) {
			lcd_color(palette[((*pixels)>>pixel_shift) & pixel_bit_mask]);
			if (!pixel_shift) {
				pixel_shift = 8 - bits_per_pixel;	//setup next mask
				pixels++;
			} else {
				pixel_shift -= bits_per_pixel;
			}
		}
		pixels_row_start += count_of_bytes_per_row;
	}
	endSPITransaction();
}


void KEDEIRPI35_t3::begin(void)
{
    // verify SPI pins are valid;
    //Serial.printf("::begin %x %x %x %d %d %d\n", (uint32_t)spi_port, (uint32_t)_pkinetisk_spi, (uint32_t)_spi_hardware, _mosi, _miso, _sclk);
    #ifdef KINETISK
	/*
    #if defined(__MK64FX512__) || defined(__MK66FX1M0__)
    // Allow to work with mimimum of MOSI and SCK
    if ((_mosi == 255 || _mosi == 11 || _mosi == 7 || _mosi == 28)  && (_sclk == 255 || _sclk == 13 || _sclk == 14 || _sclk == 27)) 
	#else
    if ((_mosi == 255 || _mosi == 11 || _mosi == 7) && (_sclk == 255 || _sclk == 13 || _sclk == 14)) 
    #endif	
    {
        
		if (_mosi != 255) spi_port->setMOSI(_mosi);
        if (_sclk != 255) spi_port->setSCK(_sclk);

        // Now see if valid MISO
	    #if defined(__MK64FX512__) || defined(__MK66FX1M0__)
	    if (_miso == 12 || _miso == 8 || _miso == 39)
		#else
	    if (_miso == 12 || _miso == 8)
	    #endif
		{	
        	spi_port->setMISO(_miso);
    	} else {
			_miso = 0xff;	// set miso to 255 as flag it is bad
		}
	} else {
        return; // not valid pins...
	}
	*/

	if ((_mosi != 255) || (_miso != 255) || (_sclk != 255)) {
		#if defined(__MK64FX512__) || defined(__MK66FX1M0__)
			if(spi_port == &SPI){
				if (SPI.pinIsMOSI(_mosi) && SPI.pinIsMISO(_miso) && SPI.pinIsSCK(_sclk)) {
					//spi_port= &SPI;
					uint32_t *pa = (uint32_t*)((void*)spi_port);
					_spi_hardware = (SPIClass::SPI_Hardware_t*)(void*)pa[1];
					_pkinetisk_spi = (KINETISK_SPI_t *)(void*)pa[0];
					_fifo_size = _spi_hardware->queue_size;		// remember the queue size
				}
				Serial.println("KEDEIRPI35_t3n: SPI automatically selected");
			} else if(spi_port == &SPI1){
				if (SPI1.pinIsMOSI(_mosi) && SPI1.pinIsMISO(_miso) && SPI1.pinIsSCK(_sclk)) {
					//spi_port= &SPI1;
					uint32_t *pa = (uint32_t*)((void*)spi_port);
					_spi_hardware = (SPIClass::SPI_Hardware_t*)(void*)pa[1];
					_pkinetisk_spi = (KINETISK_SPI_t *)(void*)pa[0];
					_fifo_size = _spi_hardware->queue_size;		// remember the queue size
				}
				Serial.println("KEDEIRPI35_t3n: SPI1 automatically selected");
			} else if(spi_port == &SPI2){
				if (SPI2.pinIsMOSI(_mosi) && SPI2.pinIsMISO(_miso) && SPI2.pinIsSCK(_sclk)) {
					//spi_port= &SPI2;
					uint32_t *pa = (uint32_t*)((void*)spi_port);
					_spi_hardware = (SPIClass::SPI_Hardware_t*)(void*)pa[1];
					_pkinetisk_spi = (KINETISK_SPI_t *)(void*)pa[0];
					_fifo_size = _spi_hardware->queue_size;		// remember the queue size
				}
				Serial.println("KEDEIRPI35_t3n: SPI2 automatically selected");
			} else {
				Serial.println("SPI Port not supported");
			}
		#elif defined(__MK20DX256__)
			if(spi_port == &SPI){
				if (SPI.pinIsMOSI(_mosi) && SPI.pinIsMISO(_miso) && SPI.pinIsSCK(_sclk)) {
					//spi_port= &SPI;
					uint32_t *pa = (uint32_t*)((void*)spi_port);
					_spi_hardware = (SPIClass::SPI_Hardware_t*)(void*)pa[1];
					_pkinetisk_spi = (KINETISK_SPI_t *)(void*)pa[0];
					_fifo_size = _spi_hardware->queue_size;		// remember the queue size
				}
				Serial.println("KEDEIRPI35_t3n: SPI automatically selected");
			} else {
				Serial.println("SPI Port not supported");
			}
		#endif

		uint8_t mosi_sck_bad = false;
		if(!(spi_port->pinIsMOSI(_mosi)))  {
			Serial.print(" MOSI");
			mosi_sck_bad = true;
		}
		if (!spi_port->pinIsSCK(_sclk)) {
			Serial.print(" SCLK");
			mosi_sck_bad = true;
		}

		// Maybe allow us to limp with only MISO bad
		if(!(spi_port->pinIsMISO(_miso))) {
			Serial.print(" MISO");
			_miso = 0xff;	// set miso to 255 as flag it is bad
		}
		Serial.println();
		
		if (mosi_sck_bad) {
			Serial.print("KEDEIRPI35_t3n: Error not valid SPI pins:");
			return; // not valid pins...
		}
		
		Serial.printf("MOSI:%d MISO:%d SCK:%d\n\r", _mosi, _miso, _sclk);			
        spi_port->setMOSI(_mosi);
        if (_miso != 0xff) spi_port->setMISO(_miso);
        spi_port->setSCK(_sclk);
	}		

	
	spi_port->begin();
	_csport = portOutputRegister(_cs);
	_cspinmask = digitalPinToBitMask(_cs);
	pinMode(_cs, OUTPUT);	
	DIRECT_WRITE_HIGH(_csport, _cspinmask);

	//Serial.println("KEDEIRPI35_t3n: Error not DC is not valid hardware CS pin");
	_touchcsport = portOutputRegister(_touchcs);
	_touchcspinmask = digitalPinToBitMask(_touchcs);
	pinMode(_touchcs, OUTPUT);	
	DIRECT_WRITE_HIGH(_touchcsport, _touchcspinmask);
#elif defined(__IMXRT1052__) || defined(__IMXRT1062__)  // Teensy 4.x 
		if(spi_port == &SPI){
			if (SPI.pinIsMOSI(_mosi) && SPI.pinIsMISO(_miso) && SPI.pinIsSCK(_sclk)) {
				//spi_port= &SPI;
				uint32_t *pa = (uint32_t*)((void*)spi_port);
				_spi_hardware = (SPIClass::SPI_Hardware_t*)(void*)pa[1];
				_pimxrt_spi = (IMXRT_LPSPI_t *)(void*)pa[0];
			}
			Serial.println("KEDEIRPI35_t3n: (T4) SPI automatically selected");
		}
		#if defined(__IMXRT1062__)
			if(spi_port == &SPI1){
				if (SPI1.pinIsMOSI(_mosi) && SPI1.pinIsMISO(_miso) && SPI1.pinIsSCK(_sclk)) {
					//spi_port= &SPI;
					uint32_t *pa = (uint32_t*)((void*)spi_port);
					_spi_hardware = (SPIClass::SPI_Hardware_t*)(void*)pa[1];
					_pimxrt_spi = (IMXRT_LPSPI_t *)(void*)pa[0];
				}
				Serial.println("KEDEIRPI35_t3n: (T4) SPI1 automatically selected");
			} else if(spi_port == &SPI2){
				if (SPI2.pinIsMOSI(_mosi) && SPI2.pinIsMISO(_miso) && SPI2.pinIsSCK(_sclk)) {
					//spi_port= &SPI;
					uint32_t *pa = (uint32_t*)((void*)spi_port);
					_spi_hardware = (SPIClass::SPI_Hardware_t*)(void*)pa[1];
					_pimxrt_spi = (IMXRT_LPSPI_t *)(void*)pa[0];
				}
				Serial.println("KEDEIRPI35_t3n: (T4) SPI2 automatically selected");
			} else if(spi_port != &SPI){
				Serial.println("T4: SPI1/2 Port not supported");
				return;
			}
		#endif
	
		uint8_t mosi_sck_bad = false;
		if(!(spi_port->pinIsMOSI(_mosi)))  {
			Serial.print(" MOSI  "); Serial.println(_mosi);
			mosi_sck_bad = true;
		}
		if (!spi_port->pinIsSCK(_sclk)) {
			Serial.print(" SCLK  "); Serial.println(_sclk);
			mosi_sck_bad = true;
		}

		// Maybe allow us to limp with only MISO bad
		if(!(spi_port->pinIsMISO(_miso))) {
			Serial.print(" MISO  "); Serial.println(_miso);
			_miso = 0xff;	// set miso to 255 as flag it is bad
		}
		Serial.println();
		
		if (mosi_sck_bad) {
			Serial.print("KEDEIRPI35_t3n: Error not valid SPI pins:");
			return; // not valid pins...
		}
		
		Serial.printf("MOSI:%d MISO:%d SCK:%d\n\r", _mosi, _miso, _sclk);			
        spi_port->setMOSI(_mosi);
        if (_miso != 0xff) spi_port->setMISO(_miso);
        spi_port->setSCK(_sclk);
	
	_pending_rx_count = 0;
	spi_port->begin();
	_csport = portOutputRegister(_cs);
	_cspinmask = digitalPinToBitMask(_cs);
	pinMode(_cs, OUTPUT);	
	DIRECT_WRITE_HIGH(_csport, _cspinmask);
	_spi_tcr_current = _pimxrt_spi->TCR; // get the current TCR value 

	//Serial.println("KEDEIRPI35_t3n: Error not DC is not valid hardware CS pin");
	_touchcsport = portOutputRegister(_touchcs);
	_touchcspinmask = digitalPinToBitMask(_touchcs);
	pinMode(_touchcs, OUTPUT);	
	DIRECT_WRITE_HIGH(_touchcsport, _touchcspinmask);
#elif defined(KINETISL)
	if ((_mosi != 255) || (_miso != 255) || (_sclk != 255)) {
		// Lets verify that all of the specifid pins are valid... right now only care about MSOI and sclk... 
		if (! (((_mosi == 255) || spi_port->pinIsMOSI(_mosi)) && ((_sclk == 255) || spi_port->pinIsSCK(_sclk)))) {
			// one of those two pins are not valid, lets try to see if there is a valid one
			// In this case we will not check for 255 as we assume both most be specified...
			if (SPI.pinIsMOSI(_mosi) && SPI.pinIsSCK(_sclk)) {
				spi_port = &SPI;
			} else if (SPI1.pinIsMOSI(_mosi) && SPI1.pinIsSCK(_sclk)) {
				spi_port = &SPI1;
			} else {
				Serial.println("SPI Pins are not valid");
				return; 	// we will probably crash!
			}
		}

		// lets setup any non standard IO pins.
		if (_mosi != 255) spi_port->setMOSI(_mosi);
		if (_sclk != 255) spi_port->setSCK(_sclk);
		if (_miso != 255) {
			if (spi_port->pinIsMISO(_miso)) 
				spi_port->setMISO(_miso);
		}
	}
	uint32_t *pa = (uint32_t*)((void*)spi_port);
	_spi_hardware = (SPIClass::SPI_Hardware_t*)(void*)pa[1];
	_pkinetisl_spi = (KINETISL_SPI_t *)(void*)pa[0];

	pcs_data = 0;
	pcs_command = 0;
	pinMode(_cs, OUTPUT);
	_csport    = portOutputRegister(digitalPinToPort(_cs));
	_cspinmask = digitalPinToBitMask(_cs);
	*_csport |= _cspinmask;
	pinMode(_touchcs, OUTPUT);
	_touchcsport    = portOutputRegister(digitalPinToPort(_touchcs));
	_touchcspinmask = digitalPinToBitMask(_touchcs);
	*_touchcsport |= _touchcspinmask;
	_touchcspinAsserted = 0;

	spi_port->begin();
#endif
  // Do Reset stuff
  beginSPITransaction();
  spi_transmit32(0x00010000L);  //
  endSPITransaction();
  delay(50);

  beginSPITransaction();
  spi_transmit32(0x00000000L);
  endSPITransaction();
  delay(120);

  beginSPITransaction();
  spi_transmit32(0x00010000L);
  endSPITransaction();
  delay(50);
  //kedei 6.3 init sequence
  beginSPITransaction();
  lcd_cmd(0x00);
  delay(20);
  lcd_cmd(0x11);
  endSPITransaction();
  delay(120);
  beginSPITransaction();
  lcd_cmd(0x13);
  lcd_cmd(0xB4); lcd_data(0x00);
  lcd_cmd(0xC0); lcd_data(0x10); lcd_data(0x3D); lcd_data(0x00); lcd_data(0x02); lcd_data(0x11);
  lcd_cmd(0xC1); lcd_data(0x10);
  lcd_cmd(0xC8); lcd_data(0x00); lcd_data(0x30); lcd_data(0x36); lcd_data(0x45);
  lcd_data(0x04); lcd_data(0x16); lcd_data(0x37); lcd_data(0x75);
  lcd_data(0x77); lcd_data(0x54); lcd_data(0x0F); lcd_data(0x00);
  lcd_cmd(0xD0); lcd_data(0x07); lcd_data(0x40); lcd_data(0x1C);
  lcd_cmd(0xD1); lcd_data(0x00); lcd_data(0x18); lcd_data(0x1D);
  lcd_cmd(0xD2); lcd_data(0x01); lcd_data(0x11);
  lcd_cmd(0xC5); lcd_data(0x08);
  lcd_cmd(0x36); lcd_data(0x28);
  lcd_cmd(0x3A); lcd_data(0x05);
  lcd_cmd(0x2A); lcd_data(0x00); lcd_data(0x00); lcd_data(0x01); lcd_data(0x3F);
  lcd_cmd(0x2B); lcd_data(0x00); lcd_data(0x00); lcd_data(0x01); lcd_data(0xE0);
  endSPITransaction();
  delay(120);
  beginSPITransaction();
  lcd_cmd(0x29);
  delay(5);
  lcd_cmd(0x36); lcd_data(0x28);
  lcd_cmd(0x3A); lcd_data(0x55);
  lcd_cmd(0x2A); lcd_data(0x00); lcd_data(0x00); lcd_data(0x01); lcd_data(0x3F);
  lcd_cmd(0x2B); lcd_data(0x00); lcd_data(0x00); lcd_data(0x01); lcd_data(0xE0);
  endSPITransaction();
  delay(120);
  beginSPITransaction();
  lcd_cmd(0x21);
  lcd_cmd(0x29);
  delay(50);
  endSPITransaction();
  setRotation(0);
}





/*
This is the core graphics library for all our displays, providing a common
set of graphics primitives (points, lines, circles, etc.).  It needs to be
paired with a hardware-specific library for each display device we carry
(to handle the lower-level functions).

Adafruit invests time and resources providing this open source code, please
support Adafruit & open-source hardware by purchasing products from Adafruit!

Copyright (c) 2013 Adafruit Industries.  All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

- Redistributions of source code must retain the above copyright notice,
  this list of conditions and the following disclaimer.
- Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
*/

//#include "glcdfont.c"
extern "C" const unsigned char glcdfont[];

// Draw a circle outline
void KEDEIRPI35_t3::drawCircle(int16_t x0, int16_t y0, int16_t r,
    uint16_t color) {
  int16_t f = 1 - r;
  int16_t ddF_x = 1;
  int16_t ddF_y = -2 * r;
  int16_t x = 0;
  int16_t y = r;

  drawPixel(x0  , y0+r, color);
  drawPixel(x0  , y0-r, color);
  drawPixel(x0+r, y0  , color);
  drawPixel(x0-r, y0  , color);

  while (x<y) {
    if (f >= 0) {
      y--;
      ddF_y += 2;
      f += ddF_y;
    }
    x++;
    ddF_x += 2;
    f += ddF_x;

    drawPixel(x0 + x, y0 + y, color);
    drawPixel(x0 - x, y0 + y, color);
    drawPixel(x0 + x, y0 - y, color);
    drawPixel(x0 - x, y0 - y, color);
    drawPixel(x0 + y, y0 + x, color);
    drawPixel(x0 - y, y0 + x, color);
    drawPixel(x0 + y, y0 - x, color);
    drawPixel(x0 - y, y0 - x, color);
  }
}

void KEDEIRPI35_t3::drawCircleHelper( int16_t x0, int16_t y0,
               int16_t r, uint8_t cornername, uint16_t color) {
  int16_t f     = 1 - r;
  int16_t ddF_x = 1;
  int16_t ddF_y = -2 * r;
  int16_t x     = 0;
  int16_t y     = r;

  while (x<y) {
    if (f >= 0) {
      y--;
      ddF_y += 2;
      f     += ddF_y;
    }
    x++;
    ddF_x += 2;
    f     += ddF_x;
    if (cornername & 0x4) {
      drawPixel(x0 + x, y0 + y, color);
      drawPixel(x0 + y, y0 + x, color);
    }
    if (cornername & 0x2) {
      drawPixel(x0 + x, y0 - y, color);
      drawPixel(x0 + y, y0 - x, color);
    }
    if (cornername & 0x8) {
      drawPixel(x0 - y, y0 + x, color);
      drawPixel(x0 - x, y0 + y, color);
    }
    if (cornername & 0x1) {
      drawPixel(x0 - y, y0 - x, color);
      drawPixel(x0 - x, y0 - y, color);
    }
  }
}

void KEDEIRPI35_t3::fillCircle(int16_t x0, int16_t y0, int16_t r,
			      uint16_t color) {
  drawFastVLine(x0, y0-r, 2*r+1, color);
  fillCircleHelper(x0, y0, r, 3, 0, color);
}

// Used to do circles and roundrects
void KEDEIRPI35_t3::fillCircleHelper(int16_t x0, int16_t y0, int16_t r,
    uint8_t cornername, int16_t delta, uint16_t color) {

  int16_t f     = 1 - r;
  int16_t ddF_x = 1;
  int16_t ddF_y = -2 * r;
  int16_t x     = 0;
  int16_t y     = r;

  while (x<y) {
    if (f >= 0) {
      y--;
      ddF_y += 2;
      f     += ddF_y;
    }
    x++;
    ddF_x += 2;
    f     += ddF_x;

    if (cornername & 0x1) {
      drawFastVLine(x0+x, y0-y, 2*y+1+delta, color);
      drawFastVLine(x0+y, y0-x, 2*x+1+delta, color);
    }
    if (cornername & 0x2) {
      drawFastVLine(x0-x, y0-y, 2*y+1+delta, color);
      drawFastVLine(x0-y, y0-x, 2*x+1+delta, color);
    }
  }
}


// Bresenham's algorithm - thx wikpedia
void KEDEIRPI35_t3::drawLine(int16_t x0, int16_t y0,
	int16_t x1, int16_t y1, uint16_t color)
{
	if (y0 == y1) {
		if (x1 > x0) {
			drawFastHLine(x0, y0, x1 - x0 + 1, color);
		} else if (x1 < x0) {
			drawFastHLine(x1, y0, x0 - x1 + 1, color);
		} else {
			drawPixel(x0, y0, color);
		}
		return;
	} else if (x0 == x1) {
		if (y1 > y0) {
			drawFastVLine(x0, y0, y1 - y0 + 1, color);
		} else {
			drawFastVLine(x0, y1, y0 - y1 + 1, color);
		}
		return;
	}

	bool steep = abs(y1 - y0) > abs(x1 - x0);
	if (steep) {
		swap(x0, y0);
		swap(x1, y1);
	}
	if (x0 > x1) {
		swap(x0, x1);
		swap(y0, y1);
	}

	int16_t dx, dy;
	dx = x1 - x0;
	dy = abs(y1 - y0);

	int16_t err = dx / 2;
	int16_t ystep;

	if (y0 < y1) {
		ystep = 1;
	} else {
		ystep = -1;
	}

	beginSPITransaction();
	int16_t xbegin = x0;
	if (steep) {
		for (; x0<=x1; x0++) {
			err -= dy;
			if (err < 0) {
				int16_t len = x0 - xbegin;
				if (len) {
					VLine(y0, xbegin, len + 1, color);
				} else {
					Pixel(y0, x0, color);
				}
				xbegin = x0 + 1;
				y0 += ystep;
				err += dx;
			}
		}
		if (x0 > xbegin + 1) {
			VLine(y0, xbegin, x0 - xbegin, color);
		}

	} else {
		for (; x0<=x1; x0++) {
			err -= dy;
			if (err < 0) {
				int16_t len = x0 - xbegin;
				if (len) {
					HLine(xbegin, y0, len + 1, color);
				} else {
					Pixel(x0, y0, color);
				}
				xbegin = x0 + 1;
				y0 += ystep;
				err += dx;
			}
		}
		if (x0 > xbegin + 1) {
			HLine(xbegin, y0, x0 - xbegin, color);
		}
	}
	endSPITransaction();
}

// Draw a rectangle
void KEDEIRPI35_t3::drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
	beginSPITransaction();
	HLine(x, y, w, color);
	HLine(x, y+h-1, w, color);
	VLine(x, y, h, color);
	VLine(x+w-1, y, h, color);
	endSPITransaction();
}

// Draw a rounded rectangle
void KEDEIRPI35_t3::drawRoundRect(int16_t x, int16_t y, int16_t w,
  int16_t h, int16_t r, uint16_t color) {
  // smarter version
  drawFastHLine(x+r  , y    , w-2*r, color); // Top
  drawFastHLine(x+r  , y+h-1, w-2*r, color); // Bottom
  drawFastVLine(x    , y+r  , h-2*r, color); // Left
  drawFastVLine(x+w-1, y+r  , h-2*r, color); // Right
  // draw four corners
  drawCircleHelper(x+r    , y+r    , r, 1, color);
  drawCircleHelper(x+w-r-1, y+r    , r, 2, color);
  drawCircleHelper(x+w-r-1, y+h-r-1, r, 4, color);
  drawCircleHelper(x+r    , y+h-r-1, r, 8, color);
}

// Fill a rounded rectangle
void KEDEIRPI35_t3::fillRoundRect(int16_t x, int16_t y, int16_t w,
				 int16_t h, int16_t r, uint16_t color) {
  // smarter version
  fillRect(x+r, y, w-2*r, h, color);

  // draw four corners
  fillCircleHelper(x+w-r-1, y+r, r, 1, h-2*r-1, color);
  fillCircleHelper(x+r    , y+r, r, 2, h-2*r-1, color);
}

// Draw a triangle
void KEDEIRPI35_t3::drawTriangle(int16_t x0, int16_t y0,
				int16_t x1, int16_t y1,
				int16_t x2, int16_t y2, uint16_t color) {
  drawLine(x0, y0, x1, y1, color);
  drawLine(x1, y1, x2, y2, color);
  drawLine(x2, y2, x0, y0, color);
}

// Fill a triangle
void KEDEIRPI35_t3::fillTriangle ( int16_t x0, int16_t y0,
				  int16_t x1, int16_t y1,
				  int16_t x2, int16_t y2, uint16_t color) {

  int16_t a, b, y, last;

  // Sort coordinates by Y order (y2 >= y1 >= y0)
  if (y0 > y1) {
    swap(y0, y1); swap(x0, x1);
  }
  if (y1 > y2) {
    swap(y2, y1); swap(x2, x1);
  }
  if (y0 > y1) {
    swap(y0, y1); swap(x0, x1);
  }

  if(y0 == y2) { // Handle awkward all-on-same-line case as its own thing
    a = b = x0;
    if(x1 < a)      a = x1;
    else if(x1 > b) b = x1;
    if(x2 < a)      a = x2;
    else if(x2 > b) b = x2;
    drawFastHLine(a, y0, b-a+1, color);
    return;
  }

  int16_t
    dx01 = x1 - x0,
    dy01 = y1 - y0,
    dx02 = x2 - x0,
    dy02 = y2 - y0,
    dx12 = x2 - x1,
    dy12 = y2 - y1,
    sa   = 0,
    sb   = 0;

  // For upper part of triangle, find scanline crossings for segments
  // 0-1 and 0-2.  If y1=y2 (flat-bottomed triangle), the scanline y1
  // is included here (and second loop will be skipped, avoiding a /0
  // error there), otherwise scanline y1 is skipped here and handled
  // in the second loop...which also avoids a /0 error here if y0=y1
  // (flat-topped triangle).
  if(y1 == y2) last = y1;   // Include y1 scanline
  else         last = y1-1; // Skip it

  for(y=y0; y<=last; y++) {
    a   = x0 + sa / dy01;
    b   = x0 + sb / dy02;
    sa += dx01;
    sb += dx02;
    /* longhand:
    a = x0 + (x1 - x0) * (y - y0) / (y1 - y0);
    b = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
    */
    if(a > b) swap(a,b);
    drawFastHLine(a, y, b-a+1, color);
  }

  // For lower part of triangle, find scanline crossings for segments
  // 0-2 and 1-2.  This loop is skipped if y1=y2.
  sa = dx12 * (y - y1);
  sb = dx02 * (y - y0);
  for(; y<=y2; y++) {
    a   = x1 + sa / dy12;
    b   = x0 + sb / dy02;
    sa += dx12;
    sb += dx02;
    /* longhand:
    a = x1 + (x2 - x1) * (y - y1) / (y2 - y1);
    b = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
    */
    if(a > b) swap(a,b);
    drawFastHLine(a, y, b-a+1, color);
  }
}

void KEDEIRPI35_t3::drawBitmap(int16_t x, int16_t y,
			      const uint8_t *bitmap, int16_t w, int16_t h,
			      uint16_t color) {

  int16_t i, j, byteWidth = (w + 7) / 8;

  for(j=0; j<h; j++) {
    for(i=0; i<w; i++ ) {
      if(pgm_read_byte(bitmap + j * byteWidth + i / 8) & (128 >> (i & 7))) {
	drawPixel(x+i, y+j, color);
      }
    }
  }
}

size_t KEDEIRPI35_t3::write(uint8_t c)
{
	if (font) {
		if (c == '\n') {
			cursor_y += font->line_space;
			if(scrollEnable && isWritingScrollArea){
				cursor_x  = scroll_x;
			}else{
				cursor_x  = 0;
			}
		} else {
			drawFontChar(c);
		}
	} else {
		if (c == '\n') {
			cursor_y += textsize*8;
			if(scrollEnable && isWritingScrollArea){
				cursor_x  = scroll_x;
			}else{
				cursor_x  = 0;
			}
		} else if (c == '\r') {
			// skip em
		} else {
			if(scrollEnable && isWritingScrollArea && (cursor_y > (scroll_y+scroll_height - textsize*8))){
				scrollTextArea(textsize*8);
				cursor_y -= textsize*8;
				cursor_x = scroll_x;
			}
			drawChar(cursor_x, cursor_y, c, textcolor, textbgcolor, textsize);
			cursor_x += textsize*6;
			if(wrap && scrollEnable && isWritingScrollArea && (cursor_x > (scroll_x+scroll_width - textsize*6))){
				cursor_y += textsize*8;
				cursor_x = scroll_x;
			}
			else if (wrap && (cursor_x > (_width - textsize*6))) {
				cursor_y += textsize*8;
				cursor_x = 0;
			}
		}
	}
	return 1;
}

// Draw a character
void KEDEIRPI35_t3::drawChar(int16_t x, int16_t y, unsigned char c,
			    uint16_t fgcolor, uint16_t bgcolor, uint8_t size)
{
	if((x >= _width)            || // Clip right
	   (y >= _height)           || // Clip bottom
	   ((x + 6 * size - 1) < 0) || // Clip left  TODO: is this correct?
	   ((y + 8 * size - 1) < 0))   // Clip top   TODO: is this correct?
		return;

	if (fgcolor == bgcolor) {
		// This transparent approach is only about 20% faster
		if (size == 1) {
			uint8_t mask = 0x01;
			int16_t xoff, yoff;
			for (yoff=0; yoff < 8; yoff++) {
				uint8_t line = 0;
				for (xoff=0; xoff < 5; xoff++) {
					if (glcdfont[c * 5 + xoff] & mask) line |= 1;
					line <<= 1;
				}
				line >>= 1;
				xoff = 0;
				while (line) {
					if (line == 0x1F) {
						drawFastHLine(x + xoff, y + yoff, 5, fgcolor);
						break;
					} else if (line == 0x1E) {
						drawFastHLine(x + xoff, y + yoff, 4, fgcolor);
						break;
					} else if ((line & 0x1C) == 0x1C) {
						drawFastHLine(x + xoff, y + yoff, 3, fgcolor);
						line <<= 4;
						xoff += 4;
					} else if ((line & 0x18) == 0x18) {
						drawFastHLine(x + xoff, y + yoff, 2, fgcolor);
						line <<= 3;
						xoff += 3;
					} else if ((line & 0x10) == 0x10) {
						drawPixel(x + xoff, y + yoff, fgcolor);
						line <<= 2;
						xoff += 2;
					} else {
						line <<= 1;
						xoff += 1;
					}
				}
				mask = mask << 1;
			}
		} else {
			uint8_t mask = 0x01;
			int16_t xoff, yoff;
			for (yoff=0; yoff < 8; yoff++) {
				uint8_t line = 0;
				for (xoff=0; xoff < 5; xoff++) {
					if (glcdfont[c * 5 + xoff] & mask) line |= 1;
					line <<= 1;
				}
				line >>= 1;
				xoff = 0;
				while (line) {
					if (line == 0x1F) {
						fillRect(x + xoff * size, y + yoff * size,
							5 * size, size, fgcolor);
						break;
					} else if (line == 0x1E) {
						fillRect(x + xoff * size, y + yoff * size,
							4 * size, size, fgcolor);
						break;
					} else if ((line & 0x1C) == 0x1C) {
						fillRect(x + xoff * size, y + yoff * size,
							3 * size, size, fgcolor);
						line <<= 4;
						xoff += 4;
					} else if ((line & 0x18) == 0x18) {
						fillRect(x + xoff * size, y + yoff * size,
							2 * size, size, fgcolor);
						line <<= 3;
						xoff += 3;
					} else if ((line & 0x10) == 0x10) {
						fillRect(x + xoff * size, y + yoff * size,
							size, size, fgcolor);
						line <<= 2;
						xoff += 2;
					} else {
						line <<= 1;
						xoff += 1;
					}
				}
				mask = mask << 1;
			}
		}
	} else {
		// This solid background approach is about 5 time faster
		beginSPITransaction();
		setAddr(x, y, x + 6 * size - 1, y + 8 * size - 1);
		lcd_cmd(KEDEIRPI35_RAMWR);
		uint8_t xr, yr;
		uint8_t mask = 0x01;
		uint16_t color;
		for (y=0; y < 8; y++) {
			for (yr=0; yr < size; yr++) {
				for (x=0; x < 5; x++) {
					if (glcdfont[c * 5 + x] & mask) {
						color = fgcolor;
					} else {
						color = bgcolor;
					}
					for (xr=0; xr < size; xr++) {
						lcd_color(color);
					}
				}
				for (xr=0; xr < size; xr++) {
					lcd_color(bgcolor);
				}
			}
			mask = mask << 1;
		}
		endSPITransaction();
	}
}

static uint32_t fetchbit(const uint8_t *p, uint32_t index)
{
	if (p[index >> 3] & (1 << (7 - (index & 7)))) return 1;
	return 0;
}

static uint32_t fetchbits_unsigned(const uint8_t *p, uint32_t index, uint32_t required)
{
	uint32_t val = 0;
	do {
		uint8_t b = p[index >> 3];
		uint32_t avail = 8 - (index & 7);
		if (avail <= required) {
			val <<= avail;
			val |= b & ((1 << avail) - 1);
			index += avail;
			required -= avail;
		} else {
			b >>= avail - required;
			val <<= required;
			val |= b & ((1 << required) - 1);
			break;
		}
	} while (required);
	return val;
}

static uint32_t fetchbits_signed(const uint8_t *p, uint32_t index, uint32_t required)
{
	uint32_t val = fetchbits_unsigned(p, index, required);
	if (val & (1 << (required - 1))) {
		return (int32_t)val - (1 << required);
	}
	return (int32_t)val;
}


void KEDEIRPI35_t3::drawFontChar(unsigned int c)
{
	uint32_t bitoffset;
	const uint8_t *data;

	//Serial.printf("drawFontChar(%c) %d\n", c, c);

	if (c >= font->index1_first && c <= font->index1_last) {
		bitoffset = c - font->index1_first;
		bitoffset *= font->bits_index;
	} else if (c >= font->index2_first && c <= font->index2_last) {
		bitoffset = c - font->index2_first + font->index1_last - font->index1_first + 1;
		bitoffset *= font->bits_index;
	} else if (font->unicode) {
		return; // TODO: implement sparse unicode
	} else {
		return;
	}
	//Serial.printf("  index =  %d\n", fetchbits_unsigned(font->index, bitoffset, font->bits_index));
	data = font->data + fetchbits_unsigned(font->index, bitoffset, font->bits_index);

	uint32_t encoding = fetchbits_unsigned(data, 0, 3);
	if (encoding != 0) return;
	uint32_t width = fetchbits_unsigned(data, 3, font->bits_width);
	bitoffset = font->bits_width + 3;
	uint32_t height = fetchbits_unsigned(data, bitoffset, font->bits_height);
	bitoffset += font->bits_height;
	//Serial.printf("  size =   %d,%d\n", width, height);
	//Serial.printf("  line space = %d\n", font->line_space);

	int32_t xoffset = fetchbits_signed(data, bitoffset, font->bits_xoffset);
	bitoffset += font->bits_xoffset;
	int32_t yoffset = fetchbits_signed(data, bitoffset, font->bits_yoffset);
	bitoffset += font->bits_yoffset;
	//Serial.printf("  offset = %d,%d\n", xoffset, yoffset);

	uint32_t delta = fetchbits_unsigned(data, bitoffset, font->bits_delta);
	bitoffset += font->bits_delta;
	//Serial.printf("  delta =  %d\n", delta);

	//Serial.printf("  cursor = %d,%d\n", cursor_x, cursor_y);

	 //horizontally, we draw every pixel, or none at all
	if (cursor_x < 0) cursor_x = 0;
	int32_t origin_x = cursor_x + xoffset;
	if (origin_x < 0) {
		cursor_x -= xoffset;
		origin_x = 0;
	}
	if (origin_x + (int)width > _width) {
		if (!wrap) return;
		origin_x = 0;
		if (xoffset >= 0) {
			cursor_x = 0;
		} else {
			cursor_x = -xoffset;
		}
		cursor_y += font->line_space;
	}
	if(wrap && scrollEnable && isWritingScrollArea && ((origin_x + (int)width) > (scroll_x+scroll_width))){
    	origin_x = 0;
		if (xoffset >= 0) {
			cursor_x = scroll_x;
		} else {
			cursor_x = -xoffset;
		}
		cursor_y += font->line_space;
    }
	
	if(scrollEnable && isWritingScrollArea && (cursor_y > (scroll_y+scroll_height - font->cap_height))){
		scrollTextArea(font->line_space);
		cursor_y -= font->line_space;
		cursor_x = scroll_x;
	} 
	if (cursor_y >= _height) return;

	// vertically, the top and/or bottom can be clipped
	int32_t origin_y = cursor_y + font->cap_height - height - yoffset;
	//Serial.printf("  origin = %d,%d\n", origin_x, origin_y);

	// TODO: compute top skip and number of lines
	int32_t linecount = height;
	//uint32_t loopcount = 0;
	int32_t y = origin_y;
	bool opaque = (textbgcolor != textcolor);


	// Going to try a fast Opaque method which works similar to drawChar, which is near the speed of writerect
	if (!opaque) {
		while (linecount > 0) {
			//Serial.printf("    linecount = %d\n", linecount);
			uint32_t n = 1;
			if (fetchbit(data, bitoffset++) != 0) {
				n = fetchbits_unsigned(data, bitoffset, 3) + 2;
				bitoffset += 3;
			}
			uint32_t x = 0;
			do {
				int32_t xsize = width - x;
				if (xsize > 32) xsize = 32;
				uint32_t bits = fetchbits_unsigned(data, bitoffset, xsize);
				//Serial.printf("    multi line %d %d %x\n", n, x, bits);
				drawFontBits(opaque, bits, xsize, origin_x + x, y, n);
				bitoffset += xsize;
				x += xsize;
			} while (x < width);


			y += n;
			linecount -= n;
			//if (++loopcount > 100) {
				//Serial.println("     abort draw loop");
				//break;
			//}
		}
	} else {
		// Now opaque mode... 
		// Now write out background color for the number of rows above the above the character
		// figure out bounding rectangle... 
		// In this mode we need to update to use the offset and bounding rectangles as we are doing it it direct.
		// also update the Origin 
		int cursor_x_origin = cursor_x + _originx;
		int cursor_y_origin = cursor_y + _originy;
		origin_x += _originx;
		origin_y += _originy;



		int start_x = (origin_x < cursor_x_origin) ? origin_x : cursor_x_origin; 	
		if (start_x < 0) start_x = 0;
		
		int start_y = (origin_y < cursor_y_origin) ? origin_y : cursor_y_origin; 
		if (start_y < 0) start_y = 0;
		int end_x = cursor_x_origin + delta; 
		if ((origin_x + (int)width) > end_x)
			end_x = origin_x + (int)width;
		if (end_x >= _displayclipx2)  end_x = _displayclipx2;	
		int end_y = cursor_y_origin + font->line_space; 
		if ((origin_y + (int)height) > end_y)
			end_y = origin_y + (int)height;
		if (end_y >= _displayclipy2) end_y = _displayclipy2;	
		end_x--;	// setup to last one we draw
		end_y--;
		int start_x_min = (start_x >= _displayclipx1) ? start_x : _displayclipx1;
		int start_y_min = (start_y >= _displayclipy1) ? start_y : _displayclipy1;

		// See if anything is in the display area.
		if((end_x < _displayclipx1) ||(start_x >= _displayclipx2) || (end_y < _displayclipy1) || (start_y >= _displayclipy2)) {
			cursor_x += delta;	// could use goto or another indent level...
		 	return;
		}
/*
		Serial.printf("drawFontChar(%c) %d\n", c, c);
		Serial.printf("  size =   %d,%d\n", width, height);
		Serial.printf("  line space = %d\n", font->line_space);
		Serial.printf("  offset = %d,%d\n", xoffset, yoffset);
		Serial.printf("  delta =  %d\n", delta);
		Serial.printf("  cursor = %d,%d\n", cursor_x, cursor_y);
		Serial.printf("  origin = %d,%d\n", origin_x, origin_y);

		Serial.printf("  Bounding: (%d, %d)-(%d, %d)\n", start_x, start_y, end_x, end_y);
		Serial.printf("  mins (%d %d),\n", start_x_min, start_y_min);
*/
		#ifdef ENABLE_KEDEIRPI35_FRAMEBUFFER
		if (_use_fbtft) {
			uint8_t * pfbPixel_row = &_pfbtft[ start_y*_width + start_x];
			uint8_t * pfbPixel;
			uint8_t textbgcolor_index = mapColorToPalletIndex(textbgcolor);
			uint8_t textcolor_index = mapColorToPalletIndex(textcolor);
			int screen_y = start_y;
			int screen_x;

			while (screen_y < origin_y) {
				pfbPixel = pfbPixel_row;
				// only output if this line is within the clipping region.
				if ((screen_y >= _displayclipy1) && (screen_y < _displayclipy2)) {
					for (screen_x = start_x; screen_x <= end_x; screen_x++) {
						if (screen_x >= _displayclipx1) {
							*pfbPixel = textbgcolor_index;
						}
						pfbPixel++;
					}
				}
				screen_y++;
				pfbPixel_row += _width;
			}

			// Now lets process each of the data lines. 
			screen_y = origin_y;

			while (linecount > 0) {
				//Serial.printf("    linecount = %d\n", linecount);
				uint32_t b = fetchbit(data, bitoffset++);
				uint32_t n;
				if (b == 0) {
					//Serial.println("Single");
					n = 1;
				} else {
					//Serial.println("Multi");
					n = fetchbits_unsigned(data, bitoffset, 3) + 2;
					bitoffset += 3;
				}
				uint32_t bitoffset_row_start = bitoffset;
				while (n--) {
					pfbPixel = pfbPixel_row;
					if ((screen_y >= _displayclipy1) && (screen_y < _displayclipy2)) {
						bitoffset = bitoffset_row_start;	// we will work through these bits maybe multiple times

						for (screen_x = start_x; screen_x < origin_x; screen_x++) {
							if (screen_x >= _displayclipx1) {
								*pfbPixel = textbgcolor_index;
							} // make sure not clipped
							pfbPixel++;
						}
					}

					screen_x = origin_x;
					uint32_t x = 0;
					do {
						uint32_t xsize = width - x;
						if (xsize > 32) xsize = 32;
						uint32_t bits = fetchbits_unsigned(data, bitoffset, xsize);
						uint32_t bit_mask = 1 << (xsize-1);
						//Serial.printf(" %d %d %x %x\n", x, xsize, bits, bit_mask);
						if ((screen_y >= _displayclipy1) && (screen_y < _displayclipy2)) {
							while (bit_mask && (screen_x <= end_x)) {
								if ((screen_x >= _displayclipx1) && (screen_x < _displayclipx2)) {
									*pfbPixel = (bits & bit_mask) ? textcolor_index : textbgcolor_index;
								}
								pfbPixel++;	
								bit_mask = bit_mask >> 1;
								screen_x++;	// increment our pixel position. 
							}
						}
							bitoffset += xsize;
						x += xsize;
					} while (x < width);
					if ((screen_y >= _displayclipy1) && (screen_y < _displayclipy2)) {
						// output bg color and right hand side
						while (screen_x++ <= end_x) {
							*pfbPixel++ = textbgcolor_index;
						}
					}			 
		 			screen_y++;
					pfbPixel_row += _width;
					linecount--;
				}
			}

			// clear below character
	 		while (screen_y++ <= end_y) {
				if ((screen_y >= _displayclipy1) && (screen_y < _displayclipy2)) {
					pfbPixel = pfbPixel_row;
					for (screen_x = start_x; screen_x <= end_x; screen_x++) {
						if (screen_x >= _displayclipx1) {
							*pfbPixel = textbgcolor_index;
						}
						pfbPixel++;
					}
				}
				pfbPixel_row += _width;
			}

		} else 
		#endif
		{
			beginSPITransaction();
			//Serial.printf("SetAddr %d %d %d %d\n", start_x_min, start_y_min, end_x, end_y);
			// output rectangle we are updating... We have already clipped end_x/y, but not yet start_x/y
			setAddr( start_x_min, start_y_min, end_x, end_y);
			lcd_cmd(KEDEIRPI35_RAMWR);
			int screen_y = start_y_min;
			int screen_x;
			while (screen_y < origin_y) {
				for (screen_x = start_x_min; screen_x <= end_x; screen_x++) {
					lcd_color(textbgcolor);
				}
				screen_y++;
			}

			// Now lets process each of the data lines. 
			screen_y = origin_y;
			while (linecount > 0) {
				//Serial.printf("    linecount = %d\n", linecount);
				uint32_t b = fetchbit(data, bitoffset++);
				uint32_t n;
				if (b == 0) {
					//Serial.println("    Single");
					n = 1;
				} else {
					//Serial.println("    Multi");
					n = fetchbits_unsigned(data, bitoffset, 3) + 2;
					bitoffset += 3;
				}
				uint32_t bitoffset_row_start = bitoffset;
				while (n--) {
					// do some clipping here. 
					bitoffset = bitoffset_row_start;	// we will work through these bits maybe multiple times
					// We need to handle case where some of the bits may not be visible, but we still need to
					// read through them
					//Serial.printf("y:%d  %d %d %d %d\n", screen_y, start_x, origin_x, _displayclipx1, _displayclipx2);
					if ((screen_y >= _displayclipy1) && (screen_y < _displayclipy2)) {
						for (screen_x = start_x; screen_x < origin_x; screen_x++) {
							if ((screen_x >= _displayclipx1) && (screen_x < _displayclipx2)) {
								//Serial.write('-');
								lcd_color(textbgcolor);
							}
						}
					}	
					uint32_t x = 0;
					screen_x = origin_x;
					do {
						uint32_t xsize = width - x;
						if (xsize > 32) xsize = 32;
						uint32_t bits = fetchbits_unsigned(data, bitoffset, xsize);
						uint32_t bit_mask = 1 << (xsize-1);
						//Serial.printf("     %d %d %x %x - ", x, xsize, bits, bit_mask);
						if ((screen_y >= _displayclipy1) && (screen_y < _displayclipy2)) {
							while (bit_mask) {
								if ((screen_x >= _displayclipx1) && (screen_x < _displayclipx2)) {
									lcd_color((bits & bit_mask) ? textcolor : textbgcolor);
									//Serial.write((bits & bit_mask) ? '*' : '.');
								}
								bit_mask = bit_mask >> 1;
								screen_x++ ; // Current actual screen X
							}
							//Serial.println();
							bitoffset += xsize;
						}
						x += xsize;
					} while (x < width) ;
					if ((screen_y >= _displayclipy1) && (screen_y < _displayclipy2)) {
						// output bg color and right hand side
						while (screen_x++ <= end_x) {
							lcd_color(textbgcolor);
							//Serial.write('+');
						}
						//Serial.println();
					}
		 			screen_y++;
					linecount--;
				}
			}

			// clear below character - note reusing xcreen_x for this
			screen_x = (end_y + 1 - screen_y) * (end_x + 1 - start_x_min); // How many bytes we need to still output
			//Serial.printf("Clear Below: %d\n", screen_x);
			while (screen_x-- > 1) {
				lcd_color(textbgcolor);
			}
			lcd_color(textbgcolor);
			endSPITransaction();
		}

	}
	// Increment to setup for the next character.
	cursor_x += delta;

}

//strPixelLen			- gets pixel length of given ASCII string
int16_t KEDEIRPI35_t3::strPixelLen(char * str)
{
//	Serial.printf("strPixelLen %s\n", str);
	if (!str) return(0);
	uint16_t len=0, maxlen=0;
	while (*str)
	{
		if (*str=='\n')
		{
			if ( len > maxlen )
			{
				maxlen=len;
				len=0;
			}
		}
		else
		{
			if (!font)
			{
				len+=textsize*6;
			}
			else
			{

				uint32_t bitoffset;
				const uint8_t *data;
				uint16_t c = *str;

//				Serial.printf("char %c(%d)\n", c,c);

				if (c >= font->index1_first && c <= font->index1_last) {
					bitoffset = c - font->index1_first;
					bitoffset *= font->bits_index;
				} else if (c >= font->index2_first && c <= font->index2_last) {
					bitoffset = c - font->index2_first + font->index1_last - font->index1_first + 1;
					bitoffset *= font->bits_index;
				} else if (font->unicode) {
					continue;
				} else {
					continue;
				}
				//Serial.printf("  index =  %d\n", fetchbits_unsigned(font->index, bitoffset, font->bits_index));
				data = font->data + fetchbits_unsigned(font->index, bitoffset, font->bits_index);

				uint32_t encoding = fetchbits_unsigned(data, 0, 3);
				if (encoding != 0) continue;
//				uint32_t width = fetchbits_unsigned(data, 3, font->bits_width);
//				Serial.printf("  width =  %d\n", width);
				bitoffset = font->bits_width + 3;
				bitoffset += font->bits_height;

//				int32_t xoffset = fetchbits_signed(data, bitoffset, font->bits_xoffset);
//				Serial.printf("  xoffset =  %d\n", xoffset);
				bitoffset += font->bits_xoffset;
				bitoffset += font->bits_yoffset;

				uint32_t delta = fetchbits_unsigned(data, bitoffset, font->bits_delta);
				bitoffset += font->bits_delta;
//				Serial.printf("  delta =  %d\n", delta);

				len += delta;//+width-xoffset;
//				Serial.printf("  len =  %d\n", len);
				if ( len > maxlen )
				{
					maxlen=len;
//					Serial.printf("  maxlen =  %d\n", maxlen);
				}
			
			}
		}
		str++;
	}
//	Serial.printf("Return  maxlen =  %d\n", maxlen);
	return( maxlen );
}

void KEDEIRPI35_t3::drawFontBits(bool opaque, uint32_t bits, uint32_t numbits, int32_t x, int32_t y, uint32_t repeat)
{
	if (bits == 0) {
		if (opaque) {
			fillRect(x, y, numbits, repeat, textbgcolor);
		}
	} else {
		int32_t x1 = x;
		uint32_t n = numbits;
		int w;
		int bgw;

		w = 0;
		bgw = 0;

		do {
			n--;
			if (bits & (1 << n)) {
				if (bgw>0) {
					if (opaque) {
						fillRect(x1 - bgw, y, bgw, repeat, textbgcolor);
					}
					bgw=0;
				}
				w++;
			} else {
				if (w>0) {
					fillRect(x1 - w, y, w, repeat, textcolor);
					w = 0;
				}
				bgw++;
			}
			x1++;
		} while (n > 0);

		if (w > 0) {
			fillRect(x1 - w, y, w, repeat, textcolor);
		}

		if (bgw > 0) {
			if (opaque) {
				fillRect(x1 - bgw, y, bgw, repeat, textbgcolor);
			}
		}
	}
}

void KEDEIRPI35_t3::setCursor(int16_t x, int16_t y) {
	if (x < 0) x = 0;
	else if (x >= _width) x = _width - 1;
	cursor_x = x;
	if (y < 0) y = 0;
	else if (y >= _height) y = _height - 1;
	cursor_y = y;
	
	if(x>=scroll_x && x<=(scroll_x+scroll_width) && y>=scroll_y && y<=(scroll_y+scroll_height)){
		isWritingScrollArea	= true;
	} else {
		isWritingScrollArea = false;
	}
}

void KEDEIRPI35_t3::getCursor(int16_t *x, int16_t *y) {
  *x = cursor_x;
  *y = cursor_y;
}

void KEDEIRPI35_t3::setTextSize(uint8_t s) {
  textsize = (s > 0) ? s : 1;
}

uint8_t KEDEIRPI35_t3::getTextSize() {
	return textsize;
}

void KEDEIRPI35_t3::setTextColor(uint16_t c) {
  // For 'transparent' background, we'll set the bg
  // to the same as fg instead of using a flag
  textcolor = textbgcolor = c;
}

void KEDEIRPI35_t3::setTextColor(uint16_t c, uint16_t b) {
  textcolor   = c;
  textbgcolor = b;
}

void KEDEIRPI35_t3::setTextWrap(boolean w) {
  wrap = w;
}

boolean KEDEIRPI35_t3::getTextWrap()
{
	return wrap;
}

uint8_t KEDEIRPI35_t3::getRotation(void) {
  return rotation;
}

void KEDEIRPI35_t3::sleep(bool enable) {
#ifdef LATER
	beginSPITransaction();
	if (enable) {
		writecommand_cont(KEDEIRPI35_DISPOFF);		
		writecommand_last(KEDEIRPI35_SLPIN);	
		  endSPITransaction();
	} else {
		writecommand_cont(KEDEIRPI35_DISPON);
		writecommand_last(KEDEIRPI35_SLPOUT);
		endSPITransaction();
		delay(5);
	}
#endif	
}


/***************************************************************************************
** Function name:           setTextDatum
** Description:             Set the text position reference datum
***************************************************************************************/
void KEDEIRPI35_t3::setTextDatum(uint8_t d)
{
  textdatum = d;
}


/***************************************************************************************
** Function name:           drawNumber
** Description:             draw a long integer
***************************************************************************************/
int16_t KEDEIRPI35_t3::drawNumber(long long_num, int poX, int poY)
{
  char str[14];
  ltoa(long_num, str, 10);
  return drawString(str, poX, poY);
}


int16_t KEDEIRPI35_t3::drawFloat(float floatNumber, int dp, int poX, int poY)
{
  char str[14];               // Array to contain decimal string
  uint8_t ptr = 0;            // Initialise pointer for array
  int8_t  digits = 1;         // Count the digits to avoid array overflow
  float rounding = 0.5;       // Round up down delta

  if (dp > 7) dp = 7; // Limit the size of decimal portion

  // Adjust the rounding value
  for (uint8_t i = 0; i < dp; ++i) rounding /= 10.0;

  if (floatNumber < -rounding)    // add sign, avoid adding - sign to 0.0!
  {
    str[ptr++] = '-'; // Negative number
    str[ptr] = 0; // Put a null in the array as a precaution
    digits = 0;   // Set digits to 0 to compensate so pointer value can be used later
    floatNumber = -floatNumber; // Make positive
  }

  floatNumber += rounding; // Round up or down

  // For error put ... in string and return (all TFT_KEDEIRPI35_ESP library fonts contain . character)
  if (floatNumber >= 2147483647) {
    strcpy(str, "...");
    //return drawString(str, poX, poY);
  }
  // No chance of overflow from here on

  // Get integer part
  unsigned long temp = (unsigned long)floatNumber;

  // Put integer part into array
  ltoa(temp, str + ptr, 10);

  // Find out where the null is to get the digit count loaded
  while ((uint8_t)str[ptr] != 0) ptr++; // Move the pointer along
  digits += ptr;                  // Count the digits

  str[ptr++] = '.'; // Add decimal point
  str[ptr] = '0';   // Add a dummy zero
  str[ptr + 1] = 0; // Add a null but don't increment pointer so it can be overwritten

  // Get the decimal portion
  floatNumber = floatNumber - temp;

  // Get decimal digits one by one and put in array
  // Limit digit count so we don't get a false sense of resolution
  uint8_t i = 0;
  while ((i < dp) && (digits < 9)) // while (i < dp) for no limit but array size must be increased
  {
    i++;
    floatNumber *= 10;       // for the next decimal
    temp = floatNumber;      // get the decimal
    ltoa(temp, str + ptr, 10);
    ptr++; digits++;         // Increment pointer and digits count
    floatNumber -= temp;     // Remove that digit
  }

  // Finally we can plot the string and return pixel length
  return drawString(str, poX, poY);
}

/***************************************************************************************
** Function name:           drawString (with or without user defined font)
** Description :            draw string with padding if it is defined
***************************************************************************************/
// Without font number, uses font set by setTextFont()
int16_t KEDEIRPI35_t3::drawString(const String& string, int poX, int poY)
{
  int16_t len = string.length() + 2;
  char buffer[len];
  string.toCharArray(buffer, len);
  return drawString1(buffer, len, poX, poY);
}

int16_t KEDEIRPI35_t3::drawString1(char string[], int16_t len, int poX, int poY)
{
  int16_t sumX = 0;
  uint8_t padding = 1;
  
  uint16_t cwidth = strPixelLen(string); // Find the pixel width of the string in the font
  uint16_t cheight = textsize*8;

  
  if (textdatum || padX)
  {
    switch(textdatum) {
      case TC_DATUM:
        poX -= cwidth/2;
        padding += 1;
        break;
      case TR_DATUM:
        poX -= cwidth;
        padding += 2;
        break;
      case ML_DATUM:
        poY -= cheight/2;
        //padding += 0;
        break;
      case MC_DATUM:
        poX -= cwidth/2;
        poY -= cheight/2;
        padding += 1;
        break;
      case MR_DATUM:
        poX -= cwidth;
        poY -= cheight/2;
        padding += 2;
        break;
      case BL_DATUM:
        poY -= cheight;
        //padding += 0;
        break;
      case BC_DATUM:
        poX -= cwidth/2;
        poY -= cheight;
        padding += 1;
        break;
      case BR_DATUM:
        poX -= cwidth;
        poY -= cheight;
        padding += 2;
        break;
	 /*
      case L_BASELINE:
        poY -= baseline;
        //padding += 0;
        break;
      case C_BASELINE:
        poX -= cwidth/2;
        poY -= baseline;
        //padding += 1;
        break;
      case R_BASELINE:
        poX -= cwidth;
        poY -= baseline;
        padding += 2;
        break;
	*/
    }
    // Check coordinates are OK, adjust if not
    if (poX < 0) poX = 0;
    if (poX+cwidth > width())   poX = width() - cwidth;
    if (poY < 0) poY = 0;
    //if (poY+cheight-baseline >_height) poY = _height - cheight;
  }
  if(font == NULL){
	  for(uint8_t i = 0; i < len-2; i++){
		drawChar((int16_t) (poX+sumX), (int16_t) poY, string[i], textcolor, textbgcolor, textsize);
		sumX += cwidth/(len-2) + padding;
	  }
  } else {
	  setCursor(poX, poY);
	  for(uint8_t i = 0; i < len-2; i++){
		drawFontChar(string[i]);
		setCursor(cursor_x, cursor_y);
	  }
  }
return sumX;
}

void KEDEIRPI35_t3::scrollTextArea(uint8_t scrollSize){
	uint16_t awColors[scroll_width];
	for (int y=scroll_y+scrollSize; y < (scroll_y+scroll_height); y++) { 
		readRect(scroll_x, y, scroll_width, 1, awColors); 
		writeRect(scroll_x, y-scrollSize, scroll_width, 1, awColors);  
	}
	fillRect(scroll_x, (scroll_y+scroll_height)-scrollSize, scroll_width, scrollSize, scrollbgcolor);
}

void KEDEIRPI35_t3::setScrollTextArea(int16_t x, int16_t y, int16_t w, int16_t h){
	scroll_x = x; 
	scroll_y = y;
	scroll_width = w; 
	scroll_height = h;
}

void KEDEIRPI35_t3::setScrollBackgroundColor(uint16_t color){
	scrollbgcolor=color;
	fillRect(scroll_x,scroll_y,scroll_width,scroll_height,scrollbgcolor);
}

void KEDEIRPI35_t3::enableScroll(void){
	scrollEnable = true;
}

void KEDEIRPI35_t3::disableScroll(void){
	scrollEnable = false;
}

void KEDEIRPI35_t3::resetScrollBackgroundColor(uint16_t color){
	scrollbgcolor=color;
}	

////////////////////////////////////////////////////////////////////////////
// ROUTINES FOR MASKING AND OVERDRAW - 3D rendering
////////////////////////////////////////////////////////////////////////////

// Functions for controlling masked and overdrawn rendering
void     KEDEIRPI35_t3::overdraw_on()  {do_overdraw = 1;}
void     KEDEIRPI35_t3::overdraw_off() {do_overdraw = 0;}
void     KEDEIRPI35_t3::masking_on()   {do_masking  = 1;}
void     KEDEIRPI35_t3::masking_off()  {do_masking  = 0;}
//void     KEDEIRPI35_t3::flip_mask()    {mask_flag  ^= FRAME_ID_FLAG8;}




void Adafruit_GFX_Button::initButton(KEDEIRPI35_t3 *gfx,
	int16_t x, int16_t y, uint8_t w, uint8_t h,
	uint16_t outline, uint16_t fill, uint16_t textcolor,
	const char *label, uint8_t textsize)
{
	_x = x;
	_y = y;
	_w = w;
	_h = h;
	_outlinecolor = outline;
	_fillcolor = fill;
	_textcolor = textcolor;
	_textsize = textsize;
	_gfx = gfx;
	strncpy(_label, label, 9);
	_label[9] = 0;
}

void Adafruit_GFX_Button::drawButton(bool inverted)
{
	uint16_t fill, outline, text;

	if (! inverted) {
		fill = _fillcolor;
		outline = _outlinecolor;
		text = _textcolor;
	} else {
		fill =  _textcolor;
		outline = _outlinecolor;
		text = _fillcolor;
	}
	_gfx->fillRoundRect(_x - (_w/2), _y - (_h/2), _w, _h, min(_w,_h)/4, fill);
	_gfx->drawRoundRect(_x - (_w/2), _y - (_h/2), _w, _h, min(_w,_h)/4, outline);
	_gfx->setCursor(_x - strlen(_label)*3*_textsize, _y-4*_textsize);
	_gfx->setTextColor(text);
	_gfx->setTextSize(_textsize);
	_gfx->print(_label);
}

bool Adafruit_GFX_Button::contains(int16_t x, int16_t y)
{
	if ((x < (_x - _w/2)) || (x > (_x + _w/2))) return false;
	if ((y < (_y - _h/2)) || (y > (_y + _h/2))) return false;
	return true;
}

//=============================================================================
// DMA - Async support
//=============================================================================
#ifdef DMA_SUPPORT
#ifdef ENABLE_KEDEIRPI35_FRAMEBUFFER
void KEDEIRPI35_t3::dmaInterrupt(void) {
	if (_dmaActiveDisplay)  {
		_dmaActiveDisplay->process_dma_interrupt();
	}
}
#endif

#ifdef DEBUG_ASYNC_UPDATE
extern void dumpDMA_TCD(DMABaseClass *dmabc);
#endif

#define COUNT_SUBFRAMES_PER_FRAME (KEDEIRPI35_TFTHEIGHT*KEDEIRPI35_TFTWIDTH/DMA_PIXELS_OUTPUT_PER_DMA)

#if defined(__IMXRT1052__) || defined(__IMXRT1062__) || defined(__MK66FX1M0__)
// T3.6 and T4 processing
void KEDEIRPI35_t3::process_dma_interrupt(void) {

#ifdef DEBUG_ASYNC_LEDS
	digitalWriteFast(DEBUG_PIN_2, HIGH);
#endif
	_dma_sub_frame_count++;
	if (_dma_sub_frame_count < COUNT_SUBFRAMES_PER_FRAME)
	{
		// We need to fill in the finished buffer with the next set of pixel data
		bool frame_complete =  fillDMApixelBuffer((_dma_sub_frame_count & 1) ? _dma_pixel_buffer0 : _dma_pixel_buffer1);
 		if (frame_complete && ((_dma_state & KEDEIRPI35_DMA_CONT) == 0) ) {
			_dmasettings[1].disableOnCompletion();
		}
	} else {

		_dma_frame_count++;
		//Serial.println("\nFrame complete");

		if ((_dma_state & KEDEIRPI35_DMA_CONT) == 0) {
			// We are in single refresh mode or the user has called cancel so
			// Lets try to release the CS pin
			// Lets wait until FIFO is not empty
			// Serial.printf("Before FSR wait: %x %x\n", _pimxrt_spi->FSR, _pimxrt_spi->SR);
			//Serial.println("End DMA transfer");
#if defined(__IMXRT1052__) || defined(__IMXRT1062__)  // Teensy 4.x
			while (_pimxrt_spi->FSR & 0x1f)  ;	// wait until this one is complete

			 //Serial.printf("Before SR busy wait: %x\n", _pimxrt_spi->SR);
			while (_pimxrt_spi->SR & LPSPI_SR_MBF)  ;	// wait until this one is complete

			_dmatx.clearComplete();
			//Serial.println("Restore FCR");
			_pimxrt_spi->FCR = LPSPI_FCR_TXWATER(15); // _spi_fcr_save;	// restore the FSR status... 
	 		_pimxrt_spi->DER = 0;		// DMA no longer doing TX (or RX)

			_pimxrt_spi->CR = LPSPI_CR_MEN | LPSPI_CR_RRF | LPSPI_CR_RTF;   // actually clear both...
			_pimxrt_spi->SR = 0x3f00;	// clear out all of the other status...


			maybeUpdateTCR(LPSPI_TCR_PCS(0) | LPSPI_TCR_FRAMESZ(7));	// output Command with 8 bits
			// Serial.printf("Output NOP (SR %x CR %x FSR %x FCR %x %x TCR:%x)\n", _pimxrt_spi->SR, _pimxrt_spi->CR, _pimxrt_spi->FSR, 
			//	_pimxrt_spi->FCR, _spi_fcr_save, _pimxrt_spi->TCR);
#elif defined(__MK66FX1M0__) 
			// T3.6
			// Maybe only have to wait for fifo not to be full so we can output NOP>>> 
			waitFifoNotFull();
#endif

	#ifdef DEBUG_ASYNC_LEDS
			digitalWriteFast(DEBUG_PIN_3, HIGH);
	#endif
			#if defined(__IMXRT1052__) || defined(__IMXRT1062__)  // Teensy 4.x
			_pending_rx_count = 0;	// Make sure count is zero
			#endif
	#ifdef DEBUG_ASYNC_LEDS
			digitalWriteFast(DEBUG_PIN_3, LOW);
	#endif

			// Serial.println("Do End transaction");
			endSPITransaction();
			_dma_state &= ~KEDEIRPI35_DMA_ACTIVE;
			_dmaActiveDisplay = 0;	// We don't have a display active any more... 

	 		 //Serial.println("After End transaction");

		} else {
			// Still going on. - setup to grab pixels from start of frame again...
			_dma_sub_frame_count = 0;
			//_dma_pixel_index = 0;
			fillDMApixelBuffer(_dma_pixel_buffer1);
		}
	}
#ifdef DEBUG_ASYNC_LEDS
	digitalWriteFast(DEBUG_PIN_2, LOW);
#endif
	_dmatx.clearInterrupt();
	asm("dsb");
}


// T3.5 - without scatter gather support
#elif defined(__MK64FX512__)
void KEDEIRPI35_t3::process_dma_interrupt(void) {
#ifdef DEBUG_ASYNC_LEDS
	digitalWriteFast(DEBUG_PIN_2, HIGH);
#endif
	// Clear out the interrupt and complete state
	_dmatx.clearInterrupt();
	_dmatx.clearComplete();

	// Guess if we we are totally done or not...
	// Hack since we are reading a page ahead look at pixel_index 
	if (_dma_pixel_index == DMA_PIXELS_OUTPUT_PER_DMA)  {
		_dma_frame_count++;
		_dma_sub_frame_count = 0;
		if ((_dma_state & KEDEIRPI35_DMA_CONT) == 0) {
			// We are done!
			_pkinetisk_spi->RSER = 0;
			//_pkinetisk_spi->MCR = SPI_MCR_MSTR | SPI_MCR_CLR_RXF | SPI_MCR_PCSIS(0x1F);  // clear out the queue
			_pkinetisk_spi->SR = 0xFF0F0000;

			endSPITransaction();
			_dma_state &= ~KEDEIRPI35_DMA_ACTIVE;
			_dmaActiveDisplay = 0;	// We don't have a display active any more... 
		}
	}

	_dma_sub_frame_count++;

	// Now lets alternate buffers
	if (_dmatx.TCD->SADDR < (void*)_dma_pixel_buffer1) {
		_dmatx.sourceBuffer(_dma_pixel_buffer1, sizeof(_dma_pixel_buffer1));
		_dmatx.enable();

		fillDMApixelBuffer(_dma_pixel_buffer0);
	} else {
		_dmatx.sourceBuffer(_dma_pixel_buffer0, sizeof(_dma_pixel_buffer0));
		_dmatx.enable();

		fillDMApixelBuffer(_dma_pixel_buffer1);
	}
#ifdef DEBUG_ASYNC_LEDS
	digitalWriteFast(DEBUG_PIN_2, LOW);
#endif
}
#endif

#ifdef ENABLE_KEDEIRPI35_FRAMEBUFFER
void	KEDEIRPI35_t3::initDMASettings(void) 
{
	// Serial.printf("initDMASettings called %d\n", _dma_state);
	if (_dma_state) {  // should test for init, but...
		return;	// we already init this. 
	}

	// T3.6 and T4... 
#if defined(__IMXRT1052__) || defined(__IMXRT1062__)  // Teensy 4.x
	// Now lets setup DMA access to this memory... 
	// Try to do like T3.6 except not kludge for first word...
	// Serial.println("DMA initDMASettings - before settings");
	// Serial.printf("  CWW: %d %d %d\n", CBALLOC, SCREEN_DMA_NUM_SETTINGS, COUNT_WORDS_WRITE);
	_dmasettings[0].sourceBuffer(_dma_pixel_buffer0, sizeof(_dma_pixel_buffer0));
	_dmasettings[0].destination(_pimxrt_spi->TDR);
//	_dmasettings[0].TCD->ATTR_DST = 0;		// This should be 2 (32 bit)
	_dmasettings[0].replaceSettingsOnCompletion(_dmasettings[1]);
	_dmasettings[0].interruptAtCompletion();

	_dmasettings[1].sourceBuffer(_dma_pixel_buffer1, sizeof(_dma_pixel_buffer1));
	_dmasettings[1].destination(_pimxrt_spi->TDR);
//	_dmasettings[1].TCD->ATTR_DST = 0;
	_dmasettings[1].replaceSettingsOnCompletion(_dmasettings[0]);
	_dmasettings[1].interruptAtCompletion();

	// Setup DMA main object
	//Serial.println("Setup _dmatx");
	// Serial.println("DMA initDMASettings - before dmatx");
	_dmatx.begin(true);
	_dmatx.triggerAtHardwareEvent(_spi_hardware->tx_dma_channel);
	_dmatx = _dmasettings[0];
	_dmatx.attachInterrupt(dmaInterrupt);
#elif defined(__MK66FX1M0__) 
	_dmasettings[0].sourceBuffer(&_dma_pixel_buffer0[3], sizeof(_dma_pixel_buffer0)-3);
	_dmasettings[0].destination(_pkinetisk_spi->PUSHR);
	_dmasettings[0].TCD->ATTR_DST = 0;
	_dmasettings[0].replaceSettingsOnCompletion(_dmasettings[1]);
	_dmasettings[0].interruptAtCompletion();

	_dmasettings[1].sourceBuffer(_dma_pixel_buffer1, sizeof(_dma_pixel_buffer1));
	_dmasettings[1].destination(_pkinetisk_spi->PUSHR);
	_dmasettings[1].TCD->ATTR_DST = 0;
	_dmasettings[1].replaceSettingsOnCompletion(_dmasettings[2]);
	_dmasettings[1].interruptAtCompletion();

	_dmasettings[2].sourceBuffer(_dma_pixel_buffer0, sizeof(_dma_pixel_buffer0));
	_dmasettings[2].destination(_pkinetisk_spi->PUSHR);
	_dmasettings[2].TCD->ATTR_DST = 0;
	_dmasettings[2].replaceSettingsOnCompletion(_dmasettings[1]);
	_dmasettings[2].interruptAtCompletion();
	// Setup DMA main object
	//Serial.println("Setup _dmatx");
	// Serial.println("DMA initDMASettings - before dmatx");
	_dmatx.begin(true);
	_dmatx.triggerAtHardwareEvent(_spi_hardware->tx_dma_channel);
	_dmatx = _dmasettings[0];
	_dmatx.attachInterrupt(dmaInterrupt);
#elif defined(__MK64FX512__)
	// Teensy 3.5
	// T3.5
	// Lets setup the write size.  For SPI we can use up to 32767 so same size as we use on T3.6...
	// But SPI1 and SPI2 max of 511.  We will use 480 in that case as even divider...
/*
	_dmarx.disable();
	_dmarx.source(_pkinetisk_spi->POPR);
	_dmarx.TCD->ATTR_SRC = 1;
	_dmarx.destination(_dma_dummy_rx);
	_dmarx.disableOnCompletion();
	_dmarx.triggerAtHardwareEvent(_spi_hardware->rx_dma_channel);
	_dmarx.attachInterrupt(dmaInterrupt);
	_dmarx.interruptAtCompletion();
*/
	// We may be using settings chain here so lets set it up. 
	// Now lets setup TX chain.  Note if trigger TX is not set
	// we need to have the RX do it for us.
	// BUGBUG:: REAL Hack
	if (!_csport) {
		// Should also probably change the masks... But
		pinMode(_cs, OUTPUT);
		_csport    = portOutputRegister(digitalPinToPort(_cs));
		_cspinmask = digitalPinToBitMask(_cs);
		Serial.println("DMASettings (T3.5) change CS pin to standard IO");
	}

	_dmatx.disable();
	_dmatx.sourceBuffer(&_dma_pixel_buffer0[3], sizeof(_dma_pixel_buffer0)-3);
	_dmatx.destination(_pkinetisk_spi->PUSHR);
	_dmatx.TCD->ATTR_DST = 0;
	_dmatx.disableOnCompletion();
	_dmatx.interruptAtCompletion();
	_dmatx.attachInterrupt(dmaInterrupt);

	// SPI1/2 only have one dma_channel
	if (_spi_hardware->tx_dma_channel) {
		_dmatx.triggerAtHardwareEvent(_spi_hardware->tx_dma_channel);
	} else {
		_dmatx.triggerAtHardwareEvent(_spi_hardware->rx_dma_channel);
	}
#endif

	_dma_state = KEDEIRPI35_DMA_INIT;  // Should be first thing set!
	// Serial.println("DMA initDMASettings - end");

}
#endif

#ifdef DEBUG_ASYNC_UPDATE
void dumpDMA_TCD(DMABaseClass *dmabc)
{
	Serial.printf("%x %x:", (uint32_t)dmabc, (uint32_t)dmabc->TCD);

	Serial.printf("SA:%x SO:%d AT:%x NB:%x SL:%d DA:%x DO: %d CI:%x DL:%x CS:%x BI:%x\n", (uint32_t)dmabc->TCD->SADDR,
		dmabc->TCD->SOFF, dmabc->TCD->ATTR, dmabc->TCD->NBYTES, dmabc->TCD->SLAST, (uint32_t)dmabc->TCD->DADDR, 
		dmabc->TCD->DOFF, dmabc->TCD->CITER, dmabc->TCD->DLASTSGA, dmabc->TCD->CSR, dmabc->TCD->BITER);
}
#endif

void KEDEIRPI35_t3::dumpDMASettings() {
#ifdef DEBUG_ASYNC_UPDATE
#if defined(__MK66FX1M0__) 
	dumpDMA_TCD(&_dmatx);
	dumpDMA_TCD(&_dmasettings[0]);
	dumpDMA_TCD(&_dmasettings[1]);
	dumpDMA_TCD(&_dmasettings[2]);
#elif defined(__MK64FX512__)
	dumpDMA_TCD(&_dmatx);
//	dumpDMA_TCD(&_dmarx);
#elif defined(__IMXRT1052__) || defined(__IMXRT1062__)  // Teensy 4.x
	// Serial.printf("DMA dump TCDs %d\n", _dmatx.channel);
	dumpDMA_TCD(&_dmatx);
	dumpDMA_TCD(&_dmasettings[0]);
	dumpDMA_TCD(&_dmasettings[1]);
#else
#endif	
#endif

}

// Fill the pixel buffer with data... 
#ifdef ENABLE_KEDEIRPI35_FRAMEBUFFER
#if defined(__IMXRT1052__) || defined(__IMXRT1062__)  // Teensy 4.x
bool KEDEIRPI35_t3::fillDMApixelBuffer(uint32_t *dma_buffer_pointer)
{
	uint8_t *frame_buffer_pixel_ptr = &_pfbtft[_dma_pixel_index];

	for (uint16_t i = 0; i < DMA_PIXELS_OUTPUT_PER_DMA; i++) {
		uint16_t color = _pallet[*frame_buffer_pixel_ptr++]; 	// extract the 16 bit color
		uint32_t r = (color & 0xF800) >> 11;
		uint32_t g = (color & 0x07E0) >> 5;
		uint32_t b = color & 0x001F;

		r = (r * 255) / 31;
		g = (g * 255) / 63;
		b = (b * 255) / 31;
	
		*dma_buffer_pointer++ = (r << 16) | (g << 8) | b;
	}
	_dma_pixel_index += DMA_PIXELS_OUTPUT_PER_DMA;
	if (_dma_pixel_index >= (KEDEIRPI35_TFTHEIGHT*KEDEIRPI35_TFTWIDTH)) {
		_dma_pixel_index = 0;
		return true;
	}

	return false;
}

#else
// T3.x - Don't have 24/32 bit SPI transfers so output byte at a time...
bool KEDEIRPI35_t3::fillDMApixelBuffer(uint8_t *dma_buffer_pointer)
{
	uint8_t *frame_buffer_pixel_ptr = &_pfbtft[_dma_pixel_index];

	for (uint16_t i = 0; i < DMA_PIXELS_OUTPUT_PER_DMA; i++) {
		uint16_t color = _pallet[*frame_buffer_pixel_ptr++]; 	// extract the 16 bit color
		uint8_t r = (color & 0xF800) >> 11;
		uint8_t g = (color & 0x07E0) >> 5;
		uint8_t b = color & 0x001F;

		r = (r * 255) / 31;
		g = (g * 255) / 63;
		b = (b * 255) / 31;
	
		*dma_buffer_pointer++ = r;
		*dma_buffer_pointer++ = g;
		*dma_buffer_pointer++ = b;
	}
	_dma_pixel_index += DMA_PIXELS_OUTPUT_PER_DMA;
	if (_dma_pixel_index >= (KEDEIRPI35_TFTHEIGHT*KEDEIRPI35_TFTWIDTH)) {
		_dma_pixel_index = 0;
		return true;
	}

	return false;
}
#endif
#endif

bool KEDEIRPI35_t3::updateScreenAsync(bool update_cont)					// call to say update the screen now.
{
	// Not sure if better here to check flag or check existence of buffer.
	// Will go by buffer as maybe can do interesting things?
	// BUGBUG:: only handles full screen so bail on the rest of it...
#ifdef ENABLE_KEDEIRPI35_FRAMEBUFFER
	if (!_use_fbtft) return false;


#ifdef DEBUG_ASYNC_LEDS
	digitalWriteFast(DEBUG_PIN_1, HIGH);
#endif
	// Init DMA settings. 
	initDMASettings();

	// Don't start one if already active.
	if (_dma_state & KEDEIRPI35_DMA_ACTIVE) {
	#ifdef DEBUG_ASYNC_LEDS
		digitalWriteFast(DEBUG_PIN_1, LOW);
	#endif
		return false;
	}

#if !defined(__MK64FX512__)
	_dmatx = _dmasettings[0];
	_dmasettings[1].TCD->CSR &= ~( DMA_TCD_CSR_DREQ);  // Don't disable on completion.
#endif
	if (!update_cont) {
		// In this case we will only run through once...
		_dma_state &= ~KEDEIRPI35_DMA_CONT;
	}
#ifdef DEBUG_ASYNC_UPDATE
	Serial.println("dumpDMASettings called");
	dumpDMASettings();
#endif

	beginSPITransaction();
	// Doing full window. 
	// We need to convert the first page of colors.  Could/should hack up
	// some of the above code to allow this to be done while waiting for
	// the startup SPI output to finish. 
	_dma_pixel_index = 0;
	_dma_frame_count = 0;  // Set frame count back to zero. 
	_dma_sub_frame_count = 0;	
	

#if defined(__IMXRT1052__) || defined(__IMXRT1062__)  // Teensy 4.x
	//==========================================
	// T4
	//==========================================
	setAddr(0, 0, _width-1, _height-1);
	fillDMApixelBuffer(_dma_pixel_buffer0);  // Fill the first buffer
	writecommand_last(KEDEIRPI35_RAMWR);

	// Update TCR to 16 bit mode. and output the first entry.
	_spi_fcr_save = _pimxrt_spi->FCR;	// remember the FCR
	_pimxrt_spi->FCR = 0;	// clear water marks... 	
	maybeUpdateTCR(LPSPI_TCR_PCS(1) | LPSPI_TCR_FRAMESZ(23) | LPSPI_TCR_RXMSK /*| LPSPI_TCR_CONT*/);
//	_pimxrt_spi->CFGR1 |= LPSPI_CFGR1_NOSTALL;
//	maybeUpdateTCR(LPSPI_TCR_PCS(1) | LPSPI_TCR_FRAMESZ(15) | LPSPI_TCR_CONT);
 	_pimxrt_spi->DER = LPSPI_DER_TDDE;
	_pimxrt_spi->SR = 0x3f00;	// clear out all of the other status...

  	//_dmatx.triggerAtHardwareEvent(DMAMUX_SOURCE_LPSPI4_TX );

 	_dmatx = _dmasettings[0];

  	_dmatx.begin(false);
  	_dmatx.enable();
	fillDMApixelBuffer(_dma_pixel_buffer1); 	// fill the second one
#elif defined(__MK66FX1M0__) 
	//==========================================
	// T3.6
	//==========================================

	setAddr(0, 0, _width-1, _height-1);
	fillDMApixelBuffer(_dma_pixel_buffer0);  // Fill the first buffer
	lcd_cmd(KEDEIRPI35_RAMWR);

	// Write the first Word out before enter DMA as to setup the proper CS/DC/Continue flaugs
	// need to deal with first pixel... 
	lcd_color(_pallet[*_pfbtft]);	
	_dma_frame_count = 0;  // Set frame count back to zero. 
	_dmaActiveDisplay = this;
	_dma_state |= KEDEIRPI35_DMA_ACTIVE;
	_pkinetisk_spi->RSER |= SPI_RSER_TFFF_DIRS |	 SPI_RSER_TFFF_RE;	 // Set DMA Interrupt Request Select and Enable register
	_pkinetisk_spi->MCR &= ~SPI_MCR_HALT;  //Start transfers.
	_dmatx.enable();
	fillDMApixelBuffer(_dma_pixel_buffer1); 	// fill the second one

#elif defined(__MK64FX512__)
	//==========================================
	// T3.5
	//==========================================

	setAddr(0, 0, _width-1, _height-1);
	fillDMApixelBuffer(_dma_pixel_buffer0);  // Fill the first buffer
	lcd_cmd(KEDEIRPI35_RAMWR);

	// Write the first Word out before enter DMA as to setup the proper CS/DC/Continue flaugs
	// need to deal with first pixel... 
	lcd_color(_pallet[*_pfbtft]);	
	_dmatx.sourceBuffer(&_dma_pixel_buffer0[3], sizeof(_dma_pixel_buffer0)-3);
	_dma_frame_count = 0;  // Set frame count back to zero. 
	_dmaActiveDisplay = this;
	_dma_state |= KEDEIRPI35_DMA_ACTIVE;
	_pkinetisk_spi->RSER |= SPI_RSER_TFFF_DIRS | SPI_RSER_TFFF_RE;	 // Set DMA Interrupt Request Select and Enable register
	_pkinetisk_spi->MCR &= ~SPI_MCR_HALT;  //Start transfers.
  	//_dmatx.begin(false);
	_dmatx.enable();
	fillDMApixelBuffer(_dma_pixel_buffer1); 	// fill the second one

#endif	
#ifdef DEBUG_ASYNC_LEDS
	digitalWriteFast(DEBUG_PIN_1, LOW);
#endif


	_dmaActiveDisplay = this;
	if (update_cont) {
		_dma_state |= KEDEIRPI35_DMA_CONT;
	} else {
		_dma_state &= ~KEDEIRPI35_DMA_CONT;

	}

	_dma_state |= KEDEIRPI35_DMA_ACTIVE;
	return true;
    #else
    return false;     // no frame buffer so will never start... 
	#endif

}			 

void KEDEIRPI35_t3::endUpdateAsync() {
	// make sure it is on
	#ifdef ENABLE_KEDEIRPI35_FRAMEBUFFER
	if (_dma_state & KEDEIRPI35_DMA_CONT) {
		_dma_state &= ~KEDEIRPI35_DMA_CONT; // Turn of the continueous mode
	}
	#endif
}
	
void KEDEIRPI35_t3::waitUpdateAsyncComplete(void) 
{
	#ifdef ENABLE_KEDEIRPI35_FRAMEBUFFER
#ifdef DEBUG_ASYNC_LEDS
	digitalWriteFast(DEBUG_PIN_3, HIGH);
#endif

	while ((_dma_state & KEDEIRPI35_DMA_ACTIVE)) {
		// asm volatile("wfi");
	};
#ifdef DEBUG_ASYNC_LEDS
	digitalWriteFast(DEBUG_PIN_3, LOW);
#endif
	#endif	
}
#else // DMA 
// no DMA support yet
bool KEDEIRPI35_t3::updateScreenAsync(bool update_cont) {
	updateScreen();
	return true;
}	// call to say update the screen optinoally turn into continuous mode. 

void KEDEIRPI35_t3::waitUpdateAsyncComplete(void) {};
void KEDEIRPI35_t3::endUpdateAsync() {};			 // Turn of the continueous mode fla
void KEDEIRPI35_t3::dumpDMASettings() {};



#endif // DMA Support




void KEDEIRPI35_t3::setAddr(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
 {
	  lcd_cmd(KEDEIRPI35_PASET);  // (y)
	  lcd_data(y0 >> 8);
	  lcd_data(y0 & 0xFF);
	  lcd_data(y1 >> 8);
	  lcd_data(y1 & 0xFF);
	  lcd_cmd(KEDEIRPI35_CASET);  // (x)
	  lcd_data(x0 >> 8);
	  lcd_data(x0 & 0xFF);
	  lcd_data(x1 >> 8);
	  lcd_data(x1 & 0xFF);
}


void KEDEIRPI35_t3::HLine(int16_t x, int16_t y, int16_t w, uint16_t color)
 {
	#ifdef ENABLE_KEDEIRPI35_FRAMEBUFFER
  	if (_use_fbtft) {
  		drawFastHLine(x, y, w, color);
  		return;
  	}
  	#endif
    x+=_originx;
    y+=_originy;

    // Rectangular clipping
    if((y < _displayclipy1) || (x >= _displayclipx2) || (y >= _displayclipy2)) return;
    if(x<_displayclipx1) { w = w - (_displayclipx1 - x); x = _displayclipx1; }
    if((x+w-1) >= _displayclipx2)  w = _displayclipx2-x;
    if (w<1) return;

	setAddr(x, y, x+w-1, y);
	lcd_cmd(KEDEIRPI35_RAMWR);
	lcd_color(color, w);
}

void KEDEIRPI35_t3::VLine(int16_t x, int16_t y, int16_t h, uint16_t color)
 {
	#ifdef ENABLE_KEDEIRPI35_FRAMEBUFFER
  	if (_use_fbtft) {
  		drawFastVLine(x, y, h, color);
  		return;
  	}
  	#endif
	x+=_originx;
    y+=_originy;

    // Rectangular clipping
    if((x < _displayclipx1) || (x >= _displayclipx2) || (y >= _displayclipy2)) return;
    if(y < _displayclipy1) { h = h - (_displayclipy1 - y); y = _displayclipy1;}
    if((y+h-1) >= _displayclipy2) h = _displayclipy2-y;
    if(h<1) return;

	setAddr(x, y, x, y+h-1);
	lcd_cmd(KEDEIRPI35_RAMWR);
	lcd_color(color, h);
}

void KEDEIRPI35_t3::Pixel(int16_t x, int16_t y, uint16_t color)
 {
    x+=_originx;
    y+=_originy;

  	if((x < _displayclipx1) ||(x >= _displayclipx2) || (y < _displayclipy1) || (y >= _displayclipy2)) return;

	#ifdef ENABLE_KEDEIRPI35_FRAMEBUFFER
  	if (_use_fbtft) {
  		_pfbtft[y*_width + x] = color;
  		return;
  	}
  	#endif
	setAddr(x, y, x, y);
	lcd_cmd(KEDEIRPI35_RAMWR);
	lcd_color(color);
}

