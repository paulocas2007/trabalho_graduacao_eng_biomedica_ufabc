/*
 * Funcao: Recepcao dos pacotes transmitidos
 * Autor: Paulo Cesar Menegon de Castro
 * Data de criacao: 20.02.2026
 * Data de modificacao: 11.03.2026
 * 
 * Referencias:  https://randomnerdtutorials.com/nrf24l01-2-4ghz-rf-transceiver-module-with-arduino/
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
#define LED 2  // Onboard LED
#define CE_PIN 4
#define CSN_PIN 5
#define IRQ 22

// Libraries
#include <SPI.h>
#include <RF24.h>
#include <printf.h>

// Instantiate an object for the nRF24L01 transceiver
RF24 radio(CE_PIN, CSN_PIN);

// Create a struct
struct payloadStruct {
  unsigned long time_stamp;  // Milliseconds sice board is on
  unsigned long number = 0;  // Number of payload
  uint16_t sg[4];            // Data from strain gauges. Value read ADC. Demands 2 bytes (0 a 4095)
};
payloadStruct payload;

// Variables
unsigned long start_chrono;  // Start of a process
unsigned long stop_chrono;   // End of process
unsigned long last_time_stamp = 0;

// Let these addresses be used for the pair
uint8_t address[][4] = { "1No", "2No" };
// It is very helpful to think of an address as a path instead of as
// an identifying device destination

// For this example, we'll be using a payload containing
// a single float number that will be incremented
// on every successful transmission
bool intNRF24L01 = false;  // Flag de interrupcaodo Timer0

void IRAM_ATTR isrReceive() {
  // Transmission received
  intNRF24L01 = true;
}

void setup() {
  // Serial begin
  Serial.begin(921600);

  // Onboard LED
  pinMode(LED, OUTPUT);

  // initialize the transceiver on the SPI bus
  if (!radio.begin()) {
    Serial.println(F("radio hardware is not responding!!"));
    while (1) {}  // hold in infinite loop
  }

  // Set the PA Level low to try preventing power supply related problems
  // because these examples are likely run with nodes in close proximity to
  // each other.
  radio.setPALevel(RF24_PA_LOW);   // RF24_PA_MAX is default.
  radio.setDataRate(RF24_2MBPS);   // MAX speed
  radio.setAutoAck(false);         // Disable ACK
  radio.disableDynamicPayloads();  // Disable dynamic payloads
  radio.disableAckPayload();       // Disable ACK payloads
  radio.disableCRC();              // Optional (fastest, less safe)
  radio.setRetries(0, 0);          // No retransmissions
  radio.setChannel(100);           // Less WiFi interference
  radio.setAddressWidth(3);        // Short address

  // save on transmission time by setting the radio to only transmit the
  // number of bytes we need to transmit a int
  radio.setPayloadSize(sizeof(payload));  // int datatype occupies 2 bytes

  // set the TX address of the RX node for use on the TX pipe (pipe 0)
  //radio.stopListening(address[1]);  // put radio in TX mode

  // set the RX address of the TX node into a RX pipe
  radio.openReadingPipe(1, address[0]);  // using pipe 1

  // RX mode
  radio.startListening();  // put radio in RX mode

  //mask all IRQ triggers except for receive (1 is mask, 0 is no mask)
  radio.maskIRQ(1, 1, 0);

  //Create interrupt: 0 for pin 2 or 1 for pin 3, the name of the interrupt function or ISR, and condition to trigger interrupt
  attachInterrupt(IRQ, isrReceive, FALLING);

  // For debugging info
  printf_begin();              // Needed only once for printing details
  radio.printPrettyDetails();  // (larger) function that prints human readable data
  //Serial.end();

  // Delay to prepare nRF24L01+
  delay(1000);
}

void loop() {
  // Use of serial communication delay reception rates
  if (intNRF24L01) {
    // Interruption flag
    intNRF24L01 = false;

    uint8_t pipe;
    if (radio.available(&pipe)) {  // Is there a payload? get the pipe number that received it

      // If a payload received
      digitalWrite(LED, HIGH);

      // Start of reception
      start_chrono = micros();

      uint8_t bytes = radio.getPayloadSize();  // get the size of the payload
      radio.read(&payload, sizeof(payload));   // fetch payload from FIFO

      // End of reception
      stop_chrono = micros();

      //Serial.println("-------------------------------------------------");
      //Serial.println("Reception successful! ");  // Payload was receiveded
      //Serial.print("Time to receive:  ");
      //Serial.println(stop_chrono - start_chrono);  // Print the timer result
      //Serial.print("Time stamp (us): ");
      //Serial.println(payload.time_stamp);  // Print the time stamp
      //Serial.print("Diference between time stamps (us): ");
      //Serial.println(payload.time_stamp - last_time_stamp);  // Print the time stamp
      //last_time_stamp = payload.time_stamp;
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
      //Serial.println();

      // Visual indicate of transmission end
      digitalWrite(LED, LOW);

      // Export serial data format csv
      Serial.print(payload.number);
      Serial.print(";");
      Serial.print(payload.time_stamp);
      Serial.print(";");
      Serial.print(stop_chrono - start_chrono);  // Time to receive
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