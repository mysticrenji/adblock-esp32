#pragma once

#include <WebServer.h>

#include "blocklist.h"
#include "dns_server.h"

class WebPortal {
 public:
  void begin(const DnsServer& dns, const Blocklist& blocklist,
             const char* username, const char* password);
  void loop();

 private:
  bool authorize();
  void sendStatus();

  WebServer server_{80};
  const DnsServer* dns_ = nullptr;
  const Blocklist* blocklist_ = nullptr;
  String username_;
  String password_;
};
