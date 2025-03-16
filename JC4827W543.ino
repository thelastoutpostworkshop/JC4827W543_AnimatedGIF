
// Use board "ESP32S3 Dev Module" (last tested on v3.1.3)
// Install "Dev Device Pins" with the Library Manager (last tested on v0.0.2)
#include <PINS_JC4827W543.h> // Install "GFX Library for Arduino" with the Library Manager (last tested on v1.5.5)
#include <AnimatedGIF.h>     // Install "AnimatedGIF" with the Library Manager (last tested on v2.2.0)

// GIF files
#include "gif_files/animated_gif_320x240_1.h" //GIF size in FLASH memory
#include "gif_files/animated_gif_320x240_2.h" //GIF size in FLASH memory
#include "gif_files/animated_gif_320x240_3.h" //GIF size in FLASH memory
#include "gif_files/animated_gif_320x240_4.h" //GIF size in FLASH memory

#include "gif_files/hud_a.h"                 //GIF size in FLASH memory is 1.7MB
#include "gif_files/x_wing.h"                //GIF size in FLASH memory is 0.9MB
#include "gif_files/death_star.h"            //GIF size in FLASH memory is 1.7MB
#include "gif_files/star_destroyer.h"        //GIF size in FLASH memory is 1MB
#include "gif_files/star_destroyer_planet.h" //GIF size in FLASH memory is 2.3MB
#include "gif_files/cat.h"                   //GIF size in FLASH memory is 1.1MB
#include "gif_files/star_trek_hud.h"         //GIF size in FLASH memory is 1.6MB
#include "gif_files/jedi_battle.h"           //GIF size in FLASH memory is 3.3MB (use partitions.csv for this one, if you your ESP32 board has 4MB Flash size)

#define GIF_NAME death_star
AnimatedGIF gif;
int16_t display_width, display_height;

void setup()
{
  Serial.begin(115200);

  // Init Display
  if (!gfx->begin())
  {
    Serial.println("gfx->begin() failed!");
  }
  gfx->fillScreen(RGB565_BLACK);

#ifdef GFX_BL
  pinMode(GFX_BL, OUTPUT);
  digitalWrite(GFX_BL, HIGH);
#endif

  display_width = gfx->width();
  display_height = gfx->height();
  gif.begin(BIG_ENDIAN_PIXELS);
}

void loop()
{
  if (openGif((uint8_t *)GIF_NAME, sizeof(GIF_NAME)))
  {
    while (gif.playFrame(false /*change to true to use the internal gif frame duration*/, NULL))
    {
    };
  }
}

// Draw a line of image directly on the screen
void GIFDraw(GIFDRAW *pDraw)
{
  uint8_t *s;
  uint16_t *d, *usPalette, usTemp[320];
  int x, y, iWidth;

  iWidth = pDraw->iWidth;
  if (iWidth + pDraw->iX > display_width)
  {
    iWidth = display_width - pDraw->iX;
  }
  usPalette = pDraw->pPalette;
  y = pDraw->iY + pDraw->y; // current line
  if (y >= display_height || pDraw->iX >= display_width || iWidth < 1)
  {
    return;
  }
  s = pDraw->pPixels;
  if (pDraw->ucDisposalMethod == 2) // restore to background color
  {
    for (x = 0; x < iWidth; x++)
    {
      if (s[x] == pDraw->ucTransparent)
      {
        s[x] = pDraw->ucBackground;
      }
    }
    pDraw->ucHasTransparency = 0;
  }

  // Apply the new pixels to the main image
  if (pDraw->ucHasTransparency) // if transparency used
  {
    uint8_t *pEnd, c, ucTransparent = pDraw->ucTransparent;
    int x, iCount;
    pEnd = s + iWidth;
    x = 0;
    iCount = 0; // count non-transparent pixels
    while (x < iWidth)
    {
      c = ucTransparent - 1;
      d = usTemp;
      while (c != ucTransparent && s < pEnd)
      {
        c = *s++;
        if (c == ucTransparent) // done, stop
        {
          s--; // back up to treat it like transparent
        }
        else // opaque
        {
          *d++ = usPalette[c];
          iCount++;
        }
      } // while looking for opaque pixels
      if (iCount) // any opaque pixels?
      {
        gfx->draw16bitBeRGBBitmap(pDraw->iX + x, y, usTemp, iCount, 1);
        x += iCount;
        iCount = 0;
      }
      // no, look for a run of transparent pixels
      c = ucTransparent;
      while (c == ucTransparent && s < pEnd)
      {
        c = *s++;
        if (c == ucTransparent)
        {
          iCount++;
        }
        else
        {
          s--;
        }
      }
      if (iCount)
      {
        x += iCount; // skip these
        iCount = 0;
      }
    }
  }
  else
  {
    s = pDraw->pPixels;
    // Translate the 8-bit pixels through the RGB565 palette (already byte reversed)
    for (x = 0; x < iWidth; x++)
    {
      usTemp[x] = usPalette[*s++];
    }
    gfx->draw16bitBeRGBBitmap(pDraw->iX, y, usTemp, iWidth, 1);
  }
} /* GIFDraw() */

// Open Gif and allocate memory
bool openGif(uint8_t *gifdata, size_t gifsize)
{
  if (gif.open(gifdata, gifsize, GIFDraw))
  {
    Serial.printf("Successfully opened GIF; Canvas size = %d x %d\n", gif.getCanvasWidth(), gif.getCanvasHeight());
    Serial.printf("GIF memory size is %ld (%2.2f MB)\n", gifsize, (float)gifsize / (1024 * 1024));
    return true;
  }
  else
  {
    printGifErrorMessage(gif.getLastError());
    return false;
  }
}

// Get human-readable error related to GIF
void printGifErrorMessage(int errorCode)
{
  switch (errorCode)
  {
  case GIF_DECODE_ERROR:
    Serial.println("GIF Decoding Error");
    break;
  case GIF_TOO_WIDE:
    Serial.println("GIF Too Wide");
    break;
  case GIF_INVALID_PARAMETER:
    Serial.println("Invalid Parameter for gif open");
    break;
  case GIF_UNSUPPORTED_FEATURE:
    Serial.println("Unsupported feature in GIF");
    break;
  case GIF_FILE_NOT_OPEN:
    Serial.println("GIF File not open");
    break;
  case GIF_EARLY_EOF:
    Serial.println("GIF early end of file");
    break;
  case GIF_EMPTY_FRAME:
    Serial.println("GIF with empty frame");
    break;
  case GIF_BAD_FILE:
    Serial.println("GIF bad file");
    break;
  case GIF_ERROR_MEMORY:
    Serial.println("GIF memory Error");
    break;
  default:
    Serial.println("Unknown Error");
    break;
  }
}