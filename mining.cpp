#include <Arduino.h>
#include <WiFi.h>

#include "mining.h"
#include "stratum.h"
#include "models/Worker.h"

static WiFiClient client;

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

void startStratum(Worker& worker, const char* pool_address, const uint16_t pool_port, float suggestDifficulty) {
  
  double currentDifficulty = suggestDifficulty;

  Serial.println("Starting stratum...");

  while(true) {

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

    }

  }
}