#include <WiFi.h>

#include "mining.h"
#include "models/Worker.h"

constexpr char WIFI_SSID[] = "Familia Maia";
constexpr char WIFI_PASSWORD[] = "26730008";

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

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Worker worker;

  worker.workerName = String(BTC_ADDRESS) + "." + WORKER_NAME;

  startStratum(worker, POOL_HOST, POOL_PORT, SUGGESTED_DIFFICULTY);

  Serial.println("Finished setup!");
}

void loop() {
  // put your main code here, to run repeatedly:

}
