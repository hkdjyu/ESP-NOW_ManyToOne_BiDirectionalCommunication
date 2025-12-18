# **ESP-NOW Bi-Directional Network Guide / ESP-NOW 雙向通訊網絡指南**

This document explains the "Many-to-One" framework used in your ESP32 project. This system allows multiple "Slave" devices (sensors, remotes) to talk to a single "Master" device (hub, display), and lets the Master send commands back.

這份文件解釋了您的 ESP32 專案中使用的「多對一 (Many-to-One)」架構。此系統允許這多個「Slave 從機」裝置（如感測器、遙控器）與單個「Master 主機」裝置（如集線器、顯示器）進行通訊，並允許主機發送指令回從機。

## **1\. System Overview Diagram / 系統概覽圖**

Think of this system like a classroom. The Master is the Teacher, and the Slaves are Students.

可以將這個系統想像成一個教室。Master (主機) 是老師，Slaves (從機) 是學生。

* **Students** know who the Teacher is (Hardcoded MAC address).  
  **學生** 知道老師是誰（MAC 位址已寫死在程式碼中）。  
* **The Teacher** doesn't memorize every Student's name beforehand; she learns them when they speak up (Dynamic Pairing).  
  **老師** 不需要預先記住每個學生的名字；當學生發言時，老師會自動認識他們（動態配對）。

![][image1]

## **2\. Core Concepts (Simplified) / 核心概念 (簡化版)**

### **What is ESP-NOW? / 什麼是 ESP-NOW？**

Unlike standard Wi-Fi, ESP-NOW does not need a Router. It is a direct connection between boards, similar to how wireless mice or game controllers work. It is very fast and uses less battery.

與標準 Wi-Fi 不同，ESP-NOW 不需要路由器。它是電路板之間的直接連線，類似於無線滑鼠或遊戲手把的運作方式。它的速度非常快且耗電量更低。

### **The "Envelope" (Struct) / 「信封」 (結構體 Struct)**

Computers can't just send "data"; they need a structure. We use a Struct as an "Envelope." Both the sender and receiver must agree on exactly what fits inside this envelope.

電腦不能隨意發送「數據」；它們需要一個結構。我們使用 Struct (結構體) 作為「信封」。發送方和接收方必須對信封內裝的內容達成一致。

**Your Envelope (struct\_message) contains / 您的信封 (struct\_message) 包含：**

1. **ID:** Who is sending this? (e.g., Robot \#1)  
   **ID:** 誰發送的？（例如：機器人 \#1）  
2. **Type:** What is inside? (Is this Sensor Data? Or a Command?)  
   **Type:** 裡面是什麼？（是感測器數據？還是指令？）  
3. **Value 1 (Float):** Decimal numbers (e.g., Temperature 25.5)  
   **Value 1 (浮點數):** 小數（例如：溫度 25.5）  
4. **Value 2 (Int):** Whole numbers (e.g., Status Code 200 or Milliseconds 5000\)  
   **Value 2 (整數):** 整數（例如：狀態碼 200 或毫秒數 5000）  
5. **Text:** A short note (e.g., "Alert\!" or "OK")  
   **Text:** 簡短筆記（例如："Alert\!" 或 "OK"）

### **MAC Address / MAC 位址**

Every ESP32 has a unique serial number called a MAC Address (e.g., 24:6F:28:A1:B2:C3).

每個 ESP32 都有一個獨一無二的序號，稱為 MAC 位址（例如：24:6F:28:A1:B2:C3）。

* **Slave Logic:** The Slave *must* know the Master's MAC address to send the first message. You write this in the code.  
  **從機邏輯:** 從機 *必須* 知道主機的 MAC 位址才能發送第一條訊息。您需要將其寫在程式碼中。  
* **Master Logic:** The Master is "smart." It reads the "Return Address" on every envelope it gets and saves it. This allows it to reply without you typing in every Slave's MAC address manually.  
  **主機邏輯:** 主機很「聰明」。它會讀取收到的每個信封上的「回信地址」並儲存起來。這讓它能夠回覆訊息，而無需您手動輸入每個從機的 MAC 位址。

## **3\. How the Code Works / 程式碼運作原理**

### **The Slave (Sender) Logic / 從機 (發送端) 邏輯**

#### **Step 1: Configuration / 設定**

The Slave must have the Master's MAC address hardcoded. It also defines the data structure.

從機必須將主機的 MAC 位址寫死在程式碼中，並定義資料結構。

```
// \[CRITICAL\] Master's MAC Address / 主機的 MAC 位址  
uint8\_t masterMacAddress\[\] \= {0x7C, 0x2C, 0x67, 0x7C, 0x87, 0xFC}; 

// The Data Structure (Envelope) / 資料結構 (信封)  
typedef struct struct\_message {  
  uint8\_t id;  
  MessageType type; // DATA or COMMAND  
  float value1;  
  int value2;  
  char text\[32\];  
} struct\_message;
```

#### **Step 2: Initialization & Pairing / 初始化與配對**

In setup(), the Slave connects to ESP-NOW and registers the Master as a "Peer" (someone it can talk to).

在 setup() 中，從機連線到 ESP-NOW 並將主機註冊為「配對裝置 (Peer)」。

```
// Register the Master / 註冊主機  
esp\_now\_peer\_info\_t peerInfo;  
memcpy(peerInfo.peer\_addr, masterMacAddress, 6);  
peerInfo.channel \= 0;    
peerInfo.encrypt \= false;

if (esp\_now\_add\_peer(\&peerInfo) \!= ESP\_OK){  
  Serial.println("Failed to add peer");  
}
```

#### **Step 3: Sending Data / 發送數據**

In the loop(), the Slave packs the envelope and sends it.

在 loop() 中，從機將資料打包進信封並發送。

```
// 1\. Fill the Envelope / 填寫信封  
outgoingData.id \= 1;           
outgoingData.type \= DATA;      
outgoingData.value1 \= 25.5;  

// 2\. Send to Master / 發送給主機  
esp\_now\_send(masterMacAddress, (uint8\_t \*) \&outgoingData, sizeof(outgoingData));
```

#### **Step 4: Receiving Commands / 接收指令**

If the Master replies, the OnDataRecv function is triggered automatically.

如果主機回覆，OnDataRecv 函式會自動被觸發。

```
void OnDataRecv(const esp\_now\_recv\_info\_t \* info, const uint8\_t \*incomingDataBytes, int len) {  
  // Open the envelope / 打開信封  
  memcpy(\&incomingData, incomingDataBytes, sizeof(incomingData));

  if (incomingData.type \== COMMAND) {  
     // Do something, e.g., Turn on LED / 執行動作，例如開燈  
     Serial.println(incomingData.text);  
  }  
}
```

### **The Master (Hub) Logic / 主機 (集線器) 邏輯**

#### **Step 1: Auto-Pairing (The "Smart" Part) / 自動配對 (智慧功能)**

The Master does **not** have Slave MAC addresses hardcoded. Instead, inside OnDataRecv, it checks if it knows the sender. If not, it adds them immediately.

主機 **沒有** 寫死從機的 MAC 位址。反之，在 OnDataRecv 中，它會檢查是否認識發送者。如果不認識，它會立刻將其新增。

```
void OnDataRecv(const esp\_now\_recv\_info\_t \* info, const uint8\_t \*incomingDataBytes, int len) {  
    
  // 1\. Get Sender's MAC / 取得發送者的 MAC  
  const uint8\_t \* mac \= info-\>src\_addr;

  // 2\. AUTO-PAIRING: Register this sender if new / 自動配對：如果是新裝置則註冊  
  registerPeer(mac); 

  // 3\. Process Data / 處理數據  
  memcpy(\&incomingData, incomingDataBytes, sizeof(incomingData));  
  Serial.print("Slave ID: ");  
  Serial.println(incomingData.id);  
    
  // 4\. (Optional) Reply immediately / (選用) 立即回覆  
  outgoingData.type \= COMMAND;  
  strcpy(outgoingData.text, "ACK");  
  sendToSlave(mac, outgoingData);  
}
```

#### **Step 2: Helper Function \- Register Peer / 輔助函式 \- 註冊配對**

This function ensures the Master creates a connection to the Slave dynamically.

此函式確保主機動態地與從機建立連線。

```
void registerPeer(const uint8\_t \*mac\_addr) {  
  // If we already know them, do nothing / 如果已經認識，什麼都不做  
  if (esp\_now\_is\_peer\_exist(mac\_addr)) {  
    return;   
  }

  // Add new node to peer list / 將新節點加入配對列表  
  esp\_now\_peer\_info\_t peerInfo;  
  memcpy(peerInfo.peer\_addr, mac\_addr, 6);  
  peerInfo.channel \= 0;   
  peerInfo.encrypt \= false;  
  esp\_now\_add\_peer(\&peerInfo);  
}
```

## **4\. Quick Start Guide / 快速入門指南**

1. **Flash the Master / 燒錄主機:** \* Upload master\_hub.ino to your first board.  
   將 master\_hub.ino 上傳到您的第一塊板子。  
   * Open Serial Monitor (115200 baud).  
     打開 序列埠監控視窗 (Serial Monitor) (鮑率 115200)。  
   * **COPY** the address shown: MASTER MAC ADDRESS: XX:XX:XX:XX:XX:XX.  
     **複製** 顯示的位址：MASTER MAC ADDRESS: XX:XX:XX:XX:XX:XX。  
2. **Configure the Slave / 設定從機:** \* Open slave\_node.ino.  
   打開 slave\_node.ino。  
   * Paste the Master's MAC address into line \~18:  
     將主機的 MAC 位址貼上到第 18 行左右：  
     uint8\_t masterMacAddress\[\] \= {0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX};

   * *Tip: Remember to add 0x before each pair of numbers\!* *提示：記得在每組數字前加上 0x！* 3\. **Flash the Slave / 燒錄從機:** \* Upload the code to your second board.  
     將程式碼上傳到您的第二塊板子。  
3. **Test / 測試:** \* Open the Serial Monitor for the Master. You should see incoming data.  
   打開主機的序列埠監控視窗。您應該會看到傳入的數據。  
   * Open the Serial Monitor for the Slave. You should see "Sent with Success" and potentially "Command Received" if the Master replies.  
     打開從機的序列埠監控視窗。您應該會看到 "Sent with Success"（發送成功），如果主機有回覆，可能還會看到 "Command Received"（收到指令）。

[image1]: <data:image/png;base64,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>