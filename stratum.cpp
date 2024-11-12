#include <Arduino.h>
#include <WiFi.h>

#include "stratum.h"
#include "utils.h"

bool stratumSubscribe(WiFiClient& client, Worker& worker) {
  char payload[BUFFER] = {0};

  sprintf(payload, "{\"id\": %u, \"method\": \"mining.subscribe\", \"params\": [\"%s\"]}\n", 1, worker.workerName);

  Serial.printf("Sending subscribe request: %s\n", payload);
  client.print(payload);
  
  vTaskDelay(200 / portTICK_PERIOD_MS);

  String line = client.readStringUntil('\n');
  if (!parseMiningSubscribe(line, worker)) return false;

  if (worker.extranonce1.length() == 0) {
    Serial.printf("Aborting mining, length of extranonce1: %u\n", worker.extranonce1.length());
    return false;
  }

  return true;
}

bool parseMiningSubscribe(String line, Worker& worker) {
  
  if(!verifyPayload(&line)) return false;

  Serial.printf("Received from subscribe request: %s\n", line);

  StaticJsonDocument<BUFFER_JSON_DOC> doc;
  
  DeserializationError error = deserializeJson(doc, line);
  if (error || checkError(doc) || !doc.containsKey("result")) return false;

  worker.extranonce1 = String((const char*) doc["result"][1]);
  worker.extranonceSize = doc["result"][2];

  return true;
}