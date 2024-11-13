#ifndef MINING_H
#define MINING_H

#include "models/Worker.h"

const unsigned int COMMUNICATION_SILENCE_LIMIT = 20000;

void startStratum(Worker& worker, const char* pool_address, const uint16_t pool_port, float suggestDifficulty);

#endif
