#ifndef STRATUM_PARAMS_H
#define STRATUM_PARAMS_H

#include "Worker.h"

typedef struct {
  Worker* worker;
  const char* poolAddress;
  uint16_t poolPort;
  float suggestDifficulty;
} StartStratumParams;

#endif
