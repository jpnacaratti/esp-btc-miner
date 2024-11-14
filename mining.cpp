#include "freertos/portmacro.h"
#include <Arduino.h>
#include <WiFi.h>

#include "mining.h"
#include "stratum.h"
#include "models/Worker.h"
#include "enums/StratumMethod.h"

static WiFiClient client;
unsigned long mLastCommunication = millis();

bool checkPoolConnection(Worker& worker, const char* pool_address, const uint16_t pool_port) {

  if (client.connected()) {
    return true;
  }

  worker.subscribed = false;

  IPAddress& poolIP = worker.poolIP;

  Serial.println("Client not connected to pool, connecting...");

  if (poolIP == IPAddress(1,1,1,1)) {
    WiFi.hostByName(pool_address, poolIP);
    Serial.println("Resolved DNS and save ip, got: " + poolIP.toString());
  }

  if (!client.connect(poolIP, pool_port)) {
    WiFi.hostByName(pool_address, poolIP);
    Serial.printf("Can't reach the server: %s. Resolved DNS got: %s\n", pool_address, poolIP.toString());
    return false;
  }

  return true;
}

bool checkPoolInactivity(unsigned long lastSocketSent, unsigned int silenceLimit) {
  return millis() - lastSocketSent > silenceLimit;
}

void startStratum(Worker& worker, const char* pool_address, const uint16_t pool_port, float suggestDifficulty) {
  
  double currentDifficulty = suggestDifficulty;

  Serial.println("Starting stratum...");

  while (true) {

    if (WiFi.status() != WL_CONNECTED) {

      // Show in display that the WiFi is reconnecting

      WiFi.reconnect();
      vTaskDelay(5000 / portTICK_PERIOD_MS);
      continue;
    }

    if (!checkPoolConnection(worker, pool_address, pool_port)) {
      srand(millis()); // Generate a random delay to avoid overloading the pool server (1 - 120s)
      vTaskDelay(((1 + rand() % 120) * 1000) / portTICK_PERIOD_MS);
      continue;
    }

    if (!worker.subscribed) {

      worker.miner.mining = false;

      if (!stratumSubscribe(client, worker)) {
        client.stop();
        continue;
      }

      stratumConfigure(client, worker);

      if (!stratumAuthorize(client, worker.workerName.c_str(), worker.workerPass.c_str())) {
        client.stop();
        continue;
      }

      stratumSuggestDifficulty(client, suggestDifficulty);

      worker.subscribed = true;
      mLastCommunication = millis();
    }

    if (checkPoolInactivity(mLastCommunication, COMMUNICATION_SILENCE_LIMIT)) {
      Serial.println("Sending keep alive socket...");
      stratumSuggestDifficulty(client, suggestDifficulty);
      mLastCommunication = millis();
    }

    while (client.connected() && client.available()) {
      
      String line = client.readStringUntil('\n');
      StratumMethod method = stratumParseMethod(line);
      mLastCommunication = millis();
      switch (method) {
        case STRATUM_PARSE_ERROR: {
          Serial.println("Error when parsing JSON");
          break;
        }
        case MINING_NOTIFY: {
          if (stratumParseNotify(line, worker.miner.job)) {

            worker.templates++;

            worker.miner.mining = false;

            worker.extranonce2 = "";

          }
          break;
        }
        case MINING_SET_DIFFICULTY: {
          stratumParseDifficulty(line, worker);
          break;
        }
        case STRATUM_SUCCESS: {
          Serial.println("Success when submiting");
          break;
        }
        default: {
          Serial.println("Unknown JSON received");
          break;
        }
      }
    }

    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
}