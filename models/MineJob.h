#ifndef MINERJOB_H
#define MINERJOB_H

#include <ArduinoJson.h>

typedef struct {
  String jobId;
  String prevBlockHash;
  String coinb1;
  String coinb2;
  StaticJsonDocument<512> merkleBuffer;
  JsonArray merkleBranch;
  String version;
  String nbits;
  String ntime;
} MineJob;

#endif
