// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define ADC_STRAIN_G 35             // Conversao AD da leitura do conversor AD
#define ADC_BATTERY 34              // Leitura da voltagem da bateria
#define SERVICE_UUID "91bad492-b950-4226-aa2b-4ede9fa42f59"
#define BLE_UUID "cba1d466-344c-4be3-ab3f-189f80dd7518"

// Libraries
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>

//BLE server name
#define bleServerName "Margot"

bool deviceConnected = false;
int strain_gauge;

// Strain gauge Characteristic and Descriptor
BLECharacteristic bleCharacteristics(BLE_UUID, BLECharacteristic::PROPERTY_NOTIFY);
BLEDescriptor bleDescriptor(BLEUUID((uint16_t)0x2902));

//Setup callbacks onConnect and onDisconnect
class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *pServer) {
    deviceConnected = true;
  };
  void onDisconnect(BLEServer *pServer) {
    deviceConnected = false;
  }
};

void setup() {
  // Start serial communication
  Serial.begin(115200);
  // Create the BLE Device
  BLEDevice::init(bleServerName);
  // Create the BLE Server
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  // Create the BLE Service
  BLEService *bleService = pServer->createService(SERVICE_UUID);
  // Create BLE Characteristics and Create a BLE Descriptor
  bleService->addCharacteristic(&bleCharacteristics);
  bleDescriptor.setValue("Strain gauge");
  bleCharacteristics.addDescriptor(&bleDescriptor);

  // Start the service
  bleService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pServer->getAdvertising()->start();
  Serial.println("Waiting a client connection to notify...");
}

void loop() {
  if (deviceConnected) {
    //Notify values reading from strain gauge
    strain_gauge = analogRead(ADC_STRAIN_G);
    static char strain_g[10];
    dtostrf(strain_gauge, 10, 0, strain_g);
    //Set temperature Characteristic value and notify connected client
    bleCharacteristics.setValue(strain_g);
    bleCharacteristics.notify();
    Serial.print("Leitura do circuito de condicionamento: ");
    Serial.println(strain_gauge);
  }
}
