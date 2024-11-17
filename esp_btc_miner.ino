#include <WiFi.h>

#include "mining.h"
#include "models/Worker.h"
#include "models/Monitor.h"
#include "models/StartStratumParams.h"
#include "models/StartMinerParams.h"

constexpr char WIFI_SSID[] = "John P";
constexpr char WIFI_PASSWORD[] = "1234567a";

constexpr char SOFTWARE_VERSION[] = "Miiner/1.0.0";

constexpr char POOL_HOST[] = "public-pool.io";
constexpr uint16_t POOL_PORT = 21496;
constexpr char POOL_PASSWORD[] = "x";
constexpr char BTC_ADDRESS[] = "bc1ql076evskm73yqgl5e79g6vpukqnh8x9t32frdx";
constexpr char WORKER_NAME[] = "miinero";

constexpr float SUGGESTED_DIFFICULTY = 0.0001f;

Worker* worker = nullptr;
Monitor* monitor = nullptr;

void setup() {
  Serial.begin(115200);
  delay(1000);

  disableCore1WDT();

  WiFi.disconnect();
  delay(1000);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  worker = new Worker{};
  monitor = new Monitor{};

  worker->workerName = String(BTC_ADDRESS) + "." + WORKER_NAME;
  worker->workerPass = String(POOL_PASSWORD);

  worker->softwareVersion = String(SOFTWARE_VERSION);

  Serial.println("Starting monitor task...");
  xTaskCreate(
    startMonitorTask,
    "Monitor",
    10000,
    (void*) monitor,
    4,
    NULL
  );

  // Creating new pointer to pass as a param in xTask
  StartStratumParams* stratumParams = new StartStratumParams;
  stratumParams->worker = worker;
  stratumParams->monitor = monitor;
  stratumParams->poolAddress = POOL_HOST;
  stratumParams->poolPort = POOL_PORT;
  stratumParams->suggestDifficulty = SUGGESTED_DIFFICULTY;

  Serial.println("Starting stratum task...");
  xTaskCreate(
    startStratumTask,
    "Stratum",
    15000,
    stratumParams,
    3,
    NULL
  );

  // Creating new pointer to pass as a param in xTaskPinned
  StartMinerParams* miner0Params = new StartMinerParams;
  miner0Params->worker = worker;
  miner0Params->monitor = monitor;
  miner0Params->miner_id = 0;

  Serial.println("Starting miner 0 task...");
  xTaskCreatePinnedToCore(
    startMinerTask,
    "Miner0",
    6000,
    miner0Params,
    1,
    NULL,
    1
  );

  // Creating new pointer to pass as a param in xTaskPinned
  StartMinerParams* miner1Params = new StartMinerParams;
  miner1Params->worker = worker;
  miner1Params->monitor = monitor;
  miner1Params->miner_id = 1;
  
  Serial.println("Starting miner 1 task...");
  xTaskCreatePinnedToCore(
    startMinerTask,
    "Miner1",
    6000,
    miner1Params,
    1,
    NULL,
    1
  );

  Serial.println("Finished setup!");
}

void loop() {
  // put your main code here, to run repeatedly:
}

void startStratumTask(void* pvParameters) {
  StartStratumParams* params = (StartStratumParams*) pvParameters;

  startStratum(*params->worker, *params->monitor, params->poolAddress, params->poolPort, params->suggestDifficulty);
}

void startMinerTask(void* pvParameters) {
  StartMinerParams* params = (StartMinerParams*) pvParameters;

  startMiner(*params->worker, *params->monitor, params->miner_id);
}

void startMonitorTask(void* pvParameters) {
  Monitor* monitor = (Monitor*) pvParameters;

  startMonitor(*monitor);
}