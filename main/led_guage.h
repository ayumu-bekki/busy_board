#ifndef LED_GUAGE_H_
#define LED_GUAGE_H_
// (C)2024 bekki.jp
// Busy Board

// Include ----------------------

#include <cstdint>
#include "mcp23017.h"

//  B0 Guage 1
//  B1 Guage 2
//  B2 Guage 3
//  B3 Guage 4
//  B4 Guage 5
//  B5 Guage 6
//  B6 Guage 7
//  B7 Guage 8
//  A7 Guage 9
//  A6 Guage 10 

class LedGuage
{
 private:
  static constexpr uint8_t GPIO_TABLE[10][2] = {
    {MCP23017::GPIO_GROUP_B, 7},
    {MCP23017::GPIO_GROUP_B, 6},
    {MCP23017::GPIO_GROUP_B, 5},
    {MCP23017::GPIO_GROUP_B, 4},
    {MCP23017::GPIO_GROUP_B, 3},
    {MCP23017::GPIO_GROUP_B, 2},
    {MCP23017::GPIO_GROUP_B, 1},
    {MCP23017::GPIO_GROUP_B, 0},
    {MCP23017::GPIO_GROUP_A, 7},
    {MCP23017::GPIO_GROUP_A, 6},
  };

 public:
  explicit LedGuage(MCP23017 *mcp23017)
   :mcp23017_(mcp23017)
   ,level_(-1)
  {
    SetGuage(0);
  }

  void SetGuage(const int level) {
    if (level == level_) { 
      return;
    }

    if (mcp23017_ == nullptr) {
      ESP_LOGW(TAG, "MCP23017 is null");
      return;
    }

    for (int i = 0; i < 10; ++i) {
      const bool is_enable = i < level;
      mcp23017_->SetOutputGpio(GPIO_TABLE[i][0], GPIO_TABLE[i][1], is_enable, true);
      //ESP_LOGI(TAG, "LED Guage Lv:%d group:%d io:%d status:%s", i, GPIO_TABLE[i][0], GPIO_TABLE[i][1], is_enable ? "O" : "X");
    }
    level_ = level;
  }

 private:
  MCP23017 *mcp23017_;
  int level_;
};

#endif // LED_GUAGE_H_

// EOF
