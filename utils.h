#ifndef UTILS_H
#define UTILS_H

#include <ArduinoJson.h>

#include "models/Worker.h"

constexpr size_t BUFFER = 1024;
constexpr size_t BUFFER_JSON_DOC = 4096;
constexpr size_t TARGET_BUFFER_SIZE = 64;

static const double TRUE_DIFF_ONE = 26959535291011309493156476344723991336010898738574164086137773096960.0;

bool verifyPayload(String* line);
bool checkError(const StaticJsonDocument<BUFFER_JSON_DOC> doc);
uint8_t hex(char ch);
int toByteArray(const char *in, size_t inSize, uint8_t *out);
void getRandomExtranonce2ESP(int extranonce2Size, char *extranonce2);
void getRandomExtranonce2(int extranonce2Size, char *extranonce2);
void getNextExtranonce2(int extranonce2Size, char *extranonce2);
void doubleSha256(const byte *input, size_t inputLength, byte *output);
String applyVersionMask(const String& version, const String& versionMask);
void updateNonce(Miner& miner, uint32_t nonce);
double le256todouble(const void *target);
double diffFromTarget(void *target);
void buildBlockHeader(Worker& worker);

#endif