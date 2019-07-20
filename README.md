Overview and Warning: 
=====
This is a modified version of the official PJRC ILI9341_t3 library (https://github.com/PaulStoffregen/ILI9341_t3) to work with KeDei Raspberry Pi  displays.  

We did all of our playing around with ones who are marked Version 6.3, so I don’t have any idea how well it will work with others. 

And it is always a Work In Progress.  Also using a lot of work from the the Raspberry Pi implementation: https://github.com/cnkz111/RaspberryPi_KeDei_35_lcd_v62


Warning: This is just an experiment.  It is discussed up on the Teensy forum thread: https://forum.pjrc.com/threads/55735-ILI9488_t3-Support-for-the-ILI9488-on-T3-x-and-beyond


Several of us purchased one of these thinking we were buying an ILI9488 display to try out.  Turns out very different.  We were finally able to 
figure out a few things about the display and had a simple program running, using information we found above and elsewhere.

So decided to see how hard it would be to get it to work as a display for a Teensy.  We got it to a point that it appears to be working.

However not sure how much additional time and energy will be spent on it, as none of us have any plans to actually use these displays.

There are several others we would much rather use. 

We make now warrantee or promises that this library is fit for any purpose other than to experiment with.

**USE AT YOUR OWN RISK!**

This library was created to allow extended use on the KeDei Raspberry Pi display and supports T3.5, t3.6 T4 and beyond.  It also has support for other T3.x boards as well as TLC.  
The TLC support could probably be generalized to work on most Arduinos. 

For further development status see: https://forum.pjrc.com/threads/55735-KEDEIRPI35_t3-Support-for-the-ILI9488-on-T3-x-and-beyond

Hardware things we learned along the way
============

As we have found in several other places on the Web, this board is very much different than most of the other display boards, and it is still unclear what the underlying display controller actually is.
Our guess is it is something like an ILI9488 or … 

The board also has an XPT2046 touch controller which uses standard SPI communications. 

SPI Communications
----

Your SPI communications on this board does not go directly to display but instead go to three shift registers.   There are also two SPI Chip select pins, one labeled, which looks like it is for the Display and the other looks like it is for the touch controller.    This is partially true.

That is if the Touch CS is low (asserted) and the other CS is high (not asserted) than the communications are processed by the XPT2046.    And our updated example sketch touchpaint uses the standard teensy library https://github.com/PaulStoffregen/XPT2046_Touchscreen to process the presses.

However the SPI communications with the display are a lot different than any other I have seen.   For example there are no reset pins, nor a Data/Command(DC) pin.  Instead this information is encoded into the SPI data that you send to the display. 

That is for each thing you want to send to the display, you typically send a 4 byte transfers.    A few of these are to reset the display, some are commands and others are data.  Example in a capture I did from starting up
on a RPI3, I had the following at the start: Note whole capture after I condensed it some was over 200,000 lines long
```
0x00,0x01,0x00,0x00,spi_transmit32(0x00010000);
0x00,0x00,0x00,0x00,spi_transmit32(0x00000000);
0x00,0x01,0x00,0x00,spi_transmit32(0x00010000);
0x00,0x11,0x00,0x00,lcd_cmd(0x0000);
0x00,0x11,0x00,0xFF,lcd_cmd(0x00FF);
0x00,0x11,0x00,0xFF,lcd_cmd(0x00FF);
0x00,0x11,0x00,0xFF,lcd_cmd(0x00FF);
0x00,0x11,0x00,0xFF,lcd_cmd(0x00FF);
0x00,0x11,0x00,0xFF,lcd_cmd(0x00FF);
0x00,0x11,0x00,0xFF,lcd_cmd(0x00FF);
0x00,0x11,0x00,0x11,lcd_cmd(0x0011);
0x00,0x11,0x00,0xB0,lcd_cmd(0x00B0);
0x00,0x15,0x00,0x00,lcd_dat(0x0000);
0,0x00,0x11,0x00,0xB3,lcd_cmd(0x00B3);
0x00,0x15,0x00,0x02,lcd_dat(0x0002);
0x00,0x15,0x00,0x00,lcd_dat(0x0000);
0x00,0x15,0x00,0x00,lcd_dat(0x0000);
0x00,0x15,0x00,0x00,lcd_dat(0x0000);
```
In the above the first few lines are sort of the reset for the display and the like.

Those lines that start with 0x00,0x15 -> generate commands (DC asserted)
Those lines that start with 0x00,0x11-> Are Data and (DC is not asserted)

Now the interesting thing.  When you output each of these 32 bit transfers, you must assert the Touch CS and then DeAssert it.  
That is our code.  Example the code in there for Teensy LC (nothing special)
```
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
```
The direct writes are just faster ways than using digitalWrite()

Multiple Versions
----------------

There appears to be at least two versions of this board, which effected things like the proper MADCTL values for the four different rotations. 

We figured it out, as the RPI startup code, did several strange SPI transfers at the beginning, which appeared like they were directed to the XPT2046 Touch controller.  
It turns out it was.  If you look at datasheet for these devices, like from: https://www.buydisplay.com/download/ic/XPT2046.pdf

You will find that these devices (XPT2046)  have a pin on them marked AUX, which on these display appear to be connected to a potential Resistor Divider made up of R1 and R2.
Where R1 is connected to GND and R2 is connected to 3.3v.  It turns out that some of the few boards we have, has R1 populated and some of them have R2 populated. 

So if you send the byte E7 to the display with Touch CS asserted, it starts a AtoD conversion on that AUX pin, and transfer of two more zeros, will return you the AtoD value from AUX,
Which we have seen:
```
R1 populated = 0000
R2 populated = 7ff8
```

Extended Features
============

This library borrows some concepts and functionality from another ILI9341 library, https://github.com/KurtE/ILI9341_t3n.  It also incorporates functionality from the TFT_ILI9341_ESP, https://github.com/Bodmer/TFT_ILI9341_ESP, for additional functions:
```c++
    int16_t  drawNumber(long long_num,int poX, int poY);
    int16_t  drawFloat(float floatNumber,int decimal,int poX, int poY);   
    int16_t drawString(const String& string, int poX, int poY);
    int16_t drawString1(char string[], int16_t len, int poX, int poY);
    void setTextDatum(uint8_t datum);
```
Frame Buffer
------------
The teensy 3.6 and now 3.5 have a lot more memory than previous Teensy processors, so on these boards, I borrowed some ideas from the ILI9341_t3DMA library and added code to be able to use a logical Frame Buffer.  To enable this I added a couple of API's 
```c++
    uint8_t useFrameBuffer(boolean b) - if b non-zero it will allocate memory and start using
    void    freeFrameBuffer(void) - Will free up the memory that was used.
    void    updateScreen(void); - Will update the screen with all of your updates...
    void    setFrameBuffer(uint16_t *frame_buffer); - Now have the ability allocate the frame buffer and pass it in, to avoid use of malloc
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


Again an WIP experiment, which may never go anywhere!
=====
