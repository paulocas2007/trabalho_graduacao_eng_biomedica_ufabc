/*
Example 1 — DMA with Interrupt (Event Queue)

This is the clean and recommended way.

What happens:
DMA fills buffer
Interrupt triggers internally
Event is sent to queue
You process data when buffer is ready
*/

#include <driver/i2s.h>

#define I2S_PORT I2S_NUM_0
#define ADC_CHANNEL ADC1_CHANNEL_0  // GPIO36

QueueHandle_t i2s_event_queue;

void setupI2S() {
  i2s_config_t config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_ADC_BUILT_IN),
    .sample_rate = 128000,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,
    .dma_buf_len = 512,
    .use_apll = false
  };

  i2s_driver_install(I2S_PORT, &config, 10, &i2s_event_queue);
  i2s_set_adc_mode(ADC_UNIT_1, ADC_CHANNEL);
  i2s_adc_enable(I2S_PORT);
}

void setup() {
  Serial.begin(115200);
  setupI2S();
}

void loop() {
  i2s_event_t event;

  if (xQueueReceive(i2s_event_queue, &event, portMAX_DELAY)) {
    if (event.type == I2S_EVENT_RX_DONE) {
      uint16_t buffer[256];
      size_t bytesRead;

      i2s_read(I2S_PORT, buffer, sizeof(buffer), &bytesRead, 0);

      int samples = bytesRead / 2;

      for (int i = 0; i < samples; i++) {
        uint16_t val = buffer[i] & 0x0FFF;
        Serial.println(val);
      }
    }
  }
}