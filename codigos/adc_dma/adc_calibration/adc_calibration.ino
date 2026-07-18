/*

Best precision intervals:
- 0dB: 100 – 950mV;
- 2.5dB: 100 – 1250mV;
- 6dB: 150 – 1750mV;
- 11dB: 150 – 2450mV.

*/

#include <driver/adc.h>
#include "esp_adc_cal.h"

#define ADC_CHANNEL ADC1_CHANNEL_7          // GPIO35
#define ADC_ATTEN ADC_ATTEN_DB_11           // Attenuation
#define ADC_WIDTH ADC_WIDTH_BIT_12          // Resolution

esp_adc_cal_characteristics_t adc_chars;    // Struct with informations for calibration

void setup() {
  Serial.begin(921600);

  // ADC configuration
  adc1_config_width(ADC_WIDTH);
  adc1_config_channel_atten(ADC_CHANNEL, ADC_ATTEN);

  // Start calibration
  esp_adc_cal_value_t val_type = esp_adc_cal_characterize(
    ADC_UNIT_1,     // ADC unit 1
    ADC_ATTEN,      // ADC attenuation
    ADC_WIDTH,      // ADC resolution
    1100,           // Default value (mV)
    &adc_chars
  );

  if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP) {
    Serial.println("Using Two Point");
  } else if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
    Serial.println("Using eFuse Vref");
  } else {
    Serial.println("Using Vref default");
  }
}

void loop() {
  // Raw values
  uint32_t raw = adc1_get_raw(ADC_CHANNEL);

  // Convert raw values to milivolts
  uint32_t voltage = esp_adc_cal_raw_to_voltage(raw, &adc_chars);

  // Results
  Serial.println(test);
  Serial.print("Raw: ");
  Serial.print(raw);
  Serial.print(" | Voltage: ");
  Serial.print(voltage);
  Serial.println(" mV");

  delay(500);
}