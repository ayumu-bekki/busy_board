#ifndef SEG8_LED_H_
#define SEG8_LED_H_
// (C)2024 bekki.jp
// 7Segment LED SPI
// https://akizukidenshi.com/catalog/g/g110360/

// Include ----------------------
#include <driver/gpio.h>
#include <driver/spi_master.h>
#include <freertos/FreeRTOS.h>

class Seg7Led {
 public:
  explicit Seg7Led() : spi_handle_(nullptr), latch_gpio_no_(GPIO_NUM_21) {}

  ~Seg7Led() { spi_device_release_bus(spi_handle_); }

  void Setup() {
    // GPIO
    gpio_config_t io_conf = {
        .pin_bit_mask = 1ull << latch_gpio_no_,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

    // SPI
    spi_bus_config_t buscfg = {
        .mosi_io_num = GPIO_NUM_23,  // MOSI pin (output)
        .miso_io_num = -1,           // MISO pin (input)
        .sclk_io_num = GPIO_NUM_22,  // SCLK pin
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .data4_io_num = -1,
        .data5_io_num = -1,
        .data6_io_num = -1,
        .data7_io_num = -1,
        .max_transfer_sz = 256,
        .flags = SPICOMMON_BUSFLAG_MASTER,
        .isr_cpu_id = ESP_INTR_CPU_AFFINITY_AUTO,
        .intr_flags = 0,
    };
    spi_device_interface_config_t devcfg = {
        .command_bits = 0,
        .address_bits = 0,
        .dummy_bits = 0,
        .mode = 0,
        .clock_source = SPI_CLK_SRC_DEFAULT,
        .duty_cycle_pos = 128,
        .cs_ena_pretrans = 0,
        .cs_ena_posttrans = 0,
        .clock_speed_hz = SPI_MASTER_FREQ_8M,  // 10 MHz
        .input_delay_ns = 0,
        .spics_io_num = -1,                // CS pin
        .flags = SPI_DEVICE_BIT_LSBFIRST,  // LSB
        .queue_size = 2,
        .pre_cb = nullptr,
        .post_cb = nullptr,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO));
    ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &devcfg, &spi_handle_));
  }

  void DisplayClear() {
    gpio_set_level(latch_gpio_no_, 0);
    Send(0b00000000);
    gpio_set_level(latch_gpio_no_, 1);
  }

  void DisplayE() {
    gpio_set_level(latch_gpio_no_, 0);
    Send(0b10011110);
    gpio_set_level(latch_gpio_no_, 1);
  }

  void DisplayFull() {
    gpio_set_level(latch_gpio_no_, 0);
    Send(0b11111111);
    gpio_set_level(latch_gpio_no_, 1);
  }

  void DisplayDot() {
    gpio_set_level(latch_gpio_no_, 0);
    Send(0b00000001);
    gpio_set_level(latch_gpio_no_, 1);
  }

  void SetNumber(int num) {
    // digits
    static const uint8_t digits[] = {
        0b11111100,  // 0
        0b01100000,  // 1
        0b11011010,  // 2
        0b11110010,  // 3
        0b01100110,  // 4
        0b10110110,  // 5
        0b10111110,  // 6
        0b11100000,  // 7
        0b11111110,  // 8
        0b11110110,  // 9
    };

    gpio_set_level(latch_gpio_no_, 0);
    // ESP_LOGI(TAG, "num:%d", num);
    Send(digits[num]);
    gpio_set_level(latch_gpio_no_, 1);
  }

 private:
  void Send(const char data) {
    spi_transaction_t trans = {
        .flags = 0,
        .cmd = 0,
        .addr = 0,
        .length = 0,    // bit
        .rxlength = 0,  // bit
        .user = nullptr,
        .tx_buffer = nullptr,
        .rx_buffer = nullptr,
    };

    trans.flags =
        SPI_TRANS_USE_TXDATA;  // SPI_TRANS_MODE_OCT;// SPI_TRANS_USE_TXDATA;//
                               // | SPI_TRANS_CS_KEEP_ACTIVE;
    trans.tx_data[0] = data;
    trans.length = 8;  // bit
    spi_device_transmit(spi_handle_, &trans);
  }

 private:
  spi_device_handle_t spi_handle_;
  gpio_num_t latch_gpio_no_;
};

#endif  // SEG8_LED_H_

// EOF
