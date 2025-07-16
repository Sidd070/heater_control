#include <OneWire.h>
#include <DallasTemperature.h>

#define ONE_WIRE_BUS 16
#define RELAY_PIN    17
#define LED_PIN      18
#define BUZZER_PIN   19

#define TARGET_TEMP      50.0
#define OVERHEAT_TEMP    70.0
#define HYSTERESIS       1.0
#define DEVICE_DISCONNECTED_C -127

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

enum State { Idle, Heating, Stabilizing, TargetReached, Overheat } currentState = Idle;
State previousState = Idle;

unsigned long lastRead = 0;
unsigned long lastStateChange = 0;
const unsigned long readInterval = 1000;
const unsigned long stateDelay = 2000;

void setup() {
  Serial.begin(115200);
  sensors.begin();
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
  digitalWrite(LED_PIN, LOW);
  noTone(BUZZER_PIN);
}

void loop() {
  unsigned long now = millis();
  if (now - lastRead < readInterval) return;
  lastRead = now;

  sensors.requestTemperatures();
  float temp = sensors.getTempCByIndex(0);

  if (temp == DEVICE_DISCONNECTED_C || temp < -55 || temp > 125) {
    Serial.println("Sensor read failed or out of range!");
    return;
  }

  Serial.printf("Temp: %.2f°C | State: %d\n", temp, currentState);

  State newState = currentState;

  if (temp >= OVERHEAT_TEMP)
    newState = Overheat;
  else if (temp < TARGET_TEMP - HYSTERESIS)
    newState = Heating;
  else if (abs(temp - TARGET_TEMP) <= HYSTERESIS)
    newState = TargetReached;
  else
    newState = Stabilizing;

  if (newState != currentState && now - lastStateChange > stateDelay) {
    previousState = currentState;
    currentState = newState;
    lastStateChange = now;

    Serial.printf("State changed: %d → %d\n", previousState, currentState);

    switch (currentState) {
      case Overheat:
        digitalWrite(RELAY_PIN, LOW);
        digitalWrite(LED_PIN, LOW);
        tone(BUZZER_PIN, 2000);
        Serial.println("🚨 OVERHEAT! Buzzer ON, Heater OFF, LED OFF");
        break;

      case Heating:
        digitalWrite(RELAY_PIN, HIGH);
        digitalWrite(LED_PIN, HIGH);
        noTone(BUZZER_PIN);
        Serial.println("🔥 HEATING! Relay ON, LED ON, Buzzer OFF");
        break;

      case TargetReached:
        digitalWrite(RELAY_PIN, LOW);
        digitalWrite(LED_PIN, LOW);
        noTone(BUZZER_PIN);
        Serial.println("✅ TARGET REACHED! All OFF");
        break;

      case Stabilizing:
        digitalWrite(RELAY_PIN, LOW);
        digitalWrite(LED_PIN, LOW);
        noTone(BUZZER_PIN);
        Serial.println("⚙️ STABILIZING... All OFF");
        break;

      default:
        digitalWrite(RELAY_PIN, LOW);
        digitalWrite(LED_PIN, LOW);
        noTone(BUZZER_PIN);
        Serial.println("IDLE or UNKNOWN STATE");
        break;
    }
  }
}
