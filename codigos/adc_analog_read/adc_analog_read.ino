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
#define LED 2           // Board LE
#define ADC_CHANNEL 35  // Define ADC channel (ADC1_CHANNEL_7 = GPIO_35)
#define MUX_A 15        // Adress pins for multiplex
#define MUX_B 14        // MSB

// Timer0 instanciation
hw_timer_t* timer0 = NULL;

// Variables
bool t0_int = false;          // Timer0 interrupt flag
unsigned long irq_total = 0;  // Number of irq to do
uint16_t sg[4];               // Data from strain gauges. Value read ADC. Demands 2 bytes (0 a 4095)
unsigned long start_chrono;   // Start chronometer
unsigned long stop_chrono;    // Stop chronometer
uint16_t adc_raw = 0;         // Read values from ADC
uint16_t adc_calib = 0;       // Average of read values from ADC for calibration

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

void read_adc() {
  // Read data from ADC
  adc_raw = analogRead(ADC_CHANNEL);
}

void calibration() {
  // Sum of read values from ADC
  int sum = 0;
  int n_samples = 100;  // Number of samples for calibration
  for (int i = 0; i < n_samples; i++) {
    read_adc();
    sum = sum + adc_raw;
  }
  adc_calib = sum / n_samples;
}

void setup() {
  // Serial port init
  Serial.begin(921600);

  // Onboard LED
  pinMode(LED, OUTPUT);

  // Multiplex address pins
  pinMode(MUX_A, OUTPUT);
  pinMode(MUX_B, OUTPUT);

  // Start calibration
  Serial.println("Calibrando...");
  calibration();
  Serial.println("Calibrado!");

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
        read_adc();              // Read from DMA ADC
        sg[sg_number] = adc_raw;
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
    Serial.print(adc_calib);
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