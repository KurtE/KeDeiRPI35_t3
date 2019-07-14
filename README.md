Overview and Warning: 
=====
This is a modified version of the official PJRC ILI9341_t3 library (https://github.com/PaulStoffregen/ILI9341_t3) to work with ILI9488 displays. And it is always a Work In Progress.  Also using a lot of work from the the Raspberry Pi implementation: https://github.com/cnkz111/RaspberryPi_KeDei_35_lcd_v62


Warning: This is a WIP, that still has several real issues to be resolved.  Like rotation may not be correct, likewise colors and the like. 

This library borrows some concepts and functionality from another ILI9341 library, https://github.com/KurtE/ILI9341_t3n.  It also incorporates functionality from the TFT_ILI9341_ESP, https://github.com/Bodmer/TFT_ILI9341_ESP, for additional functions:
```c++
    int16_t  drawNumber(long long_num,int poX, int poY);
    int16_t  drawFloat(float floatNumber,int decimal,int poX, int poY);   
    int16_t drawString(const String& string, int poX, int poY);
    int16_t drawString1(char string[], int16_t len, int poX, int poY);
    void setTextDatum(uint8_t datum);
```

This library was created to allow extended use on the ILI9488 larger display and supports T3.5, t3.6 and beyond.

For further development status see: https://forum.pjrc.com/threads/55735-KEDEIRPI35_t3-Support-for-the-ILI9488-on-T3-x-and-beyond


Frame Buffer
------------
The teensy 3.6 and now 3.5 have a lot more memory than previous Teensy processors, so on these boards, I borrowed some ideas from the ILI9341_t3DMA library and added code to be able to use a logical Frame Buffer.  To enable this I added a couple of API's 
```c++
    uint8_t useFrameBuffer(boolean b) - if b non-zero it will allocate memory and start using
    void	freeFrameBuffer(void) - Will free up the memory that was used.
    void	updateScreen(void); - Will update the screen with all of your updates...
	void	setFrameBuffer(uint16_t *frame_buffer); - Now have the ability allocate the frame buffer and pass it in, to avoid use of malloc
```

Additional APIs
---------------
In addition, this library now has some of the API's and functionality that has been requested in a pull request.  In particular it now supports, the ability to set a clipping rectangle as well as setting an origin that is used with the drawing primitives.   These new API's include:
```c++
	void setOrigin(int16_t x = 0, int16_t y = 0); 
	void getOrigin(int16_t* x, int16_t* y);
	void setClipRect(int16_t x1, int16_t y1, int16_t w, int16_t h); 
	void setClipRect();
```

Adafruit library info
=======================

But as this code is based of of their work, their original information is included below and uses fonts and the Adafruit GFX library:

------------------------------------------

This is a library for the Adafruit ILI9341 display products

This library works with the Adafruit 2.8" Touch Shield V2 (SPI)
  ----> http://www.adafruit.com/products/1651
 
Check out the links above for our tutorials and wiring diagrams.
These displays use SPI to communicate, 4 or 5 pins are required
to interface (RST is optional).

Adafruit invests time and resources providing this open source code,
please support Adafruit and open-source hardware by purchasing
products from Adafruit!

Written by Limor Fried/Ladyada for Adafruit Industries.
MIT license, all text above must be included in any redistribution

To download. click the DOWNLOADS button in the top right corner, rename the uncompressed folder Adafruit_ILI9341. Check that the Adafruit_ILI9341 folder contains Adafruit_ILI9341.cpp and Adafruit_ILI9341.

Place the Adafruit_ILI9341 library folder your arduinosketchfolder/libraries/ folder. You may need to create the libraries subfolder if its your first library. Restart the IDE

Also requires the Adafruit_GFX library for Arduino.

This code also relies a lot on the work that others have done to try to understand this display This includes

Original code - KeDei V5.0 code - Conjur
 https://www.raspberrypi.org/forums/viewtopic.php?p=1019562
 Mon Aug 22, 2016 2:12 am - Final post on the KeDei v5.0 code.
References code - Uladzimir Harabtsou l0nley
 https://github.com/l0nley/kedei35

KeDei 3.5inch LCD V5.0 module for Raspberry Pi
Modified by FREE WING, Y.Sakamoto
http://www.neko.ne.jp/~freewing/


Future Updates
==============


Again WIP
=====
