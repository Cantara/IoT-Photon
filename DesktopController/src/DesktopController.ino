// This #include statement was automatically added by the Particle IDE.
#include <MQTT.h>

// This #include statement was automatically added by the Particle IDE.
#include <ArduinoJson.h>
#include "Particle.h"



// IMPORTANT: Set pixel COUNT, PIN and TYPE
#define UP_PIN D6
#define DOWN_PIN D3

bool GO = LOW;
bool STOP = HIGH;

int moving = 0;
unsigned long stopMovingAt = 0;



void callback(char* topic, byte* payload, unsigned int length);

/**
 * if want to use IP address,
 * byte server[] = { XXX,XXX,XXX,XXX };
 * MQTT client(server, 1883, callback);
 * want to use domain name,
 * MQTT client("www.sample.com", 1883, callback);
 **/
MQTT client("mqttdev.cantara.no", 1883, callback);

void moveUp(uint16_t timeMs){
  client.publish("device/status", "DesktopController: moveUp");
  moving = 1;
  stopMovingAt = (long)(millis() + timeMs);
  digitalWrite(UP_PIN,GO);
}

void moveDown(uint16_t timeMs){
  client.publish("device/status", "DesktopController: moveDown");
  moving = 1;
  stopMovingAt = (long)(millis() + timeMs);
  digitalWrite(DOWN_PIN,GO);
}

void stopMotion() {
  digitalWrite(UP_PIN,STOP);
  digitalWrite(DOWN_PIN,STOP);
  if (moving > 0) {
      client.publish("device/status", "DesktopController: stopMotion");
  }
  moving = 0;
}



// for QoS2 MQTTPUBREL message.
// this messageid maybe have store list or array structure.
uint16_t qos2messageid = 0;


// recieve message
void callback(char* topic, byte* payload, unsigned int length) {
    char p[length + 1];
    memcpy(p, payload, length);
    p[length] = NULL;
    client.publish("device/status", "DesktopController connected.", MQTT::QOS0);
    StaticJsonBuffer<200> jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(p);
    if (root.success()){
        const char* command = root["command"];
        String str(command);
        uint16_t timeMs = root["timeMs"];
        if (str == "UP"){
            moveUp(timeMs);
        }
        if (str == "DOWN"){
            moveDown(timeMs);
        }
        if (str == "STOP"){
            stopMotion();
        }
    }
    else if (!strcmp(p, "UP")){
        client.publish("device/status", "DesktopController: UP");
        moveUp(1000);
        RGB.color(255, 0, 0);
    }
    else if (!strcmp(p, "DOWN")){
        client.publish("device/status", "DesktopController: DOWN");
        moveDown(1000);
        RGB.color(0, 255, 0);
    }
    else if (!strcmp(p, "STOP")){
        client.publish("device/status", "DesktopController: STOP");
        stopMotion();
        RGB.color(0, 0, 255);
    }

    delay(1);
}

// QOS ack callback.
// if application use QOS1 or QOS2, MQTT server sendback ack message id.
void qoscallback(unsigned int messageid) {
    Serial.print("Ack Message Id:");
    Serial.println(messageid);

    if (messageid == qos2messageid) {
        Serial.println("Release QoS2 Message");
        client.publishRelease(qos2messageid);
    }
}

void setup() {
    pinMode (UP_PIN, OUTPUT);
    pinMode (DOWN_PIN, OUTPUT);
    Serial.begin(9600);
    RGB.control(true);
    RGB.color(255, 255, 255);


    // connect to the server
    client.connect("DesktopController");

    // add qos callback. If don't add qoscallback, ACK message from MQTT server is ignored.
    client.addQosCallback(qoscallback);

    // publish/subscribe
    if (client.isConnected()) {
        // it can use messageid parameter at 4.
        uint16_t messageid;
        client.publish("device/DesktopController", "Im connected DesktopController", MQTT::QOS1, &messageid);
        Serial.println(messageid);

        // if 4th parameter don't set or NULL, application can not check the message id to the ACK message from MQTT server.
        client.publish("device/DesktopController", "hello world QOS1(message is NULL)", MQTT::QOS1);

        // QOS=2
        client.publish("device/DesktopController", "hello world QOS2", MQTT::QOS2, &messageid);
        Serial.println(messageid);

        // save QoS2 message id as global parameter.
        qos2messageid = messageid;

        client.subscribe("device/DesktopController");
    }
}

void loop() {
  if (client.isConnected()) {
    client.loop();
  }
  if (moving >= 0) {
    unsigned long now = millis();
	  if (now >= stopMovingAt) {
      stopMotion();
    }
  }



}
