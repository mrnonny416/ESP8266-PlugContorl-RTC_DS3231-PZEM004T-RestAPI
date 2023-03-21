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
#define SERVER_PORT 8888                           // Port ที่ใช้เชื่อมต่อกับ Server ของ API
const char* server_ip = "*************";  // URL Domain ที่ API ใช้งานอยู่

//Time Counter By DS3231 Initial-----
RTC_DS3231 RTC;  // ประการศตัวแปร RTC ให้ใช้งานโมดูล

//WIFI Variable initial-----
const char* ssid = "**************";     //SSID ของ WIFI ที่ต้องใช้เชื่อมต่อ
const char* password = "**************";  //Password ของ WIFI

//Electic Mornitor By PZEM-004T Initial-----
PZEM004Tv30 pzem(D5, D6);  // ประกาศพอร์ทเชื่อมต่อให้กับ โมดูล Pzem004-T

//API Method Initial-----
HTTPClient http;    //ประกาศใช้ HTTPCLient ลงในตัวแปรชื่อ http
WiFiClient client;  //ประกาศใช้ WiFiClient ลงในตัวแปรชื่อ client

//json file
DynamicJsonDocument doc1(2048);  // สร้าง Json ขนาด 2048 byte
DynamicJsonDocument doc2(2048);  // สร้าง Json ขนาด 2048 byte
DynamicJsonDocument doc3(1024);  // สร้าง Json ขนาด 1024 byte

//Set Timer Variable
const long days_delay = 86400000;
bool wera = 1;
//-------------------------------setup-----------------------------------
void setup() {
  pinMode(LED_PIN1, OUTPUT);  //ให้พอร์ท ส่งกำลังไฟออก
  pinMode(LED_PIN2, OUTPUT);  //ให้พอร์ท ส่งกำลังไฟออก
  pinMode(LED_PIN3, OUTPUT);  //ให้พอร์ท ส่งกำลังไฟออก
  pinMode(LED_PIN4, OUTPUT);  //ให้พอร์ท ส่งกำลังไฟออก
  //Active Low
  digitalWrite(LED_PIN1, HIGH);  //ให้พอร์ททำการใช้งาน
  digitalWrite(LED_PIN2, HIGH);  //ให้พอร์ททำการใช้งาน
  digitalWrite(LED_PIN3, HIGH);  //ให้พอร์ททำการใช้งาน
  digitalWrite(LED_PIN4, HIGH);  //ให้พอร์ททำการใช้งาน

  Serial.begin(115200);  //ประกาศ Serial Speed

  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);  // จะแสดง `Connecting to ********`

  WiFi.begin(ssid, password);  //ทำการเชื่อมต่อ Wifi โดยตัวแปร SSID และ Password

  while (WiFi.status() != WL_CONNECTED) {  // วนซ้ำจนกว่าจะเชื่อมต่อ Wifi สำเร็จ
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());  //แสดง `WiFi connected IP address ***.***.***.***`

  Wire.begin();                           //ประกาศใช้งาน Wire
  if (!RTC.begin()) {                     //หากหาโมดูล RTC_DS3231 ไม่เจอ
    Serial.println("Couldn't find RTC");  //แสดง `Couldn't find RTC`
    Serial.flush();                       //เคลียร์บัฟเฟอร์ของพอร์ตอนุกรม(Serial) ให้ว่าง
  }
  if (RTC.lostPower()) {                                    //หากถ่านที่ให้พลังงาน RTC_DS3231 หมด เวลาที่ตั้งไว้จะหาย
    Serial.println("RTC lost power, let's set the time!");  //แสดง `RTC lost power, let's set the time!`
  }
}
//-------------------------------------loop-----------------------------------------
void loop() {                //เริ่ม Loop Main Function
  DateTime now = RTC.now();  //เรียกค่าวันที่จาก โมดูล RTC_DS3231
  int days = now.day();      // Today
  int nub = 26;              //วันที่จะทำการส่งข้อมูลขึ้นไปที่ ฐานข้อมูล
  Serial.print("Today is : ");
  Serial.print(now.day());
  Serial.print(" : Left day to Send Energy : ");
  Serial.println(days > nub ? "1Month" : nub - days);  //แสดง `Today is : xx : Left day to send Energy : ****`
  float power = pzem.power();                          //พลังงานไฟฟ้าปัจจุบัน
  float energy = pzem.energy();                        //พลังงานไฟฟ้าที่ใช้งานในวงจร
  float current = pzem.current();                      //กระแสที่ใช้งานอยู่ในวงจร

  //---------------------------------Fetching Data----------------------------------
  if (!client.connect(server_ip, SERVER_PORT)) {  //ทำการเชื่อมต่อ API โดยใช้ตัวแปร server_ip และ SERVER_PORT
    Serial.println("Connection API failed");      // ถ้าเชื่อมต่อไม่ได้จะแสดง `Connection API failed` จนกว่าจะเชื่อมต่อได้
    return;
  }

  String url = "/channels/all";                                                                                        //URL method API ที่ต้องการใช้งาน
  client.print(String("GET ") + url + " HTTP/1.1\r\n" + "Host: " + server_ip + "\r\n" + "Connection: close\r\n\r\n");  //วิธีการเขียน `GET` API

  while (!client.available())  //วนซ้ำจนกว่าจะสามารถทำการเรียก `GET` API ได้
    ;
  String response = client.readString();            //ทำการรับค่า Response
  int bodyStart = response.indexOf("\r\n\r\n");     //ส่วนที่ทำให้ได้ Response แค่ส่วนที่ใช้งานโดยตัดด้านหัวออก
  String body = response.substring(bodyStart + 4);  // ส่วนที่ทำให้ได้ Response แค่ส่วนที่ใช้งานโดยตัดด้านท้ายออก

  DeserializationError error = deserializeJson(doc1, body);  //ทำการแปลงข้อมูล Response ที่มีแต่ส่วนที่ใช้งานได้ ให้เป็น Json

  if (error) {                         //หากแปลงไม่ผ่านจะทำการแจ้งเตือน
    Serial.println("Parsing failed");  //แสดง`"Parsing failed`
    return;                            //วนซ้ำจนกว่าจะแปลงข้อมูลสำเร็จ
  }
  //---------------------------------check Safety------------------------------------
  if (current != NAN) {  // ตัวสอบเงื่อนไขว่าค่ากระแสที่อ่านได้เป็นตัวเลขใช้หรือไม่ [Note:NAN คือ Not a Number]
    Serial.print("Power : ");
    Serial.print(power);
    Serial.println("W");                                                                 //แสดงค่าพลังงานที่ใช้งาน `Power : ** W`
    if (check_safety(power)) {                                                           //เรียกใช้ฟังก์ชัน check_safety โดยจะส่งข้อมูล(parameter) power(กำลังไฟฟ้า) เข้าไปในฟังก์ชันด้วย
                                                                                         //หากเช็คแล้วเป็น TRUE หมายถึงกำลังไฟใช้งานเกินกว่าที่ได้ตั้งไว้ จะทำการปิดพอร์ทปลั๊กไฟทั้งหมดทันที
      for (int i = 0; i < 4; i++) {                                                      //วนลูปเพื่อสั่งงานปิดปลั๊กไฟทั้งหมด
        Serial.println("---------!!!Power Over Load!!! .·´¯`(>▂<)´¯`·. -------------");  //แสดงข้อความว่าพลังงานเกิน `---------!!!Power Over Load!!!-------------`
        CONTROL_PLUG_ON_OFF(i + 1, false);                                               //เรียกใช้ฟังก์ชัน CONTROL_PLUG_ON_OFF โดยจะส่งค่า(parameter)เข้าไปคือ
        //i+1 คือ ID ของปลั๊กแต่ละตัว ตั้งแต่ 1 ถึง 4
        //false คือ สถานะที่ต้องการให้ปลั๊กไฟทำงาน false หมายถึงปิดการทำงาน
      }
    } else {                                                                             //หากฟังก์ชัน check_safety(power) ส่งคืนค่า false กลับมาแสดงว่ากำลังไฟที่ใช้งานอยู่ยังไม่เกินกว่าที่ตั้งไว้หรือไม่ได้เปิดใช้งานการจำกัดกำลังไฟ
      Serial.println("--------- Power Not Over Load O(∩_∩)O ---------");                 //แสดง `--------- Power Not Over Load O(∩_∩)O ---------`
      for (int i = 0; i < 4; i++) {                                                      //วนลูปเพื่อสั่งงานปิดปลั๊กไฟทั้งหมด
        if (!doc1["data"][i]["templatestatus"]) {                                        //ทำการตรวจสอบเงื่อนไขว่า ปลั๊กตัวที่ 1 ถึง 4 อันใดมีการตั้งเวลาการทำงานไว้หรือไม่โดยจะตรวจสอบจากตัวแปร
                                                                                         //doc1["data"][i]["templatestatus"] คือ สถานะการตั้งเวลา จะมี 2 สถานะคือ `TRUE` หมายถึง ตั้งเวลาไว้ และ `FALSE` หมายถึงไม่ได้ตั้งเวลา
          CONTROL_PLUG_ON_OFF(doc1["data"][i]["channelId"], doc1["data"][i]["status"]);  //<🚦---------------------------------------------------call function Control on off
          //เรียกใช้ฟังก์ชัน CONTROL_PLUG_ON_OFF โดยจะส่งค่า(parameter)เข้าไปคือ
          //doc1["data"][i]["channelId"] คือ ID ของปลั๊กแต่ละตัว ตั้งแต่ 1 ถึง 4
          //doc1["data"][i]["status"] คือ สถานะที่ต้องการให้ปลั๊กไฟทำงาน `true` หมายถึงให้ทำงาน และ `false` หมายถึงปิดการทำงาน
        } else {                                                                               //หากตรวจสอบแล้ว มีการตั้งเวลากับปลั๊กไฟตั้งแต่ 1 ถึง 4 จะทำการทำงานตามเวลาที่ตั้งไว้ เฉพาะตัวที่มีการตั้งเวลาเท่านั้น
          time_plug_on_off(doc1["data"][i]["channelId"], doc1["data"][i]["templateId"], now);  //<🚦---------call function time_plug_on_off
                                                                                               //เรียกใช้ฟังก์ชัน time_plug_on_off()โดยจะส่งค่า(parameter)เข้าไปคือ
                                                                                               //doc1["data"][i]["channelId"] คือ ID ของปลั๊กตัวนั้นๆที่จะสั่งงาน
                                                                                               //doc1["data"][i]["templateId"] คือ ID ของการตั้งเวลาเรียกว่า Template
                                                                                               //now //เวลาปัจจุบันที่อ่านจาก Module RTC_DS3231
        }
      }
    }
  } else {                                  //หากอ่านค่าของ พลังงานไม่ได้ จะไม่สามารถสั่งงานใดๆได้เลย
    Serial.println("Error reading power");  //แสดง `Error reading power`
  }

  //---------------------------------send energy------------------------------------

  if ((nub == days) && (wera == 1)) {  //ตรวจสอบเงื่อนไขว่า วันที่ปัจจุบันตรงกับวันที่ตั้งค่าไว้หรือไม่และในวันนั้นมีการส่งค่าไปยังฐานข้อมูลหรือยังโดยอธิบายได้ดังนี้
                                       //nub == days : nub คือ 26 และ days คือ วันที่ปัจจุบัน -> ดังนั้น หากวันที่ปัจจุบันคือวันที่ 26 จะเข้าเงื่อนไข คืนค่า `TRUE`
                                       //wera == 1 : wera คือ สถานะว่าวันที่นี้ทำการส่งค่าไปยังฐานข้อมูลหรือยัง โดย `1` หมายถึงยังไม่ได้ส่ง และ `0` คือทำการส่งค่าไปแล้ว หากเป็น 1 จะคืนค่า `TRUE`
                                       // หากเมื่อทั้ง 2  เงื่อนไขเป็น `TRUE` จะทำตามคำสั่งภายในเงื่อนไข
    if (energy > 0) {                  // ตรวจสอบว่าค่าพลังงานที่อ่านได้มีมากกว่า 0
      report_Power(energy);            //<-----------------------------------call function report_Power
      // เรียกใช้ฟังก์ชัน report_Power(energy) คือฟังก์ชันในการส่งค่าขึ้นไปยังฐานข้อมูล โดยส่งข้อมูล(parameter) เข้าไป คือ
      // ennergy คือ ค่าพลังงานที่ใช้สะสมในเดือนนั้น ๆ
      Serial.println("update energy !");    //แสดง `update energy !`
      wera = 0;                             //เปลี่ยนค่า wera ให้เป็น `0` ซึ่งหมายถึง วันนี้ทำการส่งค่าไปที่ฐานข้อมูลแล้ว
    } else {                                //หากค่าที่อ่านได้จาก energy น้อยกว่า 0 จะทำตามคำสั่งภายในเงื่อนไข else
      Serial.println("Error read energy");  //แสดง `Error read energy`
    }
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
void time_plug_on_off(int channelId, int templateId, DateTime now) {
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