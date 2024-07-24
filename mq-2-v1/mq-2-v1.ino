#define GAS_SENSOR_PIN A3         // Change this to the pin you've connected the gas sensor to
int RL_VALUE = 10000;              // 10 kohm
float RO_CLEAN_AIR_FACTOR = 9.56; // RO_CLEAR_AIR_FACTOR=(Sensor resistance in clean air)/RO,
float Ro = 10;

int CALIBARAION_SAMPLE_TIMES = 50;
int CALIBRATION_SAMPLE_INTERVAL = 500;
int READ_SAMPLE_INTERVAL = 50;
int READ_SAMPLE_TIMES = 5;

float rs_ro = 0;

float LPGLPGCurve[3] = {-2.13, 2.77};

void setup()
{
    Serial.begin(9600); // Initialize serial communication at 9600 baud rate
    pinMode(GAS_SENSOR_PIN, INPUT);
    int timeCal = (CALIBARAION_SAMPLE_TIMES * CALIBRATION_SAMPLE_INTERVAL / 1000);
    Serial.print("Calibrating gas sensor in ");
    Serial.print(timeCal);
    Serial.println(" seconds");
    Serial.println("Calibrating");
    Ro = MQCalibration(GAS_SENSOR_PIN);

    Serial.println("Calibration is done...\n");
    Serial.print("Ro=");
    Serial.print(Ro/1000);
    Serial.println("kohm");
    delay(5000);
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

    delay(5000); // Delay for 1 second
}

float MQResistanceCalculation(int raw_adc)
{
    return (((float)RL_VALUE * (1023 - raw_adc) / raw_adc));
}

float MQCalibration(int mq_pin)
{
    int i;
    float val = 0;

    for (i = 0; i < CALIBARAION_SAMPLE_TIMES; i++)
    {
        val += MQResistanceCalculation(analogRead(mq_pin));
        delay(CALIBRATION_SAMPLE_INTERVAL);
    }
    val = val / CALIBARAION_SAMPLE_TIMES;
    val = val / RO_CLEAN_AIR_FACTOR;
    return val;
}

float MQRead(int mq_pin)
{
    int i;
    float rs = 0;

    for (i = 0; i < READ_SAMPLE_TIMES; i++)
    {
        rs += MQResistanceCalculation(analogRead(mq_pin));
        // Serial.println(rs);
        delay(READ_SAMPLE_INTERVAL);
    }

    rs = rs / READ_SAMPLE_TIMES;

    return rs;
}

long MQGetPercentage(float rs_ro_ratio, float *pcurve)
{

    float res = (pow(10, (pcurve[0] * rs_ro_ratio + pcurve[1])));

    return (long)res;
}

// Power function
int power(int base, int exponent)
{
    int result = 1;
    for (int i = 0; i < exponent; i++)
    {
        result *= base;
    }
    return result;
}
