#include "display.h"

#include <WiFi.h>

namespace {
constexpr int kButtonNext = 0;  // BOOT button; hold at reset still enters bootloader.
constexpr int kButtonAux = 14;
constexpr int kDisplayPower = 15;

constexpr uint16_t color565(uint8_t red, uint8_t green, uint8_t blue) {
  return ((red & 0xF8) << 8) | ((green & 0xFC) << 3) | (blue >> 3);
}

constexpr uint16_t kBackground = color565(11, 17, 22);
constexpr uint16_t kHeader = color565(19, 29, 34);
constexpr uint16_t kSurface = color565(24, 35, 40);
constexpr uint16_t kBorder = color565(48, 65, 69);
constexpr uint16_t kText = color565(235, 243, 238);
constexpr uint16_t kMuted = color565(139, 158, 158);
constexpr uint16_t kMint = color565(81, 220, 163);
constexpr uint16_t kCyan = color565(75, 204, 224);
constexpr uint16_t kAmber = color565(255, 190, 83);
constexpr uint16_t kCoral = color565(255, 119, 108);

void drawHeader(TDisplayS3& lcd, const char* title, bool connected) {
  lcd.fillRect(0, 0, 320, 34, kHeader);
  lcd.fillRect(0, 33, 320, 1, kBorder);
  lcd.fillRoundRect(10, 9, 5, 16, 2, kMint);
  lcd.setTextSize(2);
  lcd.setTextColor(kText);
  lcd.setCursor(23, 8);
  lcd.print(title);

  const uint16_t badge = connected ? color565(25, 59, 48) : color565(65, 43, 39);
  lcd.fillRoundRect(235, 7, 76, 20, 6, badge);
  lcd.fillCircle(247, 17, 3, connected ? kMint : kCoral);
  lcd.setTextSize(1);
  lcd.setTextColor(connected ? kMint : kCoral);
  lcd.setCursor(255, 13);
  lcd.print(connected ? "ONLINE" : "OFFLINE");
}

void drawInfoCard(TDisplayS3& lcd, int x, const char* label, const char* value,
                  const char* detail, uint16_t accent) {
  lcd.fillRoundRect(x, 42, 146, 79, 7, kSurface);
  lcd.drawRoundRect(x, 42, 146, 79, 7, kBorder);
  lcd.fillRect(x, 50, 3, 62, accent);
  lcd.setTextSize(1);
  lcd.setTextColor(kMuted);
  lcd.setCursor(x + 11, 51);
  lcd.print(label);
  lcd.setTextSize(2);
  lcd.setTextColor(accent);
  lcd.setCursor(x + 11, 67);
  lcd.print(value);
  lcd.setTextSize(1);
  lcd.setTextColor(kText);
  lcd.setCursor(x + 11, 101);
  lcd.print(detail);
}

void drawMetricCard(TDisplayS3& lcd, int x, int y, const char* label,
                    uint32_t value, uint16_t accent) {
  lcd.fillRoundRect(x, y, 146, 46, 6, kSurface);
  lcd.drawRoundRect(x, y, 146, 46, 6, kBorder);
  lcd.fillRect(x, y + 7, 3, 32, accent);
  lcd.setTextSize(1);
  lcd.setTextColor(kMuted);
  lcd.setCursor(x + 11, y + 6);
  lcd.print(label);
  lcd.setTextSize(2);
  lcd.setTextColor(accent);
  lcd.setCursor(x + 11, y + 20);
  lcd.printf("%lu", static_cast<unsigned long>(value));
}
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
  lcd_.fillScreen(kBackground);
  const bool connected = WiFi.status() == WL_CONNECTED;
  if (page_ == 0) {
    drawHeader(lcd_, "DNS GUARD", connected);
    const String networkDetail = connected ? WiFi.localIP().toString() : String("Waiting for Wi-Fi");
    char listDetail[32];
    if (blocklistReady) {
      snprintf(listDetail, sizeof(listDetail), "%u domains loaded", unsigned(entries));
    } else {
      snprintf(listDetail, sizeof(listDetail), "BLOCKLIST MISSING");
    }
    drawInfoCard(lcd_, 10, "NETWORK", connected ? "CONNECTED" : "OFFLINE",
                 networkDetail.c_str(), connected ? kMint : kAmber);
    drawInfoCard(lcd_, 164, "DNS SERVICE", dns.enabled() ? "TEST MODE" : "SAFE / OFF",
                 listDetail, dns.enabled() ? kAmber : kCyan);
    lcd_.drawFastHLine(10, 132, 300, kBorder);
    lcd_.setTextSize(1);
    lcd_.setTextColor(kCyan);
    lcd_.setCursor(12, 146);
    lcd_.print("BOOT");
    lcd_.setTextColor(kMuted);
    lcd_.setCursor(48, 146);
    lcd_.print("OPEN STATS");
    lcd_.setCursor(235, 146);
    lcd_.print("T-DISPLAY S3");
  } else {
    drawHeader(lcd_, "DNS STATS", connected);
    drawMetricCard(lcd_, 10, 41, "QUERIES", s.queries, kCyan);
    drawMetricCard(lcd_, 164, 41, "BLOCKED", s.blocked, kMint);
    drawMetricCard(lcd_, 10, 92, "FORWARDED", s.forwarded, kAmber);
    drawMetricCard(lcd_, 164, 92, "REJECTED", s.rejected, kCoral);

    lcd_.drawFastHLine(10, 144, 300, kBorder);
    lcd_.setTextSize(1);
    lcd_.setTextColor(kMuted);
    lcd_.setCursor(12, 149);
    lcd_.print("LAST");
    String lastDomain = s.lastDomain;
    if (lastDomain.isEmpty()) lastDomain = "No queries yet";
    else if (lastDomain.length() > 42) lastDomain = lastDomain.substring(0, 39) + "...";
    lcd_.setTextColor(kText);
    lcd_.setCursor(46, 149);
    lcd_.print(lastDomain);
    lcd_.setTextColor(kCyan);
    lcd_.setCursor(12, 161);
    lcd_.print("BOOT");
    lcd_.setTextColor(kMuted);
    lcd_.setCursor(48, 161);
    lcd_.print("BACK TO HOME");
  }
}
