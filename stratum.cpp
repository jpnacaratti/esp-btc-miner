#include <cstring>
#include <Arduino.h>
#include <WiFi.h>

#include "stratum.h"
#include "utils.h"
#include "enums/StratumMethod.h"

bool stratumSubscribe(WiFiClient& client, Worker& worker) {
  char payload[BUFFER] = {0};

  sprintf(payload, "{\"id\": %u, \"method\": \"mining.subscribe\", \"params\": [\"%s\"]}\n", 1, worker.softwareVersion);

  Serial.printf("Sending subscribe request: %s\n", payload);
  client.print(payload);
  
  vTaskDelay(2000 / portTICK_PERIOD_MS);

  String line = client.readStringUntil('\n');
  if (!stratumParseSubscribe(line, worker)) return false;

  if (worker.extranonce1.length() == 0) {
    Serial.printf("Aborting mining, length of extranonce1: %u\n", worker.extranonce1.length());
    return false;
  }

  return true;
}

bool stratumParseSubscribe(String& line, Worker& worker) {
  
  if (!verifyPayload(&line)) return false;

  Serial.printf("Received from subscribe request: %s\n", line.c_str());

  StaticJsonDocument<BUFFER_JSON_DOC> doc;
  
  DeserializationError error = deserializeJson(doc, line);
  if (error || checkError(doc) || !doc.containsKey("result")) return false;

  worker.extranonce1 = doc["result"][1].as<String>();
  worker.extranonceSize = doc["result"][2];

  return true;
}

bool stratumConfigure(WiFiClient& client, Worker& worker) {
  char payload[BUFFER] = {0};

  sprintf(payload, "{\"id\": %u, \"method\": \"mining.configure\", \"params\": []}\n", 2);

  Serial.printf("Sending configure request: %s\n", payload);
  client.print(payload);
  
  vTaskDelay(2000 / portTICK_PERIOD_MS);

  String line = client.readStringUntil('\n');
  return stratumParseConfigure(line, worker);
}

bool stratumParseConfigure(String& line, Worker& worker) {

  if (!verifyPayload(&line)) return false;

  Serial.printf("Received from configure request: %s\n", line.c_str());

  StaticJsonDocument<BUFFER_JSON_DOC> doc;
  
  DeserializationError error = deserializeJson(doc, line);
  if (error || checkError(doc) || !doc.containsKey("result")) return false;

  worker.versionMask = doc["result"]["version-rolling.mask"].as<String>();

  return true;
}

bool stratumAuthorize(WiFiClient& client, const char *user, const char *pass) {
  char payload[BUFFER] = {0};

  sprintf(payload, "{\"id\": %u, \"method\": \"mining.authorize\", \"params\": [\"%s\", \"%s\"]}\n", 3, user, pass);

  Serial.printf("Sending authorize request: %s\n", payload);
  client.print(payload);
  
  vTaskDelay(2000 / portTICK_PERIOD_MS);

  String line = client.readStringUntil('\n');
  return stratumParseAuthorize(line);
}

bool stratumParseAuthorize(String& line) {
  
  if (!verifyPayload(&line)) return false;

  Serial.printf("Received from authorize request: %s\n", line.c_str());

  StaticJsonDocument<BUFFER_JSON_DOC> doc;
  
  DeserializationError error = deserializeJson(doc, line);
  if (error || checkError(doc) || !doc.containsKey("result")) return false;

  return true;
}

bool stratumSuggestDifficulty(WiFiClient& client, double suggestedDifficulty) {
  char payload[BUFFER] = {0};

  sprintf(payload, "{\"id\": %d, \"method\": \"mining.suggest_difficulty\", \"params\": [%.7g]}\n", 4, suggestedDifficulty);
    
  Serial.printf("Sending suggest difficulty request: %s\n", payload);
  return client.print(payload);
}

bool stratumParseDifficulty(String& line, Worker& worker) {

  if (!verifyPayload(&line)) return false;

  StaticJsonDocument<BUFFER_JSON_DOC> doc;

  DeserializationError error = deserializeJson(doc, line);
  if (error || !doc.containsKey("params")) return false;

  double difficulty = (double) doc["params"][0];

  Serial.printf("Received difficulty: %.4f\n", difficulty);

  worker.poolDifficulty = difficulty;

  return true;
}

bool stratumParseNotify(String& line, MineJob& job) {

  Serial.println("Parsing method: NOTIFY");

  if (!verifyPayload(&line)) return false;

  StaticJsonDocument<BUFFER_JSON_DOC> doc;

  DeserializationError error = deserializeJson(doc, line);
  if (error || !doc.containsKey("params")) return false;

  job.jobId = String(doc["params"][0].as<const char*>());
  job.prevBlockHash = String(doc["params"][1].as<const char*>());
  job.coinb1 = String(doc["params"][2].as<const char*>());
  job.coinb2 = String(doc["params"][3].as<const char*>());
  
  // Saving merkle branch in a separated buffer to save pointer when the method ends
  JsonArray originalArray = doc["params"][4].as<JsonArray>();
  job.merkleBranch = job.merkleBuffer.to<JsonArray>();
  for (JsonVariant v : originalArray) {
    job.merkleBranch.add(v);
  }

  job.version = String(doc["params"][5].as<const char*>());
  job.nbits = String(doc["params"][6].as<const char*>());
  job.ntime = String(doc["params"][7].as<const char*>());

  return true;
}

bool stratumSubmit(WiFiClient& client, Worker& worker, unsigned long nonce) {
  char payload[BUFFER] = {0};

  MineJob& job = worker.miner.job;

  sprintf(payload, "{\"id\": %u, \"method\": \"mining.submit\", \"params\": [\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\"]}\n",
    5,
    worker.workerName.c_str(),
    job.jobId.c_str(),
    worker.extranonce2.c_str(),
    job.ntime.c_str(),
    String(nonce, HEX).c_str(),
    worker.versionMask.c_str()
  );

  Serial.printf("Sending mine job submit request: %s\n", payload);
  return client.print(payload);
}

StratumMethod stratumParseMethod(String& line) {

  Serial.printf("Message received from pool: %s\n", line.c_str());

  if (!verifyPayload(&line)) return STRATUM_PARSE_ERROR;

  StaticJsonDocument<BUFFER_JSON_DOC> doc;
  
  DeserializationError error = deserializeJson(doc, line);
  if (error || checkError(doc)) return STRATUM_PARSE_ERROR;

  if (!doc.containsKey("method")) {
    if (doc["error"].isNull()) return STRATUM_SUCCESS;
    else return STRATUM_UNKNOWN;
  }

  StratumMethod toReturn = STRATUM_UNKNOWN;

  if (strcmp("mining.notify", (const char*) doc["method"]) == 0) {
    toReturn = MINING_NOTIFY;
  } else if (strcmp("mining.set_difficulty", (const char*) doc["method"]) == 0) {
    toReturn = MINING_SET_DIFFICULTY;
  }

  return toReturn;
}