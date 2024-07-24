float RL_VALUE = 10000;

void setup()
{
    Serial.begin(9600);
}

void loop()
{   
    float rawAdc=  analogRead(A0);
    // float voltA0 = analogRead(A0) / 1023.0 * 5.0;
    float Rs = RL_VALUE * ( 1023 -  rawAdc) / rawAdc;
    // Serial.println("volt");
    Serial.println(rawAdc);
    Serial.println(Rs);
    // Serial.println(rawAdc);
    delay(5000);
}