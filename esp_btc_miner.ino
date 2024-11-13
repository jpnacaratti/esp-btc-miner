#include <WiFi.h>

#include "mining.h"
#include "models/Worker.h"
#include "models/StartStratumParams.h"

constexpr char WIFI_SSID[] = "John P";
constexpr char WIFI_PASSWORD[] = "1234567a";

constexpr char SOFTWARE_VERSION[] = "Miiner/1.0.0";

constexpr char POOL_HOST[] = "public-pool.io";
constexpr uint16_t POOL_PORT = 21496;
constexpr char POOL_PASSWORD[] = "x";
constexpr char BTC_ADDRESS[] = "bc1ql076evskm73yqgl5e79g6vpukqnh8x9t32frdx";
constexpr char WORKER_NAME[] = "miinero";

constexpr float SUGGESTED_DIFFICULTY = 0.001f;

void setup() {
  Serial.begin(115200);
  delay(1000);

  WiFi.disconnect();
  delay(1000);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int retryCount = 0;
  while (WiFi.status() != WL_CONNECTED && retryCount < 60) {
    delay(1000);
    Serial.print(".");
    retryCount++;
  }

  Serial.printf("ESP32 IP assigned: %s\n", WiFi.localIP().toString().c_str());

  Worker worker;

  worker.workerName = String(BTC_ADDRESS) + "." + WORKER_NAME;
  worker.workerPass = String(POOL_PASSWORD);

  worker.softwareVersion = String(SOFTWARE_VERSION);

  // Creating new pointer to pass as a param in xTask
  StartStratumParams* stratumParams = new StartStratumParams;
  stratumParams->worker = &worker;
  stratumParams->poolAddress = POOL_HOST;
  stratumParams->poolPort = POOL_PORT;
  stratumParams->suggestDifficulty = SUGGESTED_DIFFICULTY;

  xTaskCreate(
    startStratumTask,
    "Stratum",
    15000,
    stratumParams,
    1,
    NULL
  );

  Serial.println("Finished setup!");
}

void loop() {
  // put your main code here, to run repeatedly:
}

void startStratumTask(void* pvParameters) {
  StartStratumParams* params = (StartStratumParams*) pvParameters;

  startStratum(*params->worker, params->poolAddress, params->poolPort, params->suggestDifficulty);
}