#include <SD.h>
#include <Wire.h>
#include <RTClib.h>

#define PIN_SPI_CS 4

File myFile;
RTC_DS3231 rtc;
char filename[20];

void setup()
{
    Serial.begin(9600);

    if (!SD.begin(PIN_SPI_CS))
    {
        Serial.println(F("SD CARD FAILED, OR NOT PRESENT!"));
        while (1)
            ; // don't do anything more:
    }

    Serial.println(F("SD CARD INITIALIZED."));

    if (!rtc.begin())
    {
        Serial.println(F("Couldn't find RTC"));
        while (1)
            ; // don't do anything more:
    }

    if (rtc.lostPower())
    {
        Serial.println("RTC lost power, let's set the time!");
        // The following line sets the RTC to the date & time this sketch was compiled
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
        // Uncomment the following line to set the date & time manually
        // rtc.adjust(DateTime(2023, 10, 10, 12, 0, 0));
    }

    DateTime now = rtc.now();
    sprintf(filename, "%04d%02d%02d_%02d%02d%02d.txt", now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second());
    Serial.print(F("Generated filename: "));
    Serial.println(filename);

    // open file for writing
    myFile = SD.open(filename, FILE_WRITE);

    if (!myFile)
    {
        // Try creating the file if it doesn't exist
        Serial.println(F("Creating setup file"));
        myFile = SD.open(filename, FILE_WRITE | O_CREAT);
    }

    if (myFile)
    {
        myFile.println("Logger started");
        myFile.close();
    }
    else
    {
        Serial.print(F("SD Card: error on opening setup file "));
        Serial.println(filename);
    }
}

void loop()
{
    // open file for writing
    myFile = SD.open(filename, FILE_WRITE);

    if (!myFile)
    {
        // Try creating the file if it doesn't exist
        myFile = SD.open(filename, FILE_WRITE | O_CREAT);
    }

    if (myFile)
    {
        DateTime now = rtc.now();
        myFile.print(now.year(), DEC);
        myFile.print('/');
        myFile.print(now.month(), DEC);
        myFile.print('/');
        myFile.print(now.day(), DEC);
        myFile.print(" ");
        myFile.print(now.hour(), DEC);
        myFile.print(':');
        myFile.print(now.minute(), DEC);
        myFile.print(':');
        myFile.print(now.second(), DEC);
        myFile.println(" - Logging data");
        myFile.close();
    }
    else
    {
        Serial.print(F("SD Card: error on opening file "));
        Serial.println(filename);
    }

    delay(10000); // Log every 10 seconds
}