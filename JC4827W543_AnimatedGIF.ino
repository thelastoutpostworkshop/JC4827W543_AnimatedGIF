// Use board "ESP32S3 Dev Module" (last tested on v3.2.0)
#include <PINS_JC4827W543.h> // Install "GFX Library for Arduino" with the Library Manager (last tested on v1.5.5)
                             // Install "Dev Device Pins" with the Library Manager (last tested on v0.0.2)
#include <AnimatedGIF.h>     // Install "AnimatedGIF" with the Library Manager (last tested on v2.2.0)
#include "Audio.h"           // install as zip in the Arduino IDE : https://github.com/pschatzmann/arduino-audio-tools.git
#include <SD_MMC.h>          // Included with the Espressif Arduino Core (last tested on v3.2.0)
#include "TAMC_GT911.h"         // Install "TAMC_GT911" with the Library Manager (last tested on v1.0.2)
#include "FreeSansBold12pt7b.h" // Included in this project

const char *root = "/root"; // Do not change this, it is needed to access files properly on the SD card
const char *GIF_FOLDER = "/gif";
AnimatedGIF gif;
int16_t display_width, display_height;

#define MAX_FILES 20 // Adjust as needed
String gifFileList[MAX_FILES];
uint32_t gifFileSizes[MAX_FILES] = {0}; // Store each GIF file's size in bytes
int fileCount = 0;
static int currentFile = 0;
static File FSGifFile; // temp gif file holder

// PSRAM for GIF playing optimization
#define PSRAM_RESERVE_SIZE (100 * 1024) // Reserve 100KB
uint8_t *psramBuffer = NULL;
size_t reservedPSRAMSize = 0;

// Touch Controller
#define TOUCH_SDA 8
#define TOUCH_SCL 4
#define TOUCH_INT 3
#define TOUCH_RST 38
#define TOUCH_WIDTH 480
#define TOUCH_HEIGHT 272
#define TITLE_REGION_Y (gfx->height()/3 - 30)
#define TITLE_REGION_H 35
#define TITLE_REGION_W (gfx->width())
TAMC_GT911 touchController = TAMC_GT911(TOUCH_SDA, TOUCH_SCL, TOUCH_INT, TOUCH_RST, TOUCH_WIDTH, TOUCH_HEIGHT);

void setup()
{
  Serial.begin(115200);
  delay(2000);
  // SD Card initialization
  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);
  SD_MMC.setPins(SD_SCK, SD_MOSI, SD_MISO);
  if (!SD_MMC.begin(root, true, false /* format_if_mount_failed */, SDMMC_FREQ_DEFAULT))
  {
    Serial.println("ERROR: SD Card mount failed!");
    while (true)
    {
      /* no need to continue */
    }
  }

  // Init Display
  if (!gfx->begin())
  {
    Serial.println("gfx->begin() failed!");
    while (true)
    {
      /* no need to continue */
    }
  }
  gfx->fillScreen(RGB565_BLACK);
  gfx->setFont(&FreeSansBold12pt7b);

  // Set the backlight of the screen to High intensity
  pinMode(GFX_BL, OUTPUT);
  digitalWrite(GFX_BL, HIGH);

  touchController.begin();
  touchController.setRotation(ROTATION_INVERTED); // Change as needed

  display_width = gfx->width();
  display_height = gfx->height();
  gif.begin(BIG_ENDIAN_PIXELS);

  if (!psramFound())
  {
    Serial.println("No PSRAM found > Enable it by selecting OPI PSRAM in the board configuration");
  }

  // Reserve PSRAM (leaving ~10KB free)
  uint8_t *myBuffer = reservePSRAM();
  if (myBuffer == NULL)
  {
    Serial.println("PSRAM reserve failed!");
    // Handle error...
  }

  Serial.println("Loading GIF files list");
  loadGifFilesList();
  displaySelectedFile();
}

// Display the selected gif file
void displaySelectedFile()
{
  // Clear the screen
  gfx->fillScreen(RGB565_BLACK);

  int screenW = gfx->width();
  int screenH = gfx->height();
  int centerY = screenH / 2;
  int arrowSize = 40; // size of the arrow icon (adjust as needed)
  int margin = 10;    // margin from screen edge

  // --- Draw Left Arrow ---
  // The left arrow is drawn as a filled triangle at the left side.
  gfx->fillTriangle(margin, centerY,
                    margin + arrowSize, centerY - arrowSize / 2,
                    margin + arrowSize, centerY + arrowSize / 2,
                    RGB565_WHITE);

  // --- Draw Right Arrow ---
  // Draw the right arrow as a filled triangle at the right side.
  gfx->fillTriangle(screenW - margin, centerY,
                    screenW - margin - arrowSize, centerY - arrowSize / 2,
                    screenW - margin - arrowSize, centerY + arrowSize / 2,
                    RGB565_WHITE);

  // --- Draw the Title ---
  // Get the file title string.
  String title = gifFileList[currentFile];
  int16_t x1, y1;
  uint16_t textW, textH;
  gfx->getTextBounds(title.c_str(), 0, 0, &x1, &y1, &textW, &textH);
  // Calculate x so the text is centered.
  int titleX = (screenW - textW) / 2 - x1;
  // Position the title above the play button; here we place it at roughly one-third of the screen height.
  int titleY = screenH / 3;
  gfx->setCursor(titleX, titleY);
  gfx->print(title);

  // --- Draw the Play Button ---
  // Define the play button size and location.
  int playButtonSize = 50;
  int playX = (screenW - playButtonSize) / 2;
  int playY = screenH - playButtonSize - 20; // 20 pixels from bottom
  // Draw a filled circle for the button background.
  gfx->fillCircle(playX + playButtonSize / 2, playY + playButtonSize / 2, playButtonSize / 2, RGB565_DARKGREEN);
  // Draw a play–icon (triangle) inside the circle.
  int triX = playX + playButtonSize / 2 - playButtonSize / 4;
  int triY = playY + playButtonSize / 2;
  gfx->fillTriangle(triX, triY - playButtonSize / 4,
                    triX, triY + playButtonSize / 4,
                    triX + playButtonSize / 2, triY,
                    RGB565_WHITE);
}

void loop()
{
  touchController.read();
  if (touchController.touches > 0)
  {
    int tx = touchController.points[0].x;
    int ty = touchController.points[0].y;
    int screenW = gfx->width();
    int screenH = gfx->height();
    int arrowSize = 40;
    int margin = 10;
    int playButtonSize = 50;
    int playX = (screenW - playButtonSize) / 2;
    int playY = screenH - playButtonSize - 20;

    // Check if touch is in the left arrow area.
    if (tx < margin + arrowSize && ty > (screenH / 2 - arrowSize) && ty < (screenH / 2 + arrowSize))
    {
      // Left arrow touched: cycle to previous file.
      currentFile--;
      if (currentFile < 0)
      currentFile = fileCount - 1;
      updateTitle();
      while (touchController.touches > 0)
      {
        touchController.read();
        delay(50);
      }
      delay(300);
    }
    else if (tx > screenW - margin - arrowSize && ty > (screenH / 2 - arrowSize) && ty < (screenH / 2 + arrowSize))
    {
      // Right arrow touched: cycle to next file.
      currentFile++;
      if (currentFile >= fileCount)
      currentFile = 0;
      updateTitle();
      while (touchController.touches > 0)
      {
        touchController.read();
        delay(50);
      }
      delay(300);
    }
    // Check if touch is in the play button area.
    else if (tx >= playX && tx <= playX + playButtonSize &&
             ty >= playY && ty <= playY + playButtonSize)
    {
      playSelectedFile(currentFile);
      // Wait until the user fully releases the touch before refreshing the UI.
      waitForTouchRelease();

      // After playback, redisplay the selection screen.
      displaySelectedFile();
      while (touchController.touches > 0)
      {
        touchController.read();
        delay(50);
      }
      delay(300);
    }
  }
  delay(50);
}

// Update the gif title on the screen
void updateTitle() {
  // Clear the entire title area
  gfx->fillRect(0, TITLE_REGION_Y, TITLE_REGION_W, TITLE_REGION_H, RGB565_BLACK);
  
  // Retrieve the new title
  String title = gifFileList[currentFile];
  
  // Get text dimensions for the new title
  int16_t x1, y1;
  uint16_t textW, textH;
  gfx->getTextBounds(title.c_str(), 0, 0, &x1, &y1, &textW, &textH);
  
  // Center the text in the fixed title region:
  int titleX = (TITLE_REGION_W - textW) / 2 - x1;
  int titleY = TITLE_REGION_Y + (TITLE_REGION_H + textH) / 2;
  
  gfx->setCursor(titleX, titleY);
  gfx->print(title);
}

// Continuously read until no touches are registered.
void waitForTouchRelease()
{
  while (touchController.touches > 0)
  {
    touchController.read();
    delay(50);
  }
  // Extra debounce delay to ensure that the touch state is fully cleared.
  delay(300);
}

void playSelectedFile(int fileindex) {
  // Use the current file and then advance the index.
  
  // Build the full path for the selected GIF.
  String fullPath = String(GIF_FOLDER) + "/" + gifFileList[fileindex];
  char gifFilename[128];
  fullPath.toCharArray(gifFilename, sizeof(gifFilename));
  
  Serial.printf("Playing %s\n", gifFilename);
  
  // Check if the file can fit in the reserved PSRAM.
  if (gifFileSizes[fileindex] <= reservedPSRAMSize)
  {
    gfx->fillScreen(RGB565_BLACK);
    gfx->setCursor(20, 100);
    gfx->print("Loading GIF in PSRAM...");

    File gifFile = SD_MMC.open(gifFilename);
    if (gifFile)
    {
      size_t fileSize = gifFile.size();
      size_t bytesRead = gifFile.read(psramBuffer, fileSize);
      gifFile.close();
      Serial.printf("Read %u bytes into PSRAM\n", bytesRead);
      
      // Try opening the GIF from the PSRAM buffer.
      if (gif.open(psramBuffer, fileSize, GIFDraw))
      {
        Serial.printf("Successfully opened GIF from PSRAM.\n");
        while (gif.playFrame(false, NULL))
        {
          // Animation loop
        }
        gif.close();
      }
      else
      {
        Serial.printf("Failed to open GIF from PSRAM, falling back to SD.\n");
        gifPlay(gifFilename);
      }
    }
    else
    {
      Serial.printf("Failed to open %s for reading into PSRAM.\n", gifFilename);
      gifPlay(gifFilename);
    }
  }
  else
  {
    // File too big to fit in reserved PSRAM; open it directly from SD.
    Serial.printf("File too big to fit in reserved PSRAM; open it directly from SD.\n");
    gifPlay(gifFilename);
  }
}

uint8_t *reservePSRAM()
{
  if (!psramFound())
  {
    Serial.println("No PSRAM found!");
    return NULL;
  }

  // Get the total free PSRAM size (in bytes)
  size_t freePSRAM = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);

  // Ensure we leave at least PSRAM_RESERVE_MARGIN free
  if (freePSRAM <= PSRAM_RESERVE_SIZE)
  {
    Serial.println("Not enough free PSRAM available to reserve!");
    return NULL;
  }

  // Calculate the amount we can reserve
  reservedPSRAMSize = freePSRAM - PSRAM_RESERVE_SIZE;

  // Allocate the buffer from PSRAM
  psramBuffer = (uint8_t *)heap_caps_malloc(reservedPSRAMSize, MALLOC_CAP_SPIRAM);

  if (psramBuffer != NULL)
  {
    Serial.printf("Reserved %u bytes from PSRAM, leaving %u bytes free.\n",
                  reservedPSRAMSize, PSRAM_RESERVE_SIZE);
  }
  else
  {
    Serial.println("Failed to allocate PSRAM!");
  }

  return psramBuffer;
}

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

void gifPlay(char *gifPath)
{

  if (!gif.open(gifPath, GIFOpenFile, GIFCloseFile, GIFReadFile, GIFSeekFile, GIFDraw))
  {
    Serial.printf("Could not open gif %s", gifPath);
  }
  else
  {
    gfx->fillScreen(RGB565_BLACK);
    gfx->setCursor(20, 100);
    gfx->print("Playing GIF from SD Card...");
    delay(1000);
    Serial.printf("Starting playing gif %s\n", gifPath);

    while (gif.playFrame(false /*change to true to use the internal gif frame duration*/, NULL))
    {
    }

    gif.close();
  }
}

static void *GIFOpenFile(const char *fname, int32_t *pSize)
{
  Serial.printf("Opening %s from SD\n", fname);
  FSGifFile = SD_MMC.open(fname);
  if (FSGifFile)
  {
    *pSize = FSGifFile.size();
    return (void *)&FSGifFile;
  }
  return NULL;
}

static void GIFCloseFile(void *pHandle)
{
  File *f = static_cast<File *>(pHandle);
  if (f != NULL)
    f->close();
}

static int32_t GIFReadFile(GIFFILE *pFile, uint8_t *pBuf, int32_t iLen)
{
  int32_t iBytesRead;
  iBytesRead = iLen;
  File *f = static_cast<File *>(pFile->fHandle);
  // Note: If you read a file all the way to the last byte, seek() stops working
  if ((pFile->iSize - pFile->iPos) < iLen)
    iBytesRead = pFile->iSize - pFile->iPos - 1; // <-- ugly work-around
  if (iBytesRead <= 0)
    return 0;
  iBytesRead = (int32_t)f->read(pBuf, iBytesRead);
  pFile->iPos = f->position();
  return iBytesRead;
}

static int32_t GIFSeekFile(GIFFILE *pFile, int32_t iPosition)
{
  int i = micros();
  File *f = static_cast<File *>(pFile->fHandle);
  f->seek(iPosition);
  pFile->iPos = (int32_t)f->position();
  i = micros() - i;
  // log_d("Seek time = %d us\n", i);
  return pFile->iPos;
}

// Read the avi file list in the avi folder
void loadGifFilesList()
{
  File gifDir = SD_MMC.open(GIF_FOLDER);
  if (!gifDir)
  {
    Serial.println("Failed to open GIF folder");
    return;
  }
  fileCount = 0;
  while (true)
  {
    File file = gifDir.openNextFile();
    if (!file)
      break;
    if (!file.isDirectory())
    {
      String name = file.name();
      if (name.endsWith(".gif") || name.endsWith(".GIF"))
      {
        gifFileList[fileCount] = name;
        gifFileSizes[fileCount] = file.size(); // Save file size (in bytes)
        fileCount++;
        if (fileCount >= MAX_FILES)
          break;
      }
    }
    file.close();
  }
  gifDir.close();
  Serial.printf("%d gif files read\n", fileCount);
  // Optionally, print out each file's size for debugging:
  for (int i = 0; i < fileCount; i++)
  {
    Serial.printf("File %d: %s, Size: %lu bytes\n", i, gifFileList[i].c_str(), gifFileSizes[i]);
  }
}

// Draw a line of image directly on the screen
void GIFDraw(GIFDRAW *pDraw)
{
  uint8_t *s;
  uint16_t *d, *usPalette, usTemp[display_width];
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