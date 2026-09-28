// Direkte Includes sind wichtig, damit PlatformIOs Library Dependency Finder
// auch die im ESP32-Arduino-Framework enthaltenen Bibliotheken erkennt.
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>

// Gemeinsame Anwendungsquelle fuer PlatformIO und die Arduino IDE.
#include "../vasner_esp32.ino"
