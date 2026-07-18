/*
* Funcao: Utiliza rotina de calibração e faz a leitura de 4 strain gauges e conversão ADC utilizando DMA
* Autor : Paulo Cesar Menegon de Castro
* Data de criacao : 20.02.2026
* Data de modificacao: 11.03.2026
* Referencias: https://espressif-docs.readthedocs-hosted.com/projects/esp-idf/en/stable/api-reference/peripherals/i2s.html
*              https://embarcados.com.br/esp32-adc-interno/
*                                      
*/

// Constants
#define LED 33                      // Board LED
#define ADC_UNIT ADC_UNIT_1         // Define ADC unit (ADC1)
#define ADC_CHANNEL ADC1_CHANNEL_7  // Define ADC channel (ADC1_CHANNEL_7 = GPIO_35)
#define ADC_ATTEN ADC_ATTEN_DB_11   // Attenuation
#define ADC_WIDTH ADC_WIDTH_BIT_12  // Resolution
#define SAMPLE_RATE 128000          // 128KHz
#define MUX_A 15                    // Adress pins for multiplex
#define MUX_B 14                    // MSB

// Libraries
#include <driver/i2s.h>
#include <driver/adc.h>
#include <esp_adc_cal.h>

// Timer0 instanciation
hw_timer_t* timer0 = NULL;

// Struct with informations for calibration
esp_adc_cal_characteristics_t adc_chars;

// Variables
bool t0_int = false;          // Timer0 interrupt flag
unsigned long irq_total = 0;  // Number of irq to do
size_t bytes_read;            // Number of bytes read from ADC
int samples_read;             // Number of samples read
uint16_t buffer[64];          // Store values read from ADC
uint16_t sg[4];               // Data from strain gauges. Value read ADC. Demands 2 bytes (0 a 4095)
unsigned long start_chrono;   // Start chronometer
unsigned long stop_chrono;    // Stop chronometer
uint16_t avg_adc = 0;         // Average of read values
uint16_t avg_adc_calib = 0;   // Average of read values

// Timer0 interrupt
void IRAM_ATTR timer0_int() {  // RAM is faster than FLASH
  t0_int = true;
  irq_total++;
  if (irq_total == 10000)
    timerStop(timer0);
}

// Timer0 inicialization
void startTimer0() {
  // Frequency of 1 MHz
  timer0 = timerBegin(1000000);  // Frequency in Hz (max 80MHz)

  // Timer0 interruption function
  timerAttachInterrupt(timer0, &timer0_int);

  // Function call interval
  timerAlarm(timer0, 1000, true, 0);  // Function called every 1000 useconds (1KHZ)
}

void i2s_init() {
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_ADC_BUILT_IN),  // I2S receive mode with ADC
    .sample_rate = SAMPLE_RATE,                                                   // Sample rate
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,                                 // 16 bit I2S
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,                                  // Only the left channel
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,                            // I2S format
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,                                     // Default interrupt priority
    .dma_buf_count = 8,                                                           // Number of DMA buffers
    .dma_buf_len = 64,                                                            // Number of samples
    .use_apll = false,                                                            // No Audio PLL
    .tx_desc_auto_clear = false,
    .fixed_mclk = 0
  };
  i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  i2s_set_adc_mode(ADC_UNIT, ADC_CHANNEL);
  i2s_adc_enable(I2S_NUM_0);
}

void read_dma_adc() {
  // Sum of read values from ADC
  float sum = 0;

  // Read data from DMA
  i2s_read(I2S_NUM_0, &buffer, sizeof(buffer), &bytes_read, portMAX_DELAY);
  samples_read = bytes_read / 2;

  // Process data and average
  for (int i = 0; i < samples_read; i++) {
    // Only upper 12 bits contain the ADC value
    sum += buffer[i] & 0x0FFF;
  }
  avg_adc = sum / samples_read;
  avg_adc_calib = esp_adc_cal_raw_to_voltage(avg_adc, &adc_chars);      // Voltage value read after calibration 
}

void setup() {
  // Serial port init
  Serial.begin(921600);

  // Onboard LED
  pinMode(LED, OUTPUT);

  // Multiplex address pins
  pinMode(MUX_A, OUTPUT);
  pinMode(MUX_B, OUTPUT);

  // i2s start
  i2s_init();

  // ADC channel attenuation
  adc1_config_channel_atten(ADC_CHANNEL, ADC_ATTEN_DB_11);  // Allows input up to 2.5V-3.3V, but the ADC is most linear between 100mV and 2450mV

  // Start calibration
  esp_adc_cal_value_t val_type = esp_adc_cal_characterize(
    ADC_UNIT_1,  // ADC unit 1
    ADC_ATTEN,   // ADC attenuation
    ADC_WIDTH,   // ADC resolution
    1100,        // Default value (mV)
    &adc_chars);

  // Type of calibration
  if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP) {
    Serial.println("Using Two Point");
  } else if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
    Serial.println("Using eFuse Vref");
  } else {
    Serial.println("Using Vref default");
  }

  // Preparing...
  delay(2000);
  // Timer0 start
  startTimer0();
}

void loop() {
  // Serial communication causes a lot of delay! Use if only necessary.
  if (t0_int) {
    // Interruption flag
    t0_int = false;

    // Indicate the start of process
    digitalWrite(LED, HIGH);

    // Number of straing gauge read
    uint8_t sg_number = 0;

    // Start ADC reading and processing data from
    start_chrono = micros();  // start of proces

    // Read from wheatstone bridges (1 to 4)
    for (int i = 0; i < 2; i++) {
      digitalWrite(MUX_B, 0);  // Multiplex Control B (MSB)
      for (int j = 0; j < 2; j++) {
        digitalWrite(MUX_A, 0);  // Multiplex Control A (LSB)
        read_dma_adc();          // Read from DMA ADC
        sg[sg_number] = avg_adc_calib;
        sg_number++;
      }
    }

    //Serial.println("-----------------");
    // End of ADC read
    stop_chrono = micros();

    // Indicate the end of conversion
    digitalWrite(LED, LOW);

    // Exporta dados para o monitor serial no formato csv
    Serial.print(irq_total);
    Serial.print(";");
    Serial.print(stop_chrono - start_chrono);
    Serial.print(";");
    Serial.print(sg[0]);
    Serial.print(";");
    Serial.print(sg[1]);
    Serial.print(";");
    Serial.print(sg[2]);
    Serial.print(";");
    Serial.println(sg[3]);
  }
}