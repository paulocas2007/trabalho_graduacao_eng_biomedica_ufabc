/*
Example 2 — Faster Sampling + Buffer Processing

This version shows how to process chunks (useful for FFT, audio, etc.)

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
#define ADC_CHANNEL ADC1_CHANNEL_6  // GPIO34

const int BUFFER_SIZE = 2048;
uint16_t buffer[BUFFER_SIZE];

void setupI2S() {
  i2s_config_t config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_ADC_BUILT_IN),
    .sample_rate = 40000,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,                            // I2S format
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,
    .dma_buf_len = 512,
    .use_apll = false
  };

  i2s_driver_install(I2S_PORT, &config, 0, NULL);
  i2s_set_adc_mode(ADC_UNIT_1, ADC_CHANNEL);
  i2s_adc_enable(I2S_PORT);
}

void setup() {
  Serial.begin(115200);
  setupI2S();
}

void loop() {
  size_t bytesRead = 0;

  i2s_read(I2S_PORT, buffer, sizeof(buffer), &bytesRead, portMAX_DELAY);

  int samples = bytesRead / 2;

  long sum = 0;
  for (int i = 0; i < samples; i++) {
    sum += (buffer[i] & 0x0FFF);
  }

  float average = sum / (float)samples;

  Serial.print("Average: ");
  Serial.println(average);
}