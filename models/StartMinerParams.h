#ifndef MINER_PARAMS_H
#define MINER_PARAMS_H

#include "Worker.h"

typedef struct {
  Worker* worker;
  uint8_t miner_id;
} StartMinerParams;

#endif