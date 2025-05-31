#define BLYNK_TEMPLATE_ID "TMPL3NxcnEp8E"
#define BLYNK_TEMPLATE_NAME "Master"
#define BLYNK_AUTH_TOKEN    "IWM1WMVgD7xPqmL3e5oR1jTDRGognJCx"

#include <ESP8266WiFi.h>
#include <espnow.h>
#include <FirebaseESP8266.h>
#include <BlynkSimpleEsp8266.h>

// --- WiFi Credentials ---
const char* ssid = "motoedge50fusion_6126";
const char* password = "4qmdtnym";

// --- Firebase Credentials ---
#define FIREBASE_HOST "esp8266-ac2e8-default-rtdb.firebaseio.com"
#define FIREBASE_AUTH "e3zp8esTYo6yGdhxY3GeLrvAZwCo08Fl3PgmhpzG"

// --- Blynk Auth Token ---
char auth[] = "IWM1WMVgD7xPqmL3e5oR1jTDRGognJCx";  // Replace with your Blynk token

// --- Slave MAC Address ---
uint8_t slaveAddress[] = {0x4C, 0xEB, 0xD6, 0x1F, 0x86, 0x8A};

// --- ESP-NOW Structs ---
typedef struct struct_command {
  int commandCode;
} struct_command;

typedef struct struct_sensorData {
  float temperature;
  float humidity;
  float distance;
} struct_sensorData;

struct_command commandToSend;
struct_sensorData receivedSensorData;
bool dataReceived = false;

// --- Firebase Objects ---
FirebaseData firebaseData;
FirebaseAuth authFb;
FirebaseConfig config;

// --- ESP-NOW Send Callback ---
void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus) {
  Serial.print("Command Delivery Status: ");
  Serial.println(sendStatus == 0 ? "✅ Success" : "❌ Fail");
}

// --- ESP-NOW Receive Callback ---
void OnDataRecv(uint8_t *mac, uint8_t *incomingData, uint8_t len) {
  if (len == sizeof(struct_sensorData)) {
    memcpy(&receivedSensorData, incomingData, sizeof(receivedSensorData));
    dataReceived = true;
  } else {
    Serial.println("❗ Unexpected data size!");
  }
}

// --- Send Command Helper ---
void sendCommand(int code) {
  commandToSend.commandCode = code;
  esp_now_send(slaveAddress, (uint8_t *)&commandToSend, sizeof(commandToSend));
}

// --- Blynk Virtual Button Handlers ---
BLYNK_WRITE(V0) { if (param.asInt()) { sendCommand(1); Serial.println("➡ Forward (Blynk)"); } }
BLYNK_WRITE(V1) { if (param.asInt()) { sendCommand(2); Serial.println("⬅ Backward (Blynk)"); } }
BLYNK_WRITE(V2) { if (param.asInt()) { sendCommand(3); Serial.println("↩ Left (Blynk)"); } }
BLYNK_WRITE(V3) { if (param.asInt()) { sendCommand(4); Serial.println("↪ Right (Blynk)"); } }
BLYNK_WRITE(V4) { if (param.asInt()) { sendCommand(0); Serial.println("⏹ Stop (Blynk)"); } }

void setup() {
  Serial.begin(115200);
  delay(100);

  WiFi.begin(ssid, password);
  Serial.print("🔌 Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  Serial.println("\n✅ Wi-Fi Connected");
  Serial.print("🌐 IP: "); Serial.println(WiFi.localIP());

  // Init Firebase
  config.database_url = FIREBASE_HOST;
  config.signer.tokens.legacy_token = FIREBASE_AUTH;
  Firebase.begin(&config, &authFb);
  Firebase.reconnectWiFi(true);

  // Init Blynk
  Blynk.begin(auth, ssid, password);

  // Init ESP-NOW
  if (esp_now_init() != 0) {
    Serial.println("❌ ESP-NOW init failed");
    return;
  }
  esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(OnDataRecv);
  esp_now_add_peer(slaveAddress, ESP_NOW_ROLE_SLAVE, 1, NULL, 0);

  Serial.println("✅ Master Ready (Blynk + Firebase + ESP-NOW)");
}

void loop() {
  Blynk.run();

  // Handle Serial Input (optional)
  if (Serial.available()) {
    char input = Serial.read();
    switch (input) {
      case 'f': sendCommand(1); Serial.println("➡ Forward"); break;
      case 'b': sendCommand(2); Serial.println("⬅ Backward"); break;
      case 'l': sendCommand(3); Serial.println("↩ Left"); break;
      case 'r': sendCommand(4); Serial.println("↪ Right"); break;
      case 's': sendCommand(0); Serial.println("⏹ Stop"); break;
      default: Serial.println("❗ Invalid input (f/b/l/r/s)"); return;
    }
  }

  // Upload sensor data to Firebase
  if (dataReceived) {
    dataReceived = false;
    Serial.println("\n📩 Sensor Data Received:");
    Serial.print("🌡 Temperature: "); Serial.println(receivedSensorData.temperature);
    Serial.print("💧 Humidity: "); Serial.println(receivedSensorData.humidity);
    Serial.print("📏 Distance: "); Serial.println(receivedSensorData.distance);

    delay(200);  // For stability

    if (Firebase.ready()) {
      FirebaseJson json;
      json.add("temperature", receivedSensorData.temperature);
      json.add("humidity", receivedSensorData.humidity);
      json.add("distance", receivedSensorData.distance);
      json.add("timestamp", (int)time(nullptr));

      if (Firebase.pushJSON(firebaseData, "/sensors/data", json)) {
        Serial.println("✅ Data uploaded to Firebase");
      } else {
        Serial.println("❌ Firebase upload failed: " + firebaseData.errorReason());
      }
    }
  }
}
