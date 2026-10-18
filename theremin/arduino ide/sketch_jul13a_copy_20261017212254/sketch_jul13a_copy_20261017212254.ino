/*
 * THEREMIN OSC - WEMOS D1 R32 (ESP32)
 * 
 * Conexiones físicas:
 * - HC-SR04 Trig         -> Pin D4 (GPIO 16)
 * - HC-SR04 Echo         -> Pin D5 (GPIO 17) [con divisor a 3.3V]
 * - Potenciómetro Centro -> Pin A2 (GPIO 34)
 */

#include <WiFi.h>
#include <WiFiUdp.h>
#include <OSCMessage.h>

// --- AJUSTES DE RED ---
const char* ssid     = "TIan-Movistarfibra";
const char* password = "12345678";

// Dirección IP de tu computadora y puerto de escucha
const IPAddress outIp(192, 168, 1, 41); 
const unsigned int outPort = 12000;

WiFiUDP Udp;

// Pines en la placa Wemos D1 R32
const int trigPin = 16; // Pin D4
const int echoPin = 17; // Pin D5
const int potPin  = 34; // Pin A2

// Tareas FreeRTOS y memoria compartida
volatile float sharedDistance = -1.0;
SemaphoreHandle_t distanceMutex;

// Tarea en segundo plano (Core 0): Medición continua del sensor
void sensorTask(void *pvParameters) {
  for (;;) {
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);

    // Timeout de 25 ms (~4 metros)
    long duration = pulseIn(echoPin, HIGH, 25000);
    float dist = (duration > 0) ? (duration * 0.0343 / 2.0) : -1.0;

    if (xSemaphoreTake(distanceMutex, (TickType_t)5) == pdTRUE) {
      sharedDistance = dist;
      xSemaphoreGive(distanceMutex);
    }
    
    // Intervalo de descanso entre pulsos ultrasónicos
    vTaskDelay(pdMS_TO_TICKS(25));
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(potPin, INPUT);

  distanceMutex = xSemaphoreCreateMutex();

  // Lanzar la tarea del sensor ultrasónico en el Core 0
  xTaskCreatePinnedToCore(sensorTask, "SensorTask", 4096, NULL, 1, NULL, 0);

  // Conexión Wi-Fi
  Serial.print("Conectando a Wi-Fi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConectado. IP del ESP32: " + WiFi.localIP().toString());
  Udp.begin(8888);
}

void loop() {
  float currentDist = -1.0;
  if (xSemaphoreTake(distanceMutex, (TickType_t)2) == pdTRUE) {
    currentDist = sharedDistance;
    xSemaphoreGive(distanceMutex);
  }

  // Lectura del potenciómetro B100K (profundidad de vibrato de 0.0 a 1.0)
  int potVal = analogRead(potPin);
  float vibratoDepth = (float)potVal / 4095.0;

  // 1. Mensaje OSC para la nota / distancia
  OSCMessage msgPitch("/theremin/pitch");
  if (currentDist >= 4.0 && currentDist <= 38.0) {
    msgPitch.add(currentDist);
  } else {
    msgPitch.add(-1.0f); // Silencio fuera de rango
  }
  Udp.beginPacket(outIp, outPort);
  msgPitch.send(Udp);
  Udp.endPacket();
  msgPitch.empty();

  // 2. Mensaje OSC para el vibrato
  OSCMessage msgVibrato("/theremin/vibrato");
  msgVibrato.add(vibratoDepth);
  Udp.beginPacket(outIp, outPort);
  msgVibrato.send(Udp);
  Udp.endPacket();
  msgVibrato.empty();

  delay(20); // Envío a 50 Hz
}