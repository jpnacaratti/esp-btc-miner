#ifndef WORKER_H
#define WORKER_H

#include "Miner.h"

typedef struct {
    IPAddress poolIP = IPAddress(1, 1, 1, 1); 
    Miner miner;
    String softwareVersion;
    String workerName;
    String workerPass;
    String extranonce1;
    String extranonce2;
    String versionMask;
    int extranonceSize;
    bool subscribed;
    double poolDifficulty;
} Worker;

#endif
