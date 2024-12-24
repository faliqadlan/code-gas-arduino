#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define SCREEN_ADDRESS 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Function prototypes
void displayText(const char *text, uint8_t textSize, uint16_t color, int16_t x, int16_t y, uint16_t bg = BLACK);
void displayNumber(uint8_t number);
void displayASCII(uint8_t ascii);
void scrollScreen();
void scrollPartialScreen();

void setup()
{
    Serial.begin(9600);
    Serial.println("Initializing...");

    if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS))
    {
        Serial.println(F("SSD1306 allocation failed"));
    }

    display.clearDisplay();
    Serial.println("Display initialized");

    displayText("Hello world!", 1, WHITE, 0, 28);
    displayText("Hello world!", 1, BLACK, 0, 28, WHITE);
    displayText("Hello!", 2, WHITE, 0, 24);
    displayText("123456789", 1, WHITE, 0, 28);
    displayNumber(0xFF);
    displayASCII(3);
    scrollScreen();
    scrollPartialScreen();
}

void loop()
{
}

void displayText(const char *text, uint8_t textSize, uint16_t color, int16_t x, int16_t y, uint16_t bg)
{
    display.clearDisplay();
    display.setTextSize(textSize);
    display.setTextColor(color, bg);
    display.setCursor(x, y);
    display.println(text);
    display.display();
    delay(2000);
    Serial.print("Displayed text: ");
    Serial.println(text);
}

void displayNumber(uint8_t number)
{
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 28);
    display.print("0x");
    display.print(number, HEX);
    display.print("(HEX) = ");
    display.print(number, DEC);
    display.println("(DEC)");
    display.display();
    delay(2000);
    Serial.print("Displayed number: 0x");
    Serial.print(number, HEX);
    Serial.print(" (HEX) = ");
    Serial.print(number, DEC);
    Serial.println(" (DEC)");
}

void displayASCII(uint8_t ascii)
{
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(0, 24);
    display.write(ascii);
    display.display();
    delay(2000);
    Serial.print("Displayed ASCII character: ");
    Serial.println(ascii);
}

void scrollScreen()
{
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("Full");
    display.println("screen");
    display.println("scrolling!");
    display.display();
    display.startscrollright(0x00, 0x07);
    delay(2000);
    display.stopscroll();
    delay(1000);
    display.startscrollleft(0x00, 0x07);
    delay(2000);
    display.stopscroll();
    delay(1000);
    display.startscrolldiagright(0x00, 0x07);
    delay(2000);
    display.startscrolldiagleft(0x00, 0x07);
    delay(2000);
    display.stopscroll();
    display.clearDisplay();
    Serial.println("Full screen scrolling completed");
}

void scrollPartialScreen()
{
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("Scroll");
    display.println("some part");
    display.println("of the screen.");
    display.display();
    display.startscrollright(0x00, 0x00);
    Serial.println("Partial screen scrolling started");
}