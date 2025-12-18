/*
 * ESP-NOW SLAVE (Sender/Node) TEMPLATE
 * COMPATIBILITY: ESP32 Board Manager v3.0+ / ESP-IDF 5.5+
 * * LOGIC:
 * 1. Hardcoded Master MAC Address (Slave knows who to talk to).
 * 2. Bi-directional: Sends sensor data, listens for commands.
 */

#include <esp_now.h>
#include <WiFi.h>

// ==========================================
// 1. CONFIGURATION
// ==========================================

// [CRITICAL] REPLACE THIS with your Master Board's MAC Address
// You can find the Master's MAC in the Serial Monitor when the Master boots.
// EXAMPLE: 7c:2c:67:7c:87:fc
uint8_t masterMacAddress[] = {0x7C, 0x2C, 0x67, 0x7C, 0x87, 0xFC}; 

// SHARED DATA STRUCTURE
// Must match the Master's structure exactly
enum MessageType {
  DATA,    // Regular sensor data sent to Master
  PING,    // Keep-alive check
  COMMAND  // Instruction received from Master
};

typedef struct struct_message {
  uint8_t id;             // Unique ID for this node (e.g., 1, 2, 3...)
  MessageType type;       // Type of message
  float value1;           // Generic float (Temp, Voltage, etc.)
  int value2;             // Generic int (Status, Counter, etc.)
  char text[32];          // Short message/Label
} struct_message;

struct_message outgoingData; // Data we send
struct_message incomingData; // Data we receive

// ==========================================
// 2. CALLBACKS
// ==========================================

// Called when data is sent. 
// UPDATED SIGNATURE FOR ESP32 CORE v3.0+ (IDF 5.5)
// Replaced 'const uint8_t *mac_addr' with 'const wifi_tx_info_t *info'
void OnDataSent(const wifi_tx_info_t *info, esp_now_send_status_t status) {
  // Serial.print("Last Packet Send Status: ");
  // Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

// Called when data is received FROM Master
// UPDATED SIGNATURE FOR ESP32 CORE v3.0+
void OnDataRecv(const esp_now_recv_info_t * info, const uint8_t *incomingDataBytes, int len) {
  // Extract MAC address from info if needed
  const uint8_t* mac = info->src_addr;

  // 1. Copy incoming bytes to our struct
  memcpy(&incomingData, incomingDataBytes, sizeof(incomingData));

  // 2. USE EXAMPLE: Handle Commands from Master
  if (incomingData.type == COMMAND) {
    Serial.println("\n--- COMMAND RECEIVED FROM MASTER ---");
    Serial.printf("From Master: %02x:%02x:%02x:%02x:%02x:%02x \n", 
      mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    Serial.printf("Command Code: %d \n", incomingData.value2);
    Serial.printf("Note: %s \n", incomingData.text);

    // Real-world example: Toggle a relay or LED
    // if (incomingData.value2 == 1) { digitalWrite(LED_PIN, HIGH); }
  }
}

// ==========================================
// 3. SETUP
// ==========================================
void setup() {
  Serial.begin(115200);
  
  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Register the Callbacks
  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(OnDataRecv);

  // Register the Master as a Peer
  // (We must do this before we can send anything to it)
  esp_now_peer_info_t peerInfo;
  memset(&peerInfo, 0, sizeof(peerInfo)); // Clear memory
  memcpy(peerInfo.peer_addr, masterMacAddress, 6);
  peerInfo.channel = 0;  // 0 means "Use current Wi-Fi channel"
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }
  
  Serial.println("Slave Ready. Sending data...");
}

// ==========================================
// 4. MAIN LOOP
// ==========================================
void loop() {
  // USE EXAMPLE: Send Mock Sensor Data every 2 seconds
  
  // 1. Prepare the data
  outgoingData.id = 1;         // Hardcode ID for this specific board
  outgoingData.type = DATA;    // Mark as DATA
  outgoingData.value1 = 25.5;  // Example: Temperature
  outgoingData.value2 = millis(); // Example: Uptime
  strcpy(outgoingData.text, "Status: OK");

  // 2. Send the message
  esp_err_t result = esp_now_send(masterMacAddress, (uint8_t *) &outgoingData, sizeof(outgoingData));

  if (result == ESP_OK) {
    Serial.println("Sent Data to Master");
  } else {
    Serial.println("Error sending data");
  }

  delay(2000);
}