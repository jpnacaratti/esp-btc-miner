#ifndef WORKER_H
#define WORKER_H

#include "Miner.h"

typedef struct {
    IPAddress poolIP = IPAddress(1, 1, 1, 1); 
    Miner miner;
    String workerName;
    String extranonce1;
    String extranonce2;
    int extranonceSize;
    bool subscribed;
} Worker;

#endif
