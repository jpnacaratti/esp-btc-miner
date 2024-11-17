#include <cstdio>
#include <stdlib.h>
#include <sys/_stdint.h>
#include "mbedtls/sha256.h"

#include "utils.h"
#include "models/Worker.h"

// Display import
#include <TFT_eSPI.h>

bool verifyPayload(String* line){
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

void getRandomExtranonce2ESP(int extranonce2Size, char *extranonce2) {
  uint32_t extranonce2Number = esp_random();

  uint8_t b0 = extranonce2Number & 0xFF;
  uint8_t b1 = (extranonce2Number >> 8) & 0xFF;
  uint8_t b2 = (extranonce2Number >> 16) & 0xFF;
  uint8_t b3 = (extranonce2Number >> 24) & 0xFF;

  char format[] = "%00x";

  sprintf(&format[1], "%02dx", extranonce2Size * 2);
  sprintf(extranonce2, format, extranonce2Number);
}

void getRandomExtranonce2(int extranonce2Size, char *extranonce2) {
  uint8_t b0, b1, b2, b3;

  srand(millis());

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

String applyVersionMask(const String& version, const String& versionMask) {

  uint32_t version_int = strtoul(version.c_str(), NULL, 16);
  uint32_t versionmask_int = strtoul(versionMask.c_str(), NULL, 16);

  uint32_t new_version_int = version_int | versionmask_int;

  char new_version_hex[9];
  snprintf(new_version_hex, sizeof(new_version_hex), "%08x", new_version_int);

  return String(new_version_hex);
}

void updateNonce(Miner& miner, uint32_t nonce) {
  memcpy(&miner.bytearray_blockheader[76], &nonce, sizeof(nonce));
}

double le256todouble(const void *target) {

	uint64_t *data64;
	double dcut64;

	data64 = (uint64_t *) (target + 24);
	dcut64 = *data64 * 6277101735386680763835789423207666416102355444464034512896.0;

	data64 = (uint64_t *) (target + 16);
	dcut64 += *data64 * 340282366920938463463374607431768211456.0;

	data64 = (uint64_t *) (target + 8);
	dcut64 += *data64 * 18446744073709551616.0;

	data64 = (uint64_t *) (target);
	dcut64 += *data64;

	return dcut64;
}

double diffFromTarget(void *target) {
	double d64, dcut64;

	d64 = TRUE_DIFF_ONE;
	dcut64 = le256todouble(target);
	
  if (unlikely(!dcut64)) dcut64 = 1;

	return d64 / dcut64;
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

  size_t size_target = toByteArray(target, 32, miner.bytearray_target);

  for (size_t j = 0; j < 8; j++) {
    miner.bytearray_target[j] ^= miner.bytearray_target[size_target - 1 - j];
    miner.bytearray_target[size_target - 1 - j] ^= miner.bytearray_target[j];
    miner.bytearray_target[j] ^= miner.bytearray_target[size_target - 1 - j];
  }

  // Calculating extranonce 2
  char extranonce2Char[2 * worker.extranonceSize + 1];	

  worker.extranonce2.toCharArray(extranonce2Char, 2 * worker.extranonceSize + 1);

  getNextExtranonce2(worker.extranonceSize, extranonce2Char);
  worker.extranonce2 = String(extranonce2Char);

  // Getting coinbase transaction
  String coinbase = job.coinb1 + worker.extranonce1 + worker.extranonce2 + job.coinb2;
  
  size_t str_len = coinbase.length() / 2;
  uint8_t bytearray[str_len];

  size_t res = toByteArray(coinbase.c_str(), str_len * 2, bytearray);

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

  char merkle_root[65];
  for (int i = 0; i < 32; i++) {
    snprintf(&merkle_root[i*2], 3, "%02x", miner.merkle_result[i]);
  }
  merkle_root[65] = 0;

  // Applying version mask and getting new version
  String newVersionMasked = applyVersionMask(job.version, worker.versionMask);

  // Blockheader in big endian
  String blockheader = newVersionMasked + job.prevBlockHash + String(merkle_root) + job.ntime + job.nbits + "00000000";
  str_len = blockheader.length() / 2;

  res = toByteArray(blockheader.c_str(), str_len * 2, miner.bytearray_blockheader);

  // Starting reversing header parts to little endian 
  uint8_t buff;
  size_t bword, bsize, boffset;

  // Parsing version to little endian
  boffset = 0;
  bsize = 4;
  for (size_t j = boffset; j < boffset + (bsize / 2); j++) {
    buff = miner.bytearray_blockheader[j];
    miner.bytearray_blockheader[j] = miner.bytearray_blockheader[2 * boffset + bsize - 1 - j];
    miner.bytearray_blockheader[2 * boffset + bsize - 1 - j] = buff;
  }

  // Parsing previous block hash to little endian (swaping words of 4 bytes)
  boffset = 4;
  bword = 4;
  bsize = 32;
  for (size_t i = 1; i <= bsize / bword; i++) {
    for (size_t j = boffset; j < boffset + bword / 2; j++) {
      buff = miner.bytearray_blockheader[j];
      miner.bytearray_blockheader[j] = miner.bytearray_blockheader[2 * boffset + bword - 1 - j];
      miner.bytearray_blockheader[2 * boffset + bword - 1 - j] = buff;
    }
    boffset += bword;
  }

  // Parsing ntime to little endian
  boffset = 68;
  bsize = 4;
  for (size_t j = boffset; j < boffset + (bsize / 2); j++) {
    buff = miner.bytearray_blockheader[j];
    miner.bytearray_blockheader[j] = miner.bytearray_blockheader[2 * boffset + bsize - 1 - j];
    miner.bytearray_blockheader[2 * boffset + bsize - 1 - j] = buff;
  }

  // Parsing nbits to little endian
  boffset = 72;
  bsize = 4;
  for (size_t j = boffset; j < boffset + (bsize / 2); j++) {
    buff = miner.bytearray_blockheader[j];
    miner.bytearray_blockheader[j] = miner.bytearray_blockheader[2 * boffset + bsize - 1 - j];
    miner.bytearray_blockheader[2 * boffset + bsize - 1 - j] = buff;
  }
}

uint16_t hexToRGB565(uint32_t hexColor) {
  uint8_t r = (hexColor >> 16) & 0xFF;
  uint8_t g = (hexColor >> 8) & 0xFF;
  uint8_t b = hexColor & 0xFF;
  return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
}

void updateText(TFT_eSprite &sprite, int x, int y, int rectWidth, int rectHeight, const char *newText, uint16_t textColor, uint16_t bgColor, const uint8_t *font, short orientation) {
  sprite.setTextDatum(orientation);

  sprite.loadFont(font);

  if (orientation == TR_DATUM) {
    sprite.fillRect(x - rectWidth, y - 2, rectWidth, rectHeight, bgColor);
  } else {
    sprite.fillRect(x, y - 2, rectWidth, rectHeight, bgColor);
  }

  sprite.setTextColor(textColor, bgColor);
  sprite.drawString(newText, x, y);

  sprite.pushSprite(0, 0);

  sprite.unloadFont();
}

void loadImage(TFT_eSprite &sprite, int x, int y, int rectWidth, int rectHeight, int imageWidth, int imageHeight, uint16_t bgColor, const unsigned short *image) {
  sprite.fillRect(x, y, rectWidth, rectHeight, bgColor);

  sprite.pushImage(x, y, imageWidth, imageHeight, image);
}

String formatNumber(double number, bool useSuffix, bool cutLastZero) {
  if (number == 0) return "";

  String result;

  if (number >= 1000000000) {
    result = String(number / 1000000000.0, 2) + (useSuffix ? "B" : "");
  } else if (number >= 1000000) {
    result = String(number / 1000000.0, 2) + (useSuffix ? "M" : "");
  } else if (number >= 1000) {
    result = String(number / 1000.0, 2) + (useSuffix ? "K" : "");
  } else if (number >= 1.0) {
    result = String(number, 2);
  } else { 
    int decimalPlaces = 0;
    double temp = number;

    while (temp < 1.0 && decimalPlaces < 6) {
      temp *= 10;
      decimalPlaces++;
    }

    result = String(number, decimalPlaces + 1);
  }

  if (result.indexOf('.') > 0 && cutLastZero) {
    while (result.endsWith("0")) {
      result.remove(result.length() - 1);
    }
    if (result.endsWith(".")) {
      result.remove(result.length() - 1);
    }
  }

  return result;
}

String formatUptime(unsigned long millis) {
  unsigned long seconds = millis / 1000;
  unsigned long minutes = seconds / 60;
  unsigned long hours = minutes / 60;
  unsigned long days = hours / 24;

  seconds = seconds % 60;
  minutes = minutes % 60;
  hours = hours % 24;

  char buffer[20];
  if (days < 10) {
    snprintf(buffer, sizeof(buffer), "%1ldd  %02ld:%02ld:%02ld", days, hours, minutes, seconds);
  } else {
    snprintf(buffer, sizeof(buffer), "%2ldd %02ld:%02ld:%02ld", days, hours, minutes, seconds);
  }

  return String(buffer);
}