/*
   MQTT Binary Sensor - Motion (PIR) for Home-Assistant - NodeMCU (ESP8266)
   https://home-assistant.io/components/binary_sensor.mqtt/

   Libraries :
    - ESP8266 core for Arduino : https://github.com/esp8266/Arduino
    - PubSubClient : https://github.com/knolleary/pubsubclient

   Sources :
    - File > Examples > ES8266WiFi > WiFiClient
    - File > Examples > PubSubClient > mqtt_auth
    - File > Examples > PubSubClient > mqtt_esp8266
    - https://learn.adafruit.com/pir-passive-infrared-proximity-motion-sensor/using-a-pir

     Schematic sensor PIR:
      - https://github.com/mertenats/open-home-automation/blob/master/ha_mqtt_binary_sensor_pir/Schematic.png
      - PIR leg 1 - VCC
      - PIR leg 2 - D1
      - PIR leg 3 - GND

     Configuration (HA) :
       binary_sensor:
        platform: mqtt
        state_topic: 'office/motion/status'
        name: 'Motion'
        sensor_class: motion

     Schematic Light1:
      - https://github.com/mertenats/open-home-automation/blob/master/ha_mqtt_light/Schematic.png
      - GND - LED - Resistor 220 Ohms - D2

     Configuration (HA) :
      light1:
        platform: mqtt
        name: 'Office light1'
        state_topic: 'office/light1/status'
        command_topic: 'office/light1/switch'
        optimistic: false

     Schematic Light2:
      - https://github.com/mertenats/open-home-automation/blob/master/ha_mqtt_light/Schematic.png
      - GND - LED - Resistor 220 Ohms - D2

     Configuration (HA) :
      light2:
        platform: mqtt
        name: 'Office light2'
        state_topic: 'office/light2/status'
        command_topic: 'office/light2/switch'
        optimistic: false

      Schematic Switch1:
       - https://github.com/mertenats/open-home-automation/blob/master/ha_mqtt_switch/Schematic.png
       - Switch leg 1 - VCC
       - Switch leg 2 - D5 - Resistor 10K Ohms - GND

     Configuration (HA) :
      switch:
        platform: mqtt
        name: 'Office Switch1'
        state_topic: 'office/switch1/status'
        command_topic: 'office/switch1/set'
        retain: true
        optimistic: false

     Schematic Switch2:
      - https://github.com/mertenats/open-home-automation/blob/master/ha_mqtt_switch/Schematic.png
      - Switch leg 1 - VCC
      - Switch leg 2 - D6 - Resistor 10K Ohms - GND

     Configuration (HA) :
      switch:
        platform: mqtt
        name: 'Office Switch2'
        state_topic: 'office/switch2/status'
        command_topic: 'office/switch2/set'
        retain: true
        optimistic: false

   Samuel M. - v1.1 - 08.2016
   If you like this example, please add a star! Thank you!
   https://github.com/mertenats/open-home-automation
*/

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <OneButton.h>

#define MQTT_VERSION MQTT_VERSION_3_1_1

// Wifi: SSID and password
const PROGMEM char* WIFI_SSID = "HomeAssistantMQTT";
const PROGMEM char* WIFI_PASSWORD = "junkilin";

// MQTT: ID, server IP, port, username and password
const PROGMEM char* MQTT_CLIENT_ID = "office";
const PROGMEM char* MQTT_SERVER_IP = "192.168.0.104";
const PROGMEM uint16_t MQTT_SERVER_PORT = 1883;
const PROGMEM char* MQTT_USER = "homeassistant";
const PROGMEM char* MQTT_PASSWORD = "raspberry";

// MQTT: topic
const PROGMEM char* MQTT_MOTION_STATUS_TOPIC = "office/motion/status";
const char* MQTT_LIGHT1_STATE_TOPIC = "office/light1/status";
const char* MQTT_LIGHT1_COMMAND_TOPIC = "office/light1/switch";
const char* MQTT_LIGHT2_STATE_TOPIC = "office/light2/status";
const char* MQTT_LIGHT2_COMMAND_TOPIC = "office/light2/switch";
const PROGMEM char* MQTT_SWITCH1_STATUS_TOPIC = "office/switch1/status";
const PROGMEM char* MQTT_SWITCH1_COMMAND_TOPIC = "office/switch1/set";
const PROGMEM char* MQTT_SWITCH2_STATUS_TOPIC = "office/switch2/status";
const PROGMEM char* MQTT_SWITCH2_COMMAND_TOPIC = "office/switch2/set";

// default payload
const PROGMEM char* MOTION_ON = "OFF";
const PROGMEM char* MOTION_OFF = "ON";
const char* LIGHT1_ON = "ON";
const char* LIGHT1_OFF = "OFF";
const char* LIGHT2_ON = "ON";
const char* LIGHT2_OFF = "OFF";
const PROGMEM char* SWITCH1_ON = "ON";
const PROGMEM char* SWITCH1_OFF = "OFF";
const PROGMEM char* SWITCH2_ON = "ON";
const PROGMEM char* SWITCH2_OFF = "OFF";

// PIR : D1
const PROGMEM uint8_t PIR_PIN = D1;
uint8_t m_pir_state = LOW; // no motion detected
uint8_t m_pir_value = 0;

// LIGHT1 : D2
const PROGMEM uint8_t LIGHT1_PIN = D2;
boolean m_light1_state = false; // light1 is turned off by default

// LIGHT2 : D3
const PROGMEM uint8_t LIGHT2_PIN = D3;
boolean m_light2_state = false; // light2 is turned off by default

// SWITCH1 : D5
const PROGMEM uint8_t SWITCH1_PIN = D5;
boolean m_switch1_state = false;

// SWITCH2 : D6
const PROGMEM uint8_t SWITCH2_PIN = D6;
boolean m_switch2_state = false;

WiFiClient wifiClient;
PubSubClient client(wifiClient);
OneButton switch1(SWITCH1_PIN, false); // false : active HIGH
OneButton switch2(SWITCH2_PIN, false); // false : active HIGH

// function called to publish the state of the switch (on/off)
void publishSwitchState() {
  if (m_switch1_state) {
    client.publish(MQTT_SWITCH1_STATUS_TOPIC, SWITCH1_ON, true);
  } else {
    client.publish(MQTT_SWITCH1_STATUS_TOPIC, SWITCH1_OFF, true);
  }

  if (m_switch2_state) {
    client.publish(MQTT_SWITCH2_STATUS_TOPIC, SWITCH2_ON, true);
  } else {
    client.publish(MQTT_SWITCH2_STATUS_TOPIC, SWITCH2_OFF, true);
  }
}

// function called on button press, toggle the state of the switch
void clickSwitch1() {

  if (m_switch1_state) {
    Serial.println("INFO: Switch1 off...");
    m_switch1_state = false;
  } else {
    Serial.println("INFO: Switch1 on...");
    m_switch1_state = true;
  }

  publishSwitchState();
}

void clickSwitch2() {
  if (m_switch2_state) {
    Serial.println("INFO: Switch2 off...");
    m_switch2_state = false;
  } else {
    Serial.println("INFO: Switch2 on...");
    m_switch2_state = true;
  }

  publishSwitchState();
}

// function called to publish the state of the pir sensor
void publishPirSensorState() {
  if (m_pir_state) {
    client.publish(MQTT_MOTION_STATUS_TOPIC, MOTION_OFF, true);
  } else {
    client.publish(MQTT_MOTION_STATUS_TOPIC, MOTION_ON, true);
  }
}

// function called to publish the state of the light (on/off)
void publishLightState() {
  if (m_light1_state) {
    client.publish(MQTT_LIGHT1_STATE_TOPIC, LIGHT1_ON, true);
  } else {
    client.publish(MQTT_LIGHT1_STATE_TOPIC, LIGHT1_OFF, true);
  }

  if (m_light2_state) {
    client.publish(MQTT_LIGHT2_STATE_TOPIC, LIGHT2_ON, true);
  } else {
    client.publish(MQTT_LIGHT2_STATE_TOPIC, LIGHT2_OFF, true);
  }
}

// function called to turn on/off the light
void setLightState() {
  if (m_light1_state) {
    digitalWrite(LIGHT1_PIN, HIGH);
    Serial.println("INFO: Turn light1 on...");
  } else {
    digitalWrite(LIGHT1_PIN, LOW);
    Serial.println("INFO: Turn light1 off...");
  }

  if (m_light2_state) {
    digitalWrite(LIGHT2_PIN, HIGH);
    Serial.println("INFO: Turn light2 on...");
  } else {
    digitalWrite(LIGHT2_PIN, LOW);
    Serial.println("INFO: Turn light2 off...");
  }
}

// function called when a MQTT message arrived
void callback(char* p_topic, byte* p_payload, unsigned int p_length) {
  // concat the payload into a string
  String payload;
  for (uint8_t i = 0; i < p_length; i++) {
    payload.concat((char)p_payload[i]);
  }

  // handle message topic
  if (String(MQTT_LIGHT1_COMMAND_TOPIC).equals(p_topic)) {
    // test if the payload is equal to "ON" or "OFF"
    if (payload.equals(String(LIGHT1_ON))) {
      if (m_light1_state != true) {
        m_light1_state = true;
        setLightState();
        publishLightState();
      }
    } else if (payload.equals(String(LIGHT1_OFF))) {
      if (m_light1_state != false) {
        m_light1_state = false;
        setLightState();
        publishLightState();
      }
    }
  }

  if (String(MQTT_LIGHT2_COMMAND_TOPIC).equals(p_topic)) {
    // test if the payload is equal to "ON" or "OFF"
    if (payload.equals(String(LIGHT2_ON))) {
      if (m_light2_state != true) {
        m_light2_state = true;
        setLightState();
        publishLightState();
      }
    } else if (payload.equals(String(LIGHT2_OFF))) {
      if (m_light2_state != false) {
        m_light2_state = false;
        setLightState();
        publishLightState();
      }
    }
  }

  if (String(MQTT_SWITCH1_COMMAND_TOPIC).equals(p_topic)) {
    // test if the payload is equal to "ON" or "OFF"
    if (payload.equals(String(SWITCH1_ON))) {
      if (m_switch1_state != true) {
        m_switch1_state = true;
        publishSwitchState();
        Serial.println("INFO message arrived: Switch1 off...");
        //clickSwitch1();
      }
    } else if (payload.equals(String(SWITCH1_OFF))) {
      if (m_switch1_state != false) {
        m_switch1_state = false;
        publishSwitchState();
        Serial.println("INFO message arrived: Switch1 on...");
      }
    }
  }

  if (String(MQTT_SWITCH2_COMMAND_TOPIC).equals(p_topic)) {
    // test if the payload is equal to "ON" or "OFF"
    if (payload.equals(String(SWITCH2_ON))) {
      if (m_switch2_state != true) {
        m_switch2_state = true;
        publishSwitchState();
        Serial.println("INFO message arrived: Switch2 off...");
        //clickSwitch2();
      }
    } else if (payload.equals(String(SWITCH2_OFF))) {
      if (m_switch2_state != false) {
        m_switch2_state = false;
        publishSwitchState();
        Serial.println("INFO message arrived: Switch2 on...");
      }
    }
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("INFO: Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASSWORD)) {
      Serial.println("INFO: connected");
      // Once connected, publish an announcement...
      publishLightState();
      publishSwitchState();
      // ... and resubscribe
      client.subscribe(MQTT_LIGHT1_COMMAND_TOPIC);
      client.subscribe(MQTT_LIGHT2_COMMAND_TOPIC);
      client.subscribe(MQTT_SWITCH1_COMMAND_TOPIC);
      client.subscribe(MQTT_SWITCH2_COMMAND_TOPIC);
    } else {
      Serial.print("ERROR: failed, rc=");
      Serial.print(client.state());
      Serial.println(" DEBUG: try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  // init the serial
  Serial.begin(115200);

  // link the click function to be called on a single click event.
  switch1.attachLongPressStart(clickSwitch1);
  switch2.attachLongPressStart(clickSwitch2);

  // init the lights
  pinMode(LIGHT1_PIN, OUTPUT);
  pinMode(LIGHT2_PIN, OUTPUT);
  setLightState();

  // init the WiFi connection
  Serial.println();
  Serial.println();
  Serial.print("INFO: Connecting to ");
  Serial.println(WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  // Fixed IP
  IPAddress IP(192,168,0,200);
  IPAddress GATEWAY(192,168,0,1);
  IPAddress SUBNET(255,255,255,0);
  WiFi.config(IP, GATEWAY, SUBNET);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("INFO: WiFi connected");
  Serial.println("INFO: IP address: ");
  Serial.println(WiFi.localIP());

  // init the MQTT connection
  client.setServer(MQTT_SERVER_IP, MQTT_SERVER_PORT);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // keep watching the push buttons:
  switch1.tick();
  // delay(10);
  switch2.tick();
  // delay(10);

  // read the PIR sensor
  m_pir_value = digitalRead(PIR_PIN);
  if (m_pir_value == HIGH) {
    if (m_pir_state == LOW) {
      // a motion is detected
      Serial.println("INFO: Motion detected");
      publishPirSensorState();
      m_pir_state = HIGH;
    }
  } else {
    if (m_pir_state == HIGH) {
      publishPirSensorState();
      Serial.println("INFO: Motion ended");
      m_pir_state = LOW;
    }
  }
  // delay(10);
}
