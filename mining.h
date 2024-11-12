#ifndef MINING_H
#define MINING_H

#include "models/Worker.h"

void startStratum(Worker& worker, const char* pool_address, const uint16_t pool_port, float suggestDifficulty);

#endif
