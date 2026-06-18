/*
 * Modulino Microphone - Audio Capture and Data Dump
 *
 * This example demonstrates how to use the Modulino Microphone class
 * to capture approx 0.5 seconds of raw PCM audio data into a local buffer 
 * without blocking execution, and subsequently dump the samples to the Serial Monitor.
 *
 * INSTRUCTIONS:
 * 1. Open the Serial Monitor and press 's' to start recording.
 * 2. Copy the printed sample values from the "=== DATA DUMP START ===" section.
 * 3. Save these numbers into a text file named "audio_data.txt".
 * 4. Run the Python script passing this file as an argument:
 * python listen_audio.py audio_data.txt
 * 5. A 'recorded_audio.wav' file will be generated in the same directory. Open and listen to it! :)
 *
 * This example code is in the public domain.
 * Copyright (C) Arduino s.r.l. and/or its affiliated companies
 * SPDX-License-Identifier: MPL-2.0
 */

#include <Arduino_Modulino.h>

ModulinoMicrophone mic;

// 61 * 2 blocks = 122 blocks. At 4ms per block, this records for approx 488ms (~0.5 seconds)
const int TOTAL_BLOCKS = 61*2;  
const int TOTAL_SAMPLES = TOTAL_BLOCKS * 64;

int16_t audio_storage[TOTAL_SAMPLES];

int blockCount = 0;
bool startRecording = false;

unsigned long last_check_micros = 0;
const unsigned long TIMING_SLOT_MICROS = 4096; // ~4ms slot to poll the microphone hardware buffer

void setup() {
  Serial.begin(115200);
  while(!Serial) {}
  Modulino.begin();

  if (mic.begin()) {
    Serial.println("Microphone ready!");
    Serial.println("Send 's' to record approx 0.5 seconds...");
  } else {
    Serial.println("Error: Microphone not detected.");
    while(1);
  }
}

void loop() {
  if (!startRecording && Serial.available() > 0) {
    char c = Serial.read();
    if (c == 's') {
      Serial.println("Recording... START!");
      startRecording = true;
      blockCount = 0;
      last_check_micros = micros();
    }
  }

  if (startRecording) {
    unsigned long current_micros = micros();

    // Check if the timing slot has elapsed to poll the mic
    if (current_micros - last_check_micros >= TIMING_SLOT_MICROS) {
      last_check_micros += TIMING_SLOT_MICROS; 

      if (mic.update()) {
        int destination_index = blockCount * 64;
        // Copy the 64-sample audio block into the storage buffer
        for (int i = 0; i < 64; i++) {
          audio_storage[destination_index + i] = mic.getPcmSample(i);
        }
        
        blockCount++;
        
        // Check if the target duration has been reached
        if (blockCount >= TOTAL_BLOCKS) {
          startRecording = false;
          Serial.println("=== DATA DUMP START ===");
          
          for (int i = 0; i < TOTAL_SAMPLES; i++) {
            Serial.println(audio_storage[i]);
          }
          
          Serial.println("=== DATA DUMP END ===");
        }
      }
    }
  }
}