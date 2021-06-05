package com.arduino;

public class Constants {

    public static final String MQT_CLIENT_ID = "MQTT Desktop Application";

    public static final String MQTT_BROKER = "tcp://io.adafruit.com:1883";

    public static final String APP_NAME = "Arduino Communicator DLDA";

    public static final String MQTT_USER_NAME = "sirbudd";
    public static final char[] MQTT_USER_KEY = "aio_XzgX05VkB0EDixeaGTY2abk9JFwh".toCharArray();

    public static final String MQTT_TOPIC = MQTT_USER_NAME + "/feeds/proiect";

    // Message sent by Arduino When it Starts
    public static final String ARDUINO_ON = "ARDUINO_ON";

    public static final String PROXIMITY_SENSOR_VALUE_START_WITH = "PSS";
    public static final String TEMPERATURE_SENSOR_VALUE_START_WITH = "TSS";

    public static final String ENABLE_PROXIMITY_SENSOR = "ENABLE_" + PROXIMITY_SENSOR_VALUE_START_WITH;
    public static final String ENABLE_TEMPERATURE_SENSOR = "ENABLE_" + TEMPERATURE_SENSOR_VALUE_START_WITH;

    public static final String DISABLE_SENSORS = "DISABLE";

    public static final String LOCK_FILE_NAME = "MQTT_LOCK.lck";
}
