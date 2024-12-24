int sensorIn = A4;

void setup()
{
    Serial.begin(9600);
    // Set the default voltage of the reference voltage
    analogReference(DEFAULT);
}

void loop()
{
    // Read voltage
    int sensorValue = analogRead(sensorIn);
    // The analog signal is converted to a voltage
    float voltage = sensorValue * (5000 / 1024.0);

    Serial.println(sensorValue);
    Serial.println(voltage);

    if (voltage == 0)
    {
        Serial.println("Fault");
    }
    else if (voltage < 400)
    {
        Serial.println("Preheating");
    }
    else
    {
        int voltage_difference = voltage - 400;
        float concentration = voltage_difference * 50.0 / 16.0;
        // Print Voltage
        Serial.print("Voltage: ");
        Serial.print(voltage);
        Serial.println(" mv");
        // Print CO2 concentration
        Serial.print("CO2 Concentration: ");
        Serial.print(concentration);
        Serial.println(" ppm");
    }
    delay(2000);
}