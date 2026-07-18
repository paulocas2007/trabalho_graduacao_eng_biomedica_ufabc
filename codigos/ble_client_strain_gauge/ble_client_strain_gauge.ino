#define SCREEN_WIDTH 128  // OLED display width, in pixels
#define SCREEN_HEIGHT 64  // OLED display height, in pixels

#include "BLEDevice.h"
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>

// Timer0
hw_timer_t* timer0 = NULL;

bool intTimer0 = false;  // Flag de interrupcaodo Timer0

// Timer0 interruption
void IRAM_ATTR rotTimer0() {  // Armazena na RAM que e mais rapida que a FLASH
  intTimer0 = true;
}

// Inicializacao do timer
void startTimer() {
  // Frequencia do timer em 1 MHz
  timer0 = timerBegin(1000000);  // Frequencia em Hz (maximo de 80MHz)
  // Funcao a ser chamada pela interrupcao do timer.
  timerAttachInterrupt(timer0, &rotTimer0);
  // Intervalo para chamada da funcao
  timerAlarm(timer0, 1000, true, 0);  // Chama a funcao a cada 1 ms (tempo em us)
  /* Parametros:
     1 - Timer a ser afetado
     2 - Ajusta o alarme para chamar a funcao a cada valor especificado
     3 - Repete o alarme
     4 - Sem limites de valor para a contagem
  */
}

// BLE Server name (the other ESP32 name running the server sketch)
#define bleServerName "Margot"

/* UUID's of the service, characteristic that we want to read*/
// BLE Service
static BLEUUID bleServiceUUID("91bad492-b950-4226-aa2b-4ede9fa42f59");

// Strain gauge characteristic
static BLEUUID characteristicUUID("cba1d466-344c-4be3-ab3f-189f80dd7518");

// Flags stating if should begin connecting and if the connection is up
static boolean doConnect = false;
static boolean connected = false;

// Address of the peripheral device. Address will be found during scanning...
static BLEAddress* pServerAddress;

// Characteristicd that we want to read
static BLERemoteCharacteristic* bleCharacteristic;

// Activate notify
const uint8_t notificationOn[] = { 0x1, 0x0 };
const uint8_t notificationOff[] = { 0x0, 0x0 };

//Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

//Variables to store strain gauge readings
char* strain_gauge;

// Flags to check whether new strain gauges readings are available
boolean newStrainGauge = false;

//Connect to the BLE Server that has the name, Service, and Characteristics
bool connectToServer(BLEAddress pAddress) {
  BLEClient* pClient = BLEDevice::createClient();

  // Connect to the remove BLE Server.
  pClient->connect(pAddress);
  Serial.println(" - Connected to server");

  // Obtain a reference to the service we are after in the remote BLE server.
  BLERemoteService* pRemoteService = pClient->getService(bleServiceUUID);
  if (pRemoteService == nullptr) {
    Serial.print("Failed to find our service UUID: ");
    Serial.println(bleServiceUUID.toString().c_str());
    return (false);
  }

  // Obtain a reference to the characteristics in the service of the remote BLE server.
  bleCharacteristic = pRemoteService->getCharacteristic(characteristicUUID);

  if (bleCharacteristic == nullptr) {
    Serial.print("Failed to find our characteristic UUID");
    return false;
  }
  Serial.println(" - Found our characteristics");

  //Assign callback functions for the Characteristics
  bleCharacteristic->registerForNotify(bleNotifyCallback);
  return true;
}

//Callback function that gets called, when another device's advertisement has been received
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
      if (advertisedDevice.getName() == bleServerName) {                 // Check if the name of the advertiser matches
        advertisedDevice.getScan()->stop();                              // Scan can be stopped, we found what we are looking for
        pServerAddress = new BLEAddress(advertisedDevice.getAddress());  // Address of advertiser is the one we need
        doConnect = true;                                                // Set indicator, stating that we are ready to connect
        Serial.println("Device found. Connecting!");
      }
    }
};

//When the BLE Server sends a new temperature reading with the notify property
static void bleNotifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic,
                              uint8_t* pData, size_t length, bool isNotify) {
  //Store strain gauge value
  strain_gauge = (char*)pData;
  newStrainGauge = true;
}

void setup() {
  // Start serial communication
  Serial.begin(115200);
  Serial.println("Starting Arduino BLE Client application...");
  // OLED display setup
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {  // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;  // Don't proceed, loop forever
  } else {
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(WHITE, 0);
    display.setCursor(0, 25);
    display.print("BLE Client");
    display.display();
  }
  //Init BLE device
  BLEDevice::init("");

  // Retrieve a Scanner and set the callback we want to use to be informed when we
  // have detected a new device.  Specify that we want active scanning and start the

  // scan to run for 30 seconds.
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);
  pBLEScan->start(5);

  startTimer();  // Inicia timers da ESP32
}

void loop() {
  // If the flag "doConnect" is true then we have scanned for and found the desired
  // BLE Server with which we wish to connect.  Now we connect to it.  Once we are
  // connected we set the connected flag to be true.
  if (doConnect == true) {
    if (connectToServer(*pServerAddress)) {
      Serial.println("We are now connected to the BLE Server.");
      //Activate the Notify property of each Characteristic
      bleCharacteristic->getDescriptor(BLEUUID((uint16_t)0x2902))->writeValue((uint8_t*)notificationOn, 2, true);
      connected = true;
    } else {
      Serial.println("We have failed to connect to the server; Restart your device to scan for nearby BLE server again.");
    }
    doConnect = false;
  }
  // If straing gauge readings are available, print in the OLED
  if (newStrainGauge) {
    newStrainGauge = false;
    Serial.println(strain_gauge);
  }
  if (intTimer0) {
    intTimer0 = false;
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(0, 25);
    display.setTextColor(WHITE, 0);
    display.print(strain_gauge);
    display.display();
  }
}
