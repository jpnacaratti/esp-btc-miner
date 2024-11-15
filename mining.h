#ifndef MINING_H
#define MINING_H

#include "models/Worker.h"

constexpr uint32_t MAX_NONCE = 25000000U;
constexpr uint32_t TARGET_NONCE = 471136297U;

const unsigned int COMMUNICATION_SILENCE_LIMIT = 15000;

void startStratum(Worker& worker, const char* pool_address, const uint16_t pool_port, float suggestDifficulty);
void startMiner(Worker& worker, uint8_t miner_id);

#endif
