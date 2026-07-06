/*
 * Modulino VoC - Raw Basic
 *
 * This example demonstrates how to read raw temperature, humidity,
 * barometric pressure, and gas resistance from the Modulino VoC sensor,
 * with no air quality fusion (no IAQ, CO2 or VOC estimation). Readings are
 * available immediately, with no warm-up or calibration wait.
 *
 * Use this if you only need the raw physical measurements. See VoC_Basic
 * for the fused air quality metrics (IAQ, CO2, VOC) instead.
 *
 * The sensor provides:
 * - Gas Resistance: Measured in Ohms (Ω)
 * Indicates Volatile Organic Compounds (VoC) presence.
 * Higher values mean cleaner air, lower values mean air contamination.
 * - Temperature: Measured in degrees Celsius (°C)
 * - Humidity: Measured in relative humidity percentage (rH%)
 * - Barometric Pressure: Measured in hectopascals (hPa)
 *
 * This example code is in the public domain.
 * Copyright (c) 2026 Arduino
 * SPDX-License-Identifier: MPL-2.0
 */

#include <Arduino_Modulino.h>

// Create object instance
ModulinoVoC_Raw voc;

void setup() {
  Serial.begin(9600);

  // Initialize Modulino I2C communication
  Modulino.begin();

  // Detect and connect to the VoC sensor module
  voc.begin();
}

void loop() {
  if (voc.update()) {
    // Print Temperature
    Serial.print("Temperature (°C): ");
    Serial.println(voc.getTemperature(), 2);

    // Print Relative Humidity
    Serial.print("Humidity (rH%): ");
    Serial.println(voc.getHumidity(), 2);

    // Print Barometric Pressure
    Serial.print("Pressure (hPa): ");
    Serial.println(voc.getPressure(), 2);

    // Print Gas Resistance
    Serial.print("Gas Resistance (Ohms): ");
    Serial.println(voc.getGasResistance(), 2);

    // Separator line for readability
    Serial.println("------------------------------------");
  }

  // Wait 2 seconds before the next reading
  delay(2000);
}
