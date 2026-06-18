/*
 * Modulino Microphone - Basics
 *
 * This example demonstrates how to use the Modulino Microphone class
 * to fetch real-time digital audio chunks and stream them to the Serial Plotter.
 *
 * This example code is in the public domain.
 * Copyright (C) Arduino s.r.l. and/or its affiliated companies
 * SPDX-License-Identifier: MPL-2.0
 */

#include <Arduino_Modulino.h>

ModulinoMicrophone mic;

// Compulsory timing synchronization variable for the audio stream.
// Since the STM32 ADF (Acoustic Digital Filter) samples continuously in the background,
// Arduino must poll the I2C bus at a fixed interval to match the hardware block rate.
unsigned long last_check_micros = 0;
const unsigned long TIMING_SLOT_MICROS = 4096; // 4.096 ms block interval (64 samples @ 15.625 kHz)

void setup() {
  Serial.begin(115200);
  Modulino.begin();

  if (!mic.begin()) {
    Serial.println("Error: Modulino Microphone not detected.");
    while(1);
  }

  // The audio data retrieved from the Modulino Microphone is formatted as Pulse-Code Modulation (PCM):
  // - Linear PCM represents analog audio signals digitally by sampling the amplitude at regular intervals.
  // - Each sample is stored as a 16-bit signed integer (int16_t), ranging from -32768 to 32767.
  // - Data is transmitted in blocks of 64 samples (128 bytes of audio payload per I2C transaction).
  last_check_micros = micros();
}

void loop() {
  unsigned long current_micros = micros();

  // Enforce a rock-solid microsecond timing slot loop execution to avoid I2C buffer overruns or underruns
  if (current_micros - last_check_micros >= TIMING_SLOT_MICROS) {
    last_check_micros += TIMING_SLOT_MICROS; 

    if (mic.update()) {
      for (int i = 0; i < 64; i++) {
        // Access each individual 16-bit signed linear PCM sample
        Serial.println(mic.getPcmSample(i)); 
        // Audio data travels compressed over I2C using the IMA ADPCM codec to optimize bandwidth,
        // and it is automatically decoded into standard linear PCM by the library.
      }
    }
  }
}