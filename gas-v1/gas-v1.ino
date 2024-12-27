#include "DHT.h"
#define DHT11_PIN 2

#define MQ_135_PIN PIN_A1                // Define the analog pin A1 for MQ135 sensor
#define MQ_136_PIN PIN_A2                // Define the analog pin A2 for MQ136 sensor
#define TGS_2602_PIN PIN_A3              // Define the analog pin A3 for TGS2602 sensor
#define NDIR_PIN PIN_A4                  // Define the analog pin A4 for NDIR sensor
int RL_MQ_135 = 1000;                    // Define the load resistance on the board, in kilo ohms
int RL_MQ_136 = 1000;                    // Define the load resistance on the board, in kilo ohms
int RL_TGS_2602 = 4700;                  // Define the load resistance on the board, in kilo ohms
float RO_MQ_135_CLEAN_AIR_FACTOR = 3.55; // RO_CLEAR_AIR_FACTOR=(Sensor resistance in clean air)/RO,
float RO_MQ_136_CLEAN_AIR_FACTOR = 3.55; // RO_CLEAR_AIR_FACTOR=(Sensor resistance in clean air)/RO,
float RO_TGS_2602_CLEAN_AIR_FACTOR = 1; // RO_CLEAR_AIR_FACTOR=(Sensor resistance in clean air)/RO,
float Ro_MQ_135 = 10;                    // Ro is initialized to 10 kilo ohms
float Ro_MQ_136 = 10;                    // Ro is initialized to 10 kilo ohms
float Ro_TGS_2602 = 10;                  // Ro is initialized to 10 kilo ohms

int CALIBARAION_SAMPLE_TIMES = 50;
int CALIBRATION_SAMPLE_INTERVAL = 500;
int READ_SAMPLE_INTERVAL = 50;
int READ_SAMPLE_TIMES = 5;

float Rs_ro_MQ_135 = 0;
float Rs_ro_MQ_136 = 0;
float Rs_ro_TGS_2602 = 0;

#define CO2_MQ135 1
#define H2S_MQ136 2
#define H2S_TGS2602 3

float CO2CurveMQ135[2] = {-0.348, 0.705};
float H2SCurveMQ136[2] = {-0.35, 1.547};
float H2SCurveTGS2602[2] = {-2.7245, -1.1293};

float MQ135TempHumCurve33[3] = {0.0004, -0.0261, 1.3869};
float MQ135TempHumCurve85[3] = {0.0003, -0.023, 1.2528};
float MQ136TempHumCurve33[3] = {0.0004, -0.0261, 1.3869};
float MQ136TempHumCurve85[3] = {0.0003, -0.023, 1.2528};
float TGS2602TempHumCurve40[3] = {0.0002, -0.0349, 1.5619};
float TGS2602TempHumCurve65[3] = {0.0002, -0.0373, 1.6632};
float TGS2602TempHumCurve85[3] = {0.0003, -0.0443, 1.8361};

DHT dht11(DHT11_PIN, DHT11);

void setup()
{
    // Set the default voltage of the reference voltage
    analogReference(DEFAULT);

    Serial.begin(9600); // Initialize serial communication at 9600 baud rate
    pinMode(MQ_135_PIN, INPUT);
    pinMode(MQ_136_PIN, INPUT);
    int timeCal = (CALIBARAION_SAMPLE_TIMES * CALIBRATION_SAMPLE_INTERVAL / 1000);
    Serial.print("Calibrating gas sensor in ");
    Serial.print(timeCal * 3);
    Serial.println(" seconds");
    Serial.println("Calibrating MQ135");
    Ro_MQ_135 = MQ135Calibration();

    Serial.println("Calibrating MQ136");
    Ro_MQ_136 = MQ136Calibration();

    Serial.println("Calibrating TGS2602");
    Ro_TGS_2602 = TGS2602Calibration();

    Serial.println("Calibration is done...\n");

    Serial.print("Ro MQ135=");
    Serial.print(Ro_MQ_135 / RL_MQ_135);
    Serial.println("kohm");

    Serial.print("Ro MQ136=");
    Serial.print(Ro_MQ_136 / RL_MQ_136);
    Serial.println("kohm");

    Serial.print("Ro TGS2602=");
    Serial.print(Ro_TGS_2602 / RL_TGS_2602);
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

    // ppm h2s tgs 2602
    long ppmH2sTgs2602 = TGS2602GetPPM(tempC, humi);

    // ppm co2 ndir
    long ppmco2ndir = readNDIRCO2(NDIR_PIN);

    Serial.print("Temperature: ");
    Serial.print(tempC);
    Serial.print(" C, Humidity: ");
    Serial.print(humi);
    Serial.print(" %, CO2 (MQ135): ");
    Serial.print(ppmCo2Mq135);
    Serial.print(" ppm, CO2 (NDIR): ");
    Serial.print(ppmco2ndir);
    Serial.print(" ppm, H2S (MQ136): ");
    Serial.print(ppmH2sMq136);
    Serial.print(" ppm, H2S (TGS2602): ");
    Serial.print(ppmH2sTgs2602);
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

float TGS2602Calibration()
{
    float val;
    val = MQCalibration(TGS_2602_PIN, RO_TGS_2602_CLEAN_AIR_FACTOR, RL_TGS_2602);

    return val;
}

float MQ135GetPPM(float x, float H)
{
    float rs;
    long ppm_val;
    float rs_ro;
    float rs_ro_corr;
    rs = MQRead(MQ_135_PIN, RL_MQ_135);

    rs_ro = rs / Ro_MQ_135;

    rs_ro_corr = RsRoCorrection(x, H, MQ135TempHumCurve33, MQ135TempHumCurve85);

    Serial.print("MQ135 rs_ro before correction: ");
    Serial.println(rs_ro);

    Serial.print("MQ135 rs_ro_corr: ");
    Serial.println(rs_ro_corr);

    rs_ro = rs_ro / rs_ro_corr;

    Serial.print("MQ135 rs_ro after correction: ");
    Serial.println(rs_ro);

    ppm_val = MQGetGasPercentage(rs_ro, CO2_MQ135);

    return ppm_val;
}

float MQ136GetPPM(float x, float H)
{
    float rs;
    long ppm_val;
    float rs_ro;
    float rs_ro_corr;
    rs = MQRead(MQ_136_PIN, RL_MQ_136);

    rs_ro = rs / Ro_MQ_136;

    rs_ro_corr = RsRoCorrection(x, H, MQ136TempHumCurve33, MQ136TempHumCurve85);

    Serial.print("MQ136 rs_ro before correction: ");
    Serial.println(rs_ro);

    Serial.print("MQ136 rs_ro_corr: ");
    Serial.println(rs_ro_corr);

    rs_ro = rs_ro / rs_ro_corr;

    Serial.print("MQ136 rs_ro after correction: ");
    Serial.println(rs_ro);

    ppm_val = MQGetGasPercentage(rs_ro, H2S_MQ136);

    return ppm_val;
}

float TGS2602GetPPM(float x, float H)
{
    float rs;
    long ppm_val;
    float rs_ro;
    float rs_ro_corr;
    rs = MQRead(TGS_2602_PIN, RL_TGS_2602);

    rs_ro = rs / Ro_TGS_2602;

    rs_ro_corr = RsRoCorrection3Curve(x, H, TGS2602TempHumCurve40, TGS2602TempHumCurve65, TGS2602TempHumCurve85);

    Serial.print("TGS2602 rs_ro before correction: ");
    Serial.println(rs_ro);

    Serial.print("TGS2602 rs_ro_corr: ");
    Serial.println(rs_ro_corr);

    rs_ro = rs_ro / rs_ro_corr;

    Serial.print("TGS2602 rs_ro after correction: ");
    Serial.println(rs_ro);

    ppm_val = MQGetGasPercentage(rs_ro, H2S_TGS2602);

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

float MQGetPercentage(float rs_ro_ratio, float *pcurve)
{

    float res = (pow(10, (pcurve[0] * log10(rs_ro_ratio) + pcurve[1])));

    return res;
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

float MQGetGasPercentage(float rs_ro_ratio, int gas_id)
{
    if (gas_id == CO2_MQ135)
    {
        return MQGetPercentage(rs_ro_ratio, CO2CurveMQ135);
    }
    else if (gas_id == H2S_MQ136)
    {
        return MQGetPercentage(rs_ro_ratio, H2SCurveMQ136);
    } else if (gas_id == H2S_TGS2602)
    {
        return MQGetPercentage(rs_ro_ratio, H2SCurveTGS2602);
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

float RsRoCorrection3Curve(float x, float H, float *curve40, float *curve65, float *curve85)
{
    // Coefficients for humidity 40%
    float a1 = curve40[0], b1 = curve40[1], c1 = curve40[2];
    // Coefficients for humidity 65%
    float a2 = curve65[0], b2 = curve65[1], c2 = curve65[2];
    // Coefficients for humidity 85%
    float a3 = curve85[0], b3 = curve85[1], c3 = curve85[2];

    // Linear interpolation or extrapolation to get new coefficients
    float a, b, c;
    if (H <= 65)
    {
        a = a1 + (a2 - a1) * (H - 40) / (65 - 40);
        b = b1 + (b2 - b1) * (H - 40) / (65 - 40);
        c = c1 + (c2 - c1) * (H - 40) / (65 - 40);
    }
    else
    {
        a = a2 + (a3 - a2) * (H - 65) / (85 - 65);
        b = b2 + (b3 - b2) * (H - 65) / (85 - 65);
        c = c2 + (c3 - c2) * (H - 65) / (85 - 65);
    }

    // Calculate Rs/Ro based on temperature x
    float y = a * x * x + b * x + c;
    return y;
}

long readNDIRCO2(int sensorIn)
{

    long ppmco2 = 0;
    // Read voltage
    int sensorValue = analogRead(sensorIn);
    // The analog signal is converted to a voltage
    float voltage = sensorValue * (5000.0 / 1023.0);

    Serial.print("NDIR Sensor: ");
    Serial.println(sensorValue);
    Serial.print("NDIR Voltage: ");
    Serial.println(voltage);

    if (voltage == 0)
    {
        Serial.println("NDIR Fault");
    }
    else if (voltage < 400)
    {
        Serial.println("NDIR Preheating");
    }
    else if (voltage > 2000)
    {
        Serial.println("NDIR Exceeding measurement range");
    }
    else
    {
        int voltage_difference = voltage - 400;
        float concentration = voltage_difference * 50.0 / 16.0;
        // Print Voltage
        Serial.print("NDIR Voltage: ");
        Serial.print(voltage);
        Serial.println(" mv");

        ppmco2 = (long)concentration;
    }

    return ppmco2;
}