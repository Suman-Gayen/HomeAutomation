#include "RMaker.h"
#include "WiFi.h"
#include "WiFiProv.h"
#include <DHT.h>
#include <SimpleTimer.h>

const char *service_name = "PROV_SmartHome";
const char *pop = "1234";

// define the Chip Id
uint32_t espChipId = 5;

// define the Node Name
char nodeName[] = "ESP32_Smarthome";

// define the Device Names
char deviceName_1[] = "Switch1";
char deviceName_2[] = "Switch2";
char deviceName_3[] = "Switch3";
char deviceName_4[] = "Switch4";

// define GPIOs for Relays and Switches
static uint8_t RelayPin1 = 23;
static uint8_t RelayPin2 = 22;
static uint8_t RelayPin3 = 21;
static uint8_t RelayPin4 = 19;

static uint8_t SwitchPin1 = 13;
static uint8_t SwitchPin2 = 12;
static uint8_t SwitchPin3 = 14;
static uint8_t SwitchPin4 = 27;

static uint8_t wifiLed    = 2;
static uint8_t gpio_reset = 0;
static uint8_t DHTPIN     = 18;
static uint8_t MQ6_PIN    = 34;  // Analog pin for MQ-6
static uint8_t buzzerPin  = 4;   // Buzzer pin

// States
bool toggleState_1 = LOW;
bool toggleState_2 = LOW;
bool toggleState_3 = LOW;
bool toggleState_4 = LOW;

bool SwitchState_1 = LOW;
bool SwitchState_2 = LOW;
bool SwitchState_3 = LOW;
bool SwitchState_4 = LOW;

float temperature1 = 0;
float humidity1 = 0;
int mq6Val = 0;

DHT dht(DHTPIN, DHT11);  // Use DHT11

SimpleTimer Timer;

// Devices
static Switch my_switch1(deviceName_1, &RelayPin1);
static Switch my_switch2(deviceName_2, &RelayPin2);
static Switch my_switch3(deviceName_3, &RelayPin3);
static Switch my_switch4(deviceName_4, &RelayPin4);
static TemperatureSensor temperature("Temperature");
static TemperatureSensor humidity("Humidity");
static TemperatureSensor mq6Sensor("Gas Level");

bool wifiConnected = false;

void sysProvEvent(arduino_event_t *sys_event)
{
  switch (sys_event->event_id) {
    case ARDUINO_EVENT_PROV_START:
#if CONFIG_IDF_TARGET_ESP32
      Serial.printf("\n Provisioning Started with name %s and PoP %s on BLE \n", service_name, pop);
      printQR(service_name, pop, "ble");
#else
      Serial.printf("\n Provisioning Started with name %s and PoP %s on SoftAP \n", service_name, pop);
      printQR(service_name, pop, "softap");
#endif
      break;
    case ARDUINO_EVENT_WIFI_STA_CONNECTED:
      Serial.println("Connected to Wi-Fi!");
      digitalWrite(wifiLed, HIGH);
      digitalWrite(buzzerPin, HIGH);  // Buzzer ON
      delay(1000);
      digitalWrite(buzzerPin, LOW);   // Buzzer OFF
      wifiConnected = true;
      break;
  }
}
void readMQ6Sensor() {
  mq6Val = analogRead(MQ6_PIN);
  Serial.print("MQ-6 Gas Sensor Value: ");
  Serial.println(mq6Val);

  // 🚨 Gas leak detection logic
  if (mq6Val > 2500) {  // Threshold (tune this experimentally)
    Serial.println("⚠ Gas leak detected!");
    digitalWrite(buzzerPin, HIGH);  // Turn buzzer ON
  } else {
    digitalWrite(buzzerPin, LOW);   // Turn buzzer OFF
  }
}

void write_callback(Device *device, Param *param, const param_val_t val, void *priv_data, write_ctx_t *ctx)
{
  const char *device_name = device->getDeviceName();
  const char *param_name = param->getParamName();

  bool *toggleState = nullptr;
  uint8_t relayPin;

  if (strcmp(device_name, deviceName_1) == 0) {
    toggleState = &toggleState_1;
    relayPin = RelayPin1;
  } else if (strcmp(device_name, deviceName_2) == 0) {
    toggleState = &toggleState_2;
    relayPin = RelayPin2;
  } else if (strcmp(device_name, deviceName_3) == 0) {
    toggleState = &toggleState_3;
    relayPin = RelayPin3;
  } else if (strcmp(device_name, deviceName_4) == 0) {
    toggleState = &toggleState_4;
    relayPin = RelayPin4;
  }

  if (toggleState && strcmp(param_name, "Power") == 0) {
    *toggleState = val.val.b;
    digitalWrite(relayPin, (*toggleState == false) ? HIGH : LOW);
    param->updateAndReport(val);
  }
}

void readSensor() {
  mq6Val = analogRead(MQ6_PIN);
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  humidity1 = h;
  temperature1 = t;

  Serial.print("Temp: "); Serial.println(t);
  Serial.print("Humidity: "); Serial.println(h);
  Serial.print("MQ-6 Gas: "); Serial.println(mq6Val);
}

void sendSensor() {
  readSensor();
  temperature.updateAndReportParam("Temperature", temperature1);
  humidity.updateAndReportParam("Temperature", humidity1);
  mq6Sensor.updateAndReportParam("Temperature", mq6Val);
}

void manual_control()
{
  struct {
    uint8_t switchPin, relayPin;
    bool &switchState, &toggleState;
    Switch &device;
  } switches[] = {
    {SwitchPin1, RelayPin1, SwitchState_1, toggleState_1, my_switch1},
    {SwitchPin2, RelayPin2, SwitchState_2, toggleState_2, my_switch2},
    {SwitchPin3, RelayPin3, SwitchState_3, toggleState_3, my_switch3},
    {SwitchPin4, RelayPin4, SwitchState_4, toggleState_4, my_switch4},
  };

  for (auto &s : switches) {
    if (digitalRead(s.switchPin) == LOW && s.switchState == LOW) {
      digitalWrite(s.relayPin, LOW);
      s.toggleState = 1;
      s.switchState = HIGH;
      s.device.updateAndReportParam(ESP_RMAKER_DEF_POWER_NAME, s.toggleState);
    } else if (digitalRead(s.switchPin) == HIGH && s.switchState == HIGH) {
      digitalWrite(s.relayPin, HIGH);
      s.toggleState = 0;
      s.switchState = LOW;
      s.device.updateAndReportParam(ESP_RMAKER_DEF_POWER_NAME, s.toggleState);
    }
  }
}

void setup()
{
  Serial.begin(115200);

  pinMode(RelayPin1, OUTPUT);
  pinMode(RelayPin2, OUTPUT);
  pinMode(RelayPin3, OUTPUT);
  pinMode(RelayPin4, OUTPUT);
  pinMode(wifiLed, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  digitalWrite(buzzerPin, LOW);

  pinMode(SwitchPin1, INPUT_PULLUP);
  pinMode(SwitchPin2, INPUT_PULLUP);
  pinMode(SwitchPin3, INPUT_PULLUP);
  pinMode(SwitchPin4, INPUT_PULLUP);
  pinMode(gpio_reset, INPUT);

  digitalWrite(RelayPin1, !toggleState_1);
  digitalWrite(RelayPin2, !toggleState_2);
  digitalWrite(RelayPin3, !toggleState_3);
  digitalWrite(RelayPin4, !toggleState_4);
  digitalWrite(wifiLed, LOW);

  dht.begin();

  Node my_node = RMaker.initNode(nodeName);

  my_switch1.addCb(write_callback);
  my_switch2.addCb(write_callback);
  my_switch3.addCb(write_callback);
  my_switch4.addCb(write_callback);

  my_node.addDevice(my_switch1);
  my_node.addDevice(my_switch2);
  my_node.addDevice(my_switch3);
  my_node.addDevice(my_switch4);
  my_node.addDevice(temperature);
  my_node.addDevice(humidity);
  my_node.addDevice(mq6Sensor);

  Timer.setInterval(2000);

  RMaker.enableOTA(OTA_USING_PARAMS);
  RMaker.enableTZService();
  RMaker.enableSchedule();

  for (int i = 0; i < 17; i = i + 8) {
    espChipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
  }

  Serial.printf("Chip ID:  %d Service Name: %s\n", espChipId, service_name);
  RMaker.start();

  WiFi.onEvent(sysProvEvent);

#if CONFIG_IDF_TARGET_ESP32
  WiFiProv.beginProvision(WIFI_PROV_SCHEME_BLE, WIFI_PROV_SCHEME_HANDLER_FREE_BTDM, WIFI_PROV_SECURITY_1, pop, service_name);
#else
  WiFiProv.beginProvision(WIFI_PROV_SCHEME_SOFTAP, WIFI_PROV_SCHEME_HANDLER_NONE, WIFI_PROV_SECURITY_1, pop, service_name);
#endif

  my_switch1.updateAndReportParam(ESP_RMAKER_DEF_POWER_NAME, false);
  my_switch2.updateAndReportParam(ESP_RMAKER_DEF_POWER_NAME, false);
  my_switch3.updateAndReportParam(ESP_RMAKER_DEF_POWER_NAME, false);
  my_switch4.updateAndReportParam(ESP_RMAKER_DEF_POWER_NAME, false);
}

void loop()
{
  if (digitalRead(gpio_reset) == LOW) {
    delay(100);
    int startTime = millis();
    while (digitalRead(gpio_reset) == LOW) delay(50);
    int endTime = millis();

    if ((endTime - startTime) > 10000) {
      RMakerFactoryReset(2);
    } else if ((endTime - startTime) > 3000) {
      RMakerWiFiReset(2);
    }
  }

  delay(100);

  if (WiFi.status() != WL_CONNECTED) {
    digitalWrite(wifiLed, LOW);
  } else {
    digitalWrite(wifiLed, HIGH);
    if (Timer.isReady()) {
      sendSensor();
      Timer.reset();
    }
  }

  manual_control();
}
