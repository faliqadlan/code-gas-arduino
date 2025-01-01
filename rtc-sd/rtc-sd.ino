#include <Wire.h>
#include <RTClib.h>
#include <SD.h>

RTC_DS1307 rtc;

void setup()
{
    Serial.begin(9600);
    // Initialize RTC
    if (!rtc.begin())
    {
        Serial.println("Couldn't find RTC");
        while (1)
            ;
    }

    // Initialize SD card
    if (!SD.begin(4))
    {
        Serial.println("Initialization failed!");
        return;
    }
    Serial.println("SD card initialized.");
}

void loop()
{
    // Check for "Start Monitor" action
    if (startMonitorTriggered())
    {
        logDateTime();
    }
}

bool startMonitorTriggered()
{
    // Implement logic to detect "Start Monitor" action
    return false; // Placeholder
}

void logDateTime()
{
    DateTime now = rtc.now();
    String logFileName = "log_" + String(now.year()) + "_" + String(now.month()) + "_" + String(now.day()) + "_" + String(now.hour()) + "_" + String(now.minute()) + "_" + String(now.second()) + ".txt";

    File logFile = SD.open(logFileName, FILE_WRITE);
    if (logFile)
    {
        logFile.print(now.year(), DEC);
        logFile.print('/');
        logFile.print(now.month(), DEC);
        logFile.print('/');
        logFile.print(now.day(), DEC);
        logFile.print(" ");
        logFile.print(now.hour(), DEC);
        logFile.print(':');
        logFile.print(now.minute(), DEC);
        logFile.print(':');
        logFile.print(now.second(), DEC);
        logFile.println();
        logFile.close();
        Serial.println("Logged date and time.");
    }
    else
    {
        Serial.println("Error opening log file.");
    }
}