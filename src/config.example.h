#pragma once

// Copy this file to config.h. Do not commit config.h.
#define WIFI_SSID "CHANGE_ME"
#define WIFI_PASSWORD "CHANGE_ME"

// Safety default: 0 means the firmware never opens UDP/53. Change to 1 only
// after a single test device has been configured to use this ESP's IP as DNS.
#define DNS_SERVICE_ENABLED 0

// A plain IP avoids an extra bootstrap DNS dependency.
#define UPSTREAM_DNS_IP "1.1.1.1"
#define UPSTREAM_DNS_PORT 53

// DNS listeners bind to all interfaces on an ESP32. This allow-list stops
// accidental use by other LAN clients; use a test device's IPv4 address.
#define ALLOWED_CLIENT_IP "192.168.1.50"
