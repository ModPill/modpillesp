#include <Arduino.h>
#include <NimBLEDevice.h>

// Casey Code

// BLE Server variables
NimBLEServer* pServer = nullptr;
NimBLEService* pService = nullptr;
NimBLECharacteristic* pCharacteristic = nullptr;
NimBLECharacteristic* pWriteCharacteristic = nullptr;
NimBLEAdvertising* pAdvertising = nullptr;

// UUIDs for service and characteristics
#define SERVICE_UUID        "12345678-1234-5678-9012-123456789abc"
#define CHARACTERISTIC_UUID "87654321-4321-8765-2109-cba987654321"
#define WRITE_CHAR_UUID     "abcdef01-2345-6789-abcd-ef0123456789"

// Device name for advertising
#define DEVICE_NAME "ModPill"

bool deviceConnected = false;
bool oldDeviceConnected = false;
std::string receivedValue = "";

// Server callbacks to handle connection events
class MyServerCallbacks: public NimBLEServerCallbacks {
    void onConnect(NimBLEServer* pServer) {
        deviceConnected = true;
        Serial.println("Client connected");
        // Stop advertising when a client connects
        NimBLEDevice::stopAdvertising();
    };

    void onDisconnect(NimBLEServer* pServer) {
        deviceConnected = false;
        Serial.println("Client disconnected - start advertising");
        // Restart advertising when client disconnects
        NimBLEDevice::startAdvertising();
    }
};

// Characteristic callbacks to handle read/write operations
class MyCharacteristicCallbacks: public NimBLECharacteristicCallbacks {
    void onRead(NimBLECharacteristic* pCharacteristic) {
        Serial.println("Characteristic read");
        // You can modify the value here before it's sent to the client
        std::string value = "Hello from ModPill!";
        pCharacteristic->setValue(value);
    }

    void onWrite(NimBLECharacteristic* pCharacteristic) {
        Serial.println("Characteristic written");
        receivedValue = pCharacteristic->getValue();
        
        if (receivedValue.length() > 0) {
            Serial.print("Received: ");
            for (int i = 0; i < receivedValue.length(); i++) {
                Serial.print(receivedValue[i]);
            }
            Serial.println();
        }
    }
};

void initBLE() {
    Serial.println("Initializing NimBLE...");
    
    // Initialize NimBLE
    NimBLEDevice::init(DEVICE_NAME);
    
    // Set the transmission power, this will affect the connection range
    NimBLEDevice::setPower(ESP_PWR_LVL_P9); // +9dBm
    
    // Create the BLE Server
    pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());
    
    // Create the BLE Service
    pService = pServer->createService(SERVICE_UUID);
    
    // Create a BLE Characteristic for reading
    pCharacteristic = pService->createCharacteristic(
                         CHARACTERISTIC_UUID,
                         NIMBLE_PROPERTY::READ |
                         NIMBLE_PROPERTY::NOTIFY
                       );
    
    // Create a BLE Characteristic for writing
    pWriteCharacteristic = pService->createCharacteristic(
                             WRITE_CHAR_UUID,
                             NIMBLE_PROPERTY::WRITE |
                             NIMBLE_PROPERTY::WRITE_NR
                           );
    
    // Set callbacks for characteristics
    pCharacteristic->setCallbacks(new MyCharacteristicCallbacks());
    pWriteCharacteristic->setCallbacks(new MyCharacteristicCallbacks());
    
    // Set initial values
    pCharacteristic->setValue("ModPill Ready");
    
    // Start the service
    pService->start();
    
    // Start advertising
    pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponseData(NimBLEAdvertisementData());
    pAdvertising->setMinInterval(0x0);  // set value to 0x00 to not advertise this parameter
    
    Serial.println("Starting BLE advertising...");
    NimBLEDevice::startAdvertising();
    
    Serial.println("BLE GATT Server initialized successfully!");
    Serial.println("Waiting for client connections...");
}

void setup() {
    // Initialize serial communication
    Serial.begin(115200);
    while (!Serial) {
        delay(100);
    }
    
    Serial.println("ModPill ESP32 NimBLE GATT Server");
    Serial.println("================================");
    
    // Initialize BLE
    initBLE();
}

void loop() {
    // Handle connection state changes
    if (!deviceConnected && oldDeviceConnected) {
        delay(500); // give the bluetooth stack the chance to get things ready
        pServer->startAdvertising(); // restart advertising
        Serial.println("Restart advertising");
        oldDeviceConnected = deviceConnected;
    }
    
    // Handle new connections
    if (deviceConnected && !oldDeviceConnected) {
        oldDeviceConnected = deviceConnected;
    }
    
    // If connected, you can send notifications here
    if (deviceConnected) {
        // Example: Send a notification every 5 seconds
        static unsigned long lastNotification = 0;
        if (millis() - lastNotification > 5000) {
            std::string notifyValue = std::string("Notification: ") + String(millis()).c_str();
            pCharacteristic->setValue(notifyValue);
            pCharacteristic->notify();
            lastNotification = millis();
        }
    }
    
    delay(100);
}