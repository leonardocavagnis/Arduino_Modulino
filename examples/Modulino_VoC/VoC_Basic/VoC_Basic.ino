/*
 * Modulino VoC - Basic
 * 
 * This example demonstrates how to read air quality, temperature, humidity,
 * and barometric pressure measurements from the Modulino VoC sensor.
 * 
 * The sensor provides:
 * - Gas Resistance: Measured in Ohms (Ω)
 * Indicates Volatile Organic Compounds (VoC) presence.
 * Higher values mean cleaner air, lower values mean air contamination.
 * - Temperature: Measured in degrees Celsius (°C)
 * Range: -40°C to +85°C
 * - Humidity: Measured in relative humidity percentage (rH%)
 * Range: 0% to 100%
 * - Barometric Pressure: Measured in hectopascals (hPa)
 * Range: 300 to 1100 hPa
 * 
 * Applications:
 * - Indoor Air Quality (IAQ) monitoring
 * - HVAC and ventilation control
 * - Environmental monitoring
 * - Toxic gas leak detection (solvents, alcohol, smoke)
 *
 * This example code is in the public domain.
 * Copyright (c) 2026 Arduino
 * SPDX-License-Identifier: MPL-2.0
 */

#include <Arduino_Modulino.h>

// Create object instance
ModulinoVoC voc;

void setup() {
  Serial.begin(9600);

  // Initialize Modulino I2C communication
  Modulino.begin();
  
  // Detect and connect to the VoC sensor module
  voc.begin();
}

void loop() {
  if (voc.update()) {
    // Read gas resistance (VoC index proxy)
    float gas = voc.getGasResistance();

    // Read ambient temperature in Celsius
    float temperature = voc.getTemperature();

    // Read relative humidity percentage
    float humidity = voc.getHumidity();

    // Read atmospheric pressure in hectopascals
    float pressure = voc.getPressure();

    // Print Gas Resistance
    Serial.print("Gas Resistance (Ohms): ");
    Serial.println(gas, 2);

    // Print Temperature
    Serial.print("Temperature (°C) is: ");
    Serial.println(temperature, 2);

    // Print Relative Humidity
    Serial.print("Humidity (rH%) is: ");
    Serial.println(humidity, 2);

    // Print Barometric Pressure
    Serial.print("Pressure (hPa) is: ");
    Serial.println(pressure, 2);

    // Separator line for readability
    Serial.println("------------------------------------");
  }

  // Wait 2 seconds before the next reading
  delay(2000);
}