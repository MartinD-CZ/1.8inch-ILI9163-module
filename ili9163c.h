#ifndef _TFT_ILI9163H_
#define _TFT_ILI9163H_

// Swap any type
template <typename T> static inline void
tftswap(T& a, T& b) { T t = a; a = b; b = t; }

//These define the ports and port bits used for the chip select (CS) and data/command (DC) lines
#define RST_HIGH		GPIOA->BSRR = GPIO_BSRR_BS_2
#define RST_LOW			GPIOA->BSRR = GPIO_BSRR_BR_2
#define CE_HIGH			GPIOA->BSRR = GPIO_BSRR_BS_3
#define CE_LOW			GPIOA->BSRR = GPIO_BSRR_BR_3
#define DC_HIGH			GPIOA->BSRR = GPIO_BSRR_BS_4
#define DC_LOW			GPIOA->BSRR = GPIO_BSRR_BR_4

#define ILI9163_TFTWIDTH  160
#define ILI9163_TFTHEIGHT 128

#define ILI9163_INIT_DELAY 0x80

// These are the ILI9163 control registers
#define ILI9163_INVOFF  0x20
#define ILI9163_INVON   0x21
#define ILI9163_CASET   0x2A
#define ILI9163_RASET   0x2B
#define ILI9163_RAMWR   0x2C
#define ILI9163_RAMRD   0x2E

#define ILI9163_PTLAR   0x30
#define ILI9163_VSCDEF  0x33

#define ILI9163_MADCTL  0x36

#define ILI9163_MADCTL_MY  0x80
#define ILI9163_MADCTL_MX  0x40
#define ILI9163_MADCTL_MV  0x20
#define ILI9163_MADCTL_ML  0x10
#define ILI9163_MADCTL_RGB 0x00
#define ILI9163_MADCTL_BGR 0x08
#define ILI9163_MADCTL_MH  0x04


// Some example colors, use color565(r, g, b) to generate new
#define TFT_BLACK       0x0000      
#define TFT_RED         0xF800      
#define TFT_WHITE       0xFFFF    

typedef struct { // Data stored PER GLYPH
	uint16_t bitmapOffset;     // Pointer into GFXfont->bitmap
	uint8_t  width, height;    // Bitmap dimensions in pixels
	uint8_t  xAdvance;         // Distance to advance cursor (x axis)
	int8_t   xOffset, yOffset; // Dist from cursor pos to UL corner
} GFXglyph;

typedef struct { // Data stored for FONT AS A WHOLE:
	uint8_t  *bitmap;      // Glyph bitmaps, concatenated
	GFXglyph *glyph;       // Glyph array
	uint8_t   first, last; // ASCII extents
	uint8_t   yAdvance;    // Newline distance (y axis)
} GFXfont;



// Class functions and variables
class ILI9163
{
 public:
  void begin(void);

  void setCursor(int16_t x, int16_t y);
	void setRotation(uint8_t m);

  void drawPixel(uint16_t x, uint16_t y, uint16_t color);
 
	void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
  void drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color);
  void drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color);
 
  uint16_t color565(uint8_t r, uint8_t g, uint8_t b);
 
  void fillScreen(uint16_t color);
  void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
  void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
 
  void drawChar(int16_t x, int16_t y, unsigned char c, uint16_t color, uint16_t bg, uint8_t size);
	void print(char *txt);
	void printxy(char *txt, uint8_t x, uint8_t y);
	void printf(char *szFormat, ...);
  
  void setTextColor(uint16_t color);
  void setFont(const GFXfont *f);
  void setTextWrap(bool wrap);

 private:
	void spiwrite(uint8_t c);
  void spiwrite16(uint16_t c, uint16_t repeat = 1);
  void writecommand(uint8_t c);
  void writedata(uint8_t d);
  void commandList(const uint8_t *addr); 
 
	void setWindow(int16_t x0, int16_t y0, int16_t x1, int16_t y1);

  int16_t  _width, _height, cursor_x, cursor_y, padX;
  uint16_t textColor, bgColor;
  uint8_t  addr_row, addr_col, win_xe, win_ye;
  uint8_t  textfont, textSize, textdatum, rotation;
  bool     textwrap; // If set, 'wrap' text at right edge of display
	const GFXfont *gfxFont;
};

#endif
