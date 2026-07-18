/*
* Funcao: Leitura de 4 strain gauges, conversão ADC utilizando DMA 
*         e transmissão dos pacotes com intervalo de 1ms
* Autor : Paulo Cesar Menegon de Castro
* Data de criacao : 20.03.2026
* Data de modificacao: 01.04.2026                                      
*/	

// Constants
#define LED 33                      // Board LED
#define ADC_UNIT ADC_UNIT_1         // Define ADC unit (ADC1)
#define ADC_CHANNEL ADC1_CHANNEL_7  // Define ADC channel (ADC1_CHANNEL_7 = GPIO_35)
#define SAMPLE_RATE 128000          // 128KHz
#define MUX_A 15                    // Adress pins for multiplex
#define MUX_B 14                    // MSB
#define CE_PIN 4                    // CE nRF24L01
#define CSN_PIN 5                   // CSN nRF24L01

// Libraries
#include <driver/i2s.h>
#include <driver/adc.h>
#include <SPI.h>
#include <RF24.h>
#include <printf.h>

// Timer0 instanciation
hw_timer_t* timer0 = NULL;

// Instantiate an object for the nRF24L01 transceiver
RF24 radio(CE_PIN, CSN_PIN);

// Create a struct
struct payloadStruct {
  unsigned long time_stamp;  // Milliseconds sice board is on
  unsigned long number = 0;  // Number of payload
  uint16_t sg[4];            // Data from 4 strain gauges. Value read ADC. Demands 2 bytes (0 a 4095)
};
payloadStruct payload;

// Variables
bool t0_int = false;          // Timer0 interrupt flag
size_t bytes_read;            // Number of bytes read from ADC
int samples_read;             // Number of samples read
uint16_t buffer[16];          // Store values read from ADC
uint16_t sg[16];              // Data from strain gauges. Value read ADC. Demands 2 bytes (0 a 4095)
unsigned long start_adc;      // Start chronometer
unsigned long stop_adc;       // Stop chronometer
unsigned long start_tx;       // Start chronometer
unsigned long stop_tx;        // Stop chronometer
uint16_t avg_adc = 0;         // Average of read values
uint16_t avg_calib = 0;       // Average of read values for calibration
unsigned long irq_total = 0;  // Number of irq to do

// Let these addresses be used for the pair
uint8_t address[][4] = { "1No", "2No" };
// It is very helpful to think of an address as a path instead of as
// an identifying device destination
// to use different addresses on a pair of radios, we need a variable to
// uniquely identify which address this radio will use to transmit

// Timer0 interrupt
void IRAM_ATTR timer0_int() {  // RAM is faster than FLASH
  t0_int = true;
  irq_total++;
  if (irq_total == 120000)
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
  unsigned long sum = 0;
 
  // Read data from DMA
  i2s_read(I2S_NUM_0, &buffer, sizeof(buffer), &bytes_read, portMAX_DELAY);
  samples_read = bytes_read / 2;

  // Process data and average
  for (int i = 0; i < samples_read; i++) {
    // Only upper 12 bits contain the ADC value
    sum += buffer[i] & 0x0FFF;
  }
  avg_adc = sum / samples_read;
}

void calibration() {
  int n_samples = 1024;  // Number of samples for calibration
  unsigned long sum = 0;
  for (int i = 0; i < n_samples; i++) {
    read_dma_adc();
    sum = sum + avg_adc;
  }
  avg_calib = sum / n_samples;
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
  Serial.println("Calibrando...");
  calibration();
  Serial.println("Calibrado!");

  // Start nRF24L01
  if (!radio.begin()) {
    Serial.println(F("radio hardware is not responding!!"));
    while (1) {}  // hold in infinite loop
  }

  // Set the PA Level low to try preventing power supply related problems
  radio.setPALevel(RF24_PA_LOW);   // RF24_PA_MAX is default.
  radio.setDataRate(RF24_2MBPS);   // MAX speed
  radio.setAutoAck(false);         // Disable ACK
  radio.disableDynamicPayloads();  // Disable dynamic payloads
  radio.disableAckPayload();       // Disable ACK payloads
  radio.disableCRC();              // Optional (fastest, less safe)
  radio.setRetries(0, 0);          // No retransmissions
  radio.setChannel(100);           // Choice channel to minimize WiFi interference
  radio.setAddressWidth(3);        // Short address

  // Save on transmission time by setting the radio to only transmit the
  // number of bytes we need to transmit
  radio.setPayloadSize(sizeof(payload));  //

  // set the TX address of the RX node for use on the TX pipe (pipe 0)
  radio.stopListening(address[0]);  // put radio in TX mode

  // set the RX address of the TX node into a RX pipe
  //radio.openReadingPipe(1, address[1]);  // using pipe 1

  // TX role
  radio.stopListening();  // put radio in TX mode

  // For debugging info
  printf_begin();              // needed only once for printing details
  radio.printPrettyDetails();  // (larger) function that prints human readable data

  // Delay to prepare
  delay(2000);

  // Timer0 start
  startTimer0();

  // Export serial data format csv
  Serial.println("payload_number;time_stamp;time_adc;time_tx;sg_1;sg_2;sg_3;sg_4");
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

    // Fill the payload
    payload.time_stamp = micros();  // Microsseconds since reset or board on

    // For tests only, select 30.000 samples for each channel
    if (payload.number < 30000) {
      // SG 1
      digitalWrite(MUX_A, 0);  // Multiplex Control A (LSB)
      digitalWrite(MUX_B, 0);  // Multiplex Control B (MSB)
      sg_number = 0;
    }
    if (payload.number >= 30000 && payload.number < 60000) {
      // SG 2
      digitalWrite(MUX_A, 1);  // Multiplex Control A (LSB)
      digitalWrite(MUX_B, 0);  // Multiplex Control B (MSB)
      sg_number = 1;
    }
    if (payload.number >= 60000 && payload.number < 90000) {
      // SG 3
      digitalWrite(MUX_A, 0);  // Multiplex Control A (LSB)
      digitalWrite(MUX_B, 1);  // Multiplex Control B (MSB)
      sg_number = 2;
    }
    if (payload.number >= 90000 && payload.number < 120000) {
      // SG 4
      digitalWrite(MUX_A, 1);  // Multiplex Control A (LSB)
      digitalWrite(MUX_B, 1);  // Multiplex Control B (MSB)
      sg_number = 3;
    }

    // Start of ADC conversion
    start_adc = micros();

    // Read from DMA ADC
    read_dma_adc();

    // End of conversion
    stop_adc = micros();

    // Fill the payload
    if (sg_number == 0) {
      payload.sg[0] = avg_adc;
      payload.sg[1] = 0;
      payload.sg[2] = 0;
      payload.sg[3] = 0;
    }
    if (sg_number == 1) {
      payload.sg[0] = 0;
      payload.sg[1] = avg_adc;
      payload.sg[2] = 0;
      payload.sg[3] = 0;
    }
    if (sg_number == 2) {
      payload.sg[0] = 0;
      payload.sg[1] = 0;
      payload.sg[2] = avg_adc;
      payload.sg[3] = 0;
    }
    if (sg_number == 3) {
      payload.sg[0] = 0;
      payload.sg[1] = 0;
      payload.sg[2] = 0;
      payload.sg[3] = avg_adc;
    }

    // Number of package
    payload.number++;

    /*
    // Read from wheatstone bridges (1 to 4)
    for (int i = 0; i < 2; i++) {
      digitalWrite(MUX_B, i);  // Multiplex Control B (MSB)
      for (int j = 0; j < 2; j++) {
        digitalWrite(MUX_A, j);  // Multiplex Control A (LSB)
        read_dma_adc();          // Read from DMA ADC
        payload.sg[sg_number] = avg_adc;
        sg_number++;
      }
    }
*/
    // Start of transmission
    start_tx = micros();

    // Transmit & save the report
    bool report = radio.writeFast(&payload, sizeof(payload));

    // End of transmission
    stop_tx = micros();

    // If transmission successfull
    if (report) {
      // Visual indicate of the end of transmission
      digitalWrite(LED, LOW);

      // Export serial data format csv
      Serial.print(payload.number);
      Serial.print(";");
      Serial.print(payload.time_stamp);
      Serial.print(";");
      Serial.print(stop_adc - start_adc);  // Time to transmit
      Serial.print(";");
      Serial.print(stop_tx - start_tx);  // Time to transmit
      Serial.print(";");
      Serial.print(payload.sg[0]);
      Serial.print(";");
      Serial.print(payload.sg[1]);
      Serial.print(";");
      Serial.print(payload.sg[2]);
      Serial.print(";");
      Serial.println(payload.sg[3]);
    }
  }
}