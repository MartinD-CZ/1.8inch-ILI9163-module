// ==========================================================================================
// This is a really simple driver for the 1.8" TFT display based on the ILI9163C driver IC.
// It's written for STM32F030F4, but should be really easy to modify.
// See http://embedblog.eu/?p=547 for more info.
// Some parts come from Adafruit, some from other contributors.
// I am not saying it is finished or ready to use - it's more like an inspiration.

#include "stm32f0xx.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include "ili9163c.h"
#include "..\delay.h"

void ILI9163::begin()
{
	//========================
	//IO init - PA5:7 as high speed alternative
	GPIOA->MODER |= GPIO_MODER_MODER5_1 | GPIO_MODER_MODER7_1;
	GPIOA->OSPEEDR |= GPIO_OSPEEDER_OSPEEDR5 | GPIO_OSPEEDER_OSPEEDR7;
	//nothing in AFIO
	
	//PA2:4 as high speed outputs
	GPIOA->MODER |= GPIO_MODER_MODER2_0 | GPIO_MODER_MODER3_0 | GPIO_MODER_MODER4_0;
	GPIOA->OSPEEDR |= GPIO_OSPEEDER_OSPEEDR2 | GPIO_OSPEEDER_OSPEEDR3 | GPIO_OSPEEDER_OSPEEDR4;
	
	//SPI init
	RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
	SPI1->CR2 |= SPI_CR2_SSOE;
	SPI1->CR1 |= SPI_CR1_SPE | SPI_CR1_BR_0 | SPI_CR1_MSTR | SPI_CR1_SSM;		//set SPI speed to 12 MHz (div /4)
	
	RST_HIGH;
	delay_ms(5);
	RST_LOW;
	delay_ms(20);
	RST_HIGH;
	delay_ms(150);
	
	_width = ILI9163_TFTWIDTH;
	_height = ILI9163_TFTHEIGHT;
	
	cursor_x = cursor_y = 0;
	
	textColor = TFT_BLACK;
	bgColor = TFT_WHITE;
	textSize = 1;
	textwrap = true;
	
	// Initialization commands for ILI9163 screens
	static const uint8_t ILI9163_cmds[] =
	{
	17,             // 17 commands follow
	0x01,  0,       // Software reset
	0x11,  0,       // Exit sleep mode
	0x3A,  1, 0x05, // Set pixel format
	0x26,  1, 0x04, // Set Gamma curve
	0xF2,  1, 0x01, // Gamma adjustment enabled
	0xE0, 15, 0x3F, 0x25, 0x1C, 0x1E, 0x20, 0x12, 0x2A, 0x90,
	          0x24, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, // Positive Gamma
	0xE1, 15, 0x20, 0x20, 0x20, 0x20, 0x05, 0x00, 0x15,0xA7,
	          0x3D, 0x18, 0x25, 0x2A, 0x2B, 0x2B, 0x3A, // Negative Gamma
	0xB1,  2, 0x08, 0x08, // Frame rate control 1
	0xB4,  1, 0x07,       // Display inversion
	0xC0,  2, 0x0A, 0x02, // Power control 1
	0xC1,  1, 0x02,       // Power control 2
	0xC5,  2, 0x50, 0x5B, // Vcom control 1
	0xC7,  1, 0x40,       // Vcom offset
	0x2A,  4+ILI9163_INIT_DELAY, 0x00, 0x00, 0x00, 0x7F, 250, // Set column address
	0x2B,  4, 0x00, 0x00, 0x00, 0x9F,            // Set page address
	0x36,  1, 0xC8,       // Set address mode
	0x29,  0,             // Set display on
	};

	commandList(ILI9163_cmds);
}

void ILI9163::commandList (const uint8_t *addr)
{
	uint8_t  numCommands, numArgs;
	uint8_t  ms;
	uint8_t pointer = 0;

	numCommands = addr[pointer++];            // Number of commands to follow
	while (numCommands--)                           // For each command...
	{
		writecommand(addr[pointer++]);    // Read, issue command
		numArgs = addr[pointer++];        // Number of args to follow
		ms = numArgs & ILI9163_INIT_DELAY;      // If hibit set, delay follows args
		numArgs &= ~ILI9163_INIT_DELAY;         // Mask out delay bit
		while (numArgs--)                       // For each argument...
		{
			writedata(addr[pointer++]); // Read, issue argument
		}

		if (ms)
		{
			ms = addr[pointer++];     // Read post-command delay time (ms)
			delay_ms((ms==255 ? 500 : ms));
		}
	}
}

void ILI9163::spiwrite(uint8_t c)
{
	*(volatile uint8_t*)&SPI1->DR = (uint8_t)c;
	
	while ((SPI1->SR & SPI_SR_BSY));
}

void ILI9163::spiwrite16(uint16_t c, uint16_t repeat)
{
	while (repeat--)
	{
		*(volatile uint16_t*)&SPI1->DR = (uint16_t)c;
	
		while ((SPI1->SR & SPI_SR_BSY));
	}
}

void ILI9163::writecommand(uint8_t c)
{
  DC_LOW;
  CE_LOW;
  spiwrite(c);
  CE_HIGH;
}


void ILI9163::writedata(uint8_t c)
{
  DC_HIGH;
  CE_LOW;
  spiwrite(c);
  CE_HIGH;
}

void ILI9163::setCursor(int16_t x, int16_t y)
{
  cursor_x = x;
  cursor_y = y;
}

void ILI9163::drawPixel(uint16_t x, uint16_t y, uint16_t color)
{
  if ((x >= ILI9163_TFTWIDTH) || (y >= ILI9163_TFTHEIGHT)) return;

  CE_LOW;

	if (addr_col != x) 
	{
		DC_LOW;
		spiwrite(ILI9163_CASET);
		addr_col = x;
		DC_HIGH;
		spiwrite(0);
		spiwrite(x);

		spiwrite(0); 
		spiwrite(x); 
	}

	if (addr_row != y) 
	{
		DC_LOW;
		spiwrite(ILI9163_RASET);
		addr_row = y;
		DC_HIGH;
		spiwrite(0); 
		spiwrite(y); 

		spiwrite(0); 
		spiwrite(y); 
	}

  DC_LOW;
  spiwrite(ILI9163_RAMWR); 
  DC_HIGH;

  spiwrite(color >> 8); 
  win_xe = x;
  spiwrite(color);
  win_ye = y;

  CE_HIGH;
}

void ILI9163::drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color)
{
  int8_t steep = abs(y1 - y0) > abs(x1 - x0);

	if (steep) 
	{
		tftswap(x0, y0);
		tftswap(x1, y1);
	}

	if (x0 > x1) 
	{
		tftswap(x0, x1);
		tftswap(y0, y1);
	}

	if (x1 < 0) return;

	int16_t dx, dy;
	dx = x1 - x0;
	dy = abs(y1 - y0);

	int16_t err = dx / 2;
	int8_t ystep = (y0 < y1) ? 1 : (-1);

  CE_LOW;
	if (steep)	// y increments every iteration (y0 is x-axis, and x0 is y-axis)
	{
	  if (x1 >= _height) x1 = _height - 1;

	  for (; x0 <= x1; x0++) {
		if ((x0 >= 0) && (y0 >= 0) && (y0 < _width)) break;
		err -= dy;
		if (err < 0) {
			err += dx;
			y0 += ystep;
		}
	  }

	  if (x0 > x1) return;

    setWindow(y0, x0, y0, _height);
		for (; x0 <= x1; x0++) 
		{
			spiwrite16(color);
			err -= dy;
			if (err < 0) 
			{
				y0 += ystep;
				if ((y0 < 0) || (y0 >= _width)) break;
				err += dx;
        setWindow(y0, x0+1, y0, _height);
			}
		}
	}
	else	// x increments every iteration (x0 is x-axis, and y0 is y-axis)
	{
	  if (x1 >= _width) x1 = _width - 1;

	  for (; x0 <= x1; x0++) {
		if ((x0 >= 0) && (y0 >= 0) && (y0 < _height)) break;
		err -= dy;
		if (err < 0) {
			err += dx;
			y0 += ystep;
		}
	  }

	  if (x0 > x1) return;

    setWindow(x0, y0, _width, y0);
		for (; x0 <= x1; x0++) 
		{
			spiwrite16(color);
			err -= dy;
			if (err < 0) 
			{
				y0 += ystep;
				if ((y0 < 0) || (y0 >= _height)) break;
				err += dx;
			  //while (!(SPSR & _BV(SPIF))); // Safe, but can comment out and rely on delay
        setWindow(x0+1, y0, _width, y0);
			}
		}
	}
	
  CE_HIGH;
}

void ILI9163::setWindow(int16_t x0, int16_t y0, int16_t x1, int16_t y1)
{
  // Column addr set
  DC_LOW;

  spiwrite(ILI9163_CASET);

  DC_HIGH;
  spiwrite(0);
  addr_col = 0xFF;
  spiwrite(x0);
	
  if(x1!=win_xe) 
	{
    spiwrite(0);
    win_xe = x1;
    spiwrite(x1);
  }
	
  // Row addr set
  DC_LOW;
  spiwrite(ILI9163_RASET);

  DC_HIGH;
  spiwrite(0);
  addr_row = 0xFF;
  spiwrite(y0);
	
  if(y1!=win_ye) 
	{
    spiwrite(0); 
    win_ye = y1;
    spiwrite(y1); 
  }
	
  // write to RAM
  DC_LOW;
  spiwrite(ILI9163_RAMWR);

  DC_HIGH;
}

void ILI9163::drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color)
{
  CE_LOW;
  setWindow(x, y, x, _height);

  spiwrite16(color, h);
  CE_HIGH;
}

void ILI9163::drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color)
{
  CE_LOW;
  setWindow(x, y, _width, y);

  spiwrite16(color, w);
  CE_HIGH;
}

void ILI9163::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
  CE_LOW;
  setWindow(x, y, x + w - 1, y + h - 1);

  if (h > w) tftswap(h, w);

  while (h--) spiwrite16(color, w);
	CE_HIGH;
}

uint16_t ILI9163::color565(uint8_t r, uint8_t g, uint8_t b)
{
	return (((uint16_t)r & 0xF8) << 8) | (((uint16_t)b & 0xFC) << 3) | ((uint16_t)g >> 3);
}

void ILI9163::fillScreen(uint16_t color)
{
  fillRect(0, 0, ILI9163_TFTWIDTH, ILI9163_TFTHEIGHT, color);
}

void ILI9163::drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
  drawFastHLine(x, y, w, color);
  drawFastHLine(x, y + h - 1, w, color);
  drawFastVLine(x, y, h, color);
  drawFastVLine(x + w - 1, y, h, color);
}

void ILI9163::setRotation(uint8_t m)
{
  addr_row = 0xFF;
  addr_col = 0xFF;
  win_xe = 0xFF;
  win_ye = 0xFF;

  rotation = m % 4;
  writecommand(ILI9163_MADCTL);
  switch (rotation) {
    case 0:
      writedata(ILI9163_MADCTL_MX | ILI9163_MADCTL_MY | ILI9163_MADCTL_BGR);
      _width  = ILI9163_TFTWIDTH;
      _height = ILI9163_TFTHEIGHT;
      break;
    case 1:
      writedata(ILI9163_MADCTL_MV | ILI9163_MADCTL_MY | ILI9163_MADCTL_BGR);
      _width  = ILI9163_TFTHEIGHT;
      _height = ILI9163_TFTWIDTH;
      break;
    case 2:
      writedata(ILI9163_MADCTL_BGR);
      _width  = ILI9163_TFTWIDTH;
      _height = ILI9163_TFTHEIGHT;
      break;
    case 3:
      writedata(ILI9163_MADCTL_MX | ILI9163_MADCTL_MV | ILI9163_MADCTL_BGR);
      _width  = ILI9163_TFTHEIGHT;
      _height = ILI9163_TFTWIDTH;
      break;
  }
}

void ILI9163::drawChar(int16_t x, int16_t y, unsigned char c, uint16_t color, uint16_t bg, uint8_t size) 
	{
		c -= (uint8_t)gfxFont->first;
		GFXglyph *glyph  = &(((GFXglyph *)gfxFont->glyph)[c]);
		uint8_t  *bitmap = (uint8_t *)gfxFont->bitmap;

		uint16_t bo = glyph->bitmapOffset;
		uint8_t  w  = glyph->width,
						 h  = glyph->height;
		int8_t   xo = glyph->xOffset,
						 yo = glyph->yOffset;
		uint8_t  xx, yy, bits = 0, bit = 0;
		int16_t  xo16 = 0, yo16 = 0;

		if(size > 1) {
				xo16 = xo;
				yo16 = yo;
		}

		for(yy=0; yy<h; yy++) {
				for(xx=0; xx<w; xx++) {
						if(!(bit++ & 7)) {
								bits = bitmap[bo++];
						}
						if(bits & 0x80) {
								if(size == 1) {
										drawPixel(x+xo+xx, y+yo+yy, color);
								} else {
										fillRect(x+(xo16+xx)*size, y+(yo16+yy)*size,
											size, size, color);
								}
						}
						bits <<= 1;
				}
		}
		
		cursor_x += glyph->xAdvance;
}
	
void ILI9163::setFont(const GFXfont *f) 
	{
    if(f) 
		{            // Font struct pointer passed in?
        if(!gfxFont) 
				{ // And no current font struct? Switching from classic to new font behavior. Move cursor pos down 6 pixels so it's on baseline.
            cursor_y += 6;
        }
    } 
		else if(gfxFont) 
		{ // NULL passed.  Current font struct defined? Switching from new to classic font behavior. Move cursor pos up 6 pixels so it's at top-left of char.
        cursor_y -= 6;
    }
    gfxFont = f;
}
	
void ILI9163::print(char *txt)
{
	while (*txt)
	{
		drawChar(cursor_x, cursor_y, *txt++, textColor, bgColor, textSize);
	}
}

void ILI9163::printxy(char *txt, uint8_t x, uint8_t y)
{
	cursor_x = x;
	cursor_y = y;
	print(txt);
}

void ILI9163::printf(char *szFormat, ...)
{
	char szBuffer[64]; //in this buffer we form the message
	int NUMCHARS = sizeof(szBuffer) / sizeof(szBuffer[0]);
	va_list pArgs;
	va_start(pArgs, szFormat);
	vsnprintf(szBuffer, NUMCHARS - 1, szFormat, pArgs);
	va_end(pArgs);
	
	print(szBuffer);
}

