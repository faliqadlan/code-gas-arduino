#define GAS_SENSOR_PIN A3 // Change this to the pin you've connected the gas sensor to

const int MQ_PIN = A0;
float RO_CLEAN_AIR_FACTOR = 9.83;

float Ro = 10;
int RL_VALUE = 10;

int CALIBARAION_SAMPLE_TIMES = 50;
int CALIBRATION_SAMPLE_INTERVAL = 500;
int READ_SAMPLE_INTERVAL = 50;
int READ_SAMPLE_TIMES = 5;

float rs_ro = 0;

float LPGCurve[3] = {2.3, 0.21, -0.47};
float COCurve[3] = {2.3, 0.72, -0.34};
float SmokeCurve[3] = {2.3, 0.53, -0.44};

#define GAS_LPG 0
#define GAS_CO 1
#define GAS_SMOKE 2

void setup()
{
    Serial.begin(9600);
    pinMode(GAS_SENSOR_PIN, INPUT);

    int timeCal = (CALIBARAION_SAMPLE_TIMES * CALIBRATION_SAMPLE_INTERVAL / 1000);

    Serial.print("Calibrating gas sensor in ");
    Serial.print(timeCal);
    Serial.println(" seconds");

    Serial.println("Calibrating");
    Ro = MQCalibration(MQ_PIN);

    Serial.println("Calibration is done...\n");
    Serial.print("Ro=");
    Serial.print(Ro);
    Serial.println("kohm");
    delay(3000);

    Serial.println("LABEL,Time,RS,Ro,RS/Ro,Smoke,CO,LPG");
}

void loop()
{
    long iPPM_LPG = 0;
    long iPPM_CO = 0;
    long iPPM_Smoke = 0;
    long rs = 0;

    iPPM_LPG = MQGetGasPercentage(MQRead(MQ_PIN) / Ro, GAS_LPG);
    iPPM_CO = MQGetGasPercentage(MQRead(MQ_PIN) / Ro, GAS_CO);
    iPPM_Smoke = MQGetGasPercentage(MQRead(MQ_PIN) / Ro, GAS_SMOKE);
    rs = MQRead(MQ_PIN);

    Serial.println();
    Serial.print("DATA,TIME,");
    Serial.print(rs);
    Serial.print(",");
    Serial.print(Ro);
    Serial.print(",");
    Serial.print(rs / Ro);
    Serial.print(",");
    Serial.print(iPPM_Smoke);
    Serial.print(",");
    Serial.print(iPPM_CO);
    Serial.print(",");
    Serial.print(iPPM_LPG);

    delay(200);
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

long MQGetGasPercentage(float rs_ro_ratio, int gas_id)
{
    if (gas_id == GAS_LPG)
    {
        return MQGetPercentage(rs_ro_ratio, LPGCurve);
    }
    else if (gas_id == GAS_CO)
    {
        return MQGetPercentage(rs_ro_ratio, COCurve);
    }
    else if (gas_id == GAS_SMOKE)
    {
        return MQGetPercentage(rs_ro_ratio, SmokeCurve);
    }

    return 0;
}

long MQGetPercentage(float rs_ro_ratio, float *pcurve)
{

    float res = (pow(10, (((log(rs_ro_ratio) - pcurve[1]) / pcurve[2]) + pcurve[0])));

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