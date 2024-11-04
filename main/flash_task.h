#ifndef FLASH_TASK_H_
#define FLASH_TASK_H_
// (C)2024 bekki.jp
// Flash task

// Include ----------------------
#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>

#include "mcp23017.h"
#include "task.h"

class FlashTask final : public Task {
 public:
  static constexpr std::string_view TASK_NAME = "FlashTask";
  static constexpr int32_t PRIORITY = Task::PRIORITY_LOW;
  static constexpr int32_t CORE_ID = PRO_CPU_NUM;

 public:
  enum FlashType {
    NONE,
    ALWAY_ON,
    FLASH_ONE_TIME,
    FLASH,
  };

 public:
  explicit FlashTask(MCP23017 *mcp23017)
      : Task(std::string(TASK_NAME).c_str(), PRIORITY, CORE_ID),
        mcp23017_(mcp23017),
        flash_type_(NONE),
        counter_(0) {}

  ~FlashTask() {}

  void Initialize() override {
    flash_type_ = NONE;
    counter_ = 0;
  }

  void Update() override {
    if (flash_type_ == NONE) {
      SetFlash(false);
    } else if (flash_type_ == ALWAY_ON) {
      SetFlash(true);
    } else if (flash_type_ == FLASH_ONE_TIME) {
      SetFlash(true);
      flash_type_ = NONE;
    } else if (flash_type_ == FLASH) {
      SetFlash(counter_ % 2 == 0);
      if (30 <= counter_) {
        flash_type_ = NONE;
      }
      ++counter_;
    }

    TickType_t lastWakeTime = xTaskGetTickCount();
    vTaskDelayUntil(&lastWakeTime, 50 / portTICK_PERIOD_MS);
  }

  void SetFlashType(FlashType flash_type) {
    counter_ = 0;
    flash_type_ = flash_type;
  }

 private:
  void SetFlash(bool is_on) {
    mcp23017_->SetOutputGpio(MCP23017::GPIO_GROUP_A, 0, is_on);  // Flash LED
  }

 private:
  MCP23017 *mcp23017_;
  FlashType flash_type_;
  int counter_;
};

#endif  // FLASH_TASK_H_

// EOF
