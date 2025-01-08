#include <EEPROM.h>

void setup()
{
    Serial.begin(9600);
    float value = 123.45; // Example value to save
    EEPROM.put(0, value); // Save the float value to EEPROM at address 0
    Serial.println("Value saved to EEPROM.");
}

void loop()
{
    float retrievedValue;
    EEPROM.get(0, retrievedValue); // Retrieve the float value from EEPROM at address 0
    Serial.print("Retrieved value: ");
    Serial.println(retrievedValue);
    delay(1000); // Wait for a second before repeating
}