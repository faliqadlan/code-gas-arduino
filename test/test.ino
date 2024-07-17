#define GAS_SENSOR_PIN A3 // Change this to the pin you've connected the gas sensor to
int RL_VALUE = 1000; //1 kohm

void setup()
{
    Serial.begin(9600); // Initialize serial communication at 9600 baud rate
}

void loop()
{
    float voltA0 = analogRead(GAS_SENSOR_PIN) / 1023.0 * 5.0;
    Serial.println(voltA0); // Print "Hello, World!" to the serial monitor
    // Serial.println("Hello, World! vs code"); // Print "Hello, World!" to the serial monitor

    // caluclate the resistance of the sensor
    float Rs = ((5.0 - voltA0) / voltA0) * RL_VALUE;

    // print the value of the sensor
    Serial.print("Rs: ");
    Serial.println(Rs);

    delay(5000);                             // Delay for 1 second
}