#include "DHT.h"
#define DHT11_PIN 2

#define MQ_135_PIN PIN_A1                // Define the analog pin A1 for MQ135 sensor
#define MQ_136_PIN PIN_A2                // Define the analog pin A2 for MQ136 sensor
int RL_MQ_135 = 1000;                    // Define the load resistance on the board, in kilo ohms
int RL_MQ_136 = 1000;                    // Define the load resistance on the board, in kilo ohms
float RO_MQ_135_CLEAN_AIR_FACTOR = 3.55; // RO_CLEAR_AIR_FACTOR=(Sensor resistance in clean air)/RO,
float RO_MQ_136_CLEAN_AIR_FACTOR = 3.55; // RO_CLEAR_AIR_FACTOR=(Sensor resistance in clean air)/RO,
float Ro_MQ_135 = 10;                    // Ro is initialized to 10 kilo ohms
float Ro_MQ_136 = 10;                    // Ro is initialized to 10 kilo ohms

int CALIBARAION_SAMPLE_TIMES = 50;
int CALIBRATION_SAMPLE_INTERVAL = 500;
int READ_SAMPLE_INTERVAL = 50;
int READ_SAMPLE_TIMES = 5;

float Rs_ro_MQ_135 = 0;
float Rs_ro_MQ_136 = 0;

#define CO2_MQ135 1
#define H2S_MQ136 2

float CO2CurveMQ135[2] = {-0.348, 0.705};
float H2SCurveMQ136[2] = {-0.35, 1.547};

float MQ135TempHumCurve33[3] = {0.0004, -0.0261, 1.3869};
float MQ135TempHumCurve85[3] = {0.0003, -0.023, 1.2528};
float MQ136TempHumCurve33[3] = {0.0004, -0.0261, 1.3869};
float MQ136TempHumCurve85[3] = {0.0003, -0.023, 1.2528};

DHT dht11(DHT11_PIN, DHT11);

void setup()
{
    Serial.begin(9600); // Initialize serial communication at 9600 baud rate
    pinMode(MQ_135_PIN, INPUT);
    pinMode(MQ_136_PIN, INPUT);
    int timeCal = (CALIBARAION_SAMPLE_TIMES * CALIBRATION_SAMPLE_INTERVAL / 1000);
    Serial.print("Calibrating gas sensor in ");
    Serial.print(timeCal * 2);
    Serial.println(" seconds");
    Serial.println("Calibrating MQ135");
    Ro_MQ_135 = MQ135Calibration();

    Serial.println("Calibrating MQ136");
    Ro_MQ_136 = MQ136Calibration();

    Serial.println("Calibration is done...\n");

    Serial.print("Ro MQ135=");
    Serial.print(Ro_MQ_135 / 1000);
    Serial.println("kohm");

    Serial.print("Ro MQ136=");
    Serial.print(Ro_MQ_136 / 1000);
    Serial.println("kohm");
    delay(5000);

    dht11.begin(); // init sensor temp hum
}

void loop()
{

    // read humidity
    float humi = dht11.readHumidity();
    // read temperature as Celsius
    float tempC = dht11.readTemperature();

    // ppm co2 mq 135
    long ppmCo2Mq135 = MQ135GetPPM(tempC, humi);

    // ppm h2s mq 136

    long ppmH2sMq136 = MQ136GetPPM(tempC, humi);

    Serial.print("Temperature: ");
    Serial.print(tempC);
    Serial.print(" C, Humidity: ");
    Serial.print(humi);
    Serial.print(" %, CO2 (MQ135): ");
    Serial.print(ppmCo2Mq135);
    Serial.print(" ppm, H2S (MQ136): ");
    Serial.print(ppmH2sMq136);
    Serial.println(" ppm");

    // delay 3s
    delay(3000);
}

float MQ135Calibration()
{
    float val;
    val = MQCalibration(MQ_135_PIN, RO_MQ_135_CLEAN_AIR_FACTOR, RL_MQ_135);

    return val;
}

float MQ136Calibration()
{
    float val;
    val = MQCalibration(MQ_136_PIN, RO_MQ_136_CLEAN_AIR_FACTOR, RL_MQ_136);

    return val;
}

long MQ135GetPPM(float x, float H)
{
    float rs;
    long ppm_val;
    float rs_ro;
    float rs_ro_corr;
    rs = MQRead(MQ_135_PIN, RL_MQ_135);

    rs_ro = rs / Ro_MQ_135;

    rs_ro_corr = RsRoCorrection(x, H, MQ135TempHumCurve33, MQ135TempHumCurve85);

    Serial.print("rs_ro before correction: ");
    Serial.println(rs_ro);

    Serial.print("rs_ro_corr: ");
    Serial.println(rs_ro_corr);

    rs_ro = rs_ro / rs_ro_corr;

    Serial.print("rs_ro after correction: ");
    Serial.println(rs_ro);

    ppm_val = MQGetGasPercentage(rs_ro, CO2_MQ135);

    return ppm_val;
}

long MQ136GetPPM(float x, float H)
{
    float rs;
    long ppm_val;
    float rs_ro;
    float rs_ro_corr;
    rs = MQRead(MQ_136_PIN, RL_MQ_136);

    rs_ro = rs / Ro_MQ_136;

    rs_ro_corr = RsRoCorrection(x, H, MQ136TempHumCurve33, MQ136TempHumCurve85);

    Serial.print("rs_ro before correction: ");
    Serial.println(rs_ro);

    Serial.print("rs_ro_corr: ");
    Serial.println(rs_ro_corr);

    rs_ro = rs_ro / rs_ro_corr;

    Serial.print("rs_ro after correction: ");
    Serial.println(rs_ro);

    ppm_val = MQGetGasPercentage(rs_ro, H2S_MQ136);

    return ppm_val;
}

float MQResistanceCalculation(int raw_adc, float rl_value)
{
    return ((rl_value * (1023 - raw_adc) / raw_adc));
}

float MQCalibration(int mq_pin, float ro_clean_air_factor, float rl_value)
{
    int i;
    float val = 0;

    for (i = 0; i < CALIBARAION_SAMPLE_TIMES; i++)
    {
        val += MQResistanceCalculation(analogRead(mq_pin), rl_value);
        delay(CALIBRATION_SAMPLE_INTERVAL);
    }
    val = val / CALIBARAION_SAMPLE_TIMES;
    val = val / ro_clean_air_factor;
    return val;
}

float MQRead(int mq_pin, float rl_value)
{
    int i;
    float rs = 0;

    for (i = 0; i < READ_SAMPLE_TIMES; i++)
    {
        rs += MQResistanceCalculation(analogRead(mq_pin), rl_value);
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
    if (gas_id == CO2_MQ135)
    {
        return MQGetPercentage(rs_ro_ratio, CO2CurveMQ135);
    }
    else if (gas_id == H2S_MQ136)
    {
        return MQGetPercentage(rs_ro_ratio, H2SCurveMQ136);
    }

    return 0;
}

float RsRoCorrection(float x, float H, float *curve33, float *curve85)
{
    // Coefficients for humidity 33%
    float a1 = curve33[0], b1 = curve33[1], c1 = curve33[2];
    // Coefficients for humidity 85%
    float a2 = curve85[0], b2 = curve85[1], c2 = curve85[2];

    // Linear interpolation or extrapolation to get new coefficients
    float a = a1 + (a2 - a1) * (H - 33) / (85 - 33);
    float b = b1 + (b2 - b1) * (H - 33) / (85 - 33);
    float c = c1 + (c2 - c1) * (H - 33) / (85 - 33);

    // Calculate Rs/Ro based on temperature x
    float y = a * x * x + b * x + c;
    return y;
}