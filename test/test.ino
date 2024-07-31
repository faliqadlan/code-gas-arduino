float RL_VALUE = 1000;

void setup()
{
    Serial.begin(9600);
}

void loop()
{   
    // float rawAdc=  analogRead(A0);
    // float voltA0 = analogRead(A0) / 1023.0 * 5.0;
    // float Rs = RL_VALUE * ( 1023 -  rawAdc) / rawAdc;
    // Serial.println("volt");
    // Serial.println(voltA0);
    // Serial.println(Rs);
    // Serial.println(rawAdc);
    Serial.println("-------start-------");
    Serial.println(analogRead(A1) / 1023.0 * 5.0);
    Serial.println(analogRead(A2) / 1023.0 * 5.0);
    Serial.println(analogRead(A3) / 1023.0 * 5.0);
    Serial.println(analogRead(A4) / 1023.0 * 5.0);
    Serial.println(analogRead(A5)  / 1023.0 * 5.0);
    Serial.println("-------end-------");
    delay(5000);
}