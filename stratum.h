#ifndef STRATUM_H
#define STRATUM_H

#include "models/Worker.h"

bool stratumSubscribe(WiFiClient& client, Worker& worker);
bool parseMiningSubscribe(String line, Worker& worker);

#endif