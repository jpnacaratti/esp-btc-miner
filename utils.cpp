#include "mbedtls/sha256.h"

#include "utils.h"
#include "models/Worker.h"

bool verifyPayload (String* line){
  if (line->length() == 0) return false;
  
  line->trim();
  if (line->isEmpty()) return false;

  return true;
}

bool checkError(const StaticJsonDocument<BUFFER_JSON_DOC> doc) {
  
  if (!doc.containsKey("error")) return false;
  
  if (doc["error"].size() == 0) return false;

  Serial.printf("ERROR: %d | reason: %s \n", (const int) doc["error"][0], (const char*) doc["error"][1]);

  return true;  
}

uint8_t hex(char ch) {
  uint8_t r = (ch > 57) ? (ch - 55) : (ch - 48);
  return r & 0x0F;
}

int toByteArray(const char *in, size_t in_size, uint8_t *out) {
  int count = 0;
  if (in_size % 2) {
    while (*in && out) {
      *out = hex(*in++);
      if (!*in)
        return count;
      *out = (*out << 4) | hex(*in++);
      *out++;
      count++;
    }
    return count;
  } else {
    while (*in && out) {
      *out++ = (hex(*in++) << 4) | hex(*in++);
      count++;
    }
    return count;
  }
}

void getRandomExtranonce2(int extranonce2Size, char *extranonce2) {
  uint8_t b0, b1, b2, b3;

  b0 = rand() % 256;
  b1 = rand() % 256;
  b2 = rand() % 256;
  b3 = rand() % 256;

  unsigned long extranonce2Number = b3 << 24 | b2 <<  16 | b1 << 8 | b0;

  char format[] = "%00x";

  sprintf(&format[1], "%02dx", extranonce2Size * 2);
  sprintf(extranonce2, format, extranonce2Number);
}

void getNextExtranonce2(int extranonce2Size, char *extranonce2) {
  unsigned long extranonce2Number = strtoul(extranonce2, NULL, 10);

  extranonce2Number++;

  char format[] = "%00x";

  sprintf(&format[1], "%02dx", extranonce2Size * 2);
  sprintf(extranonce2, format, extranonce2Number);
}

void doubleSha256(const byte *input, size_t input_length, byte *output) {
  mbedtls_sha256_context ctx;
  mbedtls_sha256_init(&ctx);

  byte interResult[32];

  mbedtls_sha256_starts(&ctx, 0);
  mbedtls_sha256_update(&ctx, input, input_length);
  mbedtls_sha256_finish(&ctx, interResult);

  mbedtls_sha256_starts(&ctx, 0);
  mbedtls_sha256_update(&ctx, interResult, 32);
  mbedtls_sha256_finish(&ctx, output);

  mbedtls_sha256_free(&ctx);
}

void buildBlockHeader(Worker& worker) {

  Miner& miner = worker.miner;
  MineJob& job = worker.miner.job;

  // Calculating block difficulty
  char target[TARGET_BUFFER_SIZE+1];
  memset(target, '0', TARGET_BUFFER_SIZE);

  int zeros = (int) strtol(job.nbits.substring(0, 2).c_str(), 0, 16) - 3;
  memcpy(target + zeros - 2, job.nbits.substring(2).c_str(), job.nbits.length() - 2);

  target[TARGET_BUFFER_SIZE] = 0;

  Serial.print("    target: "); Serial.println(target);

  size_t size_target = toByteArray(target, 32, miner.bytearray_target);

  for (size_t j = 0; j < 8; j++) {
    miner.bytearray_target[j] ^= miner.bytearray_target[size_target - 1 - j];
    miner.bytearray_target[size_target - 1 - j] ^= miner.bytearray_target[j];
    miner.bytearray_target[j] ^= miner.bytearray_target[size_target - 1 - j];
  }

  // Calculating extranonce 2
  char extranonce2Char[2 * worker.extranonceSize + 1];	

  worker.extranonce2.toCharArray(extranonce2Char, 2 * worker.extranonceSize + 1);

  if (worker.extranonce2.isEmpty()) getRandomExtranonce2(worker.extranonceSize, extranonce2Char);
  else getNextExtranonce2(worker.extranonceSize, extranonce2Char);

  worker.extranonce2 = String(extranonce2Char);

  Serial.print("    extranonce1: "); Serial.println(worker.extranonce1);
  Serial.print("    extranonce2: "); Serial.println(worker.extranonce2);

  // Getting coinbase transaction
  String coinbase = job.coinb1 + worker.extranonce1 + worker.extranonce2 + job.coinb2;
  
  Serial.print("    coinbase hex: "); Serial.println(coinbase);

  size_t str_len = coinbase.length() / 2;
  uint8_t bytearray[str_len];

  size_t res = toByteArray(coinbase.c_str(), str_len * 2, bytearray);

  Serial.print("    coinbase bytes size: "); Serial.println(res);

  byte shaCoinbase[32];
  doubleSha256(bytearray, str_len, shaCoinbase); // Coinbase hash

  // Calculating merkle root
  memcpy(miner.merkle_result, shaCoinbase, sizeof(shaCoinbase));

  byte merkleConcatenated[32 * 2];
  for (size_t k = 0; k < job.merkleBranch.size(); k++) {
    const char* merkleElement = (const char*) job.merkleBranch[k];
    uint8_t bytearray[32];
    size_t res = toByteArray(merkleElement, 64, bytearray);

    for (size_t i = 0; i < 32; i++) {
      merkleConcatenated[i] = miner.merkle_result[i];
      merkleConcatenated[32 + i] = bytearray[i];
    }

    doubleSha256(merkleConcatenated, sizeof(merkleConcatenated), miner.merkle_result);
  }

}

