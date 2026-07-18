/*Example 1 — ADC with DMA using I2S (Continuous Sampling)

This is the most common and efficient way.

Features:
Continuous sampling
Uses DMA automatically
High-speed ADC reading

GPIO mapping (ADC1):

GPIO36 → CH0
GPIO37 → CH1
GPIO38 → CH2
GPIO39 → CH3
GPIO32 → CH4
GPIO33 → CH5
GPIO34 → CH6
GPIO35 → CH7

*/

#include <driver/i2s.h>

#define I2S_PORT I2S_NUM_0
#define ADC_CHANNEL ADC1_CHANNEL_0  // GPIO36

void setupI2S() {
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_ADC_BUILT_IN),
    .sample_rate = 20000,  // 20 kHz
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = 0,
    .dma_buf_count = 4,
    .dma_buf_len = 1024,
    .use_apll = false
  };

  i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
  i2s_set_adc_mode(ADC_UNIT_1, ADC_CHANNEL);
  i2s_adc_enable(I2S_PORT);
}

void setup() {
  Serial.begin(115200);
  setupI2S();
}

void loop() {
  uint16_t buffer[1024];
  size_t bytesRead;

  i2s_read(I2S_PORT, &buffer, sizeof(buffer), &bytesRead, portMAX_DELAY);

  int samples = bytesRead / 2;

  for (int i = 0; i < samples; i++) {
    uint16_t val = buffer[i] & 0x0FFF;  // 12-bit ADC
    Serial.println(val);
  }
}