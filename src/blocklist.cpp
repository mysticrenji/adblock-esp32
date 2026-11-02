#include "blocklist.h"

namespace {
constexpr uint64_t kMask40 = 0xFFFFFFFFFFULL;

uint64_t readHash40(File& file, size_t index) {
  uint8_t bytes[5];
  if (!file.seek(index * 5, SeekSet) || file.read(bytes, sizeof(bytes)) != sizeof(bytes)) return UINT64_MAX;
  uint64_t value = 0;
  for (uint8_t byte : bytes) value = (value << 8) | byte;
  return value;
}
}  // namespace

bool Blocklist::begin(const char* path) {
  if (file_) file_.close();
  file_ = LittleFS.open(path, "r");
  if (!file_ || file_.size() == 0 || file_.size() % 5 != 0) {
    if (file_) file_.close();
    entries_ = 0;
    return false;
  }
  entries_ = file_.size() / 5;
  return true;
}

uint64_t Blocklist::fnv1a40(const String& text) {
  uint64_t hash = 2166136261UL;
  for (size_t i = 0; i < text.length(); ++i) {
    char c = text[i];
    if (c >= 'A' && c <= 'Z') c += 'a' - 'A';
    hash ^= static_cast<uint8_t>(c);
    hash *= 16777619UL;
  }
  return hash & kMask40;
}

bool Blocklist::containsHash(uint64_t hash) const {
  size_t low = 0, high = entries_;
  while (low < high) {
    const size_t middle = low + (high - low) / 2;
    const uint64_t candidate = readHash40(file_, middle);
    if (candidate == hash) return true;
    if (candidate < hash) low = middle + 1;
    else high = middle;
  }
  return false;
}

bool Blocklist::containsDomainOrParent(const String& domain) const {
  if (!ready() || domain.isEmpty()) return false;
  String candidate = domain;
  while (true) {
    if (containsHash(fnv1a40(candidate))) return true;
    const int dot = candidate.indexOf('.');
    if (dot < 0) return false;
    candidate = candidate.substring(dot + 1);
  }
}
