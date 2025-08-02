#include <Wire.h>
#include "MAX30105.h"
#include "spo2_algorithm.h"

MAX30105 particleSensor;

#define S_BUFFER_SIZE 100  // number of samples for processing

uint32_t irBuffer[S_BUFFER_SIZE];
uint32_t redBuffer[S_BUFFER_SIZE];

int32_t spo2;
int8_t validSPO2;
int32_t heartRate;
int8_t validHeartRate;

uint16_t sampleIndex = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("MAX30105 SPO2 and Heart Rate Example");

  Wire.begin();

  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println("MAX30105 was not found. Please check wiring.");
    while (1);
  }

  particleSensor.setup(60, 4, 2, 100, 411, 4096);
}

void loop() {
  long irValue = particleSensor.getIR();
  long redValue = particleSensor.getRed();

  if (irValue > 50000) {  // finger detected
    irBuffer[sampleIndex] = irValue;
    redBuffer[sampleIndex] = redValue;
    sampleIndex++;

    if (sampleIndex == BUFFER_SIZE) {
      maxim_heart_rate_and_oxygen_saturation(
        irBuffer,
        BUFFER_SIZE,
        redBuffer,
        &spo2,
        &validSPO2,
        &heartRate,
        &validHeartRate
      );

      if (validHeartRate) {
        Serial.print("Heart Rate: ");
        Serial.print(heartRate);
        Serial.println(" bpm");
      } else {
        Serial.println("Heart Rate: Invalid");
      }

      if (validSPO2) {
        Serial.print("SpO2: ");
        Serial.print(spo2);
        Serial.println(" %");
      } else {
        Serial.println("SpO2: Invalid");
      }

      Serial.println("------------------");

      sampleIndex = 0;
    }
  } else {
    Serial.println("No finger detected");
  }

  delay(20);  // sampling ~50Hz
}
