#include "DHT.h"
#include <SD.h>
#include <Wire.h>
#include <RTClib.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <EEPROM.h>

// Constants
#define DHT11_PIN 48
#define PIN_SPI_CS 53
#define CalibrationButton 46
#define MeasurementButton 47

#define MQ_135_PIN PIN_A0
#define MQ_136_PIN PIN_A2
#define TGS_2602_PIN PIN_A1
#define NDIR_PIN PIN_A3

#define RL_MQ_135 1000
#define RL_MQ_136 1000
#define RL_TGS_2602 4700

#define CO2_MQ135 1
#define H2S_MQ136 2
#define H2S_TGS2602 3

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define SCREEN_ADDRESS 0x3C

#define CALIBARAION_SAMPLE_TIMES 50
#define CALIBRATION_SAMPLE_INTERVAL 500
#define READ_SAMPLE_INTERVAL 50
#define READ_SAMPLE_TIMES 5

// Sensor Calibration Factors
const float RO_MQ_135_CLEAN_AIR_FACTOR = 3.55;
const float RO_MQ_136_CLEAN_AIR_FACTOR = 3.55;
const float RO_TGS_2602_CLEAN_AIR_FACTOR = 1.0;

// Sensor Curves
const float CO2CurveMQ135[2] = {-2.85, 2.0451};
const float H2SCurveMQ136[2] = {-3.5, 1.547};
const float H2SCurveTGS2602[2] = {-2.7245, -1.1293};

const float MQ135TempHumCurve33[3] = {0.0004, -0.0261, 1.3869};
const float MQ135TempHumCurve85[3] = {0.0003, -0.023, 1.2528};
const float MQ136TempHumCurve33[3] = {0.0004, -0.0261, 1.3869};
const float MQ136TempHumCurve85[3] = {0.0003, -0.023, 1.2528};
const float TGS2602TempHumCurve40[3] = {0.0002, -0.0349, 1.5619};
const float TGS2602TempHumCurve65[3] = {0.0002, -0.0373, 1.6632};
const float TGS2602TempHumCurve85[3] = {0.0003, -0.0443, 1.8361};

// Global Variables
DHT dht11(DHT11_PIN, DHT11);
RTC_DS3231 rtc;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
File myFile;
char filename[13];

float Ro_MQ_135 = 0;
float Ro_MQ_136 = 0;
float Ro_TGS_2602 = 0;

bool isCalibrating = false;
bool isMeasuring = false;

void setup()
{
    Serial.begin(9600);
    dht11.begin();
    initializeRTC();
    initializeDisplay();
    initializePins();
    loadCalibrationValues();
    delay(5000);
}

void loop()
{
    if (digitalRead(CalibrationButton) == LOW && !isCalibrating)
    {
        isCalibrating = true;
        calibrateSensors();
        isCalibrating = false;
    }

    if (digitalRead(MeasurementButton) == LOW && !isMeasuring)
    {
        isMeasuring = true;
        measureAndLog();
        isMeasuring = false;
    }

    delay(100); // Debounce delay
}

void initializeRTC()
{
    if (!rtc.begin())
    {
        Serial.println("Couldn't find RTC");
    }

    if (rtc.lostPower())
    {
        Serial.println("RTC lost power, let's set the time!");
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }
}

void initializeDisplay()
{
    if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS))
    {
        Serial.println(F("SSD1306 allocation failed"));
    }
    display.clearDisplay();
    Serial.println("Display initialized");
}

void initializePins()
{
    analogReference(DEFAULT);
    pinMode(MQ_135_PIN, INPUT);
    pinMode(MQ_136_PIN, INPUT);
    pinMode(CalibrationButton, INPUT_PULLUP);
    pinMode(MeasurementButton, INPUT_PULLUP);
}

void loadCalibrationValues()
{
    EEPROM.get(0, Ro_MQ_135);
    EEPROM.get(sizeof(float), Ro_MQ_136);
    EEPROM.get(2 * sizeof(float), Ro_TGS_2602);

    if (Ro_MQ_135 == 0 || Ro_MQ_136 == 0 || Ro_TGS_2602 == 0)
    {
        calibrateSensors();
    }

    printCalibrationValues();
}

void printCalibrationValues()
{
    Serial.print("Ro MQ135=");
    Serial.print(Ro_MQ_135 / RL_MQ_135);
    Serial.println("kohm");

    Serial.print("Ro MQ136=");
    Serial.print(Ro_MQ_136 / RL_MQ_136);
    Serial.println("kohm");

    Serial.print("Ro TGS2602=");
    Serial.print(Ro_TGS_2602 / RL_TGS_2602);
    Serial.println("kohm");
}

void calibrateSensors()
{
    Ro_MQ_135 = calibrateSensor(MQ_135_PIN, RO_MQ_135_CLEAN_AIR_FACTOR, RL_MQ_135, "MQ135");
    EEPROM.put(0, Ro_MQ_135);

    Ro_MQ_136 = calibrateSensor(MQ_136_PIN, RO_MQ_136_CLEAN_AIR_FACTOR, RL_MQ_136, "MQ136");
    EEPROM.put(sizeof(float), Ro_MQ_136);

    Ro_TGS_2602 = calibrateSensor(TGS_2602_PIN, RO_TGS_2602_CLEAN_AIR_FACTOR, RL_TGS_2602, "TGS2602");
    EEPROM.put(2 * sizeof(float), Ro_TGS_2602);

    Serial.println("Calibration is done...\n");
}

float calibrateSensor(int pin, float cleanAirFactor, int rlValue, const char *sensorName)
{
    float val = MQCalibration(pin, cleanAirFactor, rlValue);
    displayCalibrationResult(sensorName, val / rlValue);
    return val;
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
    if (Ro_MQ_135 == 0 || Ro_MQ_136 == 0 || Ro_TGS_2602 == 0)
    {
        Serial.println("Calibration values not found. Please calibrate first.");
        return;
    }

    float humi = dht11.readHumidity();
    float tempC = dht11.readTemperature();
    long ppmCo2Mq135 = MQ135GetPPM(tempC, humi);
    long ppmH2sMq136 = MQ136GetPPM(tempC, humi);
    long ppmH2sTgs2602 = TGS2602GetPPM(tempC, humi);
    long ppmco2ndir = readNDIRCO2(NDIR_PIN);

    logAndDisplayResults(tempC, humi, ppmCo2Mq135, ppmco2ndir, ppmH2sMq136, ppmH2sTgs2602);
}

void logAndDisplayResults(float tempC, float humi, long ppmCo2Mq135, long ppmco2ndir, long ppmH2sMq136, long ppmH2sTgs2602)
{
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

    if (SD.begin(PIN_SPI_CS))
    {
        DateTime now = rtc.now();
        snprintf(filename, sizeof(filename), "%02d%02d%02d%02d.txt", now.month(), now.day(), now.year() % 100, now.hour());
        myFile = SD.open(filename, FILE_WRITE);
        if (myFile)
        {
            myFile.print("Start Measure Time: ");
            myFile.print(now.timestamp());
            myFile.println();
            myFile.print("Temperature: ");
            myFile.print(tempC);
            myFile.print(" C, Humidity: ");
            myFile.print(humi);
            myFile.print(" %, CO2 (MQ135): ");
            myFile.print(ppmCo2Mq135);
            myFile.print(" ppm, CO2 (NDIR): ");
            myFile.print(ppmco2ndir);
            myFile.print(" ppm, H2S (MQ136): ");
            myFile.print(ppmH2sMq136);
            myFile.print(" ppm, H2S (TGS2602): ");
            myFile.print(ppmH2sTgs2602);
            myFile.println(" ppm");
            myFile.close();
        }
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
    display.print(" ppm");
    display.display();
    delay(3000); // Display result for 3 seconds
}

float MQ135GetPPM(float x, float H)
{
    return getPPM(MQ_135_PIN, RL_MQ_135, Ro_MQ_135, x, H, MQ135TempHumCurve33, MQ135TempHumCurve85, CO2_MQ135, NULL);
}

float MQ136GetPPM(float x, float H)
{
    return getPPM(MQ_136_PIN, RL_MQ_136, Ro_MQ_136, x, H, MQ136TempHumCurve33, MQ136TempHumCurve85, H2S_MQ136, NULL);
}

float TGS2602GetPPM(float x, float H)
{
    return getPPM(TGS_2602_PIN, RL_TGS_2602, Ro_TGS_2602, x, H, TGS2602TempHumCurve40, TGS2602TempHumCurve85, H2S_TGS2602, TGS2602TempHumCurve85);
}

float getPPM(int pin, int rlValue, float ro, float x, float H, const float *curve33, const float *curve85, int gasId, const float *curve65)
{
    float rs = MQRead(pin, rlValue);
    float rs_ro = rs / ro;
    float rs_ro_corr = RsRoCorrection(x, H, curve33, curve85);

    if (curve65 != NULL)
    {
        rs_ro_corr = RsRoCorrection3Curve(x, H, curve33, curve65, curve85);
    }

    Serial.print("Sensor rs_ro before correction: ");
    Serial.println(rs_ro);
    logToSD("Sensor rs_ro before correction: ", rs_ro);

    Serial.print("Sensor rs_ro_corr: ");
    Serial.println(rs_ro_corr);
    logToSD("Sensor rs_ro_corr: ", rs_ro_corr);

    rs_ro = rs_ro / rs_ro_corr;

    Serial.print("Sensor rs_ro after correction: ");
    Serial.println(rs_ro);
    logToSD("Sensor rs_ro after correction: ", rs_ro);

    return MQGetGasPercentage(rs_ro, gasId);
}

float MQResistanceCalculation(int raw_adc, float rl_value)
{
    return ((rl_value * (1023 - raw_adc) / raw_adc));
}

float MQCalibration(int mq_pin, float ro_clean_air_factor, float rl_value)
{
    float val = 0;
    for (int i = 0; i < CALIBARAION_SAMPLE_TIMES; i++)
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
    float rs = 0;
    for (int i = 0; i < READ_SAMPLE_TIMES; i++)
    {
        rs += MQResistanceCalculation(analogRead(mq_pin), rl_value);
        delay(READ_SAMPLE_INTERVAL);
    }
    return rs / READ_SAMPLE_TIMES;
}

float MQGetPercentage(float rs_ro_ratio, const float *pcurve)
{
    return pow(10, (pcurve[0] * log10(rs_ro_ratio) + pcurve[1]));
}

float MQGetGasPercentage(float rs_ro_ratio, int gas_id)
{
    switch (gas_id)
    {
    case CO2_MQ135:
        return MQGetPercentage(rs_ro_ratio, CO2CurveMQ135);
    case H2S_MQ136:
        return MQGetPercentage(rs_ro_ratio, H2SCurveMQ136);
    case H2S_TGS2602:
        return MQGetPercentage(rs_ro_ratio, H2SCurveTGS2602);
    default:
        return 0;
    }
}

float RsRoCorrection(float x, float H, const float *curve33, const float *curve85)
{
    float a1 = curve33[0], b1 = curve33[1], c1 = curve33[2];
    float a2 = curve85[0], b2 = curve85[1], c2 = curve85[2];

    float a = a1 + (a2 - a1) * (H - 33) / (85 - 33);
    float b = b1 + (b2 - b1) * (H - 33) / (85 - 33);
    float c = c1 + (c2 - c1) * (H - 33) / (85 - 33);

    return a * x * x + b * x + c;
}

float RsRoCorrection3Curve(float x, float H, const float *curve40, const float *curve65, const float *curve85)
{
    float a1 = curve40[0], b1 = curve40[1], c1 = curve40[2];
    float a2 = curve65[0], b2 = curve65[1], c2 = curve65[2];
    float a3 = curve85[0], b3 = curve85[1], c3 = curve85[2];

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

    return a * x * x + b * x + c;
}

long readNDIRCO2(int sensorIn)
{
    long ppmco2 = 0;
    int sensorValue = analogRead(sensorIn);
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
        }
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
        }
    }
}