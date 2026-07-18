/*
* Funcao: Leitura de 4 strain gauges e conversão ADC utilizando DMA
* Autor : Paulo Cesar Menegon de Castro
* Data de criacao : 20.02.2026
* Data de modificacao: 11.03.2026
* Referencias: https://embarcados.com.br/esp32-adc-interno/
*  
*                                      
*/

// Constants
#define LED 33    // LED
#define MUX_A 15  // Adress pins for multiplex
#define MUX_B 14  // MSB

// Libraries
#include <esp_check.h>
#include <esp_adc/adc_continuous.h>

// Timer0 instanciation
hw_timer_t *timer0 = NULL;

// Object for ADC continuos conversion
adc_continuous_handle_t adc_handle;

// Variables
bool t0_int = false;          // Timer0 interrupt flag
unsigned long irq_total = 0;  // Number of irq to do
uint32_t bytes_read;          // Number of bytes read from ADC
uint8_t buffer[64];           // Store values read from ADC
uint16_t adc_value;           // ADC read value
uint16_t sg[4];               // Data from strain gauges. Value read ADC. Demands 2 bytes (0 a 4095)
unsigned long start_chrono;   // Start chronometer
unsigned long stop_chrono;    // Stop chronometer
uint16_t avg_adc = 0;         // Average of read values
uint16_t avg_calib = 0;       // Average of read values for calibration

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

bool read_dma_adc() {

  // Sum of read values from ADC
  float sum = 0;

  // Start conversion
  esp_err_t ret = adc_continuous_read(adc_handle, buffer, sizeof(buffer), &bytes_read, portMAX_DELAY);

  if (ret == ESP_OK) {
    for (int i = 0; i < bytes_read; i += sizeof(adc_digi_output_data_t)) {
      adc_digi_output_data_t *p = (adc_digi_output_data_t *)&buffer[i];
      adc_value = p->type1.data;
      sum += adc_value;
    }
    avg_adc = sum / (bytes_read / 2);
    return true;  // Read buffer OK
  } else {
    return false;  // Read buffer fail
  }
}

void calibration() {
  int n_samples = 100000;  // Number of samples for calibration
  unsigned long sum = 0;
  for (int i = 0; i < n_samples; i++) {
    read_dma_adc();
    sum = sum + avg_adc;
  }
  avg_calib = sum / n_samples;
}

void setup() {
  // Start serial port
  Serial.begin(921600);

  // Onboard LED
  pinMode(LED, OUTPUT);

  // Multiplex address pins
  pinMode(MUX_A, OUTPUT);
  pinMode(MUX_B, OUTPUT);

  // Configurazione ADC continuo
  adc_continuous_handle_cfg_t adc_config = {
    .max_store_buf_size = 256,  // DMA buffer size
    .conv_frame_size = 128,     // Tamanho da amostra
  };

  // Creates new ADC continuos handle
  adc_continuous_new_handle(&adc_config, &adc_handle);

  // Pattern configuration for ADC conversion
  adc_digi_pattern_config_t adc_pattern[1];
  adc_pattern[0].atten = ADC_ATTEN_DB_11;      // ADC_ATTEN_DB_0 100 mV ~ 950 mV
                                               // ADC_ATTEN_DB_2_5 100 mV ~ 1250 mV
                                               // ADC_ATTEN_DB_6 150 mV ~ 1750 mV
                                               // ADC_ATTEN_DB_11 150 mV ~ 2450 mV
  adc_pattern[0].channel = ADC_CHANNEL_7;      // GPIO35 -> ADC1_CH7
  adc_pattern[0].unit = ADC_UNIT_1;            // ADC1 ADC
  adc_pattern[0].bit_width = ADC_BITWIDTH_12;  // 12-bits resolution (0 - 4095)

  // ADC1 in continuos conversion mode
  adc_continuous_config_t adc_chan_config = {
    .pattern_num = 1,            // One channel only
    .adc_pattern = adc_pattern,  // Conversion pattern
    .sample_freq_hz = 128000     // Samples per second
  };

  adc_continuous_config(adc_handle, &adc_chan_config);  // ADC continuos mode configuration
  adc_continuous_start(adc_handle);                     // Start of continuos conversion

  // Start calibration
  //calibration();

  // Timer0 start
  startTimer0();
}

void loop() {

  if (t0_int) {
    // Interruption flag
    t0_int = false;

    // Indicate the start of process
    digitalWrite(LED, HIGH);

    // Number of straing gauge read
    uint8_t sg_number = 0;

    // Select strain gauge to read (SG_1 = 0 0, SG_2 = 0 1, SG_3 = 1 0, SG_4 = 1 1. A = LSB, B = MSB)
    for (int i = 0; i < 2; i++) {
      digitalWrite(MUX_B, 0);  // Multiplex Control B (MSB)
      for (int j = 0; j < 2; j++) {
        digitalWrite(MUX_A, 0);  // Multiplex Control A (LSB)
        // Start ADC reading and processing data from
        start_chrono = micros();  // start of proces
        if (read_dma_adc()) {
          sg[sg_number] = avg_adc;
          sg_number++;
        }

        // Indicate the end of process
        stop_chrono = micros();
      }
    }

    // Indicate the end of conversion
    digitalWrite(LED, LOW);

    // Exporta dados para o monitor serial no formato csv
    Serial.print(irq_total);
    Serial.print(";");
    Serial.print(stop_chrono - start_chrono);
    Serial.print(";");
    Serial.print(avg_calib);
    Serial.print(";");
    Serial.print(sg[0]);
    Serial.print(";");
    Serial.print(sg[1]);
    Serial.print(";");
    Serial.print(sg[2]);
    Serial.print(";");
    Serial.print(sg[3]);
    Serial.println(";");
  }
}