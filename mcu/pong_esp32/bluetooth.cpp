/*
Author: Ameen Shaikh

Source code for Bluetooth.

Implements a Bluetooth server.

Connectable and implements the callback whenever the client writes to the characteristic.
*/

#include "bluetooth.h"

volatile char user_input = 'n';

class WriteCallback : public NimBLECharacteristicCallbacks {
  /*
  "onWrite" is called upon a characteristic write
  The written data is stored in the "user_input" variable
  */
  void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override {
    String value = pCharacteristic->getValue();
    user_input = value[0];
  }
};

#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

/*
Sets up the Bluetooth server
1. Creates the server
2. Creates the service and characteristic
3. Assigns privileges to the characteristic
4. Starts advertising
*/
void SetupBluetooth() {
  NimBLEDevice::init("Ameen ESP32");

  NimBLEServer* pServer = NimBLEDevice::createServer();
  NimBLEService* pService = pServer->createService(SERVICE_UUID);

  NimBLECharacteristic* pCharacteristic =
    pService->createCharacteristic(
      CHARACTERISTIC_UUID,
      NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR
    );

  pCharacteristic->setCallbacks(new WriteCallback());

  pService->start();

  NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->start();
}
