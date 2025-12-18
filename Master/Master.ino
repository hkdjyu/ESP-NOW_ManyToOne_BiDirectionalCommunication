/*
 * ESP-NOW MASTER (Hub/Gateway) TEMPLATE
 * * LOGIC:
 * 1. No hardcoded Slave MACs.
 * 2. "Dynamic Pairing": When a Slave sends a message, Master automatically 
 * adds it as a peer. This allows the Master to reply later.
 * 3. Handles Many-to-One data aggregation.
 */

#include <esp_now.h>
#include <esp_wifi.h>
#include <WiFi.h>

// ==========================================
// 1. CONFIGURATION
// ==========================================

// SHARED DATA STRUCTURE
// Must match the Slave's structure exactly
enum MessageType {
  DATA,    // Regular sensor data from Slave
  PING,    // Keep-alive
  COMMAND  // Instruction from Master
};

typedef struct struct_message {
  uint8_t id;             // Unique ID of the slave
  MessageType type;       // Type of message
  float value1;           // Generic float
  int value2;             // Generic int
  char text[32];          // Short message
} struct_message;

struct_message incomingData;
struct_message outgoingData;

// ==========================================
// 2. HELPER FUNCTIONS
// ==========================================

// Helper: Send data to a specific slave MAC
void sendToSlave(const uint8_t *targetMac, struct_message &msg) {
  esp_err_t result = esp_now_send(targetMac, (uint8_t *) &msg, sizeof(msg));
  if (result == ESP_OK) {
    Serial.println(">> Message Sent to Slave");
  } else {
    Serial.println(">> Error Sending Message");
  }
}

// Helper: Auto-register a slave when they talk to us
void registerPeer(const uint8_t *mac_addr) {
  // If we already know this peer, exit
  if (esp_now_is_peer_exist(mac_addr)) {
    return; 
  }

  // Otherwise, add them to the peer list
  esp_now_peer_info_t peerInfo;
  memset(&peerInfo, 0, sizeof(peerInfo));
  memcpy(peerInfo.peer_addr, mac_addr, 6);
  peerInfo.channel = 0; 
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) == ESP_OK) {
    Serial.println("NEW NODE DETECTED: Added to Peer List.");
  }
}

// Helper: Get Mac Address of the current ESP32
void readMacAddress(){
  uint8_t baseMac[6];
  esp_err_t ret = esp_wifi_get_mac(WIFI_IF_STA, baseMac);
  if (ret == ESP_OK) {
    Serial.printf("%02x:%02x:%02x:%02x:%02x:%02x\n",
                  baseMac[0], baseMac[1], baseMac[2],
                  baseMac[3], baseMac[4], baseMac[5]);
  } else {
    Serial.println("Failed to read MAC address");
  }
}

// ==========================================
// 3. CALLBACKS
// ==========================================

// ESP32 Board Lib v3.0.0+ / IDF 5.5+ Signature for Send Callback
// REPLACED: 'const uint8_t *mac_addr' with 'const wifi_tx_info_t *info'
// to match the latest ESP-IDF 5.5 signatures.
void OnDataSent(const wifi_tx_info_t *info, esp_now_send_status_t status) {
  // Optional: Debugging sending status
  // Serial.print("Last Packet Send Status: ");
  // Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

// ESP32 Board Lib v3.0.0+ Signature for Recv Callback
// Changed: First parameter is now 'esp_now_recv_info_t*' instead of 'uint8_t*'
void OnDataRecv(const esp_now_recv_info_t * info, const uint8_t *incomingDataBytes, int len) {
  
  // Extract the MAC address from the new info struct
  const uint8_t * mac = info->src_addr;

  // 1. IMPORTANT: Register this sender so we can reply later
  registerPeer(mac);

  // 2. Parse Data
  memcpy(&incomingData, incomingDataBytes, sizeof(incomingData));

  // 3. USE EXAMPLE: Display Data
  Serial.println("\n--- DATA PACKET RECEIVED ---");
  Serial.printf("From Slave MAC: %02x:%02x:%02x:%02x:%02x:%02x \n", 
      mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  Serial.printf("Slave ID: %d \n", incomingData.id);
  
  if (incomingData.type == DATA) {
    Serial.printf("Value 1: %.2f \n", incomingData.value1);
    Serial.printf("Value 2: %d \n", incomingData.value2);
    
    // LOGIC OPTION A (Immediate Reply):
    // Send an acknowledgement back to THIS specific slave
    
    outgoingData.type = COMMAND;
    outgoingData.value2 = 200; // Status OK code
    strcpy(outgoingData.text, "ACK");
    sendToSlave(mac, outgoingData);
    
  }
}

// ==========================================
// 4. SETUP
// ==========================================
void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);

  // Print Master MAC Address so you can copy it to the Slave code
  Serial.print("MASTER MAC ADDRESS: ");
  readMacAddress();

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(OnDataRecv);
  
  Serial.println("Master Ready. Listening for Slaves...");
}

// ==========================================
// 5. MAIN LOOP
// ==========================================
void loop() {
  // LOGIC OPTION C (Master Initiator):
  // To send a command to a slave *without* them talking first, 
  // you must know their MAC address and they must be registered.
  // (In this code, they are registered automatically when they say hello).
  
  /*
  // Example: Send a command every 10 seconds to a known slave
  static unsigned long lastTime = 0;
  if (millis() - lastTime > 10000) {
    lastTime = millis();
    
    // You would need to store the MAC address of the slave you want to target
    // uint8_t targetSlave[] = {0x24, 0x6F, ... }; 
    
    outgoingData.type = COMMAND;
    outgoingData.value2 = 1; // e.g. "Turn ON"
    strcpy(outgoingData.text, "Manual Command");
    
    // sendToSlave(targetSlave, outgoingData);
  }
  */
}