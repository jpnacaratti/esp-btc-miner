#ifndef MONITOR_H
#define MONITOR_H

typedef struct {
  unsigned int templates;
  unsigned int shares;
  unsigned int hashes;
  unsigned short bigShares;
  double bestDiff;
  String lastTemplates;
  String lastShares;
  String lastHashRate;
  String lastBigShares;
  String lastBestDiff;
  bool lastWifiConnected;
  bool lastPoolConnected;
} Monitor;

#endif