#pragma once

#include <Arduino.h>
#include <LittleFS.h>

class Blocklist {
 public:
  bool begin(const char* path = "/blocklist.bin");
  bool containsDomainOrParent(const String& domain) const;
  size_t entries() const { return entries_; }
  bool ready() const { return file_ && entries_ > 0; }

 private:
  static uint64_t fnv1a40(const String& text);
  bool containsHash(uint64_t hash) const;

  mutable File file_;
  size_t entries_ = 0;
};
