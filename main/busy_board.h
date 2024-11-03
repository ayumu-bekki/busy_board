#ifndef BUSY_BOARD_H_
#define BUSY_BOARD_H_
// (C)2024 bekki.jp
// Busy Board

// Include ----------------------
#include <driver/gpio.h>
#include <driver/i2c.h>
#include <driver/ledc.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <cmath>
#include <memory>

#include "flash_task.h"
#include "gpio_input_watch_task.h"
#include "i2c_util.h"
#include "i2s_sound.h"
#include "led_guage.h"
#include "logger.h"
#include "mcp23017.h"
#include "pwm.h"
#include "seg7_led.h"

class BusyBoard {
 private:
  enum BOARD_STATUS {
    STATUS_NORMAL,
    STATUS_BOOT,
    STATUS_ROBO,
  };

  // GPIO Ports
  static constexpr gpio_num_t GPIO_EMERGENCY_BUTTON = GPIO_NUM_36;
  static constexpr gpio_num_t GPIO_WARNING_BUTTON = GPIO_NUM_39;
  static constexpr gpio_num_t GPIO_START_FN_BUTTON = GPIO_NUM_34;
  static constexpr gpio_num_t GPIO_SELECTER = GPIO_NUM_35;
  static constexpr gpio_num_t GPIO_MISSILE_0_SWITCH = GPIO_NUM_32;
  static constexpr gpio_num_t GPIO_MISSILE_1_SWITCH = GPIO_NUM_33;
  static constexpr gpio_num_t GPIO_ARCADE_0_BUTON = GPIO_NUM_25;
  static constexpr gpio_num_t GPIO_ARCADE_1_BUTON = GPIO_NUM_26;
  static constexpr gpio_num_t GPIO_ARCADE_2_BUTON = GPIO_NUM_27;
  static constexpr gpio_num_t GPIO_ARCADE_3_BUTON = GPIO_NUM_14;
  static constexpr gpio_num_t GPIO_ARCADE_4_BUTON =
      GPIO_NUM_12;  // GPIO12 is Default Pulldown
  static constexpr gpio_num_t GPIO_ARCADE_5_BUTON = GPIO_NUM_13;
  static constexpr gpio_num_t GPIO_I2C_SDA = GPIO_NUM_17;
  static constexpr gpio_num_t GPIO_I2C_SCL = GPIO_NUM_16;
  static constexpr gpio_num_t GPIO_CURRENT_METER = GPIO_NUM_4;
  static constexpr gpio_num_t GPIO_I2S_BCLK = GPIO_NUM_5;
  static constexpr gpio_num_t GPIO_I2S_LRCLK = GPIO_NUM_18;
  static constexpr gpio_num_t GPIO_I2S_DATAOUT = GPIO_NUM_19;

  // MCP23017
  //  A0 Flash LED
  //  A1 Warning
  //  A2 Missile Switch LED 1
  //  A3 Missile Switch LED 2
  //  A4 Arcaede 0 LED
  //  A5
  //  A6 Guage 10
  //  A7 Guage 9
  //  B0 Guage 1
  //  B1 Guage 2
  //  B2 Guage 3
  //  B3 Guage 4
  //  B4 Guage 5
  //  B5 Guage 6
  //  B6 Guage 7
  //  B7 Guage 8

 public:
  BusyBoard();
  ~BusyBoard() {}

  void Run();

 private:
  void SwitchStatusRobo();
  void SetWarningTarget();
  void SetGuegeTarget();
  bool CommonArcadeButton();

  void OnButtonEmergency();
  void OffButtonEmergency();
  void OnButtonWarning();
  void OnButtonStartFn();
  void OffButtonStartFn();
  void OnSelecterPos0();
  void OnSelecterPos1();
  void OnButtonMissile0();
  void OffButtonMissile0();
  void OnButtonMissile1();
  void OffButtonMissile1();
  void OnButtonArcade0();
  void OnButtonArcade1();
  void OnButtonArcade2();
  void OnButtonArcade3();
  void OnButtonArcade4();
  void OnButtonArcade5();

 private:
  GpioInputWatchTask gpio_watcher_;
  MCP23017 mcp23017_;
  I2SSoundMix *i2s_;

  Seg7Led seg7_;
  int counter_;

  Pwm pwm_;
  float power_rate_;
  float power_rate_min_;

  LedGuage *led_guage_;
  float guage_level_;
  float guage_level_target_;
  bool is_guage_ok;
  bool is_guage_max;

  bool is_warning_;
  int warning_counter_;
  int warning_target_;

  bool is_on_start_fn_;
  bool is_emargency_;
  bool is_on_missile_0_;
  bool is_on_missile_1_;
  bool is_pos0_selector_;

  FlashTask *flash_;

  BOARD_STATUS status_;
};

#endif  // BUSY_BOARD_H_

// EOF
