#include <stdint.h>
#include "freertos/portmacro.h"
#include <Arduino.h>
#include <WiFi.h>

#include "mining.h"
#include "stratum.h"
#include "utils.h"
#include "models/Monitor.h"
#include "models/Worker.h"
#include "enums/StratumMethod.h"
#include "nerdSHA256plus.h"

// Display imports
#include <TFT_eSPI.h>
#include "data/poppins_40.h"
#include "data/poppins_18.h"
#include "data/bg_screen_1.h"
#include "data/check_icon.h"
#include "data/error_icon.h"

#include "esp_system.h"
#include "driver/adc.h"

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite sprite = TFT_eSprite(&tft);

static WiFiClient client;
unsigned long mLastCommunication = millis();
unsigned long mLastSubmit = millis();

uint32_t hashes = 0;

float lastTemperature = 0;

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

void stopMining(Worker& worker) {
  worker.miner.mining = false;
  worker.miner.newJob = false;
  worker.extranonce2 = "";
}

void startStratum(Worker& worker, Monitor& monitor, const char* pool_address, const uint16_t pool_port, float suggestDifficulty) {
  
  Serial.println("Starting stratum...");

  while (true) {

    if (lastTemperature >= 65.0) {
      stopMining(worker);
      client.stop();
      Serial.println("Too high temperature in chip, taking a breath...");
      vTaskDelay(300000 / portTICK_PERIOD_MS);
      continue;
    }

    if (WiFi.status() != WL_CONNECTED) {
      stopMining(worker);
      client.stop();
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
      mLastSubmit = millis();
    }

    if (checkPoolInactivity(mLastCommunication, COMMUNICATION_SILENCE_LIMIT)) {

      if (millis() - mLastSubmit > SUBMIT_SILENCE_LIMIT) {
        stopMining(worker);
        client.stop();
        Serial.println("More than five minutes without sending shares, restarting...");
        continue;
      }

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
            
            if (!worker.miner.job.jobId.equals(worker.lastJobId)) {
              monitor.templates++;
              worker.lastJobId = worker.miner.job.jobId;
            }

            stopMining(worker);

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

void startMiner(Worker& worker, Monitor& monitor, uint8_t miner_id) {
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

      if (!worker.subscribed) {
        Serial.printf("Miner: %u stoped because lost connection with the host\n", miner_id);
        break;
      }

      if (miner_id == 0)
        memcpy(miner.bytearray_blockheader + 76, &nonce, 4);
      else
        memcpy(miner.bytearray_blockheader2 + 76, &nonce, 4);

      nerd_sha256d(&nerdMidstate, header64, hash);

      monitor.hashes++;

      // 16 bit share
      if(hash[31] !=0 || hash[30] !=0) {
        nonce += 2;
        continue;
      }

      double difficulty = diffFromTarget(hash);

      if (difficulty > monitor.bestDiff) monitor.bestDiff = difficulty;

      if (difficulty >= worker.poolDifficulty) {
        lastTemperature = temperatureRead();

        Serial.println("SHARE FOUND!");
        Serial.printf("-> Nonce: %u\n", nonce);
        Serial.printf("-> Miner who found: %u\n", miner_id);
        Serial.printf("-> Share difficulty: %.10f\n", difficulty);
        Serial.printf("-> Pool difficulty: %.4f\n", worker.poolDifficulty);
        Serial.printf("-> Best difficulty: %.4f\n", monitor.bestDiff);
        Serial.printf("-> Temperatura do chip: %.2f°C\n", lastTemperature);

        monitor.shares++;

        stratumSubmit(client, worker, nonce);
        mLastCommunication = millis();
        mLastSubmit = millis();
      }
      
      // 32 bit share
      if(hash[29] !=0 || hash[28] !=0) {
        nonce += 2;
        continue;
      }

      monitor.bigShares++;

      nonce += 2;
    }

    vTaskDelay(250 / portTICK_PERIOD_MS);
  }
}

void startMonitor(Monitor& monitor) {

  tft.init();
  tft.setRotation(1);

  sprite.createSprite(320, 170);
  sprite.setSwapBytes(true);
  sprite.fillSprite(TFT_BLACK);

  sprite.pushImage(0, 0, 320, 170, BG_SCREEN_1);

  vTaskDelay(1000 / portTICK_PERIOD_MS);

  uint16_t hrTextColor = hexToRGB565(0x050505);
  uint16_t hrBackground = hexToRGB565(0xD0E1E9);

  uint16_t genericTextColor = hexToRGB565(0xFBFBFB);
  uint16_t genericBackground = hexToRGB565(0x343B3D);

  // HashRate
  updateText(sprite, 19, 123, 115, 35, "0", hrTextColor, hrBackground, POPPINS_40, TL_DATUM);

  // Block templates
  updateText(sprite, 166, 20, 95, 17, "0", genericTextColor, genericBackground, POPPINS_18, TL_DATUM);

  // Best difficulty
  updateText(sprite, 166, 43, 95, 17, "0", genericTextColor, genericBackground, POPPINS_18, TL_DATUM);

  // 32 bits shares
  updateText(sprite, 166, 66, 95, 17, "0", genericTextColor, genericBackground, POPPINS_18, TL_DATUM);

  // Active time
  updateText(sprite, 194, 96, 113, 17, "0d  00:00:00", genericTextColor, genericBackground, POPPINS_18, TL_DATUM);

  // Shares found
  updateText(sprite, 268, 143, 60, 17, "0", genericTextColor, genericBackground, POPPINS_18, TR_DATUM);

  // Wifi icon
  loadImage(sprite, 269, 28, 20, 20, 20, 20, genericBackground, ERROR_ICON);

  // Pool icon
  loadImage(sprite, 269, 62, 20, 20, 20, 20, genericBackground, ERROR_ICON);

  while (true) {

    bool newWifiStatus = WiFi.status() == WL_CONNECTED;
    if (newWifiStatus != monitor.lastWifiConnected) {
      loadImage(sprite, 269, 28, 20, 20, 20, 20, genericBackground, newWifiStatus ? CHECK_ICON : ERROR_ICON);
      monitor.lastWifiConnected = newWifiStatus;
    }
    
    bool newPoolStatus = (bool) client.connected();
    if (newPoolStatus != monitor.lastPoolConnected) {
      loadImage(sprite, 269, 62, 20, 20, 20, 20, genericBackground, newPoolStatus ? CHECK_ICON : ERROR_ICON);
      monitor.lastPoolConnected = newPoolStatus;
    }

    String hr = formatNumber(monitor.hashes / 2, false, false);
    if (!hr.equals(monitor.lastHashRate) && hr.length() == 5) { // WA - TODO: Remove later
      updateText(sprite, 19, 123, 115, 35, hr.c_str(), hrTextColor, hrBackground, POPPINS_40, TL_DATUM);
      monitor.lastHashRate = hr;
    }

    String blockTemplates = formatNumber(monitor.templates);
    if (!blockTemplates.equals(monitor.lastTemplates)) {
      updateText(sprite, 166, 20, 95, 17, blockTemplates.c_str(), genericTextColor, genericBackground, POPPINS_18, TL_DATUM);
      monitor.lastTemplates = blockTemplates;
    }

    String bestDifficulty = formatNumber(monitor.bestDiff);
    if (!bestDifficulty.equals(monitor.lastBestDiff)) {
      updateText(sprite, 166, 43, 95, 17, bestDifficulty.c_str(), genericTextColor, genericBackground, POPPINS_18, TL_DATUM);
      monitor.lastBestDiff = bestDifficulty;
    }

    String bigShares = formatNumber(monitor.bigShares);
    if (!bigShares.equals(monitor.lastBigShares)) {
      updateText(sprite, 166, 66, 95, 17, bigShares.c_str(), genericTextColor, genericBackground, POPPINS_18, TL_DATUM);
      monitor.lastBigShares = bigShares;
    }

    String activeTime = formatUptime(millis());
    updateText(sprite, 190, 96, 120, 17, activeTime.c_str(), genericTextColor, genericBackground, POPPINS_18, TL_DATUM);

    String sharesFound = formatNumber(monitor.shares);
    if (!sharesFound.equals(monitor.lastShares)) {
      updateText(sprite, 268, 143, 60, 17, sharesFound.c_str(), genericTextColor, genericBackground, POPPINS_18, TR_DATUM);
      monitor.lastShares = sharesFound;
    }

    monitor.hashes = 0;

    vTaskDelay(2000 / portTICK_PERIOD_MS);
  }
}
