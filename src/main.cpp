#include <Arduino.h>
#include <LittleFS.h>
#include <WiFi.h>

#include "config.h"
#include "blocklist.h"
#include "display.h"
#include "dns_server.h"
#include "web_portal.h"

// Preserve compatibility with existing private config.h files.
#ifndef WEB_PORTAL_USERNAME
#define WEB_PORTAL_USERNAME "admin"
#endif
#ifndef WEB_PORTAL_PASSWORD
#define WEB_PORTAL_PASSWORD ""
#endif

namespace {
Blocklist blocklist;
DnsServer dns;
Dashboard dashboard;
WebPortal portal;

IPAddress parseIp(const char* value) {
  IPAddress result;
  if (!result.fromString(value)) Serial.printf("Invalid IPv4 configuration: %s\n", value);
  return result;
}
}

void setup() {
  Serial.begin(115200);
  delay(300);
  dashboard.begin();
  if (!LittleFS.begin(true)) Serial.println("LittleFS mount failed");
  const bool listReady = blocklist.begin();
  Serial.printf("Blocklist: %s (%u entries)\n", listReady ? "ready" : "missing/invalid", unsigned(blocklist.entries()));

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.printf("Connecting to %s", WIFI_SSID);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print('.'); }
  Serial.printf("\nIP: %s\n", WiFi.localIP().toString().c_str());

  const size_t allowedClientCount = sizeof(ALLOWED_CLIENTS) / sizeof(ALLOWED_CLIENTS[0]);
  const bool started = dns.begin(parseIp(UPSTREAM_DNS_IP), ALLOWED_CLIENTS, allowedClientCount,
                                 DNS_SERVICE_ENABLED == 1, &blocklist);
  Serial.printf("DNS service: %s\n", started && DNS_SERVICE_ENABLED ? "test mode enabled" : "off or failed");
  portal.begin(dns, blocklist, WEB_PORTAL_USERNAME, WEB_PORTAL_PASSWORD);
}

void loop() {
  dns.loop();
  portal.loop();
  dashboard.update(dns, blocklist.ready(), blocklist.entries());
}
