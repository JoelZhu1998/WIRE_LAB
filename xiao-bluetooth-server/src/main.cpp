#include <Arduino.h>
#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
 
// HC-SR04 Pins
const int triggerPin = 9; // Adjust as per your setup
const int echoPin = 8;    // Adjust as per your setup
 
// BLE Server
BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;
 
// Sensor data
float rawDistance = 0.0;
float denoisedDistance = 0.0;
 
// DSP Algorithm: Moving Average Filter
const int numReadings = 10;
float readings[numReadings]; // the readings from the analog input
int readIndex = 0; // the index of the current reading
float total = 0; // the running total
float average = 0; // the average
 
// UUIDs
#define SERVICE_UUID        "654b4887-31b2-4614-b8ca-d25c87be87ee"
#define CHARACTERISTIC_UUID "db7bd39b-f945-4c8a-a642-c456dfc9a32b"
 
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        deviceConnected = true;
    };
 
    void onDisconnect(BLEServer* pServer) {
        deviceConnected = false;
    }
};
 
void setup() {
    // Serial port for debugging purposes
    Serial.begin(115200);
    // Initialize the HC-SR04 sensor
    pinMode(triggerPin, OUTPUT);
    pinMode(echoPin, INPUT);
    for (int thisReading = 0; thisReading < numReadings; thisReading++) {
        readings[thisReading] = 0;
    }
 
    // BLE Device setup
    BLEDevice::init("MyBLEDevice");
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());
    BLEService *pService = pServer->createService(SERVICE_UUID);
    pCharacteristic = pService->createCharacteristic(
                        CHARACTERISTIC_UUID,
                        BLECharacteristic::PROPERTY_READ |
                        BLECharacteristic::PROPERTY_WRITE |
                        BLECharacteristic::PROPERTY_NOTIFY
                      );
    pCharacteristic->addDescriptor(new BLE2902());
    pService->start();
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);
    BLEDevice::startAdvertising();
    Serial.println("Setup done");
}
 
float readDistance() {
    // Clears the triggerPin
    digitalWrite(triggerPin, LOW);
    delayMicroseconds(2);
    // Sets the triggerPin on HIGH state for 10 micro seconds
    digitalWrite(triggerPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(triggerPin, LOW);
    // Reads the echoPin, returns the sound wave travel time in microseconds
    float duration = pulseIn(echoPin, HIGH);
    // Calculating the distance
    float distance= duration*0.034/2;
    return distance;
}
 
void loop() {
    // Reading the distance
    rawDistance = readDistance();
 
    // Update the total to get a new average
    total = total - readings[readIndex];
    // Read from the sensor
    readings[readIndex] = rawDistance;
    total = total + readings[readIndex];
    // Advance to the next position in the array
    readIndex = (readIndex + 1) % numReadings;
    // Calculate the average
    average = total / numReadings;
    denoisedDistance = average;
 
    // Print both raw and denoised data
    Serial.print("Raw Distance: ");
    Serial.print(rawDistance);
    Serial.print(" cm, Denoised Distance: ");
    Serial.println(denoisedDistance);
    Serial.println(" cm");
 
    // Check if the device is connected and the denoised distance is less than 30 cm
    if (deviceConnected && denoisedDistance < 30) {
        // Change the following code to send your denoised data
        String message = "Denoised Distance: " + String(denoisedDistance);
        pCharacteristic->setValue(message.c_str());
        pCharacteristic->notify();
    }
 
    // Handling disconnection
    if (!deviceConnected && oldDeviceConnected) {
        delay(500); // give the bluetooth stack the chance to get things ready
        pServer->startAdvertising(); // restart advertising
        oldDeviceConnected = deviceConnected;
    }
 
    // Handling reconnection
    if (deviceConnected && !oldDeviceConnected) {
        oldDeviceConnected = deviceConnected;
    }
 
    delay(1000); // Delay between readings
}