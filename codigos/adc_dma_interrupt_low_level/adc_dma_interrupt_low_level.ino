/*
Example 2 — ISR-style Callback (Advanced / Low-Level)

If you want something closer to a real interrupt handler, you can hook into I2S driver interrupts — 
but this is more advanced and less Arduino-friendly.
*/
#include <driver/i2s.h>
#include "esp_intr_alloc.h"

#define I2S_PORT I2S_NUM_0

volatile bool dataReady = false;

void IRAM_ATTR i2s_isr(void* arg) {
  dataReady = true;  // minimal ISR work
}

void setupI2S() {
  i2s_config_t config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_ADC_BUILT_IN),
    .sample_rate = 128000,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_IRAM,
    .dma_buf_count = 2,
    .dma_buf_len = 128,
    .use_apll = false
  };

  i2s_driver_install(I2S_PORT, &config, 0, NULL);

  // Attach ISR manually (low-level)
  esp_intr_alloc(ETS_I2S0_INTR_SOURCE, ESP_INTR_FLAG_IRAM, i2s_isr, NULL, NULL);

  i2s_set_adc_mode(ADC_UNIT_1, ADC1_CHANNEL_0);
  i2s_adc_enable(I2S_PORT);
}

void setup() {
  Serial.begin(115200);
  setupI2S();
}

void loop() {
  if (dataReady) {
    dataReady = false;

    uint16_t buffer[128];
    size_t bytesRead;

    i2s_read(I2S_PORT, buffer, sizeof(buffer), &bytesRead, 0);

    Serial.println("ISR Triggered!");
  }
}