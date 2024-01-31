#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
 
// The remote service and characteristic we wish to connect to
static BLEUUID serviceUUID("654b4887-31b2-4614-b8ca-d25c87be87ee");
static BLEUUID charUUID("db7bd39b-f945-4c8a-a642-c456dfc9a32b");
 
static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;
static BLERemoteCharacteristic* pRemoteCharacteristic;
static BLEAdvertisedDevice* myDevice;
 
// Notification callback
static void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {
    Serial.println("Received notification from server.");
 
    // Convert received data to string
    std::string receivedData((char*)pData, length);
 
    Serial.print("Distance: ");
    Serial.print(receivedData.c_str());
    Serial.println(" cm");
}
 
class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
    connected = true;
    Serial.println("Connected to server");
  }
 
  void onDisconnect(BLEClient* pclient) {
    connected = false;
    Serial.println("Disconnected from server");
  }
};
 
bool connectToServer() {
    Serial.print("Connecting to ");
    Serial.println(myDevice->getAddress().toString().c_str());
 
    BLEClient* pClient = BLEDevice::createClient();
    Serial.println(" - Created client");
 
    pClient->setClientCallbacks(new MyClientCallback());
    pClient->connect(myDevice);
    Serial.println(" - Connected to server");
 
    BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService == nullptr) {
      Serial.print("Failed to find service UUID: ");
      Serial.println(serviceUUID.toString().c_str());
      return false;
    }
    Serial.println(" - Found our service");
 
    pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
    if (pRemoteCharacteristic == nullptr) {
      Serial.print("Failed to find characteristic UUID: ");
      Serial.println(charUUID.toString().c_str());
      return false;
    }
    Serial.println(" - Found our characteristic");
 
    if(pRemoteCharacteristic->canNotify()) {
      pRemoteCharacteristic->registerForNotify(notifyCallback);
      Serial.println("Registered for notifications");
    }
 
    return true;
}
 
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    Serial.print("Found Device: ");
    Serial.println(advertisedDevice.toString().c_str());
 
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {
      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
      doScan = true;
      Serial.println("Device with required service found");
    } 
  }
};
 
void setup() {
  Serial.begin(115200);
  Serial.println("Starting BLE Client...");
 
  BLEDevice::init("");
 
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(5, false);
}
 
void loop() {
  if (doConnect) {
    if (connectToServer()) {
      Serial.println("Connected to the BLE Server.");
    } else {
      Serial.println("Failed to connect to server");
      doConnect = false; // Reset the flag
    }
  }
 
  if (connected) {
    // Code here to interact with the server
  } else if(doScan) {
    BLEDevice::getScan()->start(0); // Restart scanning
  }
 
  delay(1000);
}