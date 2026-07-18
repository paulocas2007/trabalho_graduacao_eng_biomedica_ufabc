/*
 * Funcao: Transmissao de pacotes contendo 4 valores aleatorios de 0 a 4095 (simulando leituras dos strain gauges)
 * Autor: Paulo Cesar Menegon de Castro
 * Data de criacao: 20.02.2026
 * Data de modificacao: 11.03.2026
 * 
 * Referencias:  https://randomnerdtutorials.com/esp32-spi-communication-arduino/
 *               https://randomnerdtutorials.com/nrf24l01-2-4ghz-rf-transceiver-module-with-arduino/
 * 
 * ----------------------------------- 
 * | Pino do Modulo |   Pino ESP32   | 
 * ----------------------------------- 
 * |      GND       |      GND       |
 * ----------------------------------- 
 * |      3.3V      |      3.3V      |
 * ----------------------------------- 
 * |       CE       |        4       |
 * ----------------------------------- 
 * |      CSN       |        5       |
 * ----------------------------------- 
 * |      SCK       |       18       |
 * ----------------------------------- 
 * |     MOSI       |       23       |
 * ----------------------------------- 
 * |     MISO       |       19       |
 * ----------------------------------- 
 * |      IRQ       |       22       |
 * ----------------------------------- 
*/

// Constants
#define LED 33     // GPIO_33
#define CE_PIN 4   // CE nRF24L01
#define CSN_PIN 5  // CSN nRF24L01

// Libraries
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
bool t0_int = false;               // Timer0 interrupt flag
unsigned long irq_total = 0;       // Number of irq to do
unsigned long tx_irq = 0;          // Number of interruptions requests to transmission
unsigned long tx_ok = 0;           // Number of transmissions success
unsigned long tx_failure = 0;      // Number of transmissions failures
unsigned long package_number = 0;  // Number of package sent
unsigned long start_chrono;        // Start of a process
unsigned long stop_chrono;         // End of process
unsigned long last_time_stamp = 0;

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

void setup() {
  // Serial port init
  Serial.begin(921600);

  // Onboard LED
  pinMode(LED, OUTPUT);

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

  // Delay to prepare nRF24L01+
  delay(1000);

  // Timer0 start
  startTimer0();
}

void loop() {
  // Serial communication causes a lot of delay! Use if only necessary.

  if (t0_int) {
    // Interruption flag
    t0_int = false;

    if (tx_irq < irq_total) {  // Number of interruptions choose
      // Fill the payload
      payload.time_stamp = micros();  // Microsseconds since reset or board on
      for (int i = 0; i < 4; i++) {
        payload.sg[i] = random(4095);  // Random numbers between 0 and 4095
      }

      // Start transmission
      start_chrono = micros();  // start the timer

      // Transmit & save the report
      bool report = radio.writeFast(&payload, sizeof(payload));

      // End of transmission
      stop_chrono = micros();

      // If transmission successfull
      if (report) {
        // Visual indicate of the start of transmission
        digitalWrite(LED, HIGH);
        //Serial.println("-------------------------------------------------");
        //Serial.println("Transmission successful! ");  // Payload was delivered
        //Serial.print("Time to transmit:  ");
        //Serial.println(stop_chrono - start_chrono);  // Print the timer result
        //Serial.print("Time stamp (us): ");
        //Serial.println(payload.time_stamp);  // Print the time stamp
        //Serial.print("Diference between time stamps (us): ");
        //Serial.println(payload.time_stamp - last_time_stamp);  // Print the time stamp

        //Serial.print("Data: ");
        //Serial.print("SG_1: ");
        //Serial.print(payload.sg[0]);
        //Serial.print(" SG_2: ");
        //Serial.print(payload.sg[1]);
        //Serial.print(" SG_3: ");
        //Serial.print(payload.sg[2]);
        //Serial.print(" SG_4: ");
        //Serial.println(payload.sg[3]);
        //Serial.print("Payload number: ");
        //Serial.println(payload.number);

        payload.number++;
        last_time_stamp = payload.time_stamp;

        // Export serial data format csv
        Serial.print(payload.number);
        Serial.print(";");
        Serial.print(payload.time_stamp);
        Serial.print(";");
        Serial.print(stop_chrono - start_chrono);  // Time to transmit
        Serial.print(";");
        Serial.print(payload.sg[0]);
        Serial.print(";");
        Serial.print(payload.sg[1]);
        Serial.print(";");
        Serial.print(payload.sg[2]);
        Serial.print(";");
        Serial.println(payload.sg[3]);
        tx_ok++;
      } else {
        tx_failure++;
      }
      // Visual indicate of the end of transmission
      digitalWrite(LED, LOW);
    } else {
      timerStop(timer0);
      Serial.println("-------------------------------------------------");
      Serial.print("Requested transmissions: ");
      Serial.println(tx_irq);
      Serial.print("Transmissions succesfull: ");
      Serial.println(tx_ok);
      Serial.print("Transmissions failures: ");
      Serial.println(tx_failure);
      Serial.println("-------------------------------------------------");
    }
  }
}