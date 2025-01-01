#include <SPI.h>
#include <SD.h>

const int chipSelect = 10;

void setup()
{
    Serial.begin(9600);
    while (!Serial)
    {
        ; // wait for serial port to connect. Needed for native USB port only
    }

    Serial.print("Initializing SD card...");

    if (!SD.begin(chipSelect))
    {
        Serial.println("initialization failed!");
        return;
    }
    Serial.println("initialization done.");

    // Open a file for writing
    File dataFile = SD.open("example.txt", FILE_WRITE);

    // If the file opened okay, write to it:
    if (dataFile)
    {
        Serial.println("Writing to example.txt...");
        dataFile.println("Hello, world!");
        dataFile.close();
        Serial.println("done.");
    }
    else
    {
        // If the file didn't open, print an error:
        Serial.println("error opening example.txt");
    }

    // Re-open the file for reading:
    dataFile = SD.open("example.txt");
    if (dataFile)
    {
        Serial.println("example.txt:");

        // Read from the file until there's nothing else in it:
        while (dataFile.available())
        {
            Serial.write(dataFile.read());
        }
        dataFile.close();
    }
    else
    {
        // If the file didn't open, print an error:
        Serial.println("error opening example.txt");
    }
}

void loop()
{
    // nothing happens after setup
}