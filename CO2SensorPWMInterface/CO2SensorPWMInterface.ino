/*!
 * @file  CO2SensorPWMInterface.ino
 * @brief  This example The sensors detect CO2
 * @details  Infrared CO2 Sensor range : 400-4980ppm
 * @copyright  Copyright (c) 2010 DFRobot Co.Ltd (http://www.dfrobot.com)
 * @license  The MIT License (MIT)
 * @author  [qsjhyy](yihuan.huang@dfrobot.com)
 * @version  V2.0
 * @date  2023-01-15
 */

#define SENSOR_DATA_PIN (2)  // Sensor PWM interface
#define INTERRUPT_NUMBER (0) // interrupt number

// Used in interrupt, calculate pulse width variable
volatile unsigned long pwmHighStartTicks = 0, pwmHighEndTicks = 0;
volatile unsigned long pwmHighVal = 0, pwmLowVal = 0;
// interrupt flag
volatile uint8_t flag = 0;

void interruptChange()
{   
    Serial.println(digitalRead(SENSOR_DATA_PIN));
    if (digitalRead(SENSOR_DATA_PIN))
    {
        pwmHighStartTicks = micros(); // store the current micros() value
        Serial.print("Rising edge detected. pwmHighStartTicks: ");
        Serial.println(pwmHighStartTicks);
        if (2 == flag)
        {
            flag = 4;
            if (pwmHighStartTicks > pwmHighEndTicks)
            {
                pwmLowVal = pwmHighStartTicks - pwmHighEndTicks;
                Serial.print("pwmLowVal: ");
                Serial.println(pwmLowVal);
            }
        }
        else
        {
            flag = 1;
        }
    }
    else
    {
        pwmHighEndTicks = micros(); // store the current micros() value
        Serial.print("Falling edge detected. pwmHighEndTicks: ");
        Serial.println(pwmHighEndTicks);
        if (1 == flag)
        {
            flag = 2;
            if (pwmHighEndTicks > pwmHighStartTicks)
            {
                pwmHighVal = pwmHighEndTicks - pwmHighStartTicks;
                Serial.print("pwmHighVal: ");
                Serial.println(pwmHighVal);
            }
        }
    }
}

void setup()
{
    // put your setup code here, to run once:
    Serial.begin(9600);
    Serial.println("beginning...");
    pinMode(SENSOR_DATA_PIN, INPUT);
    attachInterrupt(INTERRUPT_NUMBER, interruptChange, CHANGE);
}

void loop()
{
    if (flag == 4)
    {
        flag = 1;
        float pwmHighVal_ms = (pwmHighVal * 1000.0) / (pwmLowVal + pwmHighVal);
        Serial.print("pwmHighVal_ms: ");
        Serial.println(pwmHighVal_ms);

        if (pwmHighVal_ms < 0.01)
        {
            Serial.println("Fault");
        }
        else if (pwmHighVal_ms < 80.00)
        {
            Serial.println("preheating");
        }
        else if (pwmHighVal_ms < 998.00)
        {
            float concentration = (pwmHighVal_ms - 2) * 5;
            // Print pwmHighVal_ms
            Serial.print("pwmHighVal_ms: ");
            Serial.print(pwmHighVal_ms);
            Serial.println(" ms");
            // Print CO2 concentration
            Serial.print("CO2 concentration: ");
            Serial.print(concentration);
            Serial.println(" ppm");
        }
        else
        {
            Serial.println("Beyond the maximum range : 398~4980ppm");
        }
        Serial.println();
    }
}