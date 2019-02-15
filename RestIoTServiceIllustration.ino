#include <stdio.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>

#define HTTP_REST_PORT 80
#define WIFI_RETRY_DELAY 500
#define MAX_WIFI_INIT_RETRY 50

//-------------------------
// defines pins numbers
const int trigPin = 2;  //D4
const int echoPin = 0;  //D3

// defines variables
long duration;
double distance;

//---------------------

byte led_gpio = 12;
byte led_id = 1;
byte led_status = 0;

const char* wifi_ssid = "WiFi-2.4-7313";
const char* wifi_passwd = "C79218B603";

ESP8266WebServer http_rest_server(HTTP_REST_PORT);

int init_wifi() {
    int retries = 0;
    Serial.println("Connecting to WiFi AP..........");
    WiFi.mode(WIFI_STA);
    WiFi.begin(wifi_ssid, wifi_passwd);
    // check the status of WiFi connection to be WL_CONNECTED
    while ((WiFi.status() != WL_CONNECTED) && (retries < MAX_WIFI_INIT_RETRY)) {
        retries++;
        delay(WIFI_RETRY_DELAY);
        Serial.print("#");
    }
    return WiFi.status(); // return the WiFi connection status
}

void get_ic_Light_Warning() {
    StaticJsonBuffer<200> jsonBuffer;
    JsonObject& jsonObj = jsonBuffer.createObject();
    char JSONmessageBuffer[200];
    if (led_status == 0) 
      jsonObj["status"] = "The light signal is Off";
    else
        jsonObj["status"] = "The light signal is On";    
    jsonObj.prettyPrintTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
    http_rest_server.send(200, "application/json", JSONmessageBuffer);
}

void put_ic_Light_Warning() {
    StaticJsonBuffer<500> jsonBuffer;
    String post_body = http_rest_server.arg("plain");
    Serial.println(post_body);
    JsonObject& jsonBody = jsonBuffer.parseObject(http_rest_server.arg("plain"));
    Serial.print("HTTP Method: ");
    Serial.println(http_rest_server.method());
    if (!jsonBody.success()) {
        Serial.println("error in parsin json body");
        http_rest_server.send(400);
    }
    else {   
         if (http_rest_server.method() == HTTP_PUT) {
            if ((jsonBody["status"].as<String>() == "switch-on") or (jsonBody["status"].as<String>() == "switch-off")) {
                if (jsonBody["status"].as<String>() == "switch-on") {
                    led_status=1;
                    http_rest_server.sendHeader("Location", "/ic-Light-Warning/" + jsonBody["status"].as<String>());
                    http_rest_server.send(200);                
                    digitalWrite(led_gpio, led_status);
                }
                else
                    if (jsonBody["status"].as<String>() == "switch-off") {
                        led_status=0;
                        http_rest_server.sendHeader("Location", "/ic-Light-Warning/" + jsonBody["status"].as<String>());
                        http_rest_server.send(200);                
                        digitalWrite(led_gpio, led_status);
                    }
                    else
                        http_rest_server.send(503);  
            }
            else
              http_rest_server.send(502);
        }
        else
            http_rest_server.send(404);        
    }
}

void put_ic_Obstacle_Warning() {
    StaticJsonBuffer<500> jsonBuffer;
    String post_body = http_rest_server.arg("plain");
    JsonObject& jsonBody = jsonBuffer.parseObject(http_rest_server.arg("plain"));
    if (!jsonBody.success()) 
        http_rest_server.send(400);
    else {   
         if (http_rest_server.method() == HTTP_PUT) {
            StaticJsonBuffer<200> jsonBuffer;
            JsonObject& jsonObj = jsonBuffer.createObject();
            char JSONmessageBuffer[200];
            if (jsonBody["obstacle"].as<double>() < 1.5)  
                jsonObj["status"] = "switch-on";
            else
                jsonObj["status"] = "switch-off";
            jsonObj.prettyPrintTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
            http_rest_server.send(200, "application/json", JSONmessageBuffer);
        }
        else
            http_rest_server.send(404);        
    }
}

void get_ic_Check_4_Obstacle() {
    StaticJsonBuffer<200> jsonBuffer;
    JsonObject& jsonObj = jsonBuffer.createObject();
    char JSONmessageBuffer[200];

    // Clears the trigPin
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    
    // Sets the trigPin on HIGH state for 10 micro seconds
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
    
    // Reads the echoPin, returns the sound wave travel time in microseconds
    duration = pulseIn(echoPin, HIGH);
    
    // Calculating the distance
    distance= duration*0.034/2;
    // Prints the distance on the Serial Monitor
    Serial.print("Distance: ");
    Serial.println(distance);
    jsonObj["obstacle"] = distance/100;    
    jsonObj.prettyPrintTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
    http_rest_server.send(200, "application/json", JSONmessageBuffer);
}

void config_rest_server_routing() {
    http_rest_server.on("/", HTTP_GET, []() {
        http_rest_server.send(200, "text/html",
            "Welcome to the ESP8266 REST Web Server");
    });
    http_rest_server.on("/ic-Light-Warning", HTTP_GET, get_ic_Light_Warning);
    http_rest_server.on("/ic-Light-Warning", HTTP_PUT, put_ic_Light_Warning);
    http_rest_server.on("/ic-Obstacle-Warning", HTTP_PUT, put_ic_Obstacle_Warning);
    http_rest_server.on("/ic-Check-4-Obstacle", HTTP_GET, get_ic_Check_4_Obstacle);
}

void setup(void) {
    Serial.begin(115200);
    
    pinMode(led_gpio, OUTPUT);
    pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
    pinMode(echoPin, INPUT); // Sets the echoPin as an Input

    if (init_wifi() == WL_CONNECTED) {
        Serial.print("Connetted to ");
        Serial.print(wifi_ssid);
        Serial.print("--- IP: ");
        Serial.println(WiFi.localIP());
    }
    else {
        Serial.print("Error connecting to: ");
        Serial.println(wifi_ssid);
    }
    config_rest_server_routing();
    http_rest_server.begin();
    Serial.println("HTTP REST Server Started");
}

void loop(void) {
    http_rest_server.handleClient();
}
