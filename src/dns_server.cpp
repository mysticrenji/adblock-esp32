#include "dns_server.h"

namespace {
constexpr uint16_t kDnsPort = 53;
constexpr size_t kMaxDnsPacket = 512;

uint16_t read16(const uint8_t* p) { return (uint16_t(p[0]) << 8) | p[1]; }
void write16(uint8_t* p, uint16_t value) { p[0] = value >> 8; p[1] = value & 0xff; }
void write32(uint8_t* p, uint32_t value) { p[0] = value >> 24; p[1] = value >> 16; p[2] = value >> 8; p[3] = value; }
}  // namespace

bool DnsServer::begin(const IPAddress& upstream, const IPAddress* allowedClients,
                      size_t allowedClientCount, bool enabled, Blocklist* blocklist) {
  upstream_ = upstream;
  allowedClients_ = allowedClients;
  allowedClientCount_ = allowedClientCount;
  blocklist_ = blocklist;
  enabled_ = enabled;
  if (!enabled_) return true;
  upstreamSocket_.begin(0);
  return listener_.begin(kDnsPort) == 1;
}

bool DnsServer::parseQuestion(const uint8_t* packet, size_t length, String* name, size_t* questionEnd, uint16_t* type) const {
  if (length < 17 || read16(packet + 4) != 1) return false;
  size_t offset = 12;
  name->remove(0);
  while (offset < length) {
    const uint8_t labelLength = packet[offset++];
    if (labelLength == 0) break;
    // Compression in a question is not needed for ordinary client requests;
    // refusing it avoids pointer loops and malformed packets.
    if ((labelLength & 0xc0) || labelLength > 63 || offset + labelLength > length) return false;
    if (!name->isEmpty()) *name += '.';
    for (uint8_t i = 0; i < labelLength; ++i) *name += char(packet[offset++]);
    if (name->length() > 253) return false;
  }
  if (offset + 4 > length || name->isEmpty()) return false;
  *type = read16(packet + offset);
  *questionEnd = offset + 4;
  return true;
}

void DnsServer::sendBlocked(const uint8_t* query, size_t questionEnd, uint16_t queryType) {
  // Return a syntactically valid zero address only for A/AAAA. Other records
  // are forwarded so this minimal sinkhole never invents record semantics.
  const bool ipv6 = queryType == 28;
  const size_t answerLength = 2 + 2 + 2 + 4 + 2 + (ipv6 ? 16 : 4);
  uint8_t response[kMaxDnsPacket] = {};
  if (questionEnd + answerLength > sizeof(response)) return;
  memcpy(response, query, questionEnd);
  response[2] = uint8_t((response[2] | 0x80) & 0xfe);  // QR=1, clear TC
  response[3] = uint8_t((response[3] | 0x80) & 0xf0);  // RA=1, RCODE=0
  write16(response + 6, 1);                             // one answer
  write16(response + 8, 0);
  write16(response + 10, 0);
  size_t offset = questionEnd;
  response[offset++] = 0xc0; response[offset++] = 0x0c;
  write16(response + offset, queryType); offset += 2;
  write16(response + offset, 1); offset += 2;
  write32(response + offset, 60); offset += 4;
  write16(response + offset, ipv6 ? 16 : 4); offset += 2;
  memset(response + offset, 0, ipv6 ? 16 : 4); offset += ipv6 ? 16 : 4;
  listener_.beginPacket(listener_.remoteIP(), listener_.remotePort());
  listener_.write(response, offset);
  listener_.endPacket();
}

void DnsServer::forward(const uint8_t* query, size_t length) {
  upstreamSocket_.beginPacket(upstream_, kDnsPort);
  upstreamSocket_.write(query, length);
  upstreamSocket_.endPacket();
  const uint32_t deadline = millis() + 1500;
  while (int32_t(millis() - deadline) < 0) {
    const int replyLength = upstreamSocket_.parsePacket();
    if (replyLength <= 0) { delay(1); continue; }
    uint8_t reply[kMaxDnsPacket];
    const size_t bytes = upstreamSocket_.read(reply, min(replyLength, int(sizeof(reply))));
    listener_.beginPacket(listener_.remoteIP(), listener_.remotePort());
    listener_.write(reply, bytes);
    listener_.endPacket();
    ++stats_.forwarded;
    return;
  }
}

void DnsServer::loop() {
  if (!enabled_) return;
  const int length = listener_.parsePacket();
  if (length <= 0) return;
  uint8_t packet[kMaxDnsPacket];
  const size_t bytes = listener_.read(packet, min(length, int(sizeof(packet))));
  const IPAddress client = listener_.remoteIP();
  bool allowed = false;
  for (size_t i = 0; i < allowedClientCount_; ++i) {
    if (client == allowedClients_[i]) { allowed = true; break; }
  }
  if (!allowed) { ++stats_.rejected; return; }
  String domain;
  size_t questionEnd = 0;
  uint16_t type = 0;
  if (!parseQuestion(packet, bytes, &domain, &questionEnd, &type)) { ++stats_.rejected; return; }
  ++stats_.queries;
  stats_.lastDomain = domain;
  if ((type == 1 || type == 28) && blocklist_ && blocklist_->containsDomainOrParent(domain)) {
    sendBlocked(packet, questionEnd, type);
    ++stats_.blocked;
  } else {
    forward(packet, bytes);
  }
}
