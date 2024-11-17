#ifndef MINER_PARAMS_H
#define MINER_PARAMS_H

#include "Worker.h"
#include "Monitor.h"

typedef struct {
  Worker* worker;
  Monitor* monitor;
  uint8_t miner_id;
} StartMinerParams;

#endif