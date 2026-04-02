#include <SPI.h>
#include <TFT_eSPI.h>       // Hardware-specific library

TFT_eSPI tft = TFT_eSPI();  // Invoke custom library

void setup(void) {
  tft.init();

  tft.fillScreen(TFT_BLACK);
  tft.drawRect(0, 0, tft.width(), tft.height(), TFT_BLUE);

  // Set "cursor" at top left corner of display (0,0) and select font 4
  tft.setCursor(42, 4, 4);

  // Set the font colour to be white with a black background
  tft.setTextColor(TFT_WHITE);

  // We can now plot text on screen using the "print" class
  tft.println(" Initialised default\n");
  tft.println(" White text");
  
  tft.setTextColor(TFT_RED);
  tft.println(" RED text ...");
  
  tft.setTextColor(TFT_GREEN);
  tft.println(" GREEN text ...");
  
  tft.setTextColor(TFT_BLUE);
  tft.println(" BLUE text ...");

  delay(5000);
}

void loop() {

  tft.invertDisplay( false ); // Where i is true or false
 
  tft.fillScreen(TFT_BLACK);
  tft.drawRect(0, 0, tft.width(), tft.height(), TFT_BLUE);

  tft.setCursor(42, 4, 4);

  tft.setTextColor(TFT_WHITE);
  tft.println(" Invert OFF\n");

  tft.println(" White text ...");
  
  tft.setTextColor(TFT_RED);
  tft.println(" RED text ...");
  
  tft.setTextColor(TFT_GREEN);
  tft.println(" GREEN text ...");
  
  tft.setTextColor(TFT_BLUE);
  tft.println(" BLUE text");

  delay(5000);


  // Binary inversion of colours
  tft.invertDisplay( true ); // Where i is true or false
 
  tft.fillScreen(TFT_BLACK);
  tft.drawRect(0, 0, tft.width(), tft.height(), TFT_BLUE);

  tft.setCursor(42, 4, 4);

  tft.setTextColor(TFT_WHITE);
  tft.println(" Invert ON\n");

  tft.println(" White text ...");
  
  tft.setTextColor(TFT_RED);
  tft.println(" Red text ...");
  
  tft.setTextColor(TFT_GREEN);
  tft.println(" Green text ...");
  
  tft.setTextColor(TFT_BLUE);
  tft.println(" Blue text ...");

  delay(5000);
}
