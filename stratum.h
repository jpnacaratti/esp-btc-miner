#ifndef STRATUM_H
#define STRATUM_H

#include "models/Worker.h"
#include "enums/StratumMethod.h"

bool stratumSubscribe(WiFiClient& client, Worker& worker);
bool stratumParseSubscribe(String& line, Worker& worker);

bool stratumConfigure(WiFiClient& client, Worker& worker);
bool stratumParseConfigure(String& line, Worker& worker);

bool stratumAuthorize(WiFiClient& client, const char *user, const char *pass);
bool stratumParseAuthorize(String& line);

bool stratumSuggestDifficulty(WiFiClient& client, double suggestedDifficulty);
bool stratumParseDifficulty(String& line, Worker& worker);

bool stratumParseNotify(String& line, MineJob& job);

StratumMethod stratumParseMethod(String& line); 

#endif