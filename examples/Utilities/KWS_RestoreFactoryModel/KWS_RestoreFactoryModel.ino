/*
 * Modulino Microphone - Restore the Factory Keyword
 *
 * Brings the Modulino Microphone back to the keyword it was shipped with,
 * undoing a KWS_UpdateModel upload. The factory model is always kept inside
 * the module and is never overwritten, so this is instant and always works.
 *
 * This example code is in the public domain.
 * Copyright (C) Arduino s.r.l. and/or its affiliated companies
 * SPDX-License-Identifier: MPL-2.0
 */

#include <Arduino_Modulino.h>

ModulinoMicrophone mic;

void setup() {
  Serial.begin(115200);
  while (!Serial) { }

  Modulino.begin();

  if (!mic.begin()) {
    Serial.println("Error: Modulino Microphone not detected.");
    while (1);
  }

  if (mic.restoreFactoryModel()) {
    Serial.println("Factory keyword restored.");
  } else {
    Serial.print("Restore failed: ");
    Serial.println(mic.lastError());
  }
}

void loop() {
}
