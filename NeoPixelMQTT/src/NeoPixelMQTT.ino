
#include "MQTT.h"
#include "Particle.h"
#include "neopixel.h"
#include "math.h"
#include <ArduinoJson.h>
#include "NeoPixelMQTT.h"

SYSTEM_MODE(AUTOMATIC);
unsigned long nextHeartBeat = millis()+1000;


Adafruit_NeoPixel strip(PIXEL_COUNT, PIXEL_PIN, PIXEL_TYPE);




// Prototypes for local build, ok to leave in for Build IDE
void rainbow(uint8_t wait);
uint32_t Wheel(byte WheelPos);


void rainbow(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256; j++) {
    for(i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel((i+j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

void rainbowNew(uint16_t width, uint16_t centre, uint16_t shift, uint16_t freqT){
  uint16_t i;

  double red, green, blue, freq;
  freq = (double)freqT*6/1000;
  Spark.publish("freqT",10);
  for(i=0; i<strip.numPixels();i++){
    red = sin(freq*i+0+shift*freq)*width+centre;
    green = sin(freq*i+2+shift*freq)*width+centre;
    blue = sin(freq*i+4+shift*freq)*width+centre;
    strip.setColor(i, (int)green,(int)red,(int)blue);
  }
  strip.show();
}

void setStripRed(){
  uint16_t i;
  for (i=0; i<strip.numPixels();i++){
    strip.setColor(i,0,255,0);
    strip.show();
  }
}

void setStripGreen(){
  uint16_t i;
  for (i=0; i<strip.numPixels();i++){
    strip.setColor(i,255,0,0);
    strip.show();
  }
}

void setStripBlue(){
  uint16_t i;
  for (i=0; i<strip.numPixels();i++){
    strip.setColor(i,0,0,255);
    strip.show();
  }
}


// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  if(WheelPos < 85) {
   return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } else if(WheelPos < 170) {
   WheelPos -= 85;
   return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else {
   WheelPos -= 170;
   return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}

uint32_t WheelNew(byte WheelPos){

}


void callback(char* topic, byte* payload, unsigned int length);

/**
 * if want to use IP address,
 * byte server[] = { XXX,XXX,XXX,XXX };
 * MQTT client(server, 1883, callback);
 * want to use domain name,
 * MQTT client("www.sample.com", 1883, callback);
 **/
MQTT client("mqttdev.cantara.no", 1883, callback);

// for QoS2 MQTTPUBREL message.
// this messageid maybe have store list or array structure.
uint16_t qos2messageid = 0;


// recieve message
void callback(char* topic, byte* payload, unsigned int length) {
    char p[length + 1];
    memcpy(p, payload, length);
    p[length] = NULL;
    client.publish("device/status", "hello world QOS1", MQTT::QOS0);
    StaticJsonBuffer<200> jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(p);
    if (root.success()){
        uint16_t centre = root["centre"];
        uint16_t width = root["width"];
        uint16_t shift = root["shift"];
        uint16_t freqT = root["freqT"];
        RGB.color(100, 110, 0);
        rainbowNew(centre,width,shift,freqT);
    }
    else if (!strcmp(p, "RED")){
        RGB.color(255, 0, 0);
        setStripRed();
    }
    else if (!strcmp(p, "GREEN")){
      RGB.color(0, 255, 0);
      setStripGreen();
    }

    else if (!strcmp(p, "BLUE")){
      RGB.color(0, 0, 255);
      setStripBlue();
    }
    else if (!strcmp(p, "RAINBOWNEW")){
      RGB.color(0, 0, 0);
      rainbowNew(128,127,0,3);
    }

    else if (!strcmp(p, "RAINBOW"))
        rainbow(10);
    else
        RGB.color(255, 255, 255);


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
    strip.begin();
    strip.show(); // Initialize all pixels to 'off'
    Serial.begin(9600);
    RGB.control(true);

    // connect to the server
    client.connect(CLIENT_ID);

    // add qos callback. If don't add qoscallback, ACK message from MQTT server is ignored.
    client.addQosCallback(qoscallback);
    char inboundTopic[50];
    sprintf(inboundTopic, "/device/%s",CLIENT_ID);
    char statusTopic[50];
    sprintf(statusTopic, "/status/%s",CLIENT_ID);



    // publish/subscribe
    if (client.isConnected()) {
        // it can use messageid parameter at 4.
        uint16_t messageid;
        client.publish(statusTopic, "hello world QOS1", MQTT::QOS1, &messageid);
        Serial.println(messageid);

        // if 4th parameter don't set or NULL, application can not check the message id to the ACK message from MQTT server.
        client.publish(statusTopic, "hello world QOS1(message is NULL)", MQTT::QOS1);

        // QOS=2
        client.publish(statusTopic, "hello world QOS2", MQTT::QOS2, &messageid);
        Serial.println(messageid);

        // save QoS2 message id as global parameter.
        qos2messageid = messageid;

        client.subscribe(inboundTopic);
    }
}

void heartBeat(){
  if (client.isConnected()) {
    char heartBeatTopic[50];
    char statusMessage[50];
    sprintf(heartBeatTopic, "/heartbeat/%s",CLIENT_ID);
    sprintf(statusMessage, "%s is Alive!",CLIENT_ID);
    client.publish(heartBeatTopic, statusMessage, MQTT::QOS0);
    nextHeartBeat = millis()+1000;
    }
}


void loop() {
    if (client.isConnected())
        client.loop();
    unsigned long now = millis();
    if (now >= nextHeartBeat){
      heartBeat();
    }
}
