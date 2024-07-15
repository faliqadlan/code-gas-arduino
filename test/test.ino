void setup()
{
    Serial.begin(9600); // Initialize serial communication at 9600 baud rate
}

void loop()
{
    // Serial.println(analogRead(A0) / 1023.0 * 5.0); // Print "Hello, World!" to the serial monitor
    Serial.println("Hello, World! vs code"); // Print "Hello, World!" to the serial monitor
    delay(1000);                             // Delay for 1 second
}