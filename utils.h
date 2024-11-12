#ifndef UTILS_H
#define UTILS_H

#include <ArduinoJson.h>

constexpr size_t BUFFER = 1024;
constexpr size_t BUFFER_JSON_DOC = 4096;

bool verifyPayload(String* line);
bool checkError(const StaticJsonDocument<BUFFER_JSON_DOC> doc);

#endif