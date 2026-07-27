/*
 * Modulino Microphone - Keyword Spotting
 *
 * The Modulino Microphone can recognise a spoken keyword all by itself:
 * the audio never leaves the module, which simply tells you when it hears
 * the word it knows.
 *
 * Say the keyword and watch the Serial Monitor. Out of the box the module
 * listens for "go"; to teach it a different word, run the KWS_UpdateModel
 * utility sketch once.
 *
 * This example code is in the public domain.
 * Copyright (C) Arduino s.r.l. and/or its affiliated companies
 * SPDX-License-Identifier: MPL-2.0
 */

#include <Arduino_Modulino.h>

ModulinoMicrophone mic;

void setup() {
  Serial.begin(115200);
  Modulino.begin();

  if (!mic.begin()) {
    Serial.println("Error: Modulino Microphone not detected.");
    while (1);
  }

  // From now on the module listens on its own instead of streaming audio
  if (!mic.beginKeywordSpotting()) {
    Serial.println(mic.lastError());
    while (1);
  }

  Serial.println("Listening... say the keyword!");
}

void loop() {
  // Each spoken keyword is reported exactly once
  if (mic.keywordDetected()) {
    Serial.print("Keyword detected! (confidence ");
    Serial.print(mic.keywordConfidence());
    Serial.print("%, detection #");
    Serial.print(mic.keywordCount());
    Serial.println(")");
  }

  delay(100);
}
