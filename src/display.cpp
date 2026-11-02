#include "display.h"

#include <WiFi.h>

namespace {
constexpr int kButtonNext = 0;  // BOOT button; hold at reset still enters bootloader.
constexpr int kButtonAux = 14;
constexpr int kDisplayPower = 15;
}

TDisplayS3::TDisplayS3() {
  // LilyGO T-Display-S3: ST7789 on an 8-bit parallel bus.
  {
    auto cfg = bus_.config();
    cfg.freq_write = 20000000;
    cfg.pin_wr = 8;
    cfg.pin_rd = 9;
    cfg.pin_rs = 7;
    cfg.pin_d0 = 39;
    cfg.pin_d1 = 40;
    cfg.pin_d2 = 41;
    cfg.pin_d3 = 42;
    cfg.pin_d4 = 45;
    cfg.pin_d5 = 46;
    cfg.pin_d6 = 47;
    cfg.pin_d7 = 48;
    bus_.config(cfg);
    panel_.setBus(&bus_);
  }
  {
    auto cfg = panel_.config();
    cfg.pin_cs = 6;
    cfg.pin_rst = 5;
    cfg.pin_busy = -1;
    cfg.panel_width = 170;
    cfg.panel_height = 320;
    cfg.memory_width = 240;
    cfg.memory_height = 320;
    cfg.offset_x = 35;
    cfg.offset_y = 0;
    cfg.readable = false;
    cfg.invert = true;
    cfg.rgb_order = false;
    cfg.dlen_16bit = false;
    panel_.config(cfg);
  }
  {
    auto cfg = backlight_.config();
    cfg.pin_bl = 38;
    cfg.invert = false;
    cfg.freq = 44100;
    cfg.pwm_channel = 7;
    backlight_.config(cfg);
    panel_.setLight(&backlight_);
  }
  setPanel(&panel_);
}

void Dashboard::begin() {
  pinMode(kButtonNext, INPUT_PULLUP);
  pinMode(kButtonAux, INPUT_PULLUP);
  // Enable the LCD power rail before resetting/initialising the panel.
  pinMode(kDisplayPower, OUTPUT);
  digitalWrite(kDisplayPower, HIGH);
  lcd_.init(); lcd_.setRotation(1); lcd_.setBrightness(128); lcd_.fillScreen(TFT_BLACK);
}

void Dashboard::update(const DnsServer& dns, bool blocklistReady, size_t entries) {
  const bool button = digitalRead(kButtonNext);
  if (lastButton_ && !button) page_ = (page_ + 1) % 2;
  lastButton_ = button;
  if (millis() - lastDraw_ < 500) return;
  lastDraw_ = millis();
  const DnsStats& s = dns.stats();
  lcd_.fillScreen(TFT_BLACK); lcd_.setTextColor(TFT_WHITE, TFT_BLACK); lcd_.setTextSize(2); lcd_.setCursor(8, 8);
  if (page_ == 0) {
    lcd_.println("DNS Adblock"); lcd_.setTextSize(1); lcd_.printf("WiFi: %s\n", WiFi.status() == WL_CONNECTED ? "connected" : "connecting");
    lcd_.printf("IP: %s\n", WiFi.localIP().toString().c_str()); lcd_.printf("DNS: %s\n", dns.enabled() ? "TEST MODE" : "OFF (safe default)");
    lcd_.printf("List: %s (%u)\n", blocklistReady ? "ready" : "missing", unsigned(entries));
    lcd_.println("BOOT: stats");
  } else {
    lcd_.println("DNS stats"); lcd_.setTextSize(1); lcd_.printf("queries:  %lu\n", (unsigned long)s.queries);
    lcd_.printf("blocked:  %lu\n", (unsigned long)s.blocked); lcd_.printf("forwarded:%lu\n", (unsigned long)s.forwarded);
    lcd_.printf("rejected: %lu\n", (unsigned long)s.rejected); lcd_.print("last: "); lcd_.println(s.lastDomain);
    lcd_.println("BOOT: home");
  }
}
