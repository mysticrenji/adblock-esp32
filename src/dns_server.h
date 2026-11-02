#pragma once

#include <Arduino.h>
#include <WiFiUdp.h>

#include "blocklist.h"

struct DnsStats {
  uint32_t queries = 0;
  uint32_t blocked = 0;
  uint32_t forwarded = 0;
  uint32_t rejected = 0;
  String lastDomain;
};

class DnsServer {
 public:
  bool begin(const IPAddress& upstream, const IPAddress* allowedClients,
             size_t allowedClientCount, bool enabled, Blocklist* blocklist);
  void loop();
  const DnsStats& stats() const { return stats_; }
  bool enabled() const { return enabled_; }

 private:
  bool parseQuestion(const uint8_t* packet, size_t length, String* name, size_t* questionEnd, uint16_t* type) const;
  void sendBlocked(const uint8_t* query, size_t questionEnd, uint16_t queryType);
  void forward(const uint8_t* query, size_t length);

  WiFiUDP listener_;
  WiFiUDP upstreamSocket_;
  IPAddress upstream_;
  const IPAddress* allowedClients_ = nullptr;
  size_t allowedClientCount_ = 0;
  Blocklist* blocklist_ = nullptr;
  bool enabled_ = false;
  DnsStats stats_;
};
