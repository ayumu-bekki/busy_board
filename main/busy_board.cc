// (C)2024 bekki.jp
// Busy Board

// Include ----------------------
#include "busy_board.h"

#include <cmath>

#include "esp_random.h"
#include "sounds/sounds.h"

BusyBoard::BusyBoard()
    : gpio_watcher_(),
      mcp23017_(),
      i2s_(nullptr),
      seg7_(),
      counter_(0),
      pwm_(),
      power_rate_(0.0f),
      power_rate_min_(0.0f),
      led_guage_(nullptr),
      guage_level_(0.0f),
      guage_level_target_(0.0f),
      is_guage_ok(false),
      is_guage_max(false),
      is_warning_(false),
      warning_counter_(0),
      warning_target_(0),
      is_on_start_fn_(false),
      is_emargency_(false),
      is_on_missile_0_(false),
      is_on_missile_1_(false),
      is_pos0_selector_(false),
      flash_(nullptr),
      status_(STATUS_NORMAL) {
  // GPIO監視
  // InputOnly
  gpio_watcher_.AddMonitor(GpioInputWatchTask::GpioInfo(
      GPIO_EMERGENCY_BUTTON, std::bind(&BusyBoard::OffButtonEmergency, this),
      std::bind(&BusyBoard::OnButtonEmergency, this))); // EmargenctボタンはNC
  gpio_watcher_.AddMonitor(GpioInputWatchTask::GpioInfo(
      GPIO_WARNING_BUTTON, std::bind(&BusyBoard::OnButtonWarning, this),
      nullptr));
  gpio_watcher_.AddMonitor(GpioInputWatchTask::GpioInfo(
      GPIO_START_FN_BUTTON, std::bind(&BusyBoard::OnButtonStartFn, this),
      std::bind(&BusyBoard::OffButtonStartFn, this)));
  gpio_watcher_.AddMonitor(GpioInputWatchTask::GpioInfo(
      GPIO_SELECTER, std::bind(&BusyBoard::OnSelecterPos1, this),
      std::bind(&BusyBoard::OnSelecterPos0, this)));
  // Input And Output
  gpio_watcher_.AddMonitor(GpioInputWatchTask::GpioInfo(
      GPIO_MISSILE_0_SWITCH, std::bind(&BusyBoard::OnButtonMissile0, this),
      std::bind(&BusyBoard::OffButtonMissile0, this)));
  gpio_watcher_.AddMonitor(GpioInputWatchTask::GpioInfo(
      GPIO_MISSILE_1_SWITCH, std::bind(&BusyBoard::OnButtonMissile1, this),
      std::bind(&BusyBoard::OffButtonMissile1, this)));
  gpio_watcher_.AddMonitor(GpioInputWatchTask::GpioInfo(
      GPIO_ARCADE_0_BUTON, std::bind(&BusyBoard::OnButtonArcade0, this),
      nullptr));
  gpio_watcher_.AddMonitor(GpioInputWatchTask::GpioInfo(
      GPIO_ARCADE_1_BUTON, std::bind(&BusyBoard::OnButtonArcade1, this),
      nullptr));
  gpio_watcher_.AddMonitor(GpioInputWatchTask::GpioInfo(
      GPIO_ARCADE_2_BUTON, std::bind(&BusyBoard::OnButtonArcade2, this),
      nullptr));
  gpio_watcher_.AddMonitor(GpioInputWatchTask::GpioInfo(
      GPIO_ARCADE_3_BUTON, std::bind(&BusyBoard::OnButtonArcade3, this),
      nullptr));
  gpio_watcher_.AddMonitor(
      GpioInputWatchTask::GpioInfo(GPIO_ARCADE_4_BUTON,
                                   std::bind(&BusyBoard::OnButtonArcade4, this),
                                   nullptr),
      GpioInputWatchTask::PULL_UP_REGISTOR_ENABLE);
  gpio_watcher_.AddMonitor(GpioInputWatchTask::GpioInfo(
      GPIO_ARCADE_5_BUTON, std::bind(&BusyBoard::OnButtonArcade5, this),
      nullptr));
  gpio_watcher_.Start();

  is_on_start_fn_ =  (gpio_get_level(GPIO_START_FN_BUTTON) == 0);
  is_emargency_ = (gpio_get_level(GPIO_EMERGENCY_BUTTON) != 0); // EmargenctボタンはNC
  is_on_missile_0_ = (gpio_get_level(GPIO_MISSILE_0_SWITCH) == 0);
  is_on_missile_1_ = (gpio_get_level(GPIO_MISSILE_1_SWITCH) == 0);
  is_pos0_selector_ = (gpio_get_level(GPIO_SELECTER) != 0);

  // I2Sサウンド
  i2s_ =
      new I2SSoundMix(GPIO_I2S_BCLK, GPIO_I2S_LRCLK, GPIO_I2S_DATAOUT, 44100u);
  i2s_->Setup();
  i2s_->Start();

  // 7Segディスプレイ
  seg7_.Setup();
  seg7_.DisplayClear();

  // I2C通信
  constexpr i2c_port_t i2c_port_no = I2C_NUM_0;
  I2CUtil::InitializeMaster(i2c_port_no, GPIO_I2C_SDA, GPIO_I2C_SCL);

  constexpr uint8_t address = 0x20;
  mcp23017_.Setup(i2c_port_no, address);
  for (int group = 0; group < MCP23017::GPIO_GROUP_NUM; ++group) {
    for (int gpio_no = 0; gpio_no < MCP23017::GPIO_NUM; ++gpio_no) {
      mcp23017_.SetOutputGpio(group, gpio_no, false, true);
    }
  }

  // Led Guage
  led_guage_ = new LedGuage(&mcp23017_);
  led_guage_->SetGuage(1);

  // Flash
  flash_ = new FlashTask(&mcp23017_);
  flash_->Start();

  // Pwm
  constexpr uint32_t FREQUENCY = 80000;  // 80kHz
  constexpr ledc_timer_t LEDC_TIMER = LEDC_TIMER_0;
  pwm_.Initialize(static_cast<ledc_channel_t>(LEDC_CHANNEL_0), LEDC_TIMER,
                  static_cast<gpio_num_t>(GPIO_CURRENT_METER), FREQUENCY);
  pwm_.SetRate(0.0f);
}

void BusyBoard::Run() {
  while (true) {
    if (status_ == STATUS_NORMAL) {
      if (is_emargency_) {
        seg7_.DisplayDot();
      } else {
        seg7_.DisplayClear();
      }
    } else if (status_ == STATUS_BOOT) {
      pwm_.SetRate(0.0f);
      seg7_.DisplayClear();
      led_guage_->SetGuage(0);
      flash_->SetFlashType(FlashTask::NONE);
      mcp23017_.SetOutputGpio(MCP23017::GPIO_GROUP_A, 1, false);  // Warning
      mcp23017_.SetOutputGpio(MCP23017::GPIO_GROUP_A, 2, false);  // Missile 1
      mcp23017_.SetOutputGpio(MCP23017::GPIO_GROUP_A, 3, false);  // Missile 2
      mcp23017_.SetOutputGpio(MCP23017::GPIO_GROUP_A, 4, false);  // Arcade

      TickType_t lastWakeTime = xTaskGetTickCount();
      vTaskDelayUntil(&lastWakeTime, 200 / portTICK_PERIOD_MS);

      i2s_->Play(I2SSoundMix::SoundInfo(boot2_pcm, boot2_pcm_len));
      pwm_.SetRate(1.0f);
      seg7_.DisplayFull();
      led_guage_->SetGuage(10);
      flash_->SetFlashType(FlashTask::ALWAY_ON);
      mcp23017_.SetOutputGpio(MCP23017::GPIO_GROUP_A, 1, true);  // Warning
      mcp23017_.SetOutputGpio(MCP23017::GPIO_GROUP_A, 2, true);  // Missile 1
      mcp23017_.SetOutputGpio(MCP23017::GPIO_GROUP_A, 3, true);  // Missile 2
      mcp23017_.SetOutputGpio(MCP23017::GPIO_GROUP_A, 4, true);  // Arcade

      lastWakeTime = xTaskGetTickCount();
      vTaskDelayUntil(&lastWakeTime, 2000 / portTICK_PERIOD_MS);

      SwitchStatusRobo();
    } else if (status_ == STATUS_ROBO) {
      if (power_rate_min_ < power_rate_) {
        power_rate_ -= 0.1f;
        if (power_rate_ <= power_rate_min_) {
          power_rate_ = power_rate_min_;
        }
      }
      pwm_.SetRate(std::min(power_rate_, 1.0f));

      if (is_emargency_) {
        seg7_.DisplayDot();
      } else {
        seg7_.SetNumber(counter_ % 10);
      }

      if (guage_level_ < guage_level_target_) {
        guage_level_ += 0.1f;
        if (guage_level_target_ <= guage_level_) {
          guage_level_ = guage_level_target_;
        }
      } else if (guage_level_target_ < guage_level_) {
        guage_level_ -= is_emargency_ ? 0.4 : 0.15f;
        if (guage_level_ <= guage_level_target_) {
          guage_level_ = guage_level_target_;
        }
      }

      if (6.0f <= guage_level_) {
        if (!is_guage_ok) {
          is_guage_ok = true;
          i2s_->Play(I2SSoundMix::SoundInfo(accept2_pcm, accept2_pcm_len));
        }
      } else {
        is_guage_ok = false;
      }

      if (10.0f <= guage_level_) {
        if (!is_guage_max) {
          is_guage_max = true;
          i2s_->Play(I2SSoundMix::SoundInfo(guage_pcm, guage_pcm_len));
        }
      } else {
        is_guage_max = false;
      }

      led_guage_->SetGuage(static_cast<int>(guage_level_));
      mcp23017_.SetOutputGpio(MCP23017::GPIO_GROUP_A, 4,
                        10.0f <= guage_level_);  // Arcade

      if (is_warning_) {
        if (warning_counter_ % 15 == 0) {
          i2s_->Play(I2SSoundMix::SoundInfo(beep2_pcm, beep2_pcm_len));
        }
        mcp23017_.SetOutputGpio(MCP23017::GPIO_GROUP_A, 1,
                          (warning_counter_ % 2 == 0));  // Warning
        ++warning_counter_;
      } else {
        if (warning_target_ == warning_counter_) {
          is_warning_ = true;
          warning_counter_ = 0;
        } else {
          ++warning_counter_;
        }
      }
    }

    TickType_t lastWakeTime = xTaskGetTickCount();
    vTaskDelayUntil(&lastWakeTime, 100 / portTICK_PERIOD_MS);
  }
}

void BusyBoard::SwitchStatusRobo() {
  status_ = STATUS_ROBO;

  power_rate_ = 1.0f;
  power_rate_min_ = 0.2f;
  seg7_.SetNumber(0);
  mcp23017_.SetOutputGpio(MCP23017::GPIO_GROUP_A, 1, false);  // Warning
  mcp23017_.SetOutputGpio(MCP23017::GPIO_GROUP_A, 2, is_on_missile_0_);  // Missile 0
  mcp23017_.SetOutputGpio(MCP23017::GPIO_GROUP_A, 3, is_on_missile_1_);  // Missile 1
  mcp23017_.SetOutputGpio(MCP23017::GPIO_GROUP_A, 4, false);             // Arcade
  flash_->SetFlashType(FlashTask::NONE);
  guage_level_ = 0.0f;
  SetGuegeTarget();
  led_guage_->SetGuage(guage_level_);
  is_warning_ = false;
  SetWarningTarget();
}

void BusyBoard::SetWarningTarget() {
  warning_target_ = esp_random() % 6000 + 3000;
  warning_counter_ = 0;
  ESP_LOGI(TAG, "Next Warning Target : %d", warning_target_);
}

void BusyBoard::SetGuegeTarget() {
  if (status_ == STATUS_ROBO) {
    if (is_emargency_) {  // Emargency
      guage_level_target_ = 2.0f;
    } else if (is_on_missile_1_) {  // MS1 On
      guage_level_target_ = 10.0f;
    } else {
      guage_level_target_ = 7.0f;
    }
  }
}

bool BusyBoard::CommonArcadeButton() {
  if (guage_level_ < 6.0f ) {
    i2s_->Play(I2SSoundMix::SoundInfo(beep_pcm, beep_pcm_len));
    return false;
  }

  power_rate_ = std::min(power_rate_ + 0.4f, 0.7f);
  guage_level_ = std::max(guage_level_ - 1, 6.0f);
  flash_->SetFlashType(FlashTask::FLASH_ONE_TIME);
  ++counter_;
  return true;
}

void BusyBoard::OnButtonEmergency() {
  ESP_LOGI(TAG, "On Emargency");

  is_emargency_ = true;

  if (status_ == STATUS_ROBO) {
    SetGuegeTarget();
    i2s_->Play(I2SSoundMix::SoundInfo(emargency_on_pcm, emargency_on_pcm_len));
  }
}

void BusyBoard::OffButtonEmergency() {
  ESP_LOGI(TAG, "Off Emargency");

  is_emargency_ = false;

  if (status_ == STATUS_ROBO) {
    SetGuegeTarget();
    i2s_->Play(
        I2SSoundMix::SoundInfo(emargency_off_pcm, emargency_off_pcm_len));
  }
}

void BusyBoard::OnButtonWarning() {
  if (status_ == STATUS_NORMAL) {
    if (is_warning_) {
      flash_->SetFlashType(FlashTask::NONE);
      mcp23017_.SetOutputGpio(MCP23017::GPIO_GROUP_A, 1, false);  // Warning
      is_warning_ = false;
    } else {
      flash_->SetFlashType(FlashTask::ALWAY_ON);
      mcp23017_.SetOutputGpio(MCP23017::GPIO_GROUP_A, 1, true);  // Warning
      is_warning_ = true;
    }
  } else if (status_ == STATUS_ROBO) {
    if (is_warning_) {
      SetWarningTarget();
      is_warning_ = false;
      mcp23017_.SetOutputGpio(MCP23017::GPIO_GROUP_A, 1, false);  // Warning
    }
  }

  ESP_LOGI(TAG, "On Warning");
}

void BusyBoard::OnButtonStartFn() {
  ESP_LOGI(TAG, "On StartFn");
  is_on_start_fn_ = true;

  if (status_ == STATUS_NORMAL) {
    // boot可能かチェックする
    if (!is_emargency_ &&     // EmargencyはOFF
        is_on_missile_0_ &&   // MS0はON
        !is_on_missile_1_ &&  // MS1はOFF
        is_pos0_selector_) {  // SelectorはPos0
      // ブート開始
      status_ = STATUS_BOOT;

    } else {
      // Beep音
      seg7_.DisplayE();
      i2s_->Play(I2SSoundMix::SoundInfo(beep_pcm, beep_pcm_len));
      ESP_LOGI(TAG, "STATUS e:%s ms1:%s ms2:%s sel:%s", !is_emargency_ ? "o" : "x",
               is_on_missile_0_ ? "o" : "x",
               !is_on_missile_1_ ? "o" : "x",
               is_pos0_selector_ ? "o" : "x");
    }
  }
}

void BusyBoard::OffButtonStartFn() {
  ESP_LOGI(TAG, "Off StartFn");
  is_on_start_fn_ = false;
}

void BusyBoard::OnSelecterPos0() { 
  ESP_LOGI(TAG, "On Select Pos 0");
  is_pos0_selector_ = true;
}

void BusyBoard::OnSelecterPos1() { 
  ESP_LOGI(TAG, "On Select Pos 1"); 
  is_pos0_selector_ = false;
}

void BusyBoard::OnButtonMissile0() {
  ESP_LOGI(TAG, "On Missile0");
  is_on_missile_0_ = true;
  mcp23017_.SetOutputGpio(MCP23017::GPIO_GROUP_A, 2, true);  // Missile 1

  if (status_ == STATUS_NORMAL) {
    pwm_.SetRate(0.1f);
  }
}

void BusyBoard::OffButtonMissile0() {
  ESP_LOGI(TAG, "Off Missile0");
  is_on_missile_0_ = false;
  mcp23017_.SetOutputGpio(MCP23017::GPIO_GROUP_A, 2, false);  // Missile 1

  if (status_ == STATUS_NORMAL) {
    pwm_.SetRate(0.0f);
  }
}

void BusyBoard::OnButtonMissile1() {
  ESP_LOGI(TAG, "On Missile1");
  is_on_missile_1_ = true;
  mcp23017_.SetOutputGpio(MCP23017::GPIO_GROUP_A, 3, true);  // Missile 2

  SetGuegeTarget();
}

void BusyBoard::OffButtonMissile1() {
  ESP_LOGI(TAG, "Off Missile1");
  is_on_missile_1_ = false;
  mcp23017_.SetOutputGpio(MCP23017::GPIO_GROUP_A, 3, false);  // Missile 2

  SetGuegeTarget();
}

void BusyBoard::OnButtonArcade0() {
  ESP_LOGI(TAG, "On Arcade0");
  if (is_emargency_) {
    return;
  }
  if (status_ == STATUS_NORMAL) {
    if (is_pos0_selector_) {
      i2s_->Play(I2SSoundMix::SoundInfo(cat_pcm, cat_pcm_len));
    } else {
      i2s_->Play(I2SSoundMix::SoundInfo(dram000_pcm, dram000_pcm_len));
    }
  } else if (status_ == STATUS_ROBO) {
    if (CommonArcadeButton()) {
      if (is_pos0_selector_) {
        i2s_->Play(I2SSoundMix::SoundInfo(attack_pcm, attack_pcm_len));
      } else {
        i2s_->Play(I2SSoundMix::SoundInfo(goofy_pcm, goofy_pcm_len));
      }
    }    
  }
}

void BusyBoard::OnButtonArcade1() {
  ESP_LOGI(TAG, "On Arcade1");
  if (is_emargency_) {
    return;
  }
  if (status_ == STATUS_NORMAL) {
    if (is_pos0_selector_) {
      i2s_->Play(I2SSoundMix::SoundInfo(dog_pcm, dog_pcm_len));
    } else {
      i2s_->Play(I2SSoundMix::SoundInfo(dram001_pcm, dram001_pcm_len));
    }
  } else if (status_ == STATUS_ROBO) {
    if (CommonArcadeButton()) {
      if (is_pos0_selector_) {
        i2s_->Play(I2SSoundMix::SoundInfo(attack2_pcm, attack2_pcm_len));
      } else {
        i2s_->Play(I2SSoundMix::SoundInfo(runaway_pcm, runaway_pcm_len));
      }
    }    
  }
}

void BusyBoard::OnButtonArcade2() {
  ESP_LOGI(TAG, "On Arcader2");
  if (is_emargency_) {
    return;
  }
  if (status_ == STATUS_NORMAL) {
    if (is_pos0_selector_) {
      i2s_->Play(I2SSoundMix::SoundInfo(sheep_pcm, sheep_pcm_len));
    } else {
      i2s_->Play(I2SSoundMix::SoundInfo(dram002_pcm, dram002_pcm_len));
    }
  } else if (status_ == STATUS_ROBO) {
    if (CommonArcadeButton()) {
      if (is_pos0_selector_) {
        i2s_->Play(I2SSoundMix::SoundInfo(beem_pcm, beem_pcm_len));
      } else {
        i2s_->Play(I2SSoundMix::SoundInfo(scream_pcm, scream_pcm_len));
      }
    }    
  }
}

void BusyBoard::OnButtonArcade3() {
  ESP_LOGI(TAG, "On Arcader3");
  if (is_emargency_) {
    return;
  }
  if (status_ == STATUS_NORMAL) {
    if (is_pos0_selector_) {
      i2s_->Play(I2SSoundMix::SoundInfo(hourse_pcm, hourse_pcm_len));
    } else {
      i2s_->Play(I2SSoundMix::SoundInfo(dram003_pcm, dram003_pcm_len));
    }
  } else if (status_ == STATUS_ROBO) {
    if (CommonArcadeButton()) {
      if (is_pos0_selector_) {
        i2s_->Play(I2SSoundMix::SoundInfo(beem2_pcm, beem2_pcm_len));
      } else {
        i2s_->Play(I2SSoundMix::SoundInfo(shine_pcm, shine_pcm_len));
      }
    }    
  }
}

void BusyBoard::OnButtonArcade4() {
  ESP_LOGI(TAG, "On Arcader4");
  if (is_emargency_) {
    return;
  }
  if (status_ == STATUS_NORMAL) {
    if (is_pos0_selector_) {
      i2s_->Play(I2SSoundMix::SoundInfo(cuculus_pcm, cuculus_pcm_len));
    } else {
      i2s_->Play(I2SSoundMix::SoundInfo(dram004_pcm, dram004_pcm_len));
    }
  } else if (status_ == STATUS_ROBO) {
    if (CommonArcadeButton()) {
      if (is_pos0_selector_) {
        i2s_->Play(I2SSoundMix::SoundInfo(beem3_pcm, beem3_pcm_len));
      } else {
        i2s_->Play(I2SSoundMix::SoundInfo(supo_pcm, supo_pcm_len));
      }
    }    
  }
}

void BusyBoard::OnButtonArcade5() {
  ESP_LOGI(TAG, "On Arcader5");
  if (is_emargency_) {
    return;
  }
  if (status_ == STATUS_NORMAL) {
    if (is_pos0_selector_) {
      i2s_->Play(I2SSoundMix::SoundInfo(elephant_pcm, elephant_pcm_len));
    } else {
      i2s_->Play(I2SSoundMix::SoundInfo(dram005_pcm, dram005_pcm_len));
    }
  } else if (status_ == STATUS_ROBO) {
    // MS1 + MS2 + Fn同時押し時のみ実行可能
    if (is_on_missile_1_ &&  // MS1 On
        is_on_start_fn_ &&   // Fn On
        10.0f <= guage_level_) {
      i2s_->Play(I2SSoundMix::SoundInfo(bomb_pcm, bomb_pcm_len));
      power_rate_ = 1.3f;
      guage_level_ = 3.0f;
      flash_->SetFlashType(FlashTask::FLASH);
    } else {
      i2s_->Play(I2SSoundMix::SoundInfo(beep_pcm, beep_pcm_len));
    }
  }
}
