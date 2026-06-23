/*
 * Modulino Microphone - Firmware Updater
 * 
 * This utility updates the firmware on Modulino Microphone modules.
 * 
 * IMPORTANT: This is an advanced tool for updating module firmware.
 * Only use this if instructed by Arduino support or if you need to
 * restore a module to working condition.
 * 
 * Instructions:
 * 1. Connect ONLY ONE Modulino Microphone module at a time
 * 2. Upload this sketch to your Arduino
 * 3. The sketch will automatically detect and flash the appropriate firmware
 * 4. On UNO R4 WiFi, the LED matrix will show "PASS" or "FAIL" when done
 * 5. Wait for the update to complete before disconnecting
 * 
 * NOTE: This uses the STM32 bootloader protocol to flash firmware.
 * Do not disconnect power during the update process.
 * 
 * Reference: STM32 I2C bootloader protocol
 * https://www.st.com/resource/en/application_note/an4221-i2c-protocol-used-in-the-stm32-bootloader-stmicroelectronics.pdf
 *
 * This example code is in the public domain. 
 * Copyright (c) 2026 Arduino
 * SPDX-License-Identifier: MPL-2.0
 */

#if defined(ARDUINO_UNOWIFIR4)
#include "ArduinoGraphics.h"
#include "Arduino_LED_Matrix.h"
#endif

#include <Arduino_Modulino.h>
#include "Wire.h"
#include "fw_microphone.h"

// Reference: STM32 I2C bootloader protocol documentation
// https://www.st.com/resource/en/application_note/an4221-i2c-protocol-used-in-the-stm32-bootloader-stmicroelectronics.pdf
#define BL_U3_I2C_ADDRESS 0x6C  // STM32U3 bootloader address

bool flashU3(uint8_t bl_i2c_addr, const uint8_t* binary, size_t lenght, bool verbose = true);
Module modulino;

// Change this to true if programming a blank Modulino Microphone
// For all other modules, keep this false
bool force_microphone = false;

bool is_boot_mode = false;

void setup() {
  Serial.begin(115200);
  while(!Serial) { }

  // Initialize Modulino communication
  Modulino.begin();
  // Set I2C clock to 400kHz for faster communication
  modulino.getWire()->setClock(400000);

  // Check if the Modulino Microphone is in bootloader mode (address 0x6C - STM32U3)
  modulino.getWire()->beginTransmission(BL_U3_I2C_ADDRESS); 
  is_boot_mode |= (modulino.getWire()->endTransmission() == 0);

  if (is_boot_mode) {
    Serial.println("boot mode");
  }

  bool is_microphone = false;

  // Send reset command to module if not already in boot mode
  // IMPORTANT: Connect only ONE module at a time
  if (!is_boot_mode) {
    // Check if connected module is a microphone (address 0x2A)
    modulino.getWire()->beginTransmission(0x2A);
    is_microphone = (modulino.getWire()->endTransmission() == 0);

    if (is_microphone) {
      Serial.println("microphone mode");
    }

    // Send reset command to enter bootloader mode
    if (sendReset() != 0) {
      Serial.println("Send reset failed");
      while(1);
    }
  }

  // Restart the I2C bus after reset to clear any pending states and ensure a clean connection to the bootloader
  modulino.getWire()->end();
  delay(50);
  modulino.getWire()->begin();
  modulino.getWire()->setClock(400000);

  // Flash the appropriate firmware based on module type
  bool result;
  if (is_microphone || force_microphone) {
    // Flash Microphone firmware
    Serial.println("Start flashing Modulino Microphone...");
    result = flashU3(BL_U3_I2C_ADDRESS, microphone_node_base_bin, microphone_node_base_bin_len);
  } else {
    Serial.println("Unknown modulino type. Aborting firmware update.");
    result = false;
  }

#if defined(ARDUINO_UNOWIFIR4)
  // Display result on UNO R4 WiFi LED matrix
  if (result) {
    matrixInitAndDraw("PASS");
  } else {
    matrixInitAndDraw("FAIL");
  }
#endif
}

void loop() {
  // put your main code here, to run repeatedly:
}

class SerialVerbose {
public:
  SerialVerbose(bool verbose)
    : _verbose(verbose) {}
  int print(String s) {
    if (_verbose) {
      Serial.print(s);
    }
  }
  int println(String s) {
    if (_verbose) {
      Serial.println(s);
    }
  }
  int println(int num, int base) {
    if (_verbose) {
      Serial.println(num, base);
    }
  }
private:
  bool _verbose;
};

#if defined(ARDUINO_UNOWIFIR4)
ArduinoLEDMatrix matrix;

void matrixInitAndDraw(char* text) {
  matrix.begin();
  matrix.beginDraw();

  matrix.stroke(0xFFFFFFFF);
  matrix.textFont(Font_4x6);
  matrix.beginText(0, 1, 0xFFFFFF);
  matrix.println(text);
  matrix.endText();

  matrix.endDraw();
}
#endif

bool flashU3(uint8_t bl_i2c_addr, const uint8_t* binary, size_t lenght, bool verbose) {

  SerialVerbose SerialDebug(verbose);

  uint8_t resp_buf[255];
  int resp;
  SerialDebug.println("GET_COMMAND");
  resp = command(bl_i2c_addr, 0, nullptr, 0, resp_buf, 21, verbose); // NOTE: GET_COMMAND returns 21 bytes for STM32U3

  if (resp < 0) {
    SerialDebug.println("Failed :(");
    return false;
  }

  for (int i = 0; i < resp; i++) {
    SerialDebug.println(resp_buf[i], HEX);
  }

  SerialDebug.println("GET_ID");
  resp = command(bl_i2c_addr, 2, nullptr, 0, resp_buf, 3, verbose);
  for (int i = 0; i < resp; i++) {
    SerialDebug.println(resp_buf[i], HEX);
  }

  SerialDebug.println("GET_ID");
  resp = command(bl_i2c_addr, 2, nullptr, 0, resp_buf, 3, verbose);
  for (int i = 0; i < resp; i++) {
    SerialDebug.println(resp_buf[i], HEX);
  }

  delay(50);

  SerialDebug.println("MASS_ERASE");
  uint8_t erase_buf[3] = { 0xFF, 0xFF, 0x0 };
  resp = command(bl_i2c_addr, 0x44, erase_buf, 3, nullptr, 0, verbose);
  for (int i = 0; i < resp; i++) {
    SerialDebug.println(resp_buf[i], HEX);
  }

  delay(50); // Short delay to allow the STM32U3 internal Flash controller to complete the mass erase operation

  for (size_t i = 0; i < lenght; i += 128) {
    SerialDebug.print("WRITE_PAGE ");
    SerialDebug.println(i, HEX);

    // Calculate the absolute 32-bit destination address in STM32U3 Flash
    uint32_t target_address = 0x08000000 + i;
    uint8_t write_buf[5];
    // Prepare the 4-byte address buffer in Big-Endian format (MSB first)
    write_buf[0] = (target_address >> 24) & 0xFF; // Base Flash indicator (0x08)
    write_buf[1] = (target_address >> 16) & 0xFF; // Address high byte (0x00)
    write_buf[2] = (target_address >> 8)  & 0xFF; // Address mid byte (MSB of offset)
    write_buf[3] = target_address & 0xFF;         // Address low byte (LSB of offset)
    write_buf[4] = 0; // Reserved for the XOR checksum, calculated inside command_write_page

    // Prevent buffer overflow: handle the last chunk if it is smaller than 128 bytes
    size_t chunk_size = 128;
    if (i + chunk_size > lenght) {
      chunk_size = lenght - i;
    }

    // Send the 128-byte aligned chunk to the bootloader via I2C WRITE command (0x32)
    resp = command_write_page(bl_i2c_addr, 0x32, write_buf, 5, &binary[i], chunk_size, verbose);
    if (resp < 0) {
      SerialDebug.print("Failed to write at address 0x");
      SerialDebug.println(target_address, HEX);
      return false;
    }

    // Short delay to allow the STM32U3 internal Flash controller to complete the write cycle
    delay(10);
  }
  SerialDebug.println("GO");
  // Extract the real execution entry point (Reset Handler) from bytes 4-7 of the binary (Little-Endian)
  uint32_t reset_handler = ((uint32_t)binary[7] << 24) |
                           ((uint32_t)binary[6] << 16) |
                           ((uint32_t)binary[5] << 8)  |
                           ((uint32_t)binary[4]);

  uint8_t jump_buf[5];
  jump_buf[0] = (reset_handler >> 24) & 0xFF;
  jump_buf[1] = (reset_handler >> 16) & 0xFF;
  jump_buf[2] = (reset_handler >> 8)  & 0xFF;
  jump_buf[3] = reset_handler & 0xFF;
  jump_buf[4] = jump_buf[0] ^ jump_buf[1] ^ jump_buf[2] ^ jump_buf[3]; // Calculate XOR Checksum

  resp = command(bl_i2c_addr, 0x21, jump_buf, 5, nullptr, 0, verbose);
  return true;
}

int howmany;
int command_write_page(uint8_t bl_i2c_addr, uint8_t opcode, uint8_t* buf_cmd, size_t len_cmd, const uint8_t* buf_fw, size_t len_fw, bool verbose) {

  SerialVerbose SerialDebug(verbose);

  uint8_t cmd[2];
  cmd[0] = opcode;
  cmd[1] = 0xFF ^ opcode;
  modulino.getWire()->beginTransmission(bl_i2c_addr);
  modulino.getWire()->write(cmd, 2);
  if (len_cmd > 0) {
    buf_cmd[len_cmd - 1] = 0;
    for (int i = 0; i < len_cmd - 1; i++) {
      buf_cmd[len_cmd - 1] ^= buf_cmd[i];
    }
    modulino.getWire()->endTransmission(true);
    modulino.getWire()->requestFrom(bl_i2c_addr, 1);
    auto c = modulino.getWire()->read();
    if (c != 0x79) {
      SerialDebug.print("error first ack: ");
      SerialDebug.println(c, HEX);
      return -1;
    }
    modulino.getWire()->beginTransmission(bl_i2c_addr);
    modulino.getWire()->write(buf_cmd, len_cmd);
  }
  modulino.getWire()->endTransmission(true);
  modulino.getWire()->requestFrom(bl_i2c_addr, 1);
  auto c = modulino.getWire()->read();
  if (c != 0x79) {
    while (c == 0x76) {
      delay(10);
      modulino.getWire()->requestFrom(bl_i2c_addr, 1);
      c = modulino.getWire()->read();
    }
    if (c != 0x79) {
      SerialDebug.print("error second ack: ");
      SerialDebug.println(c, HEX);
      return -1;
    }
  }
  uint8_t tmpbuf[len_fw + 2] = { len_fw - 1 };
  memcpy(&tmpbuf[1], buf_fw, len_fw);
  for (int i = 0; i < len_fw + 1; i++) {
    tmpbuf[len_fw + 1] ^= tmpbuf[i];
  }
  modulino.getWire()->beginTransmission(bl_i2c_addr);
  modulino.getWire()->write(tmpbuf, len_fw + 2);
  modulino.getWire()->endTransmission(true);
  modulino.getWire()->requestFrom(bl_i2c_addr, 1);
  c = modulino.getWire()->read();
  if (c != 0x79) {
    while (c == 0x76) {
      delay(10);
      modulino.getWire()->requestFrom(bl_i2c_addr, 1);
      c = modulino.getWire()->read();
    }
    if (c != 0x79) {
      SerialDebug.print("error: ");
      SerialDebug.println(c, HEX);
      return -1;
    }
  }
final_ack:
  return howmany + 1;
}

int command(uint8_t bl_i2c_addr, uint8_t opcode, uint8_t* buf_cmd, size_t len_cmd, uint8_t* buf_resp, size_t len_resp, bool verbose) {

  SerialVerbose SerialDebug(verbose);

  uint8_t cmd[2];
  cmd[0] = opcode;
  cmd[1] = 0xFF ^ opcode;
  modulino.getWire()->beginTransmission(bl_i2c_addr);
  modulino.getWire()->write(cmd, 2);
  if (len_cmd > 0) {
    modulino.getWire()->endTransmission(true);
    modulino.getWire()->requestFrom(bl_i2c_addr, 1);
    auto c = modulino.getWire()->read();
    if (c != 0x79) {
      Serial.print("error first ack: ");
      Serial.println(c, HEX);
      return -1;
    }
    modulino.getWire()->beginTransmission(bl_i2c_addr);
    modulino.getWire()->write(buf_cmd, len_cmd);
  }
  modulino.getWire()->endTransmission(true);
  modulino.getWire()->requestFrom(bl_i2c_addr, 1);
  auto c = modulino.getWire()->read();
  if (c != 0x79) {
    while (c == 0x76) {
      delay(100);
      modulino.getWire()->requestFrom(bl_i2c_addr, 1);
      c = modulino.getWire()->read();
      SerialDebug.println("retry");
    }
    if (c != 0x79) {
      SerialDebug.print("error second ack: ");
      SerialDebug.println(c, HEX);
      return -1;
    }
  }
  int howmany = -1;
  if (len_resp == 0) {
    goto final_ack;
  }
  modulino.getWire()->requestFrom(bl_i2c_addr, len_resp);
  howmany = modulino.getWire()->read();
  for (int j = 0; j < howmany + 1; j++) {
    buf_resp[j] = modulino.getWire()->read();
  }

  modulino.getWire()->requestFrom(bl_i2c_addr, 1);
  c = modulino.getWire()->read();
  if (c != 0x79) {
    SerialDebug.print("error: ");
    SerialDebug.println(c, HEX);
    return -1;
  }
final_ack:
  return howmany + 1;
}

int sendReset() {
  uint8_t buf[3] = { 'D', 'I', 'E' };
  int ret;
  for (int i = 0; i < 0x78; i++) {
    modulino.getWire()->beginTransmission(i);
    ret = modulino.getWire()->endTransmission();
    if (ret == 0) { // Check if a device successfully acknowledged (ACK) the address scan
      modulino.getWire()->beginTransmission(i);
      modulino.getWire()->write(buf, 40);
      ret = modulino.getWire()->endTransmission();
      return ret;
    }
  }
  return ret;
}