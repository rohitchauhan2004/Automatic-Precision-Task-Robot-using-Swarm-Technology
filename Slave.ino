#include <ESP8266WiFi.h>
#include <espnow.h>
#include <DHT.h>

// ----- DHT11 Setup -----
#define DHTPIN D7  // GPIO13
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// ----- Ultrasonic Sensor -----
#define TRIG_PIN D5  // GPIO14
#define ECHO_PIN D6  // GPIO12

// ----- Motor Pins -----
#define IN1 D1
#define IN2 D2
#define IN3 D3
#define IN4 D4

// ----- Structures for ESP-NOW -----
typedef struct struct_command {
  int commandCode;
} struct_command;

typedef struct struct_sensorData {
  float temperature;
  float humidity;
  float distance;
} struct_sensorData;

struct_command receivedCommand;
struct_sensorData sensorDataToSend;

// ----- Flags and Variables -----
volatile bool newCommandReceived = false;
uint8_t masterMac[6];

unsigned long lastSensorSendTime = 0;
const unsigned long sensorInterval = 10000;

unsigned long commandStartTime = 0;
bool commandActive = false;
const unsigned long motorRunTime = 3000;

int currentCommand = 0;

// Precise turn duration (adjust experimentally)
const unsigned long TURN_DURATION_MS = 300; // ~90-degree turn timing

// ----- Get Distance -----
float getDistanceCM() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH);
  float distance = duration * 0.034 / 2;
  return distance;
}

// ----- Motor Control -----
void executeCommand(int command) {
  switch (command) {
    case 1: // Forward
      digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
      digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
      break;
    case 2: // Backward
      digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);
      digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);
      break;
    case 3: // Left (Precise 90° Turn)
      digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);
      digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
      delay(TURN_DURATION_MS);
      executeCommand(0); // Stop after turning
      commandActive = false;
      currentCommand = 0;
      Serial.println("↩  Left Turn Completed");
      break;
    case 4: // Right (Precise 90° Turn)
      digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
      digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);
      delay(TURN_DURATION_MS);
      executeCommand(0); // Stop after turning
      commandActive = false;
      currentCommand = 0;
      Serial.println("↪  Right Turn Completed");
      break;
    default: // Stop
      digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
      digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
      break;
  }
}

// ----- ESP-NOW Receive Callback -----
void OnDataRecv(uint8_t *mac, uint8_t *incomingData, uint8_t len) {
  if (len == sizeof(receivedCommand)) {
    memcpy(&receivedCommand, incomingData, sizeof(receivedCommand));
    memcpy(masterMac, mac, 6);

    currentCommand = receivedCommand.commandCode;
    commandStartTime = millis();
    commandActive = true;
    newCommandReceived = true;

    Serial.print("📥 Command Received: ");
    Serial.println(currentCommand);
  } else {
    Serial.println("⚠ Invalid data size received!");
  }
}

// ----- Setup -----
void setup() {
  Serial.begin(115200);

  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);
  pinMode(TRIG_PIN, OUTPUT); pinMode(ECHO_PIN, INPUT);

  dht.begin();

  WiFi.mode(WIFI_STA);
  WiFi.begin("motoedge50fusion_6126", "4qmdtnym");

  Serial.print("🔌 Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n✅ Connected to Wi-Fi");

  int channel = WiFi.channel();
  Serial.print("📡 Wi-Fi Channel: ");
  Serial.println(channel);

  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  wifi_set_channel(channel);

  if (esp_now_init() != 0) {
    Serial.println("❌ ESP-NOW initialization failed!");
    return;
  }

  Serial.println("✅ ESP-NOW initialized");

  esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);
  esp_now_register_recv_cb(OnDataRecv);

  Serial.println("✅ ESP-NOW Slave Ready");
}

// ----- Loop -----
void loop() {
  // Stop motors after timeout (only for forward/backward)
  if (commandActive && millis() - commandStartTime >= motorRunTime &&
      (currentCommand == 1 || currentCommand == 2)) {
    executeCommand(0);
    Serial.println("⏹ Forward/Backward Command Timed Out. Motors stopped.");
    commandActive = false;
    currentCommand = 0;
  }

  // Handle new command
  if (newCommandReceived) {
    newCommandReceived = false;

    executeCommand(currentCommand);

    // Send sensor data if interval met
    unsigned long currentTime = millis();
    if (currentTime - lastSensorSendTime >= sensorInterval) {
      lastSensorSendTime = currentTime;

      float temp = dht.readTemperature();
      float hum = dht.readHumidity();
      float dist = getDistanceCM();

      if (isnan(temp) || isnan(hum)) {
        Serial.println("⚠ Failed to read from DHT sensor!");
        return;
      }

      sensorDataToSend.temperature = temp;
      sensorDataToSend.humidity = hum;
      sensorDataToSend.distance = dist;

      esp_now_send(masterMac, (uint8_t *)&sensorDataToSend, sizeof(sensorDataToSend));

      Serial.println("📤 Sensor Data Sent:");
      Serial.print("   🌡 Temp: "); Serial.println(temp);
      Serial.print("   💧 Humidity: "); Serial.println(hum);
      Serial.print("   📏 Distance: "); Serial.println(dist);
    } else {
      Serial.println("⏳ Skipping sensor send (interval not met)");
    }
  }
}
