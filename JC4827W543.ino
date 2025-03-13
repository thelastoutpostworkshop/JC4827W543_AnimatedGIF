// Master Animated GIF on a 320x240 SPI LCD Screen
// Youtube Tutorial: https://youtu.be/omUWkUqFYrQ
// Tested with Espressif ESP32 Arduino Core v3.0.2, 3.0.3
// Using ESP32-S3 with 8MB of PSRAM
// See also the Tutorial on how to create ESP32 custom partitions:
//
#include <bb_spi_lcd.h>  // Install this library with the Arduino IDE Library Manager
                         // Tested on version 2.5.4, 2.6.0
#include <AnimatedGIF.h> // Install this library with the Arduino IDE Library Manager
                         // Tested on version 2.1.1

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

// Push Button
#define PUSH_BUTTION_PIN 13
unsigned long lastDebounceTime;
int lastState = -1;
int state = -1;

// Main object for the display driver
BB_SPI_LCD tft;
AnimatedGIF gif;

// GIFs to display
#define GIF_COUNT 4                                                                                                                                                  // Number of GIFs to cycle through, if you have enough space on flash memory
const uint8_t *gifData[GIF_COUNT] = {animated_gif_320x240_1, animated_gif_320x240_2, animated_gif_320x240_3_2, animated_gif_320x240_4};                                // Add more GIFs here if you have enough space on flash memory
const size_t gifSizes[GIF_COUNT] = {sizeof(animated_gif_320x240_1), sizeof(animated_gif_320x240_2), sizeof(animated_gif_320x240_3_2), sizeof(animated_gif_320x240_4)}; // Add corresponding sizes here

int currentGif = 0;

void setup()
{
  //  pinMode(PUSH_BUTTION_PIN, INPUT_PULLUP);
  //  lastDebounceTime = millis();

  Serial.begin(115200);
  // tft.begin(LCD_ILI9341, FLAGS_NONE, 40000000, 8, 18, 17, -1, -1, 9, 3); // Scott Pinout
  // tft.begin(LCD_ILI9341, FLAGS_NONE, 40000000, 15, 2, -1, 21, 12, 13, 14); // Charles Cheap Yellow display pinout
  tft.begin(DISPLAY_CYD_543); // Charles Cheap Yellow display pinout

  // tft.setRotation(LCD_ORIENTATION_90); // Make sure you have the right orientation based on your GIF
                                       // or the GIF will show incorrectly, even garbage output
                                       // Values : LCD_ORIENTATION_0, LCD_ORIENTATION_90, LCD_ORIENTATION_180 or LCD_ORIENTATION_270
  tft.fillScreen(TFT_BLACK);

  gif.begin(GIF_PALETTE_RGB565_LE); // Set the cooked output type we want (compatible with SPI LCDs)
}

void loop()
{
  playGif(currentGif);
  currentGif++;
  if (currentGif == GIF_COUNT)
  {
    currentGif = 0;
  }
}

void playGif(int gifNum)
{
  const bool opened = openGif((uint8_t *)gifData[gifNum], gifSizes[gifNum]);
  if (!opened)
  {
    Serial.printf("Cannot open GIF number %d\n", gifNum);
    return;
  }
  tft.fillScreen(TFT_BLACK);
  while (gif.playFrame(true, NULL))
    ;
}


// Open Gif and allocate memory
bool openGif(uint8_t *gifdata, size_t gifsize)
{
  if (gif.open(gifdata, gifsize, GIFDraw))
  {
    Serial.printf("Successfully opened GIF; Canvas size = %d x %d\n", gif.getCanvasWidth(), gif.getCanvasHeight());
    Serial.printf("GIF memory size is %ld (%2.2f MB)\n", gifsize, (float)gifsize / (1024 * 1024));
    gif.setDrawType(GIF_DRAW_COOKED); // We want the Animated GIF library to generate ready-made pixels
    if (!gif.getFrameBuf())
    {
      if (gif.allocFrameBuf(GIFAlloc) != GIF_SUCCESS)
      {
        Serial.println("Not Enough RAM memory for frame buffer");
        return false;
      }
    }
    return true;
  }
  else
  {
    printGifErrorMessage(gif.getLastError());
    return false;
  }
}

bool ButtonPressed()
{
  int currentState = digitalRead(PUSH_BUTTION_PIN);
  if (currentState != lastState)
  {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > 1)
  {
    if (currentState != state)
    {
      state = currentState;
      lastState = currentState;
      return state == LOW; // Returns true if the button is pressed
    }
  }

  lastState = currentState;
  return false; // No change in state
}

//
// The memory management functions are needed to keep operating system
// dependencies out of the core library code
//
// memory allocation callback function
void *GIFAlloc(uint32_t u32Size)
{
  return malloc(u32Size);
} /* GIFAlloc() */
// memory free callback function
void GIFFree(void *p)
{
  free(p);
}

// Draw callback from the AnimatedGIF decoder
void GIFDraw(GIFDRAW *pDraw)
{
  if (pDraw->y == 0)
  { // set the memory window (once per frame) when the first line is rendered
    tft.setAddrWindow(pDraw->iX, pDraw->iY, pDraw->iWidth, pDraw->iHeight);
  }
  // For all other lines, just push the pixels to the display. We requested 'COOKED'big-endian RGB565 and
  tft.pushPixels((uint16_t *)pDraw->pPixels, pDraw->iWidth);
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