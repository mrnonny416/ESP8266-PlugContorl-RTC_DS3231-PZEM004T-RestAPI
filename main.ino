#include <ESP8266WiFi.h>        //Connect WIFI To INTERNET
#include <PZEM004Tv30.h>        //Capure CIRCUIT value
#include <RTClib.h>             //Time Counter Module
#include <ArduinoJson.h>        //Manage JSON File
#include <ESP8266HTTPClient.h>  //API Connection


//Digital Port Initial-----
#define LED_PIN1 D0  //พอร์ทเชื่อมต่อ ดิจิตอลช่องที่ 0
#define LED_PIN2 D3  //พอร์ทเชื่อมต่อ ดิจิตอลช่องที่ 3
#define LED_PIN3 D4  //พอร์ทเชื่อมต่อ ดิจิตอลช่องที่ 4
#define LED_PIN4 D7  //พอร์ทเชื่อมต่อ ดิจิตอลช่องที่ 7

//API Method Initial-----
#define SERVER_PORT 8888 // Port ที่ใช้เชื่อมต่อกับ Server ของ API
const char* server_ip = "ln-web.ichigozdata.win"; // URL Domain ที่ API ใช้งานอยู่

//Time Counter By DS3231 Initial-----
RTC_DS3231 RTC; // ประการศตัวแปร RTC ให้ใช้งานโมดูล

//WIFI Variable initial-----
// const char* ssid = "DonausDev_2G";
const char* ssid = "3BB_Ni-Nack-Non_2.4GHz";
// const char* password = "1212312121A";
const char* password = "0850393221";
char WiFi_Disconnected[50];
char WiFi_Connected[50];

//Electic Mornitor By PZEM-004T Initial-----
PZEM004Tv30 pzem(
  D5, D6);

//API Method Initial-----
HTTPClient http;
WiFiClient client;

//json file
DynamicJsonDocument doc1(2048);
DynamicJsonDocument doc2(2048);
DynamicJsonDocument doc3(1024);

//Set Timer Variable
const long days_delay = 86400000;
bool wera = 1;



void setup() {
  pinMode(LED_PIN1, OUTPUT);
  pinMode(LED_PIN2, OUTPUT);
  pinMode(LED_PIN3, OUTPUT);
  pinMode(LED_PIN4, OUTPUT);
  //Active Low
  digitalWrite(LED_PIN1, HIGH);
  digitalWrite(LED_PIN2, HIGH);
  digitalWrite(LED_PIN3, HIGH);
  digitalWrite(LED_PIN4, HIGH);

  Serial.begin(115200);

  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  Wire.begin();
  if (!RTC.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
  }
  if (RTC.lostPower()) {
    Serial.println("RTC lost power, let's set the time!");
  }
}

void loop() {
  DateTime now = RTC.now();
  Serial.print("day : ");
  Serial.println(now.day());
  int days = now.day();
  int nub = 26;
  Serial.print("days : ");
  Serial.println(days);
  float power = pzem.power();
  float energy = pzem.energy();
  float current = pzem.current();

  //---------------------------------Fetching Data----------------------------------
  if (!client.connect(server_ip, SERVER_PORT)) {
    Serial.println("Connection API failed");
    return;
  }


  String url = "/channels/all";
  client.print(String("GET ") + url + " HTTP/1.1\r\n" + "Host: " + server_ip + "\r\n" + "Connection: close\r\n\r\n");

  while (!client.available())
    ;
  String response = client.readString();
  int bodyStart = response.indexOf("\r\n\r\n");
  String body = response.substring(bodyStart + 4);

  DeserializationError error = deserializeJson(doc1, body);

  if (error) {
    Serial.println("Parsing failed");
    return;
  }
  //---------------------------------check Safety------------------------------------
  if (current != NAN) {
    Serial.print("Power: ");
    Serial.print(power);
    Serial.println("W");
    if (check_safety(power)) {
      for (int i = 0; i < 4; i++) {
        Serial.println("---------!!!Power Over Load!!!-------------");
        CONTROL_PLUG_ON_OFF(i + 1, false);
      }
    } else {
      Serial.println("--------- Power Not Over Load 🙂 ---------");
      for (int i = 0; i < 4; i++) {
        if (!doc1["data"][i]["templateId"] || !doc1["data"][i]["templatestatus"]) {
          CONTROL_PLUG_ON_OFF(doc1["data"][i]["channelId"], doc1["data"][i]["status"]);  //<-----------------call function Control on off
        } else {
          time_plug_on_off(doc1["data"][i]["channelId"], doc1["data"][i]["status"], doc1["data"][i]["templateId"], doc1["data"][i]["templatestatus"], now);  //<---------call function time_plug_on_off
        }
      }
    }
  } else {
    Serial.println("Error reading power");
  }

  //---------------------------------send energy------------------------------------

  if ((nub == days) && (wera == 1)) {
    wera = 0;
    if (energy > 0) {
      report_Power(energy);  //<-----------------------------------call function report_Power
      Serial.println("update energy !");
    }
  } else {
    Serial.println("Error read energy");
  }
  if ((nub != days) && (wera == 0)) {
    wera = 1;
  }

  return;
}  // -- End Loop()
//------------------control_plug_on_off-------------------------------------------
void CONTROL_PLUG_ON_OFF(int channelId, bool status) {
  int LED_PIN;
  switch (channelId) {
    case 1:
      LED_PIN = LED_PIN1;
      break;
    case 2:
      LED_PIN = LED_PIN2;
      break;
    case 3:
      LED_PIN = LED_PIN3;
      break;
    case 4:
      LED_PIN = LED_PIN4;
      break;
    default:
      LED_PIN = 0;
      break;
  }
  digitalWrite(LED_PIN, (status ? LOW : HIGH));
}
//----------------------------------time_plug_on_off----------------------------------------
void time_plug_on_off(int channelId, bool status, int templateId, bool templatestatus, DateTime now) {
  int LED_PIN;
  switch (channelId) {
    case 1:
      LED_PIN = LED_PIN1;
      break;
    case 2:
      LED_PIN = LED_PIN2;
      break;
    case 3:
      LED_PIN = LED_PIN3;
      break;
    case 4:
      LED_PIN = LED_PIN4;
      break;
    default:
      LED_PIN = 0;
      break;
  }
  Serial.print("Have Template : ID :");  //<-----------------check Template Get
  Serial.print(templateId);              //<-----------------check Template Get
  String url = "/template/detail?id=" + String(templateId);
  Serial.println(url);  //<-----------------check Template Get

  if (!client.connect(server_ip, SERVER_PORT)) {
    Serial.println("Connection API failed");
    return;
  }
  client.print(String("GET ") + url + " HTTP/1.1\r\n" + "Host: " + server_ip + "\r\n" + "Connection: close\r\n\r\n");
  while (!client.available())
    ;
  String response = client.readString();
  int bodyStart = response.indexOf("\r\n\r\n");
  String body = response.substring(bodyStart + 4);
  DeserializationError error = deserializeJson(doc2, body);
  Serial.println("Fetch Complete");  //<-----------------check Template Get
  if (error) {
    Serial.print(response);
    Serial.println("Parsing doc 2 failed");  //<-----------------check Template Get
    return;
  }
  if (doc2["data"]["type"]) {       //type is 1 mean time set
    Serial.print("Get type 1 : ");  //<-----------------check Template Get
    String dateStart = doc2["data"]["datestart"];
    String dateEnd = doc2["data"]["dateend"];
    int dayStart = dateStart.substring(8, 10).toInt();
    int monthStart = dateStart.substring(5, 7).toInt();
    int yearStart = dateStart.substring(0, 4).toInt();
    int dayEnd = dateStart.substring(8, 10).toInt();
    int monthEnd = dateStart.substring(5, 7).toInt();
    int yearEnd = dateStart.substring(0, 4).toInt();
    Serial.print(dateStart);  //<-----------------check Template Get
    Serial.print("to");       //<-----------------check Template Get
    Serial.print(dateEnd);    //<-----------------check Template Get

    if (now.year() >= yearStart && now.year() <= yearEnd && now.month() >= monthStart && now.month() <= monthEnd && now.day() >= dayStart && now.day() <= dayEnd) {
      // date is within range]
      Serial.print(" -- status In date");  //<-----------------check Template Get


      String timestart = doc2["data"]["timestart"];
      int hour_start = timestart.substring(0, 2).toInt();
      int minute_start = timestart.substring(3, 5).toInt();
      String timeend = doc2["data"]["timeend"];
      int hour_end = timeend.substring(0, 2).toInt();
      int minute_end = timeend.substring(3, 5).toInt();
      if (now.hour() >= hour_start && now.minute() >= minute_start && now.hour() <= hour_end && now.minute() <= minute_end) {
        digitalWrite(LED_PIN, LOW);
        Serial.print(" -- status In time");  //<-----------------check Template Get
        Serial.println("");                  //<-----------------check Template Get
      } else {
        digitalWrite(LED_PIN, HIGH);
        Serial.print(" -- status OUT time");  //<-----------------check Template Get
        Serial.println("");                   //<-----------------check Template Get
      }
    }
  } else {                            //type is 0 mean weekly set
    Serial.println("Get type 0 : ");  //<-----------------check Template Get
    if (doc2["data"]["daystart"] <= now.dayOfTheWeek() && doc2["data"]["dayend"] >= now.dayOfTheWeek()) {
      String timestart = doc2["data"]["timestart"];
      int hour_start = timestart.substring(0, 2).toInt();
      int minute_start = timestart.substring(3, 5).toInt();
      String timeend = doc2["data"]["timeend"];
      int hour_end = timeend.substring(0, 2).toInt();
      int minute_end = timeend.substring(3, 5).toInt();
      Serial.print(timestart);  //<-----------------check Template Get
      Serial.print("to");       //<-----------------check Template Get
      Serial.print(timeend);    //<-----------------check Template Get
      if (now.hour() >= hour_start && now.minute() >= minute_start && now.hour() <= hour_end && now.minute() <= minute_end) {
        Serial.print(" -- status In time");  //<-----------------check Template Get
        Serial.println("");                  //<-----------------check Template Get
        digitalWrite(LED_PIN, LOW);
      } else {
        Serial.print(" -- status Out time");  //<-----------------check Template Get
        Serial.println("");                   //<-----------------check Template Get
        digitalWrite(LED_PIN, HIGH);
      }
    }
  }
}

void report_Power(float power_unit) {
  http.begin(client, "http://ln-web.ichigozdata.win:8888/saveunit/add");
  http.addHeader("Content-Type", "application/json");
  StaticJsonDocument<200> doc;
  doc["unit"] = power_unit;
  doc["date"] = nullptr;

  String json;
  serializeJson(doc, json);

  int httpCode = http.POST(json);
  if (httpCode > 0) {
    String response = http.getString();
    Serial.println(response);
  } else {
    Serial.println("Error sending data to server");
  }

  http.end();
}

bool check_safety(float power) {
  if (!client.connect(server_ip, SERVER_PORT)) {
    Serial.println("Connection API failed");
    return false;
  }

  String url = "/savety/all";
  client.print(String("GET ") + url + " HTTP/1.1\r\n" + "Host: " + server_ip + "\r\n" + "Connection: close\r\n\r\n");

  while (!client.available())
    ;
  String response = client.readString();
  int bodyStart = response.indexOf("\r\n\r\n");
  String body = response.substring(bodyStart + 4);
  DeserializationError error = deserializeJson(doc3, body);

  if (error) {
    Serial.println("Parsing failed");
    return false;
  }

  if (power > doc3["data"]["volt"] && doc3["data"]["status"]) {
    return true;
  } else {
    Serial.println("");
    Serial.print("Safety Over Load at : ");
    Serial.print(doc3["data"]["volt"] ? doc3["data"]["volt"] : 0);
    Serial.println(" W");
    return false;
  }
}