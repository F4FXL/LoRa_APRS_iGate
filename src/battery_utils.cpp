#include <Arduino.h>
#include "battery_utils.h"
#include "configuration.h"
#include "boards_pinout.h"
#include "power_utils.h"
#include "utils.h"

extern  Configuration               Config;
extern  uint32_t                    lastBatteryCheck;

bool    shouldSleepLowVoltage       = false;

float   adcReadingTransformation    = (3.3/4095);
float   voltageDividerCorrection    = 0.288;
float   readingCorrection           = 0.125;
float   multiplyCorrection          = 0.035;


namespace BATTERY_Utils {

    float mapVoltage(float voltage, float in_min, float in_max, float out_min, float out_max) {
        return (voltage - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
    }

    float checkInternalVoltage() { 
        #if defined(HAS_AXP192) || defined(HAS_AXP2101)
            if(POWER_Utils::isBatteryConnected()) {
                return POWER_Utils::getBatteryVoltage();
            } else {
                return 0.0;
            }
        #else
            int sample;
            int sampleSum = 0;
            #ifdef ADC_CTRL
                #if defined(HELTEC_WIRELESS_TRACKER)
                    digitalWrite(ADC_CTRL, HIGH);
                #endif
                #if defined(HELTEC_V3) || defined(HELTEC_V2) || defined(HELTEC_WSL_V3)
                    digitalWrite(ADC_CTRL, LOW);
                #endif
            #endif

            for (int i = 0; i < 100; i++) {
                #ifdef BATTERY_PIN
                    sample = analogRead(BATTERY_PIN);
                #endif
                #if defined(ESP32_DIY_LoRa) || defined(ESP32_DIY_1W_LoRa)
                    sample = 0;
                #endif
                sampleSum += sample;
                delayMicroseconds(50); 
            }

            #ifdef ADC_CTRL
                #if defined(HELTEC_WIRELESS_TRACKER)
                    digitalWrite(ADC_CTRL, LOW);
                #endif
                #if defined(HELTEC_V3) || defined(HELTEC_V2) || defined(HELTEC_WSL_V3)
                    digitalWrite(ADC_CTRL, HIGH);
                #endif
                double inputDivider = (1.0 / (390.0 + 100.0)) * 100.0;  // The voltage divider is a 390k + 100k resistor in series, 100k on the low side.
                return (((sampleSum/100) * adcReadingTransformation) / inputDivider) + 0.285; // Yes, this offset is excessive, but the ADC on the ESP32s3 is quite inaccurate and noisy. Adjust to own measurements.
            #else
                return (2 * (sampleSum/100) * adcReadingTransformation) + voltageDividerCorrection; // raw voltage without mapping
            #endif

            // return mapVoltage(voltage, 3.34, 4.71, 3.0, 4.2); // mapped voltage
        #endif
    }

    float checkExternalVoltage() {
        int sample;
        int sampleSum = 0;
        for (int i = 0; i < 100; i++) {
            sample = analogRead(Config.battery.externalVoltagePin);
            sampleSum += sample;
            delayMicroseconds(50); 
        }
        float voltageDividerTransformation = (Config.battery.voltageDividerR1 + Config.battery.voltageDividerR2) / Config.battery.voltageDividerR2;
        float voltage = ((((sampleSum/100)* adcReadingTransformation) + readingCorrection) * voltageDividerTransformation) - multiplyCorrection;
        
        return voltage; // raw voltage without mapping

        // return mapVoltage(voltage, 5.05, 6.32, 4.5, 5.5); // mapped voltage
    }

    void checkIfShouldSleep() {
        if (lastBatteryCheck == 0 || millis() - lastBatteryCheck >= 15 * 60 * 1000) {
            lastBatteryCheck = millis();            
            if (checkInternalVoltage() < Config.lowVoltageCutOff) {
                ESP.deepSleep(1800000000); // 30 min sleep (60s = 60e6)
            }
        }
    }

    void startupBatteryHealth() {
        #ifdef BATTERY_PIN
            if (Config.battery.monitorInternalVoltage && checkInternalVoltage() < Config.battery.internalSleepVoltage + 0.1) {
                shouldSleepLowVoltage = true;
            }
        #endif
        if (Config.battery.monitorExternalVoltage && checkExternalVoltage() < Config.battery.externalSleepVoltage + 0.1) {
            shouldSleepLowVoltage = true;
        }
        if (shouldSleepLowVoltage) {
            Utils::checkSleepByLowBatteryVoltage(0);
        }
    }

}