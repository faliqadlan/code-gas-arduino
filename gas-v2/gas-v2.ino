#include "DHT.h"
#include <SD.h>
#include <Wire.h>
#include <RTClib.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <EEPROM.h>

#define DHT11_PIN 46
#define PIN_SPI_CS 53
#define CalibrationButton 49
#define MeasurementButton 48

#define MQ_135_PIN PIN_A0                // Define the analog pin A1 for MQ135 sensor
#define MQ_136_PIN PIN_A2                // Define the analog pin A2 for MQ136 sensor
#define TGS_2602_PIN PIN_A1              // Define the analog pin A3 for TGS2602 sensor
#define NDIR_PIN PIN_A3                  // Define the analog pin A4 for NDIR sensor
int R1000kohm = 1000;                    // Define the load resistance on the board, in kilo ohms
int RL_MQ_135 = 1000;                    // Define the load resistance on the board, in kilo ohms
int RL_MQ_136 = 1000;                    // Define the load resistance on the board, in kilo ohms
int RL_TGS_2602 = 4700;                  // Define the load resistance on the board, in kilo ohms
float RO_MQ_135_CLEAN_AIR_FACTOR = 3.55; // RO_CLEAR_AIR_FACTOR=(Sensor resistance in clean air)/RO,
float RO_MQ_136_CLEAN_AIR_FACTOR = 3.55; // RO_CLEAR_AIR_FACTOR=(Sensor resistance in clean air)/RO,
float RO_TGS_2602_CLEAN_AIR_FACTOR = 1;  // RO_CLEAR_AIR_FACTOR=(Sensor resistance in clean air)/RO,
float Ro_MQ_135 = 10;                    // Ro is initialized to 10 kilo ohms
float Ro_MQ_136 = 10;                    // Ro is initialized to 10 kilo ohms
float Ro_TGS_2602 = 10;                  // Ro is initialized to 10 kilo ohms

int CALIBARAION_SAMPLE_TIMES = 50;     // Number of samples for calibration
int CALIBRATION_SAMPLE_INTERVAL = 500; // Interval between samples during calibration
int READ_SAMPLE_INTERVAL = 50;         // Interval between samples during reading
int READ_SAMPLE_TIMES = 5;             // Number of samples for reading

float Rs_ro_MQ_135 = 0;
float Rs_ro_MQ_136 = 0;
float Rs_ro_TGS_2602 = 0;

#define CO2_MQ135 1
#define H2S_MQ136 2
#define H2S_TGS2602 3

float CO2CurveMQ135[2] = {-2.85, 2.0451};      // Curve for CO2 gas for MQ135 sensor
float H2SCurveMQ136[2] = {-3.5, 1.547};        // Curve for H2S gas for MQ136 sensor
float H2SCurveTGS2602[2] = {-2.7245, -1.1293}; // Curve for H2S gas for TGS2602 sensor

float MQ135TempHumCurve33[3] = {0.0004, -0.0261, 1.3869};   // Temp-Humidity correction curve for MQ135 at 33% humidity
float MQ135TempHumCurve85[3] = {0.0003, -0.023, 1.2528};    // Temp-Humidity correction curve for MQ135 at 85% humidity
float MQ136TempHumCurve33[3] = {0.0004, -0.0261, 1.3869};   // Temp-Humidity correction curve for MQ136 at 33% humidity
float MQ136TempHumCurve85[3] = {0.0003, -0.023, 1.2528};    // Temp-Humidity correction curve for MQ136 at 85% humidity
float TGS2602TempHumCurve40[3] = {0.0002, -0.0349, 1.5619}; // Temp-Humidity correction curve for TGS2602 at 40% humidity
float TGS2602TempHumCurve65[3] = {0.0002, -0.0373, 1.6632}; // Temp-Humidity correction curve for TGS2602 at 65% humidity
float TGS2602TempHumCurve85[3] = {0.0003, -0.0443, 1.8361}; // Temp-Humidity correction curve for TGS2602 at 85% humidity

DHT dht11(DHT11_PIN, DHT11);

RTC_DS3231 rtc;

File myFile;
char filename[13];

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define SCREEN_ADDRESS 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

bool isCalibrating = false;
bool isMeasuring = false;

void displayText(const char *text, uint8_t textSize, uint16_t color, int16_t x, int16_t y, uint16_t bg = BLACK);
void displayNumber(uint8_t number);

void setup()
{
    Serial.begin(9600); // Initialize serial communication at 9600 baud rate

    dht11.begin(); // init sensor temp humidity

    if (!rtc.begin())
    {
        Serial.println("Couldn't find RTC");
    }

    if (rtc.lostPower())
    {
        Serial.println("RTC lost power, let's set the time!");
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }

    if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS))
    {
        Serial.println(F("SSD1306 allocation failed"));
    }

    if (!SD.begin(PIN_SPI_CS))
    {
        Serial.println(F("SD CARD FAILED, OR NOT PRESENT!"));
    }

    displayText("Start...", 1, WHITE, 0, 28);
    display.clearDisplay();
    Serial.println("Display initialized");

    analogReference(DEFAULT);

    pinMode(MQ_135_PIN, INPUT);
    pinMode(MQ_136_PIN, INPUT);
    pinMode(CalibrationButton, INPUT_PULLUP);
    pinMode(MeasurementButton, INPUT_PULLUP);

    EEPROM.get(0, Ro_MQ_135);
    EEPROM.get(sizeof(float), Ro_MQ_136);
    EEPROM.get(2 * sizeof(float), Ro_TGS_2602);

    if (isnan(Ro_MQ_135) || isnan(Ro_MQ_136) || isnan(Ro_TGS_2602))
    {
        int timeCal = (CALIBARAION_SAMPLE_TIMES * CALIBRATION_SAMPLE_INTERVAL / 1000);
        displayText("Calibrating gas sensor in", 1, WHITE, 0, 0);
        displayText(String(timeCal * 3).c_str(), 1, WHITE, 0, 10);
        displayText("seconds", 1, WHITE, 0, 20);
        delay(1000);
        display.clearDisplay();

        Serial.println("Calibrating MQ135");
        displayText("Calibrating MQ135", 1, WHITE, 0, 0);
        Ro_MQ_135 = MQ135Calibration();
        EEPROM.put(0, Ro_MQ_135);
        displayCalibrationResult("MQ135", Ro_MQ_135 / R1000kohm);

        Serial.println("Calibrating MQ136");
        displayText("Calibrating MQ136", 1, WHITE, 0, 0);
        Ro_MQ_136 = MQ136Calibration();
        EEPROM.put(sizeof(float), Ro_MQ_136);
        displayCalibrationResult("MQ136", Ro_MQ_136 / R1000kohm);

        Serial.println("Calibrating TGS2602");
        displayText("Calibrating TGS2602", 1, WHITE, 0, 0);
        Ro_TGS_2602 = TGS2602Calibration();
        EEPROM.put(2 * sizeof(float), Ro_TGS_2602);
        displayCalibrationResult("TGS2602", Ro_TGS_2602 / R1000kohm);

        displayText("Calibration done", 1, WHITE, 0, 0);
        delay(2000);
        display.clearDisplay();
    }

    saveCalibrationToSD("MQ135", Ro_MQ_135 / R1000kohm);
    saveCalibrationToSD("MQ136", Ro_MQ_136 / R1000kohm);
    saveCalibrationToSD("TGS2602", Ro_TGS_2602 / R1000kohm);

    Serial.print("Ro MQ135=");
    Serial.print(Ro_MQ_135 / R1000kohm);
    Serial.println("kohm");

    Serial.print("Ro MQ136=");
    Serial.print(Ro_MQ_136 / R1000kohm);
    Serial.println("kohm");

    Serial.print("Ro TGS2602=");
    Serial.print(Ro_TGS_2602 / R1000kohm);
    Serial.println("kohm");

    delay(5000);
}

void loop()
{
    displayText("Press Blue for Calibration", 1, WHITE, 0, 0);
    displayText("Press Green for Measurement", 1, WHITE, 0, 10);

    if (digitalRead(CalibrationButton) == LOW && !isCalibrating)
    {
        Serial.println("Calibrating...");
        isCalibrating = true;
        displayText("Calibrating...", 1, WHITE, 0, 0);
        calibrateSensors();
        isCalibrating = false;
    }

    if (digitalRead(MeasurementButton) == LOW && !isMeasuring)
    {
        Serial.println("Measuring...");
        isMeasuring = true;
        displayText("Measuring...", 1, WHITE, 0, 0);
        measureAndLog();
        isMeasuring = false;
    }

    delay(100); // Debounce delay
}

void calibrateSensors()
{
    Ro_MQ_135 = MQ135Calibration();
    EEPROM.put(0, Ro_MQ_135);
    displayCalibrationResult("MQ135", Ro_MQ_135 / R1000kohm);
    saveCalibrationToSD("MQ135", Ro_MQ_135 / R1000kohm);

    Ro_MQ_136 = MQ136Calibration();
    EEPROM.put(sizeof(float), Ro_MQ_136);
    displayCalibrationResult("MQ136", Ro_MQ_136 / R1000kohm);
    saveCalibrationToSD("MQ136", Ro_MQ_136 / R1000kohm);

    Ro_TGS_2602 = TGS2602Calibration();
    EEPROM.put(2 * sizeof(float), Ro_TGS_2602);
    displayCalibrationResult("TGS2602", Ro_TGS_2602 / R1000kohm);
    saveCalibrationToSD("TGS2602", Ro_TGS_2602 / R1000kohm);

    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("Calibration Results:");
    display.print("MQ135 Ro=");
    display.print(Ro_MQ_135 / R1000kohm);
    display.print(" kohm\nMQ136 Ro=");
    display.print(Ro_MQ_136 / R1000kohm);
    display.print(" kohm\nTGS2602 Ro=");
    display.print(Ro_TGS_2602 / R1000kohm);
    display.print(" kohm");
    display.display();
    delay(10000); // Display result for 5 seconds

    Serial.println("Calibration is done...\n");
}

void displayCalibrationResult(const char *sensor, float value)
{
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print(sensor);
    display.print(" Ro=");
    display.print(value);
    display.println(" kohm");
    display.display();
    delay(2000); // Display result for 2 seconds
}

void measureAndLog()
{
    if (isnan(Ro_MQ_135) || isnan(Ro_MQ_136) || isnan(Ro_TGS_2602))
    {
        Serial.println("Calibration values not found. Please calibrate first.");
        return;
    }

    Serial.println("Loop is running");
    int totalcount = 10;

    for (int i = 0; i < totalcount; i++)
    {
        float humi = dht11.readHumidity();
        float tempC = dht11.readTemperature();

        long ppmCo2Mq135 = MQ135GetPPM(tempC, humi);
        long ppmH2sMq136 = MQ136GetPPM(tempC, humi);
        long ppmH2sTgs2602 = TGS2602GetPPM(tempC, humi);
        long ppmco2ndir = readNDIRCO2(NDIR_PIN);

        int analogMQ135 = analogRead(MQ_135_PIN);
        int analogMQ136 = analogRead(MQ_136_PIN);
        int analogTGS2602 = analogRead(TGS_2602_PIN);
        int analogNDIR = analogRead(NDIR_PIN);

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

        Serial.print("Analog Readings - MQ135: ");
        Serial.print(analogMQ135);
        Serial.print(", NDIR: ");
        Serial.print(analogNDIR);
        Serial.print(", MQ136: ");
        Serial.print(analogMQ136);
        Serial.print(", TGS2602: ");
        Serial.println(analogTGS2602);

        display.clearDisplay();
        display.setCursor(0, 0);
        display.print("Analog MQ135: ");
        display.print(analogMQ135);
        display.print("\nAnalog NDIR: ");
        display.print(analogNDIR);
        display.print("\nAnalog MQ136: ");
        display.print(analogMQ136);
        display.print("\nAnalog TGS2602: ");
        display.print(analogTGS2602);

        display.display();
        delay(5000); // Display result for few second

        if (i == totalcount - 1)
        {
            delay(10000); // Delay for 10 seconds on the last iteration
        }

        display.clearDisplay();
        display.setCursor(0, 0);
        display.print("Temp: ");
        display.print(tempC);
        display.print(" C\nHum: ");
        display.print(humi);
        display.print(" %\nCO2 (MQ135): ");
        display.print(ppmCo2Mq135);
        display.print(" ppm\nCO2 (NDIR): ");
        display.print(ppmco2ndir);
        display.print(" ppm\nH2S (MQ136): ");
        display.print(ppmH2sMq136);
        display.print(" ppm\nH2S (TGS2602): ");
        display.print(ppmH2sTgs2602);
        display.print(" ppm\nCount left: ");
        display.print(totalcount - i + 1);
        display.display();
        delay(5000); // Display result for few second

        if (i == totalcount - 1)
        {
            delay(10000); // Delay for 10 seconds on the last iteration
        }

        logToSD("Temp: ", tempC);
        logToSD("Hum: ", humi);
        logToSD("PPM CO2 (MQ135): ", ppmCo2Mq135);
        logToSD("PPM CO2 (NDIR): ", ppmco2ndir);
        logToSD("PPM H2S (MQ136): ", ppmH2sMq136);
        logToSD("PPM H2S (TGS2602): ", ppmH2sTgs2602);
        logToSD("Analog MQ135: ", analogMQ135);
        logToSD("Analog NDIR: ", analogNDIR);
        logToSD("Analog MQ136: ", analogMQ136);
        logToSD("Analog TGS2602: ", analogTGS2602);
    }
}

/**
 * Calibrates the MQ135 sensor.
 * @return The calibrated Ro value for the MQ135 sensor.
 */
float MQ135Calibration()
{
    float val;
    val = MQCalibration(MQ_135_PIN, RO_MQ_135_CLEAN_AIR_FACTOR, RL_MQ_135);

    return val;
}

/**
 * Calibrates the MQ136 sensor.
 * @return The calibrated Ro value for the MQ136 sensor.
 */
float MQ136Calibration()
{
    float val;
    val = MQCalibration(MQ_136_PIN, RO_MQ_136_CLEAN_AIR_FACTOR, RL_MQ_136);

    return val;
}

/**
 * Calibrates the TGS2602 sensor.
 * @return The calibrated Ro value for the TGS2602 sensor.
 */
float TGS2602Calibration()
{
    float val;
    val = MQCalibration(TGS_2602_PIN, RO_TGS_2602_CLEAN_AIR_FACTOR, RL_TGS_2602);

    return val;
}

/**
 * Gets the CO2 PPM value from the MQ135 sensor.
 * @param x The temperature value.
 * @param H The humidity value.
 * @return The CO2 PPM value.
 */
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
    logToSD("MQ135 rs_ro before correction: ", rs_ro);

    Serial.print("MQ135 rs_ro_corr: ");
    Serial.println(rs_ro_corr);
    logToSD("MQ135 rs_ro_corr: ", rs_ro_corr);

    rs_ro = rs_ro / rs_ro_corr;

    Serial.print("MQ135 rs_ro after correction: ");
    Serial.println(rs_ro);
    logToSD("MQ135 rs_ro after correction: ", rs_ro);

    ppm_val = MQGetGasPercentage(rs_ro, CO2_MQ135);

    return ppm_val;
}

/**
 * Gets the H2S PPM value from the MQ136 sensor.
 * @param x The temperature value.
 * @param H The humidity value.
 * @return The H2S PPM value.
 */
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
    logToSD("MQ136 rs_ro before correction: ", rs_ro);

    Serial.print("MQ136 rs_ro_corr: ");
    Serial.println(rs_ro_corr);
    logToSD("MQ136 rs_ro_corr: ", rs_ro_corr);

    rs_ro = rs_ro / rs_ro_corr;

    Serial.print("MQ136 rs_ro after correction: ");
    Serial.println(rs_ro);
    logToSD("MQ136 rs_ro after correction: ", rs_ro);

    ppm_val = MQGetGasPercentage(rs_ro, H2S_MQ136);

    return ppm_val;
}

/**
 * Gets the H2S PPM value from the TGS2602 sensor.
 * @param x The temperature value.
 * @param H The humidity value.
 * @return The H2S PPM value.
 */
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
    logToSD("TGS2602 rs_ro before correction: ", rs_ro);

    Serial.print("TGS2602 rs_ro_corr: ");
    Serial.println(rs_ro_corr);
    logToSD("TGS2602 rs_ro_corr: ", rs_ro_corr);

    rs_ro = rs_ro / rs_ro_corr;

    Serial.print("TGS2602 rs_ro after correction: ");
    Serial.println(rs_ro);
    logToSD("TGS2602 rs_ro after correction: ", rs_ro);

    ppm_val = MQGetGasPercentage(rs_ro, H2S_TGS2602);

    return ppm_val;
}

/**
 * Calculates the resistance of the sensor.
 * @param raw_adc The raw ADC value.
 * @param rl_value The load resistance value.
 * @return The calculated resistance.
 */
float MQResistanceCalculation(int raw_adc, float rl_value)
{
    return ((rl_value * (1023 - raw_adc) / raw_adc));
}

/**
 * Calibrates the sensor.
 * @param mq_pin The pin number of the sensor.
 * @param ro_clean_air_factor The clean air factor for the sensor.
 * @param rl_value The load resistance value.
 * @return The calibrated Ro value.
 */
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

/**
 * Reads the sensor value.
 * @param mq_pin The pin number of the sensor.
 * @param rl_value The load resistance value.
 * @return The sensor resistance value.
 */
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

/**
 * Gets the gas percentage.
 * @param rs_ro_ratio The Rs/Ro ratio.
 * @param pcurve The curve for the gas.
 * @return The gas percentage.
 */
float MQGetPercentage(float rs_ro_ratio, float *pcurve)
{
    float res = (pow(10, (pcurve[0] * log10(rs_ro_ratio) + pcurve[1])));

    return res;
}

/**
 * Power function.
 * @param base The base value.
 * @param exponent The exponent value.
 * @return The result of base raised to the power of exponent.
 */
int power(int base, int exponent)
{
    int result = 1;
    for (int i = 0; i < exponent; i++)
    {
        result *= base;
    }
    return result;
}

/**
 * Gets the gas percentage for a specific gas.
 * @param rs_ro_ratio The Rs/Ro ratio.
 * @param gas_id The gas ID.
 * @return The gas percentage.
 */
float MQGetGasPercentage(float rs_ro_ratio, int gas_id)
{
    if (gas_id == CO2_MQ135)
    {
        return MQGetPercentage(rs_ro_ratio, CO2CurveMQ135);
    }
    else if (gas_id == H2S_MQ136)
    {
        return MQGetPercentage(rs_ro_ratio, H2SCurveMQ136);
    }
    else if (gas_id == H2S_TGS2602)
    {
        return MQGetPercentage(rs_ro_ratio, H2SCurveTGS2602);
    }

    return 0;
}

/**
 * Corrects the Rs/Ro ratio based on temperature and humidity.
 * @param x The temperature value.
 * @param H The humidity value.
 * @param curve33 The curve for 33% humidity.
 * @param curve85 The curve for 85% humidity.
 * @return The corrected Rs/Ro ratio.
 */
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

/**
 * Corrects the Rs/Ro ratio based on temperature and humidity using three curves.
 * @param x The temperature value.
 * @param H The humidity value.
 * @param curve40 The curve for 40% humidity.
 * @param curve65 The curve for 65% humidity.
 * @param curve85 The curve for 85% humidity.
 * @return The corrected Rs/Ro ratio.
 */
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

/**
 * Reads the CO2 concentration from the NDIR sensor.
 * @param sensorIn The pin number of the NDIR sensor.
 * @return The CO2 concentration in PPM.
 */
long readNDIRCO2(int sensorIn)
{
    long ppmco2 = 0;
    // Read voltage
    int sensorValue = analogRead(sensorIn);
    // The analog signal is converted to a voltage
    float voltage = sensorValue * (5000.0 / 1023.0);

    Serial.print("NDIR Sensor: ");
    Serial.println(sensorValue);
    logToSD("NDIR Sensor: ", sensorValue);

    Serial.print("NDIR Voltage: ");
    Serial.println(voltage);
    logToSD("NDIR Voltage: ", voltage);

    if (voltage == 0)
    {
        Serial.println("NDIR Fault");
        logToSD("NDIR Fault");
    }
    else if (voltage < 400)
    {
        Serial.println("NDIR Preheating");
        logToSD("NDIR Preheating");
    }
    else if (voltage > 2000)
    {
        Serial.println("NDIR Exceeding measurement range");
        logToSD("NDIR Exceeding measurement range");
    }
    else
    {
        int voltage_difference = voltage - 400;
        float concentration = voltage_difference * 50.0 / 16.0;
        // Print Voltage
        Serial.print("NDIR Voltage: ");
        Serial.print(voltage);
        Serial.println(" mv");
        logToSD("NDIR Voltage: ", voltage);

        ppmco2 = (long)concentration;
    }

    return ppmco2;
}

void logToSD(const char *message, float value)
{
    if (SD.begin(PIN_SPI_CS))
    {
        DateTime now = rtc.now();
        snprintf(filename, sizeof(filename), "%02d%02d%02d%02d.txt", now.month(), now.day(), now.year() % 100, now.hour());
        myFile = SD.open(filename, FILE_WRITE);
        if (myFile)
        {
            myFile.print(now.timestamp());
            myFile.print(" - ");
            myFile.print(message);
            myFile.println(value);
            myFile.close();

            // Log success message
            Serial.println("Data written to SD card successfully.");
        }
        else
        {
            // Log failure message
            Serial.println("Failed to open file for writing.");
        }
    }
    else
    {
        // Log failure message
        Serial.println("SD card initialization failed.");
    }
}

void logToSD(const char *message)
{
    if (SD.begin(PIN_SPI_CS))
    {
        DateTime now = rtc.now();
        snprintf(filename, sizeof(filename), "%02d%02d%02d%02d.txt", now.month(), now.day(), now.year() % 100, now.hour());
        myFile = SD.open(filename, FILE_WRITE);
        if (myFile)
        {
            myFile.print(now.timestamp());
            myFile.print(" - ");
            myFile.println(message);
            myFile.close();

            // Log success message
            Serial.println("Data written to SD card successfully.");
        }
        else
        {
            // Log failure message
            Serial.println("Failed to open file for writing.");
        }
    }
    else
    {
        // Log failure message
        Serial.println("SD card initialization failed.");
    }
}

void saveCalibrationToSD(const char *sensor, float value)
{
    if (SD.begin(PIN_SPI_CS))
    {
        DateTime now = rtc.now();
        snprintf(filename, sizeof(filename), "%02d%02d%02d%02d.txt", now.month(), now.day(), now.year() % 100, now.hour());
        myFile = SD.open(filename, FILE_WRITE);
        if (myFile)
        {
            myFile.print(now.timestamp());
            myFile.print(" - ");
            myFile.print(sensor);
            myFile.print(" Ro=");
            myFile.print(value);
            myFile.println(" kohm");
            myFile.close();
            Serial.println("Calibration data written to SD card successfully.");
        }
        else
        {
            Serial.println("Failed to open file for writing calibration data.");
        }
    }
    else
    {
        Serial.println("SD card initialization failed.");
    }
}

void displayText(const char *text, uint8_t textSize, uint16_t color, int16_t x, int16_t y, uint16_t bg)
{
    display.clearDisplay();
    display.setTextSize(textSize);
    display.setTextColor(color, bg);
    display.setCursor(x, y);
    display.println(text);
    display.display();
    delay(2000);
    Serial.print("Displayed text: ");
    Serial.println(text);
}

void displayNumber(uint8_t number)
{
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 28);
    display.print("0x");
    display.print(number, HEX);
    display.print("(HEX) = ");
    display.print(number, DEC);
    display.println("(DEC)");
    display.display();
    delay(2000);
    Serial.print("Displayed number: 0x");
    Serial.print(number, HEX);
    Serial.print(" (HEX) = ");
    Serial.print(number, DEC);
    Serial.println(" (DEC)");
}