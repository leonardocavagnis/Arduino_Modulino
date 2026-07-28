/*
 * Modulino Microphone - Keyword Model Updater
 *
 * Teaches the Modulino Microphone a new keyword. Run this sketch once: the
 * model is stored inside the module and stays there, also after a power
 * cycle. Then use KWS_Basic (or your own sketch) to listen for the word.
 *
 * PICK THE KEYWORD: change the #include below, nothing else.
 *   "model_hey_arduino.h" -> "hey arduino"
 *   "model_stop.h"        -> "stop"
 *
 * To train your own word, see tools/README.md in the Modulino Microphone
 * firmware repository: it turns your recordings into a .h file to drop next
 * to this sketch.
 *
 * Safe to interrupt: the new model is only activated once it has been fully
 * received and verified, so unplugging the module halfway through simply
 * leaves the previous keyword working. KWS_RestoreFactoryModel always brings
 * back the keyword the module was shipped with.
 *
 * This example code is in the public domain.
 * Copyright (C) Arduino s.r.l. and/or its affiliated companies
 * SPDX-License-Identifier: MPL-2.0
 */

#include <Arduino_Modulino.h>

#include "model_hey_arduino.h"
//#include "model_stop.h"

ModulinoMicrophone mic;

// Called while the model is being sent, to show a progress percentage
void onProgress(uint32_t sent, uint32_t total) {
  static uint8_t last = 255;
  uint8_t percent = (sent * 100) / total;
  if (percent != last && percent % 10 == 0) {
    last = percent;
    Serial.print(percent);
    Serial.println("%");
  }
}

void setup() {
  Serial.begin(115200);
  while (!Serial) { }

  Modulino.begin();

  if (!mic.begin()) {
    Serial.println("Error: Modulino Microphone not detected.");
    while (1);
  }

  Serial.print("Teaching the keyword \"");
  Serial.print(ModulinoMicrophone::modelKeyword(KWS_MODEL));
  Serial.println("\" to the module...");

  if (mic.updateModel(KWS_MODEL, KWS_MODEL_SIZE, onProgress)) {
    Serial.println("Done! Run KWS_Basic to try the new keyword.");
  } else {
    Serial.print("Update failed: ");
    Serial.println(mic.lastError());
  }
}

void loop() {
}
