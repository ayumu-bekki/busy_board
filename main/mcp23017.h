#ifndef MCP23017_H_
#define MCP23017_H_

#include <driver/i2c.h>
#include "logger.h"

class MCP23017 {
 public:
  static constexpr uint8_t GPIO_GROUP_A = 0;
  static constexpr uint8_t GPIO_GROUP_B = 1;
  static constexpr uint8_t GPIO_GROUP_NUM = GPIO_GROUP_B + 1;
  static constexpr uint8_t GPIO_MIN = 0;
  static constexpr uint8_t GPIO_MAX = 7;
  static constexpr uint8_t GPIO_NUM = GPIO_MAX + 1;

 private:
  /// GPIOA INPUT OUTPUT DIR (IOCON.BANK = 0)
  static constexpr uint8_t REGADDR_IODIRA = 0x00;
  /// GPIOA INPUT OUTPUT DIR (IOCON.BANK = 0)
  static constexpr uint8_t REGADDR_IODIRB = 0x01;
  /// GPPUA PULLUP (IOCON.BANK = 0)
  static constexpr uint8_t REGADDR_GPPUA = 0x0c;
  /// GPIOB PULLUP (IOCON.BANK = 0)
  static constexpr uint8_t REGADDR_GPPUB = 0x0d;
  // GPIOA (IOCON.BANK = 0)
  static constexpr uint8_t REGADDR_GPIOA = 0x12;
  // GPIOB (IOCON.BANK = 0)
  static constexpr uint8_t REGADDR_GPIOB = 0x13;

  static constexpr uint8_t GPIO_REGADDR_IODIR_TBLE[GPIO_GROUP_NUM] = {
      REGADDR_IODIRA,
      REGADDR_IODIRB,
  };

  static constexpr uint8_t GPIO_REGADDR_GPPU_TBLE[GPIO_GROUP_NUM] = {
      REGADDR_GPPUA,
      REGADDR_GPPUB,
  };

  static constexpr uint8_t GPIO_REGADDR_GPIO_TBL[GPIO_GROUP_NUM] = {
      REGADDR_GPIOA,
      REGADDR_GPIOB,
  };

 public:
  class GPIO {
   public:
    GPIO() : is_input_(false), is_up_(false) {}

    bool is_input_;
    bool is_up_;
  };

  class GPIOGroup {
   public:
    GPIOGroup() : gpio_() {}

    uint8_t GetIsInputData() const {
      return (gpio_[0].is_input_ ? 1 : 0) | (gpio_[1].is_input_ ? 1 : 0) << 1 |
             (gpio_[2].is_input_ ? 1 : 0) << 2 |
             (gpio_[3].is_input_ ? 1 : 0) << 3 |
             (gpio_[4].is_input_ ? 1 : 0) << 4 |
             (gpio_[5].is_input_ ? 1 : 0) << 5 |
             (gpio_[6].is_input_ ? 1 : 0) << 6 |
             (gpio_[7].is_input_ ? 1 : 0) << 7;
    }

    uint8_t GetIsUpData() const {
      return (gpio_[0].is_up_ ? 1 : 0) | (gpio_[1].is_up_ ? 1 : 0) << 1 |
             (gpio_[2].is_up_ ? 1 : 0) << 2 | (gpio_[3].is_up_ ? 1 : 0) << 3 |
             (gpio_[4].is_up_ ? 1 : 0) << 4 | (gpio_[5].is_up_ ? 1 : 0) << 5 |
             (gpio_[6].is_up_ ? 1 : 0) << 6 | (gpio_[7].is_up_ ? 1 : 0) << 7;
    }

    void SetIsUp(uint8_t data) {
      gpio_[0].is_up_ = (data & 0x01) != 0;
      gpio_[1].is_up_ = (data >> 1 & 0x01) != 0;
      gpio_[2].is_up_ = (data >> 2 & 0x01) != 0;
      gpio_[3].is_up_ = (data >> 3 & 0x01) != 0;
      gpio_[4].is_up_ = (data >> 4 & 0x01) != 0;
      gpio_[5].is_up_ = (data >> 5 & 0x01) != 0;
      gpio_[6].is_up_ = (data >> 6 & 0x01) != 0;
      gpio_[7].is_up_ = (data >> 7 & 0x01) != 0;
    }

    GPIO gpio_[GPIO_NUM];
  };

 public:
  MCP23017() : i2c_port_(I2C_NUM_0), address_(0), gpio_group_() {}

  /// Input/Output設定 (Setup関数の前に呼ぶ必要があります)
  void SetInputOutput(uint8_t group_id, uint8_t gpio_no, bool is_input) {
    if (GPIO_GROUP_NUM <= group_id || GPIO_NUM <= gpio_no) {
      return;
    }
    gpio_group_[group_id].gpio_[gpio_no].is_input_ = is_input;
  }

  /// 初期化用関数
  void Setup(i2c_port_t i2c_port, uint8_t address) {
    i2c_port_ = i2c_port;
    address_ = address;

    // Input/Output設定反映
    for (uint8_t group_id = GPIO_GROUP_A; group_id < GPIO_GROUP_NUM;
         ++group_id) {
      i2c_cmd_handle_t cmd = i2c_cmd_link_create();
      i2c_master_start(cmd);

      i2c_master_write_byte(cmd, address_ << 1 | I2C_MASTER_WRITE, true);
      i2c_master_write_byte(cmd, GPIO_REGADDR_IODIR_TBLE[group_id], true);
      i2c_master_write_byte(cmd, gpio_group_[group_id].GetIsInputData(), true);
      // ESP_LOGI(TAG, "SET GPIO DIR group:%d data:%08x",
      //   static_cast<int>(group_id), gpio_group_[group_id].GetIsInputData());

      i2c_master_stop(cmd);

      i2c_master_cmd_begin(i2c_port_, cmd, 1000 / portTICK_PERIOD_MS);
      i2c_cmd_link_delete(cmd);
    }

    // Input内部プルアップ有効化
    for (uint8_t group_id = GPIO_GROUP_A; group_id < GPIO_GROUP_NUM;
         ++group_id) {
      i2c_cmd_handle_t cmd = i2c_cmd_link_create();
      i2c_master_start(cmd);

      i2c_master_write_byte(cmd, address_ << 1 | I2C_MASTER_WRITE, true);
      i2c_master_write_byte(cmd, GPIO_REGADDR_GPPU_TBLE[group_id], true);
      i2c_master_write_byte(cmd, gpio_group_[group_id].GetIsInputData(), true);

      i2c_master_stop(cmd);

      i2c_master_cmd_begin(i2c_port_, cmd, 1000 / portTICK_PERIOD_MS);
      i2c_cmd_link_delete(cmd);
    }
  }

  /// GPIO出力初期化(ALL DOWN)
  void Clear() {
    for (int group = 0; group < GPIO_GROUP_NUM; ++group) {
      for (int gpio_no = 0; gpio_no < GPIO_NUM; ++gpio_no) {
        SetOutputGpio(group, gpio_no, false, true);
      }
    }
  }

  /// GPIO出力設定
  void SetOutputGpio(uint8_t group_id, uint8_t gpio_no, bool is_output,
                     bool is_force = false) {
    if (GPIO_GROUP_NUM <= group_id || GPIO_NUM <= gpio_no) {
      return;
    }

    const bool output = gpio_group_[group_id].gpio_[gpio_no].is_up_;
    if (output == is_output && !is_force) {
      return;
    }

    // ESP_LOGI(TAG, "Set GPIO group:%d gpio_no:%d output:%s",
    // static_cast<int>(group_id), static_cast<int>(gpio_no), is_output ? "1" :
    // "0");
    gpio_group_[group_id].gpio_[gpio_no].is_up_ = is_output;

    {
      i2c_cmd_handle_t cmd = i2c_cmd_link_create();
      i2c_master_start(cmd);

      i2c_master_write_byte(cmd, address_ << 1 | I2C_MASTER_WRITE, true);
      i2c_master_write_byte(cmd, GPIO_REGADDR_GPIO_TBL[group_id], true);
      i2c_master_write_byte(cmd, gpio_group_[group_id].GetIsUpData(), true);
      // ESP_LOGI(TAG, "SET GPIO group:%d data:%08x",
      // static_cast<int>(group_id), gpio_group_[group_id].GetIsUpData());

      i2c_master_stop(cmd);

      i2c_master_cmd_begin(i2c_port_, cmd, 1000 / portTICK_PERIOD_MS);
      i2c_cmd_link_delete(cmd);
    }
  }

  /// GPIO入力状況取得 (true:High)
  bool GetInputGpio(uint8_t group_id, uint8_t gpio_no) {
    uint8_t buffer = 0;

    {
      i2c_cmd_handle_t cmd = i2c_cmd_link_create();
      i2c_master_start(cmd);

      i2c_master_write_byte(cmd, address_ << 1 | I2C_MASTER_WRITE, true);
      i2c_master_write_byte(cmd, GPIO_REGADDR_GPIO_TBL[group_id], true);

      i2c_master_start(cmd);

      i2c_master_write_byte(cmd, address_ << 1 | I2C_MASTER_READ, true);
      i2c_master_read_byte(cmd, &buffer, I2C_MASTER_NACK);

      i2c_master_stop(cmd);

      i2c_master_cmd_begin(i2c_port_, cmd, 1000 / portTICK_PERIOD_MS);
      i2c_cmd_link_delete(cmd);
    }

    // ESP_LOGI(TAG, "GET GPIO group:%d data:%08x",
    //     static_cast<int>(group_id), buffer);

    gpio_group_[group_id].SetIsUp(buffer);

    return gpio_group_[group_id].gpio_[gpio_no].is_up_ != 0;
  }

 private:
  i2c_port_t i2c_port_;
  uint8_t address_;
  GPIOGroup gpio_group_[GPIO_GROUP_NUM];
};

#endif  // MCP23017_H_

// EOF
