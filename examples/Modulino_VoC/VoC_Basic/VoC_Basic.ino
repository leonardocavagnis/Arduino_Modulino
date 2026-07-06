/*
 * Modulino VoC - Basic
 *
 * This example demonstrates how to read air quality (IAQ, CO2, VOC),
 * temperature, humidity, and barometric pressure from the Modulino VoC
 * sensor.
 *
 * NOTE: IAQ, CO2 and VOC are estimated by an internal fusion algorithm, not
 * measured directly. Right after power-on they show neutral default values
 * (IAQ/VOC = 50, CO2 = 500 ppm) and their accuracy is 0. It takes some time
 * -- and exposure to both clean and "dirty" air -- for the algorithm to
 * calibrate and give trustworthy readings. Watch the accuracy value: once
 * it reaches "good", the numbers can be trusted.
 *
 * The sensor provides:
 * - IAQ: Index of Air Quality (0-500, lower is cleaner)
 * - CO2: Estimated CO2 equivalent, in ppm
 * - VOC: Estimated Volatile Organic Compounds, in ppb
 * - Gas Resistance: Measured in Ohms (Ω), raw signal used by the algorithm
 * - Temperature: Measured in degrees Celsius (°C)
 * - Humidity: Measured in relative humidity percentage (rH%)
 * - Barometric Pressure: Measured in hectopascals (hPa)
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

// Short phrase for an accuracy value (0-3)
const char* accuracyText(uint8_t accuracy) {
  switch (accuracy) {
    case 0:  return "calibrating";
    case 1:  return "low";
    case 2:  return "medium";
    case 3:  return "good";
    default: return "?";
  }
}

void setup() {
  Serial.begin(9600);

  // Initialize Modulino I2C communication
  Modulino.begin();

  // Detect and connect to the VoC sensor module
  voc.begin();
}

void loop() {
  // IMPORTANT: call update() as often as possible, with no delay() in
  // between. Internally it drives the BSEC algorithm's own measurement
  // timing (roughly every 3 seconds), so it must be polled frequently for
  // that timing to work; it simply returns false on calls where no new
  // sample is ready yet, so it's safe (and required) to call it in a tight
  // loop like this.
  if (voc.update()) {
    // Print Air Quality Index
    Serial.print("IAQ: ");
    Serial.print(voc.getIAQ(), 2);
    Serial.print(" (accuracy: ");
    Serial.print(accuracyText(voc.getIAQAccuracy()));
    Serial.println(")");

    // Print CO2 equivalent
    Serial.print("CO2 (ppm): ");
    Serial.print(voc.getCO2(), 2);
    Serial.print(" (accuracy: ");
    Serial.print(accuracyText(voc.getCO2Accuracy()));
    Serial.println(")");

    // Print VOC equivalent
    Serial.print("VOC (ppb): ");
    Serial.print(voc.getTVOC(), 2);
    Serial.print(" (accuracy: ");
    Serial.print(accuracyText(voc.getTVOCAccuracy()));
    Serial.println(")");

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
  // No delay() here: see the comment above loop().
}
