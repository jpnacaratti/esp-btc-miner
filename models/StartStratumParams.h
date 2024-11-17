#ifndef STRATUM_PARAMS_H
#define STRATUM_PARAMS_H

#include "Worker.h"
#include "Monitor.h"

typedef struct {
  Worker* worker;
  Monitor* monitor;
  const char* poolAddress;
  uint16_t poolPort;
  float suggestDifficulty;
} StartStratumParams;

#endif
