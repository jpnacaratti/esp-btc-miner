#include <stdint.h>
#include "freertos/portmacro.h"
#include <Arduino.h>
#include <WiFi.h>

#include "mining.h"
#include "stratum.h"
#include "utils.h"
#include "models/Worker.h"
#include "enums/StratumMethod.h"
#include "nerdSHA256plus.h"

static WiFiClient client;
unsigned long mLastCommunication = millis();

uint32_t hashes = 0;

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
            worker.miner.newJob = false;
            worker.extranonce2 = "";

            buildBlockHeader(worker);
            worker.miner.newJob = true;
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

void startMiner(Worker& worker, uint8_t miner_id) {
  vTaskDelay(5000 / portTICK_PERIOD_MS);

  Serial.printf("Init hashing with miner: %u\n", miner_id);

  while (true) {

    if (worker.miner.bytearray_blockheader[0] == 0 &&
        worker.miner.bytearray_blockheader[1] == 0 &&
        worker.miner.bytearray_blockheader[2] == 0) {
      vTaskDelay(100 / portTICK_PERIOD_MS);
      continue;
    }

    Miner& miner = worker.miner;

    if (!miner.newJob) {
      vTaskDelay(100 / portTICK_PERIOD_MS);
      continue;
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);

    nerdSHA256_context nerdMidstate;
    uint8_t hash[32];

    nerd_mids(&nerdMidstate, miner.bytearray_blockheader);

    miner.mining = true;

    uint32_t nonce = TARGET_NONCE - MAX_NONCE;
    nonce += miner_id;

    unsigned char *header64;
    memcpy(miner.bytearray_blockheader2, &miner.bytearray_blockheader, 80);
    if (miner_id == 0)
      header64 = miner.bytearray_blockheader + 64;
    else
      header64 = miner.bytearray_blockheader2 + 64;

    Serial.printf("Miner: %u started hashing nonces\n", miner_id);
    while (true) {
      
      if (nonce > TARGET_NONCE) {
        Serial.printf("Miner: %u asked for a new header because exceed the max TARGET_NONCE\n", miner_id);
        break;
      }

      if (!miner.mining) {
        Serial.printf("Miner: %u stoped to start mine a new JOB\n", miner_id);
        break;
      }

      if (miner_id == 0)
        memcpy(miner.bytearray_blockheader + 76, &nonce, 4);
      else
        memcpy(miner.bytearray_blockheader2 + 76, &nonce, 4);

      nerd_sha256d(&nerdMidstate, header64, hash);

      hashes++;

      if(hash[31] !=0 || hash[30] !=0) {
        nonce += 2;
        continue;
      }

      double difficulty = diffFromTarget(hash);

      if (difficulty > worker.bestDiff) worker.bestDiff = difficulty;

      if (difficulty >= worker.poolDifficulty) {
        Serial.println("SHARE FOUND!");
        Serial.printf("-> Nonce: %u\n", nonce);
        Serial.printf("-> Miner who found: %u\n", miner_id);
        Serial.printf("-> Share difficulty: %.10f\n", difficulty);
        Serial.printf("-> Pool difficulty: %.4f\n", worker.poolDifficulty);
        Serial.printf("-> Best difficulty: %.4f\n", worker.bestDiff);
        Serial.printf("-> Hashes: %u\n", hashes);

        stratumSubmit(client, worker, nonce);
        mLastCommunication = millis();
      }

      nonce += 2;
    }

    vTaskDelay(250 / portTICK_PERIOD_MS);
  }
}
