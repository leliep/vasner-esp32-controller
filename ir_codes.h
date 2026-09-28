#pragma once
#include <Arduino.h>

// Mit Flipper Zero aus Vasner.ir ermittelt.
// Protokoll: NEC, 8-Bit-Adresse: 0x18
constexpr uint16_t IR_ADDRESS = 0x18;

constexpr uint8_t IR_CMD_OFF   = 0x00;
constexpr uint8_t IR_CMD_ON    = 0x01;
constexpr uint8_t IR_CMD_UP    = 0x02;
constexpr uint8_t IR_CMD_DOWN  = 0x03;
constexpr uint8_t IR_CMD_LEFT  = 0x06;
constexpr uint8_t IR_CMD_MAX   = 0x07;
constexpr uint8_t IR_CMD_RIGHT = 0x08;
