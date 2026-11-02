#pragma once

// Copy this file to config.h. Do not commit config.h.
#define WIFI_SSID "CHANGE_ME"
#define WIFI_PASSWORD "CHANGE_ME"

// Safety default: 0 means the firmware never opens UDP/53. Change to 1 only
// after authorized test devices are configured to use this ESP's IP as DNS.
#define DNS_SERVICE_ENABLED 0

// A plain IP avoids an extra bootstrap DNS dependency.
#define UPSTREAM_DNS_IP "1.1.1.1"
#define UPSTREAM_DNS_PORT 53

// DNS listeners bind to all interfaces. Add one reserved IPv4 address per client.
static const IPAddress ALLOWED_CLIENTS[] = {
	IPAddress(192, 168, 1, 50),
	IPAddress(192, 168, 1, 51),
};
