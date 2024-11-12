#ifndef MINERJOB_H
#define MINERJOB_H

#include <ArduinoJson.h>

typedef struct {
  String jobId;
  String prevBlockHash;
  String coinb1;
  String coinb2;
  JsonArray merkle_branch;
  String version;
  String nbits;
  String ntime;
} MineJob;

#endif
