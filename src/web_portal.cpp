#include "web_portal.h"

#include <WiFi.h>
#include <esp_system.h>

#include "web_portal_page.h"

namespace {
String jsonString(const String& value) {
  String result = "\"";
  for (size_t i = 0; i < value.length(); ++i) {
    const uint8_t c = value[i];
    if (c == '"' || c == '\\') { result += '\\'; result += char(c); }
    else if (c < 0x20 || c >= 0x7f) {
      char escaped[7];
      snprintf(escaped, sizeof(escaped), "\\u%04x", c);
      result += escaped;
    } else result += char(c);
  }
  return result + '"';
}
}

void WebPortal::begin(const DnsServer& dns, const Blocklist& blocklist,
                      const char* username, const char* password) {
  dns_ = &dns;
  blocklist_ = &blocklist;
  username_ = username;
  password_ = password;
  if (username_.isEmpty()) username_ = "admin";
  if (password_.isEmpty()) {
    // Wi-Fi is running, so the ESP hardware RNG has its entropy source.
    uint8_t random[16];
    esp_fill_random(random, sizeof(random));
    for (uint8_t byte : random) {
      char hex[3];
      snprintf(hex, sizeof(hex), "%02x", byte);
      password_ += hex;
    }
    Serial.printf("Portal temporary login: %s / %s (changes on reboot)\n",
                  username_.c_str(), password_.c_str());
  }
  server_.on("/", HTTP_GET, [this]() {
    if (authorize()) server_.send_P(200, "text/html; charset=utf-8", kPortalPage);
  });
  server_.on("/api/status", HTTP_GET, [this]() {
    if (authorize()) sendStatus();
  });
  server_.onNotFound([this]() {
    if (authorize()) server_.send(404, "text/plain", "Not found");
  });
  server_.begin();
  Serial.printf("Portal: http://%s/ (user: %s)\n",
                WiFi.localIP().toString().c_str(), username_.c_str());
}

bool WebPortal::authorize() {
  server_.sendHeader("Cache-Control", "no-store");
  server_.sendHeader("X-Content-Type-Options", "nosniff");
  server_.sendHeader("X-Frame-Options", "DENY");
  server_.sendHeader("Referrer-Policy", "no-referrer");
  server_.sendHeader("Content-Security-Policy",
      "default-src 'none'; style-src 'unsafe-inline'; script-src 'unsafe-inline'; "
      "connect-src 'self'; frame-ancestors 'none'; base-uri 'none'; form-action 'none'");
  // WebServer also accepts Basic by default; require Digest explicitly.
  if (server_.header("Authorization").startsWith("Digest ") &&
      server_.authenticate(username_.c_str(), password_.c_str())) return true;
  server_.requestAuthentication(DIGEST_AUTH, "ESP32 Adblock", "Login required");
  return false;
}

void WebPortal::sendStatus() {
  const DnsStats& stats = dns_->stats();
  String body;
  body.reserve(640);
  body = "{\"ip\":" + jsonString(WiFi.localIP().toString());
  body += ",\"wifi_connected\":"; body += WiFi.status() == WL_CONNECTED ? "true" : "false";
  body += ",\"rssi\":" + String(WiFi.RSSI());
  body += ",\"uptime_seconds\":" + String(millis() / 1000);
  body += ",\"free_heap\":" + String(ESP.getFreeHeap());
  body += ",\"dns_enabled\":"; body += dns_->enabled() ? "true" : "false";
  body += ",\"blocklist_ready\":"; body += blocklist_->ready() ? "true" : "false";
  body += ",\"blocklist_entries\":" + String(unsigned(blocklist_->entries()));
  body += ",\"queries\":" + String(stats.queries);
  body += ",\"blocked\":" + String(stats.blocked);
  body += ",\"forwarded\":" + String(stats.forwarded);
  body += ",\"rejected\":" + String(stats.rejected);
  body += ",\"last_domain\":" + jsonString(stats.lastDomain) + '}';
  server_.send(200, "application/json", body);
}

void WebPortal::loop() { server_.handleClient(); }
