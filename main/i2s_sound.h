#ifndef I2S_SOUND_H_
#define I2S_SOUND_H_
// (C)2024 bekki.jp
// I2S Sound

// Include ----------------------
#include <driver/i2s_std.h>
#include <freertos/FreeRTOS.h>

#include <cstring>
#include <memory>
#include <mutex>
#include <thread>
#include <list>

#include "task.h"

class I2SSoundMix final : public Task {
 public:
  static constexpr std::string_view TASK_NAME = "SoundMixTask";
  static constexpr int32_t PRIORITY = Task::PRIORITY_HIGH;
  static constexpr int32_t CORE_ID = APP_CPU_NUM;

  static constexpr size_t I2S_DMA_SAMPLING = 480;
  static constexpr size_t I2S_DMA_BUFFER_LEN = I2S_DMA_SAMPLING * 2;
  static constexpr float VOLUME_DEFAULT = 0.8f;

 public:
  struct SoundInfo {
   public:
    SoundInfo(const uint8_t *const data, size_t length)
        : cur(0), data(data), length(length) {}

    int cur;
    const uint8_t *data;
    size_t length;
  };

 public:
  I2SSoundMix(gpio_num_t bclk_no, gpio_num_t lrclk_no, gpio_num_t dataout_no,
              uint32_t sample_rate_hz)
      : Task(std::string(TASK_NAME).c_str(), PRIORITY, CORE_ID),
        bclk_no_(bclk_no),
        lrclk_no_(lrclk_no),
        dataout_no_(dataout_no),
        sample_rate_hz_(sample_rate_hz),
        i2s_handle_(nullptr),
        sound_buffer_(nullptr),
        sound_list_(),
        volume_(VOLUME_DEFAULT),
        mtx_() {
    sound_buffer_ = new uint8_t[I2S_DMA_BUFFER_LEN];
  }

  ~I2SSoundMix() {
    Disable();
    i2s_del_channel(i2s_handle_);
  }

  void Setup() {
    i2s_chan_config_t chan_cfg = {
        .id = I2S_NUM_AUTO,
        .role = I2S_ROLE_MASTER,
        .dma_desc_num = 6,
        .dma_frame_num = 240,
        .auto_clear = false,
        .intr_priority = 0,
    };

    i2s_new_channel(&chan_cfg, &i2s_handle_, nullptr);

    i2s_std_config_t std_cfg = {
        .clk_cfg =
            {
                .sample_rate_hz = sample_rate_hz_,
                .clk_src = I2S_CLK_SRC_DEFAULT,
                .mclk_multiple = I2S_MCLK_MULTIPLE_256,
            },
        .slot_cfg =
            {
                .data_bit_width = I2S_DATA_BIT_WIDTH_16BIT,
                .slot_bit_width = I2S_SLOT_BIT_WIDTH_AUTO,
                .slot_mode = I2S_SLOT_MODE_MONO,
                .slot_mask = I2S_STD_SLOT_LEFT,
                .ws_width = I2S_DATA_BIT_WIDTH_16BIT,
                .ws_pol = false,
                .bit_shift = true,
                .msb_right = true,
            },
        .gpio_cfg =
            {
                .mclk = I2S_GPIO_UNUSED,
                .bclk = bclk_no_,
                .ws = lrclk_no_,
                .dout = dataout_no_,
                .din = I2S_GPIO_UNUSED,
                .invert_flags =
                    {
                        .mclk_inv = false,
                        .bclk_inv = false,
                        .ws_inv = false,
                    },
            },
    };
    i2s_channel_init_std_mode(i2s_handle_, &std_cfg);

    Enable();
  }

  void Update() override {
    std::memset(sound_buffer_, 0, I2S_DMA_BUFFER_LEN);

    {
      std::lock_guard<std::mutex> lock(mtx_);
      for (std::list<SoundInfo>::iterator itr = sound_list_.begin();
           itr != sound_list_.end();) {
        for (size_t frame = 0; frame < I2S_DMA_SAMPLING; ++frame) {
          if (itr->length <= itr->cur) {
            break;
          }

          int16_t base = sound_buffer_[frame * 2]
                       | sound_buffer_[frame * 2 + 1] << 8;
  
          const int16_t add = (*(itr->data + itr->cur))
                            | (*(itr->data + itr->cur + 1) << 8);
  
          const int32_t temp = std::max(static_cast<int32_t>(INT16_MIN),
                               std::min(static_cast<int32_t>(INT16_MAX), 
                               base + static_cast<int32_t>(add * volume_)));
  
          base = static_cast<int16_t>(temp);

          sound_buffer_[frame * 2] = base & 0xff;
          sound_buffer_[frame * 2 + 1] = (base >> 8) & 0xff;

          itr->cur += 2;
        }

        if (itr->length <= itr->cur) {
          //ESP_LOGI(TAG, "Erase cur:%d length:%u", itr->cur, itr->length);
          itr = sound_list_.erase(itr);
        } else {
          ++itr;
        }
      }
    }

    Write(sound_buffer_, I2S_DMA_BUFFER_LEN);
  }

  void Play(SoundInfo sound_info) {
    std::lock_guard<std::mutex> lock(mtx_);
    sound_list_.emplace_back(sound_info);
  }

 private:
  void Enable() { i2s_channel_enable(i2s_handle_); }

  esp_err_t Write(const unsigned char *const data, size_t length) {
    size_t bytes_written = 0;
    return i2s_channel_write(i2s_handle_, data, length, &bytes_written,
                             portMAX_DELAY);
  }

  void Disable() { i2s_channel_disable(i2s_handle_); }

 private:
  gpio_num_t bclk_no_;     // BCLK pin number
  gpio_num_t lrclk_no_;    // LRCLK pin number
  gpio_num_t dataout_no_;  // Data Out pin number
  uint32_t sample_rate_hz_;
  i2s_chan_handle_t i2s_handle_;
  uint8_t *sound_buffer_;
  std::list<SoundInfo> sound_list_;
  float volume_;
  mutable std::mutex mtx_;
};

#endif  // I2S_SOUND_H_

// EOF
