#ifndef MINER_H
#define MINER_H

#include "MineJob.h"

typedef struct {
  MineJob job = {};
  uint8_t bytearray_target[32];
  uint8_t bytearray_blockheader[80];
  uint8_t bytearray_blockheader2[80];
  uint8_t merkle_result[32];
  bool mining;
  bool newJob = false;
} Miner;

#endif
