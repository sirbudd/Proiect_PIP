#include "Constants.h"
#include "Types.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h> // Includem clientul de MQTT
#include <MPU6050.h> // Senzorul de temperatura
#include <NewPing.h> // Senzorul de proximitate

//FOLOSIM VOLATILE PT A NU OPTIMIZA.
volatile int g_sensor_used = SensorEnabled::NONE; //SENZORUL FOLOSIT AT THIS MOMENT.   NONE = NU FOLOSIM SENZORI

// The Object Interacts with ESP8266 Module
WiFiClient g_esp8266_wifi_client;

// Aici se face conectarea la WIFI.
void connectToWifi()
{
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print("Unable to connect to ");
    Serial.print(WIFI_SSID);
    Serial.print(" WiFi");
    Serial.println();
    delay(2000 /*Wait for 2 Seconds*/);
    Serial.print("Retrying...");
    Serial.println();
  }
  Serial.print("Connected to ");
  Serial.println(WIFI_SSID);
}

// Mqtt Client Defined Here
PubSubClient g_mqtt_client{};

volatile unsigned long g_prev_proximity_reading = 0; // Store the previous reading of Proximity Sensor
volatile int16_t g_prev_temperature_reading = 0; // Stores the previous reading of the Temperature Sensor

// AICI SE FACE CONECTAREA LA MQTT
void connectToMQTT()
{
  g_mqtt_client.setClient(g_esp8266_wifi_client);
  g_mqtt_client.setServer(MQTT_BROKER_SERVER, MQTT_BROKER_PORT);
  g_mqtt_client.setCallback(&mqtt_callback);

  while (!g_mqtt_client.connect(MQTT_CLIENT_ID, MQTT_BROKER_USER_NAME, MQTT_BROKER_KEY))
  {
    Serial.print("Unable to Connect to ");
    Serial.print(MQTT_BROKER_USER_NAME);
    Serial.print(" at ");
    Serial.print(MQTT_BROKER_SERVER);
    Serial.println();
    delay(2000 /*Wait for 2 Seconds*/);
    Serial.print("Retrying...");
    Serial.println();
  }
  Serial.print("MQTT Connected");
}

// FUNCTIE CA SA VERIFICAM DACA INFORMATIA A FOST TRIMISA
void mqtt_callback(char* p_topic, byte* p_payload, unsigned int p_payload_length)
{
  if (!g_mqtt_client.connected() || strcmp(p_topic, MQTT_TOPIC) != 0)   // Check if the Call Back is About the right topic
    return;


  const String msg = convertFromByteArray(p_payload, p_payload_length);

  // Disable All Sensors
  if (msg == MSG_DISABLE_SENSOR)
  {
    // Disable All Sensors
    g_sensor_used = SensorEnabled::NONE;
  }
  else if (msg == MSG_PROXIMITY_SENSOR_ENABLE)
  {
    g_sensor_used = SensorEnabled::PROXIMITY;
    g_prev_proximity_reading = 0;     // Reset Proximity Sensor Reading         0 is sent when no object detected
  }
  else if (msg == MSG_TEMPERATURE_SENSOR_ENABLE)
  {
    g_sensor_used = SensorEnabled::TEMPERATURE;
    g_prev_temperature_reading = 0;     // Reset Temperature Sensor Reading
  }
  else if (msg == MSG_LED_ON)
  {
    led_on();
  }
  else if (msg == MSG_LED_OFF)
  {
    led_off();
  }
}


//AICI TRIMITEM INFORMATIA
void sendMqttMessage(const char* msg_send)
{
  if (!g_mqtt_client.connected())  // Check if it is connected or not
    return;

  g_mqtt_client.publish(MQTT_TOPIC, msg_send);
}
void sendMqttMessage(const StringSumHelper& p_msg_send)
{
  sendMqttMessage(p_msg_send.c_str());
}
void sendMqttMessage(const byte* p_msg_send, const unsigned int p_length)
{
  if (!g_mqtt_client.connected())   // Check if it is connected or not
    return;

  g_mqtt_client.publish(MQTT_TOPIC, p_msg_send, p_length);
}

// FUNCTIA PT CONTROLUL LEDULUI
void led_on()
{
  digitalWrite(LED_CONNECTED, HIGH);
}
void led_off()
{
  digitalWrite(LED_CONNECTED, LOW);
}


// FUNCTII PENTRU SENZORUL DE TEMPERATURA
MPU6050 g_temperature_sensor;
void connectToTemperatureSensor()
{
  Wire.begin();

  g_temperature_sensor.initialize(); //INITIALIZARE SENZOR TEMPERATURA

  while (!g_temperature_sensor.testConnection())
  {
    Serial.print("Unable to connect to Temperature Sensor");
    Serial.println();
    delay(2000/*Wait for 2 Seconds*/);
    Serial.print("Retrying...");
    Serial.println();
  }
}

void sendTemperatureSensorReadingViaMQTT()
{
  if (g_sensor_used != SensorEnabled::TEMPERATURE) //VERIF DACA SENZORUL ESTE BUSY
    return;
  if (!g_mqtt_client.connected())
    return;

  // Celsius = (temp/340 + 36.53)
  const int16_t temperature_current = (static_cast<float>(g_temperature_sensor.getTemperature()) / 340.0f + 36.53f);

  if (temperature_current < TEMPERATURE_MIN_CELSIUS && g_prev_temperature_reading != 0)
  {
    sendMqttMessage("TSS:0");
    g_prev_temperature_reading = 0;
  }
  else if (g_prev_temperature_reading != temperature_current)
  {
    sendMqttMessage(String(MSG_TEMPERATURE_SENSOR_START_CODE) + String(temperature_current));
    // Make Previous Temperature Sensor Reading to Current Reading
    g_prev_temperature_reading = temperature_current;
  }
}


//FUNCTII PENTRU SENZORUL DE PROXIMITATE
NewPing g_proximity_sensor{ PROXIMITY_TRIGGER, PROXIMITY_ECHO, PROXIMITY_MAX_DIST_CM };

void sendProximitySensorReadingViaMQTT()
{

  if (g_sensor_used != SensorEnabled::PROXIMITY)   // VERIFICAM DACA Proximity Sensor is Enabled or Not
  if (!g_mqtt_client.connected())
    return;

  const unsigned long distance_of_obj = g_proximity_sensor.ping_cm(PROXIMITY_MAX_DIST_CM);

  if (distance_of_obj < PROXMITY_MIN_DIST_CM && g_prev_proximity_reading != 0)
  {
    sendMqttMessage("PSS:0");
    g_prev_proximity_reading = 0;
  }

  else if (distance_of_obj != g_prev_proximity_reading)
  {
    sendMqttMessage(String(MSG_PROXIMITY_SENSOR_START_CODE) + String(distance_of_obj));
    // Make Previous Proximity Sensor Reading to Current Reading
    g_prev_proximity_reading = distance_of_obj;
  }
}

//Functia care ruleaza la rulare
void setup()
{
  // Set the Baud Rate 
  Serial.begin(9600);

  pinMode(LED_CONNECTED, OUTPUT);

  //Connect to temperature sensor
  connectToTemperatureSensor();

  connectToWifi();
  connectToMQTT();

  // Subscribe to a given topic
  g_mqtt_client.subscribe(MQTT_TOPIC);

  // Send Message that Arduino Device is ON
  sendMqttMessage(MSG_ARDUINO_ON);

  // LED Must be Off by Default
  led_off();

}

// Add the main program code into the continuous loop() function
void loop()
{
  static uint32_t loop_time_prev = millis();
  const uint32_t loop_time_cur = millis();

  g_mqtt_client.loop();
  // Run Mqtt Loop only in a fixed time duration
  if ((loop_time_cur - loop_time_prev) > MIN_TIME_LOOP_CHECK_MS)
  {
    g_mqtt_client.loop();
    loop_time_prev = loop_time_cur;
  }

  static uint32_t data_send_time_prev = millis();
  // Get Current Time
  const uint32_t data_send_time_cur = millis();

  if ((data_send_time_cur - data_send_time_prev) > MIN_TIME_DATA_SEND_MS)
  {
    switch (g_sensor_used)
    {
      case SensorEnabled::PROXIMITY:
        sendProximitySensorReadingViaMQTT();
        break;
      case SensorEnabled::TEMPERATURE:
        sendTemperatureSensorReadingViaMQTT();
        break;
      default:
        break;
    }
    data_send_time_prev = data_send_time_cur;
  }
}
