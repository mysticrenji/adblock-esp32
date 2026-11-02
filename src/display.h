#pragma once

#include <LovyanGFX.hpp>

#include "dns_server.h"

class TDisplayS3 : public lgfx::LGFX_Device {
 public:
  TDisplayS3();
 private:
  lgfx::Bus_Parallel8 bus_;
  lgfx::Panel_ST7789 panel_;
  lgfx::Light_PWM backlight_;
};

class Dashboard {
 public:
  void begin();
  void update(const DnsServer& dns, bool blocklistReady, size_t entries);
 private:
  TDisplayS3 lcd_;
  lgfx::LGFX_Sprite frame_{&lcd_};
  bool frameReady_ = false;
  uint8_t page_ = 0;
  uint32_t lastDraw_ = 0;
  bool lastButton_ = true;
};
