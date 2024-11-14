#ifndef UTILS_H
#define UTILS_H

#include <ArduinoJson.h>

#include "models/Worker.h"

constexpr size_t BUFFER = 1024;
constexpr size_t BUFFER_JSON_DOC = 4096;
constexpr size_t TARGET_BUFFER_SIZE = 64;

bool verifyPayload(String* line);
bool checkError(const StaticJsonDocument<BUFFER_JSON_DOC> doc);
uint8_t hex(char ch);
int toByteArray(const char *in, size_t inSize, uint8_t *out);
void getRandomExtranonce2(int extranonce2Size, char *extranonce2);
void getNextExtranonce2(int extranonce2Size, char *extranonce2);
void doubleSha256(const byte *input, size_t inputLength, byte *output);
void buildBlockHeader(Worker& worker);

#endif