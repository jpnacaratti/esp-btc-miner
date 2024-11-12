#include "utils.h"

bool verifyPayload (String* line){
  if (line -> length() == 0) return false;
  
  line -> trim();
  if (line -> isEmpty()) return false;

  return true;
}

bool checkError(const StaticJsonDocument<BUFFER_JSON_DOC> doc) {
  
  if (!doc.containsKey("error")) return false;
  
  if (doc["error"].size() == 0) return false;

  Serial.printf("ERROR: %d | reason: %s \n", (const int) doc["error"][0], (const char*) doc["error"][1]);

  return true;  
}