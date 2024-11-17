#ifndef MINING_H
#define MINING_H

#include "models/Worker.h"
#include "models/Monitor.h"

constexpr uint32_t MAX_NONCE = 25000000U;
constexpr uint32_t TARGET_NONCE = 471136297U;

const unsigned int COMMUNICATION_SILENCE_LIMIT = 15000;

void startStratum(Worker& worker, Monitor& monitor, const char* pool_address, const uint16_t pool_port, float suggestDifficulty);
void startMiner(Worker& worker, Monitor& monitor, uint8_t miner_id);
void startMonitor(Monitor& monitor);

#endif
