#define MQ_135_PIN PIN_A1         // Define the analog pin A1 for MQ135 sensor
#define MQ_136_PIN PIN_A2         // Define the analog pin A2 for MQ136 sensor
int RL_VALUE = 1000;              // 1 kohm
float RO_CLEAN_AIR_FACTOR = 3.55; // RO_CLEAR_AIR_FACTOR=(Sensor resistance in clean air)/RO,
float Ro = 10;

int CALIBARAION_SAMPLE_TIMES = 50;
int CALIBRATION_SAMPLE_INTERVAL = 500;
int READ_SAMPLE_INTERVAL = 50;
int READ_SAMPLE_TIMES = 5;

float rs_ro = 0;

#define GAS_LPG 0
#define GAS_CO2 1

float LPGLPGCurve[2] = {-2.14, 2.99};
float CO2CurveMQ135[2] = {-0.348, 0.705};

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
    Serial.print(Ro / 1000);
    Serial.println("kohm");
    delay(5000);
}

void loop()
{
    // long iPPM_LPG = 0;
    long iPPM_CO2 = 0;
    long rs = 0;

    // iPPM_LPG = MQGetGasPercentage(MQRead(GAS_SENSOR_PIN) / Ro, GAS_LPG);
    iPPM_CO2 = MQGetGasPercentage(MQRead(GAS_SENSOR_PIN) / Ro, GAS_CO2);
    rs = MQRead(GAS_SENSOR_PIN);

    Serial.println();
    Serial.print("DATA,TIME,");
    Serial.print(rs);
    Serial.print(",");
    Serial.print(Ro);
    Serial.print(",");
    Serial.print(rs / Ro);
    Serial.print(",");
    Serial.print(log10(rs / Ro));
    Serial.print(",");
    Serial.print(iPPM_CO2);

    // delay 3s
    delay(3000);
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

    float res = (pow(10, (pcurve[0] * log10(rs_ro_ratio) + pcurve[1])));

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

long MQGetGasPercentage(float rs_ro_ratio, int gas_id)
{
    if (gas_id == GAS_LPG)
    {
        return MQGetPercentage(rs_ro_ratio, LPGLPGCurve);
    }
    else if (gas_id == GAS_CO2)
    {
        return MQGetPercentage(rs_ro_ratio, CO2CurveMQ135);
    }

    return 0;
}