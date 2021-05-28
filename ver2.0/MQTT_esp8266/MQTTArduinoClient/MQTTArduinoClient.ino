#include <ESP8266WiFi.h>
#include <MPU6050.h>
#include <PubSubClient.h> //Header to include the MQTT Client
#include "Constants.h"
#include "Types.h"


volatile int g_sensor_used = SensorEnabled::NONE; // Defines the sensor used currently

WiFiClient g_esp8266_wifi_client;

// This Function Connects to the Wifi
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

// Store the previous reading of Proximity Sensor
// Helps Minimise the amount of data sent
// volatile Ensure this variable is not Optimised Away
volatile unsigned long g_prev_proximity_reading = 0;

// Stores the previous reading of the Temperature Sensor
// Helps Minimise the amount of data sent
// volatile Ensure this variable is not Optimised Away
volatile int16_t g_prev_temperature_reading = 0;

// This function connects to MQTT
void connectToMQTT()
{
  g_mqtt_client.setClient(g_esp8266_wifi_client);
  g_mqtt_client.setServer(MQTT_BROKER_SERVER, MQTT_BROKER_PORT);
  // This is a function that gets called
  // When callback is required
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

// The Function is called when information gets transferred
void mqtt_callback(char* p_topic, byte* p_payload, unsigned int p_payload_length)
{
  // Check if the Call Back is About the right topic
  // It's possible our account has subscribed to multiple topics
  // If not, do nothing
  if (!g_mqtt_client.connected() || strcmp(p_topic, MQTT_TOPIC) != 0)
    return;

  // Note here that
  // Message is in the form Byte
  // And needs to be converted to char type
  // As such we need to Call convert Function
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
    // Reset Proximity Sensor Reading
    // 0 is sent when no object detected
    g_prev_proximity_reading = 0;
  }
  else if (msg == MSG_TEMPERATURE_SENSOR_ENABLE)
  {
    g_sensor_used = SensorEnabled::TEMPERATURE;
    // Reset Temperature Sensor Reading
    g_prev_temperature_reading = 0;
  }
  else if (msg == MSG_LED_ON)
  {
    // If Message Asks to Enable LED,
    // Enable LED
    led_on();
  }
  else if (msg == MSG_LED_OFF)
  {
    // If Message Asks to Disable LED,
    // Disable LED
    led_off();
  }
}

void sendMqttMessage(const char* msg_send)
{
  // Check if it is connected or not
  if (!g_mqtt_client.connected())
    return;

  g_mqtt_client.publish(MQTT_TOPIC, msg_send);
}
void sendMqttMessage(const StringSumHelper& p_msg_send)
{
  // Convert given string
  // to character array
  sendMqttMessage(p_msg_send.c_str());
}
void sendMqttMessage(const byte* p_msg_send, const unsigned int p_length)
{
  // Check if it is connected or not
  if (!g_mqtt_client.connected())
    return;

  g_mqtt_client.publish(MQTT_TOPIC, p_msg_send, p_length);
}

// Required functions to control the given LED

// Call this function to Start Blinking the LED
void led_on()
{
  digitalWrite(LED_CONNECTED, HIGH);
}
// Call this function to Stop Blinking the LED
void led_off()
{
  digitalWrite(LED_CONNECTED, LOW);
}

// Portion dealing with Temperature Sensor

MPU6050 g_temperature_sensor;

// Verify if Temperature Sensor is connected

void connectToTemperatureSensor()
{

  Wire.begin();

  // Initialise the Temperature Sensor
  g_temperature_sensor.initialize();

  // Loop till Connection Test is Successful
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
  // Verify if the Current Sensor under use
  // is the temperature Sensor
  if (g_sensor_used != SensorEnabled::TEMPERATURE)
    return;
  // Verify if MQTT Client is connected                           //TODO SENSORS
  // Do Nothing if not connected
  if (!g_mqtt_client.connected())
    return;

  
// The setup() function runs once each time the micro-controller starts
void setup()
{
  // Set the Baud Rate to print debugging Info on Computer
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
  // This should only get initialised when the
  // loop is first executed
  static uint32_t loop_time_prev = millis();
  // Get current time
  const uint32_t loop_time_cur = millis();

  g_mqtt_client.loop();
  // Run Mqtt Loop only in a fixed time duration
  if ((loop_time_cur - loop_time_prev) > MIN_TIME_LOOP_CHECK_MS)
  {
    g_mqtt_client.loop();
    loop_time_prev = loop_time_cur;
  }

  // Run the code updation in a fixed time duration only
  static uint32_t data_send_time_prev = millis();
  // Get Current Time
  const uint32_t data_send_time_cur = millis();

  // Run Mqtt Loop Only in A fixed Time Duration
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
