#include <ESP8266WiFi.h>        //Connect WIFI To INTERNET
#include <PZEM004Tv30.h>        //Capure CIRCUIT value
#include <RTClib.h>             //Time Counter Module
#include <ArduinoJson.h>        //Manage JSON File
#include <ESP8266HTTPClient.h>  //API Connection
#include <NTPClient.h>          //https://randomnerdtutorials.com/esp8266-nodemcu-date-time-ntp-client-server-arduino/
#include <WiFiUdp.h>






//Digital Port Initial-----
#define LED_PIN1 D0  //พอร์ตเชื่อมต่อ ดิจิตอลช่องที่ 0
#define LED_PIN2 D3  //พอร์ตเชื่อมต่อ ดิจิตอลช่องที่ 3
#define LED_PIN3 D4  //พอร์ตเชื่อมต่อ ดิจิตอลช่องที่ 4
#define LED_PIN4 D7  //พอร์ตเชื่อมต่อ ดิจิตอลช่องที่ 7

//API Method Initial-----
#define SERVER_PORT 8888                                   // Port ที่ใช้เชื่อมต่อกับ Server ของ API
const char* server_ip = "ln-web.ichigozdata.win";  // URL Domain ที่ API ใช้งานอยู่

//Time Counter By DS3231 Initial-----
RTC_DS3231 RTC;  // ประการศตัวแปร RTC ให้ใช้งานโมดูล

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

//WIFI Variable initial-----
const char* ssid = "*****";  //SSID ของ WIFI ที่ต้องใช้เชื่อมต่อ
const char* password = "****";          //Password ของ WIFI

unsigned long previousMillis = 0;
const unsigned long interval = 10000;  // 10 วินาที

//Electic Mornitor By PZEM-004T Initial-----
PZEM004Tv30 pzem(D5, D6);  // ประกาศพอร์ตเชื่อมต่อให้กับ โมดูล Pzem004-T

//API Method Initial-----
HTTPClient http;    //ประกาศใช้ HTTPCLient ลงในตัวแปรชื่อ http
WiFiClient client;  //ประกาศใช้ WiFiClient ลงในตัวแปรชื่อ client

//json file
DynamicJsonDocument doc_channel(2048);   // สร้าง Json ขนาด 2048 byte
DynamicJsonDocument doc_template(2048);  // สร้าง Json ขนาด 2048 byte
DynamicJsonDocument doc_savety(1024);    // สร้าง Json ขนาด 1024 byte

//Set Timer Variable
const long days_delay = 86400000;
bool wera = 1;

float etime = 00;
//-------------------------------setup-----------------------------------
void setup() {
  pinMode(LED_PIN1, OUTPUT);  //ให้พอร์ต ส่งกำลังไฟออก
  pinMode(LED_PIN2, OUTPUT);  //ให้พอร์ต ส่งกำลังไฟออก
  pinMode(LED_PIN3, OUTPUT);  //ให้พอร์ต ส่งกำลังไฟออก
  pinMode(LED_PIN4, OUTPUT);  //ให้พอร์ต ส่งกำลังไฟออก
  //Active Low
  digitalWrite(LED_PIN1, HIGH);  //ให้พอร์ตทำการใช้งาน
  digitalWrite(LED_PIN2, HIGH);  //ให้พอร์ตทำการใช้งาน
  digitalWrite(LED_PIN3, HIGH);  //ให้พอร์ตทำการใช้งาน
  digitalWrite(LED_PIN4, HIGH);  //ให้พอร์ตทำการใช้งาน

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
  timeClient.begin();
  timeClient.setTimeOffset(0);  //+0 Epoch GMT
}
//-------------------------------------loop-----------------------------------------
void loop() {                //เริ่ม Loop Main Function
  DateTime now = RTC.now();  //เรียกค่าวันที่จาก โมดูล RTC_DS3231
  int indays = now.hour();   // Today
  timeClient.update();
  //วันที่จะทำการส่งข้อมูลขึ้นไปที่ ฐานข้อมูล
  unsigned long currentMillisedcond = millis();
  if (currentMillisedcond - previousMillis >= interval) {
    previousMillis = currentMillisedcond;
    checkstatus(now);  // เข้าสู่ฟังก์ชันส่งข้อมูลไปให้ฐานข้อมูล
  }
  Serial.print("minute is : ");
  Serial.println(now.minute());
  Serial.println(" : Left day to Send Energy : ");

  float power = pzem.power();      //พลังงานไฟฟ้าปัจจุบัน
  float energy = pzem.energy();    //พลังงานไฟฟ้าที่ใช้งานในวงจร
  float current = pzem.current();  //กระแสที่ใช้งานอยู่ในวงจร

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
  int bodyStart = response.indexOf("\r\n\r\n");     //หาว่าส่วนที่เป็นค่าว่าและการเว้นบรรทัดอยู่ที่ส่วนไหนแล้วทำการบันทึกลงในตัวแปร `bodyStart`
  String body = response.substring(bodyStart + 4);  // ทำการตัดส่วนที่เป็นค่าว่าส่วนหัวออก รวมถึงอีก 4 ตัวอักษรถัดมาด้วย

  DeserializationError error = deserializeJson(doc_channel, body);  //ทำการแปลงข้อมูล Response ที่มีแต่ส่วนที่ใช้งานได้ ให้เป็น Json

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
                                                                                         //หากเช็คแล้วเป็น TRUE หมายถึงกำลังไฟใช้งานเกินกว่าที่ได้ตั้งไว้ จะทำการปิดพอร์ตปลั๊กไฟทั้งหมดทันที
      for (int i = 0; i < 4; i++) {                                                      //วนลูปเพื่อสั่งงานปิดปลั๊กไฟทั้งหมด
        Serial.println("---------!!!Power Over Load!!! .·´¯`(>▂<)´¯`·. -------------");  //แสดงข้อความว่าพลังงานเกิน `---------!!!Power Over Load!!!-------------`
        CONTROL_PLUG_ON_OFF(i + 1, false);                                               //เรียกใช้ฟังก์ชัน CONTROL_PLUG_ON_OFF โดยจะส่งค่า(parameter)เข้าไปคือ
        //i+1 คือ ID ของปลั๊กแต่ละตัว ตั้งแต่ 1 ถึง 4
        //false คือ สถานะที่ต้องการให้ปลั๊กไฟทำงาน false หมายถึงปิดการทำงาน
      }
    } else {                                                                                           //หากฟังก์ชัน check_safety(power) ส่งคืนค่า false กลับมาแสดงว่ากำลังไฟที่ใช้งานอยู่ยังไม่เกินกว่าที่ตั้งไว้หรือไม่ได้เปิดใช้งานการจำกัดกำลังไฟ
      Serial.println("--------- Power Not Over Load O(∩_∩)O ---------");                               //แสดง `--------- Power Not Over Load O(∩_∩)O ---------`
      for (int i = 0; i < 4; i++) {                                                                    //วนลูปเพื่อสั่งงานปิดปลั๊กไฟทั้งหมด
        if (!doc_channel["data"][i]["templatestatus"]) {                                               //ทำการตรวจสอบเงื่อนไขว่า ปลั๊กตัวที่ 1 ถึง 4 อันใดมีการตั้งเวลาการทำงานไว้หรือไม่โดยจะตรวจสอบจากตัวแปร
                                                                                                       //doc_channel["data"][i]["templatestatus"] คือ สถานะการตั้งเวลา จะมี 2 สถานะคือ `TRUE` หมายถึง ตั้งเวลาไว้ และ `FALSE` หมายถึงไม่ได้ตั้งเวลา
          CONTROL_PLUG_ON_OFF(doc_channel["data"][i]["channelId"], doc_channel["data"][i]["status"]);  //<🚦---------------------------------------------------call function Control on off
          //เรียกใช้ฟังก์ชัน CONTROL_PLUG_ON_OFF โดยจะส่งค่า(parameter)เข้าไปคือ
          //doc_channel["data"][i]["channelId"] คือ ID ของปลั๊กแต่ละตัว ตั้งแต่ 1 ถึง 4
          //doc_channel["data"][i]["status"] คือ สถานะที่ต้องการให้ปลั๊กไฟทำงาน `true` หมายถึงให้ทำงาน และ `false` หมายถึงปิดการทำงาน
        } else {                                                                                             //หากตรวจสอบแล้ว มีการตั้งเวลากับปลั๊กไฟตั้งแต่ 1 ถึง 4 จะทำการทำงานตามเวลาที่ตั้งไว้ เฉพาะตัวที่มีการตั้งเวลาเท่านั้น
          time_plug_on_off(doc_channel["data"][i]["channelId"], doc_channel["data"][i]["templateId"], now);  //<🚦---------call function time_plug_on_off
                                                                                                             //เรียกใช้ฟังก์ชัน time_plug_on_off()โดยจะส่งค่า(parameter)เข้าไปคือ
                                                                                                             //doc_channel["data"][i]["channelId"] คือ ID ของปลั๊กตัวนั้นๆที่จะสั่งงาน
                                                                                                             //doc_channel["data"][i]["templateId"] คือ ID ของการตั้งเวลาเรียกว่า Template
                                                                                                             //now //เวลาปัจจุบันที่อ่านจาก Module RTC_DS3231
        }
      }
    }
  } else {                                  //หากอ่านค่าของ พลังงานไม่ได้ จะไม่สามารถสั่งงานใดๆได้เลย
    Serial.println("Error reading power");  //แสดง `Error reading power`
  }

  //---------------------------------send energy------------------------------------

  if (etime == now.minute()) {  //(millis() - previousMillis1 >= 60000) {  //ตรวจสอบเงื่อนไขว่า วันที่ปัจจุบันตรงกับวันที่ตั้งค่าไว้หรือไม่และในวันนั้นมีการส่งค่าไปยังฐานข้อมูลหรือยังโดยอธิบายได้ดังนี้
                                //nub == days : nub คือ 26 และ days คือ วันที่ปัจจุบัน -> ดังนั้น หากวันที่ปัจจุบันคือวันที่ 26 จะเข้าเงื่อนไข คืนค่า `TRUE`
    // previousMillis1 = millis();                           //wera == 1 : wera คือ สถานะว่าวันที่นี้ทำการส่งค่าไปยังฐานข้อมูลหรือยัง โดย `1` หมายถึงยังไม่ได้ส่ง และ `0` คือทำการส่งค่าไปแล้ว หากเป็น 1 จะคืนค่า `TRUE`
    // หากเมื่อทั้ง 2  เงื่อนไขเป็น `TRUE` จะทำตามคำสั่งภายในเงื่อนไข
    if (power > 0) {
      String real = String(now.year()) + "-" + String(now.month()) + "-" + String(now.day()) + " " + String(now.hour()) + ":" + String(now.minute()) + ":" + String(now.second());  // ตรวจสอบว่าค่าพลังงานที่อ่านได้มีมากกว่า 0
      report_Power(power, real);
      //<-----------------------------------call function
      // เรียกใช้ฟังก์ชัน report_Power(energy) คือฟังก์ชันในการส่งค่าขึ้นไปยังฐานข้อมูล โดยส่งข้อมูล(parameter) เข้าไป คือ
      // ennergy คือ ค่าพลังงานที่ใช้สะสมในเดือนนั้น ๆ

      Serial.println("update powerenergy !");  //แสดง `update energy !`
                                               //เปลี่ยนค่า wera ให้เป็น `0` ซึ่งหมายถึง วันนี้ทำการส่งค่าไปที่ฐานข้อมูลแล้ว
    } else {                                   //หากค่าที่อ่านได้จาก energy น้อยกว่า 0 จะทำตามคำสั่งภายในเงื่อนไข else
      Serial.println("Error read power");      //แสดง `Error read energy`
    }
  }

  return;
}  // -- End Loop()
//------------------control_plug_on_off-------------------------------------------
//ฟังก์ชัน CONTROL_PLUG_ON_OFF คือฟังก์ชันในการสั่งงานเปิดปิด ปลั๊กไฟ ตาม ID และ สถานะ ที่รับมาจากฐานข้อมูล โดยมีค่าที่รับเข้ามา(Argument) ดังนี้
// int channelId คือ ID หรือตัวแปรที่ใช้ชี้ว่าพอร์ตที่กำลังจะสั่งงานคือพอร์ตใด โดยจะเป็นค่าตัวเลข int(Integer) ตั้งแต่ 1 ถึง 4
// bool status สถานะที่ตั้งค่ามาจาก WebApplication แล้วทำการเรียกผ่าน API โดยค่าจะเป็นตรรกะ bool(Boolean) `TRUE` หรือ `FALSE`
void CONTROL_PLUG_ON_OFF(int channelId, bool status) {
  int LED_PIN;             //ประกาศตัวแปร LED_PIN เพื่อใช้ในการเก็บ พอร์ตการทำงานเพื่อสั่งงานในขั้นถัดไป
  switch (channelId) {     // switch ฟังก์ชันคือการตรวจสอบค่าที่ต้องการ ในที่นี้คือ channelID โดยจะมีเงื่อนไขดังนี้
                           //หาก channelID มีค่าเป็น 1 ให้ทำการ เปลี่ยน LED_PIN เป็น LED_PIN1 หรือ D0 (digital Port ช่องที่ 0)
                           //หาก channelID มีค่าเป็น 2 ให้ทำการ เปลี่ยน LED_PIN เป็น LED_PIN2 หรือ D3 (digital Port ช่องที่ 3)
                           //หาก channelID มีค่าเป็น 3 ให้ทำการ เปลี่ยน LED_PIN เป็น LED_PIN3 หรือ D4 (digital Port ช่องที่ 4)
                           //หาก channelID มีค่าเป็น 4 ให้ทำการ เปลี่ยน LED_PIN เป็น LED_PIN4 หรือ D5 (digital Port ช่องที่ 7)
    case 1:                //เงื่อนไข เช็คว่า channelID เท่ากับ 1 หรือไม่
      LED_PIN = LED_PIN1;  //ทำการแปลงค่าให้กับ LED_PIN เป็น พอร์ตการเชื่อมต่อ LED_PIN1 หรือ D0
      break;               //ออกจากฟังก์ชัน switch
    case 2:                //เงื่อนไข เช็คว่า channelID เท่ากับ 2 หรือไม่
      LED_PIN = LED_PIN2;  //ทำการแปลงค่าให้กับ LED_PIN เป็น พอร์ตการเชื่อมต่อ LED_PIN2 หรือ D3
      break;               //ออกจากฟังก์ชัน switch
    case 3:                //เงื่อนไข เช็คว่า channelID เท่ากับ 3 หรือไม่
      LED_PIN = LED_PIN3;  //ทำการแปลงค่าให้กับ LED_PIN เป็น พอร์ตการเชื่อมต่อ LED_PIN3 หรือ D4
      break;               //ออกจากฟังก์ชัน switch
    case 4:                //เงื่อนไข เช็คว่า channelID เท่ากับ 4 หรือไม่
      LED_PIN = LED_PIN4;  //ทำการแปลงค่าให้กับ LED_PIN เป็น พอร์ตการเชื่อมต่อ LED_PIN4 หรือ D7
      break;               //ออกจากฟังก์ชัน switch
    default:               //หากไม่เข้าเงือนไขใดๆจะทำการทำคำสั่งของ default เปรียบได้เหมือนกับ else ของ if..else
      return;              //ทำการออกจากฟังก์ชัน CONTROL_PLUG_ON_OFF
      break;               //ออกจากฟังก์ชัน switch
  }
  digitalWrite(LED_PIN, (status ? LOW : HIGH));  //สั่งงานไฟที่พอร์ต โดยจะส่งค่าเข้าไปในฟังก์ชัน digitalWrite จะประกอบด้วย
  //LED_PIN คือ พอร์ตที่ต้องการสั่งงาน
  //status คือ สถานะที่ต้องการสั่งงาน หากเป็น `TRUE` จะเขียน LOW ไปที่ digital port และหากเป็น `FALSE` จะเขียน HIGH ไปที่ digital port
  //NOTE:เนื่องจากเป็น Active Low การสั่งงาน `LOW` จึงหมายถึงการเปิด และ `HIGH` หมายถึงการปิด
}
//----------------------------------time_plug_on_off----------------------------------------
//ฟังก์ชัน time_plug_on_off คือฟังก์ชันที่จะสั่งงานให้ปลั๊กไฟทำงานตามเวลาและวันที่ได้ตั้งค่าไว้ โดยที่ค่า่ที่รับเข้ามา(Argument) มีดังนี้
// int channelId คือ ID หรือตัวแปรที่ใช้ชี้ว่าพอร์ตที่กำลังจะสั่งงานคือพอร์ตใด โดยจะเป็นค่าตัวเลข int(Integer) ตั้งแต่ 1 ถึง 4
// int templateId คือ ID หรือตัวเลขที่บ่งบอกถึงการตั้งค่าที่อยู่บนฐานข้อมูล โดยจะเป็นค่าตัวเลข int(Integer) ตั้งแต่ 0 ถึง n(จำนวนขึ้นอยู่กับการตั้งค่า)
// DateTime now คือ วันเวลาปัจจุบัน
void time_plug_on_off(int channelId, int templateId, DateTime now) {
  int LED_PIN;             //ประกาศตัวแปร LED_PIN เพื่อใช้ในการเก็บ พอร์ตการทำงานเพื่อสั่งงานในขั้นถัดไป
  switch (channelId) {     // switch ฟังก์ชันคือการตรวจสอบค่าที่ต้องการ ในที่นี้คือ channelID โดยจะมีเงื่อนไขดังนี้
                           //หาก channelID มีค่าเป็น 1 ให้ทำการ เปลี่ยน LED_PIN เป็น LED_PIN1 หรือ D0 (digital Port ช่องที่ 0)
                           //หาก channelID มีค่าเป็น 2 ให้ทำการ เปลี่ยน LED_PIN เป็น LED_PIN2 หรือ D3 (digital Port ช่องที่ 3)
                           //หาก channelID มีค่าเป็น 3 ให้ทำการ เปลี่ยน LED_PIN เป็น LED_PIN3 หรือ D4 (digital Port ช่องที่ 4)
                           //หาก channelID มีค่าเป็น 4 ให้ทำการ เปลี่ยน LED_PIN เป็น LED_PIN4 หรือ D5 (digital Port ช่องที่ 7)
    case 1:                //เงื่อนไข เช็คว่า channelID เท่ากับ 1 หรือไม่
      LED_PIN = LED_PIN1;  //ทำการแปลงค่าให้กับ LED_PIN เป็น พอร์ตการเชื่อมต่อ LED_PIN1 หรือ D0
      break;               //ออกจากฟังก์ชัน switch
    case 2:                //เงื่อนไข เช็คว่า channelID เท่ากับ 2 หรือไม่
      LED_PIN = LED_PIN2;  //ทำการแปลงค่าให้กับ LED_PIN เป็น พอร์ตการเชื่อมต่อ LED_PIN2 หรือ D3
      break;               //ออกจากฟังก์ชัน switch
    case 3:                //เงื่อนไข เช็คว่า channelID เท่ากับ 3 หรือไม่
      LED_PIN = LED_PIN3;  //ทำการแปลงค่าให้กับ LED_PIN เป็น พอร์ตการเชื่อมต่อ LED_PIN3 หรือ D4
      break;               //ออกจากฟังก์ชัน switch
    case 4:                //เงื่อนไข เช็คว่า channelID เท่ากับ 4 หรือไม่
      LED_PIN = LED_PIN4;  //ทำการแปลงค่าให้กับ LED_PIN เป็น พอร์ตการเชื่อมต่อ LED_PIN4 หรือ D7
      break;               //ออกจากฟังก์ชัน switch
    default:               //หากไม่เข้าเงือนไขใดๆจะทำการทำคำสั่งของ default เปรียบได้เหมือนกับ else ของ if..else
      return;              //ทำการออกจากฟังก์ชัน CONTROL_PLUG_ON_OFF
      break;               //ออกจากฟังก์ชัน switch
  }
  Serial.print("Have Template : ID :");                      //<-----------------check Template Get
  Serial.print(templateId);                                  //<-----------------check Template Get
  String url = "/template/detail?id=" + String(templateId);  //ตรวจสอบ URL ที่ทำการเรียกข้อมูลการทำงานตามเวลาที่ได้ตั้งไว้
  Serial.println(url);                                       //<-----------------check Template Get
                                                             //แสดง `Have Template : ID : ** /template/detail?id=**`
  if (!client.connect(server_ip, SERVER_PORT)) {             //ทำการเชื่อมต่อ API โดยใช้ตัวแปร server_ip และ SERVER_PORT
    Serial.println("Connection API failed");                 // ถ้าเชื่อมต่อไม่ได้จะแสดง `Connection API failed` จนกว่าจะเชื่อมต่อได้
    return;                                                  //วนกลับไปเริ่มต้นการทำงาน function loop ใหม่
  }
  client.print(String("GET ") + url + " HTTP/1.1\r\n" + "Host: " + server_ip + "\r\n" + "Connection: close\r\n\r\n");  //วิธีการเขียน `GET` API
  while (!client.available())                                                                                          //วนซ้ำจนกว่าจะสามารถทำการเรียก `GET` API ได้
    ;
  String response = client.readString();                             //ทำการรับค่า Response
  int bodyStart = response.indexOf("\r\n\r\n");                      //หาว่าส่วนที่เป็นค่าว่าและการเว้นบรรทัดอยู่ที่ส่วนไหนแล้วทำการบันทึกลงในตัวแปร `bodyStart`
  String body = response.substring(bodyStart + 4);                   // ทำการตัดส่วนที่เป็นค่าว่าส่วนหัวออก รวมถึงอีก 4 ตัวอักษรถัดมาด้วย
  DeserializationError error = deserializeJson(doc_template, body);  //ทำการแปลงข้อมูล Response ที่มีแต่ส่วนที่ใช้งานได้ ให้เป็น Json
  Serial.println("Fetch Complete");                                  //<-----------------check Template Get
  //ตรวจสอบว่าทำการดึงได้สำเร็จหรือไม่
  if (error) {                                      //หากแปลงไม่ผ่านจะทำการแจ้งเตือน
    Serial.println("Parsing doc_template failed");  //แสดง`Parsing doc_template failed`
    return;                                         //วนซ้ำจนกว่าจะแปลงข้อมูลสำเร็จ
  }

  if (doc_template["data"]["type"]) {                      //type is 1 mean time set
                                                           //เงื่อนไขการทำงานระบบตั้งเวลาจะมี 2 รูปแบบ คือแบบตั้งโดยวันที่(01-12-2023) และแบบที่ตั้งโดยเป็นวัน(จันทร์ อังคาร ... เสาร์ อาทิตย์)
                                                           //โดยที่ทั้ง 2 รูปแบบจะมีการบ่งบอกคือ type ในที่นี้ มีการดึงมาจากฐานข้อมูลคือ doc_template["data"]["type"] โดยจะมีข้อมูลคือ
                                                           // `1` คือ ตั้งค่าแบบวันที่
                                                           // `0` คือ ตั้งค่าแบบวัน
                                                           //⬆เป็นการตรวจสอบเงื่อนไขว่า type หรือ ประเภทการตั้งเวลาเป็นแบบไหนในที่นี้คือ `1` จะทำงานตามเงื่อนไขหลัก และ `0` จะทำงานตามเงื่อนไข else
    String dateStart = doc_template["data"]["datestart"];  //นำค่าที่รับมาจากฐานข้อมูลในรูปแบบ json มาใส่ไว้ในตัวแปร | ข้อมูลคือ doc_template["data"]["datestart"] หรือ วัน/เดือน/ปี ที่เริ่มทำงาน
    String dateEnd = doc_template["data"]["dateend"];      //นำค่าที่รับมาจากฐานข้อมูลในรูปแบบ json มาใส่ไว้ในตัวแปร | ข้อมูลคือ doc_template["data"]["dateend"] หรือ วัน/เดือน/ปี ที่สิ้นสุดการทำงาน
    //format ที่ได้จาก json จะเป็น yyyy-mm-dd (2023-03-15)
    // ________________________
    // |0|1|2|3|4|5|6|7|8|9|10|
    // --------|-|---|-|-------
    // |y|y|y|y|-|m|m|-|d|d|  |
    int dayStart = dateStart.substring(8, 10).toInt();   //ทำการตัดข้อมูลเฉพาะตัวหลักที่ 8 เป็นต้นไปถึง 10 จะได้ข้อมูล yyyy-mm-dd -> dd | คือวันที่เริ่ม
    int monthStart = dateStart.substring(5, 7).toInt();  //ทำการตัดข้อมูลเฉพาะตัวหลักที่ 5 เป็นต้นไปถึง 7 จะได้ข้อมูล yyyy-mm-dd -> mm | คือเดือนที่เริ่ม
    int yearStart = dateStart.substring(0, 4).toInt();   //ทำการตัดข้อมูลเฉพาะตัวหลักที่ 0 เป็นต้นไปถึง 4 จะได้ข้อมูล yyyy-mm-dd -> yyyy | คือปีที่เริ่ม
    int dayEnd = dateStart.substring(8, 10).toInt();     //ทำการตัดข้อมูลเฉพาะตัวหลักที่ 8 เป็นต้นไปถึง 10 จะได้ข้อมูล yyyy-mm-dd -> dd | คือวันที่จบ
    int monthEnd = dateStart.substring(5, 7).toInt();    //ทำการตัดข้อมูลเฉพาะตัวหลักที่ 5 เป็นต้นไปถึง 7 จะได้ข้อมูล yyyy-mm-dd -> mm | คือเดือนที่จบ
    int yearEnd = dateStart.substring(0, 4).toInt();     //ทำการตัดข้อมูลเฉพาะตัวหลักที่ 0 เป็นต้นไปถึง 4 จะได้ข้อมูล yyyy-mm-dd -> yyyy | คือปีที่จบ
    Serial.print("Get type 1 : ");                       //<-----------------check Template Get
    Serial.print(dateStart);                             //<-----------------check Template Get
    Serial.print("to");                                  //<-----------------check Template Get
    Serial.println(dateEnd);                             //<-----------------check Template Get
    //แสดง `Get type 1 : yyyy-mm-dd to yyyy-mm-dd`

    if (now.year() >= yearStart && now.year() <= yearEnd && now.month() >= monthStart && now.month() <= monthEnd && now.day() >= dayStart && now.day() <= dayEnd) {
      //เงื่อนไขที่จะทำการตรวจสอบว่า วัน เดือน ปี ปัจจุบัน อยู่ในช่วงเวลาที่ได้มีการตั้งเวลาไว้หรือไม่ จะประกอบด้วยเงื่อนไขดังนี้
      //now.year() >= yearStart คือ ปีปัจจุบัน เป็นปีที่มากกว่าหรือเป็นปีเดียวกับที่ตั้งค่าไว้หรือไม่
      //now.year() <= yearEnd คือ ปีปัจจุบัน เป็นปีที่น้อยกว่าหรือเป็นปีเดียวกับที่ตั้งค่าไว้หรือไม่
      //now.month() >= monthStart คือ เดือนปัจจุบัน เป็นเดือนที่มากกว่าหรือเป็นเดือนเดียวกับที่ตั้งค่าไว้หรือไม่
      //now.month() <= monthEnd คือ เดือนปัจจุบัน เป็นเดือนที่น้อยกว่าหรือเป็นเดือนเดียวกับที่ตั้งค่าไว้หรือไม่
      //now.day() >= dayStart คือ วันที่ปัจจุบัน เป็นวันที่ที่มากกว่าหรือเป็นวันที่เดียวกับที่ตั้งค่าไว้หรือไม่
      //now.day() <= dayEnd คือ วันที่ปัจจุบัน เป็นวันที่ที่น้อยกว่าหรือเป็นวันที่เดียวกับที่ตั้งค่าไว้หรือไม่
      //สรุปง่ายๆ ว่า วันที่ปัจจุบันอยู่ในช่วงเวลาที่ได้ตั้งเวลาไว้หรือไม่ ถ้าใช้ ทั้ง 6 เงื่อนไข จะเป็น `TRUE` แล้วจะทำงานตามคำสั่งภายใน IF
      Serial.print("Today is Have Timer Set Working");       //<-----------------check Template Get
                                                             // แสดง `Today is Have Timer Set Working`
      String timestart = doc_template["data"]["timestart"];  // ประกาศตัวแปร timestart มาเพื่อเก็บเวลาเริ่มต้นในการทำงานในวั้นนั้น โดยจะได้ข้อมูลมาจาก json และเลือกเอาเฉพาะ `doc_template["data"]["timestart"]` | จะอยู่ในรูปแบบ HH:MM (11:00)
      String timeend = doc_template["data"]["timeend"];      // ประกาศตัวแปร timeend มาเพื่อเก็บเวลาเริ่มต้นในการทำงานในวั้นนั้น โดยจะได้ข้อมูลมาจาก json และเลือกเอาเฉพาะ `doc_template["data"]["timeend"]` | จะอยู่ในรูปแบบ HH:MM (11:00)
                                                             // _____________
                                                             // |0|1|2|3|4|5|
                                                             // ----|-|---|-|
                                                             // |H|H|:|M|M| |
      int hour_start = timestart.substring(0, 2).toInt();    // ทำการตัดข้อมูลเฉพาะตัวหลักที่ 0 เป็นต้นไปถึง 2 จะได้ข้อมูล HH:MM -> HH | คือเวลาหลักชั่วโมงที่เริ่ม
      int minute_start = timestart.substring(3, 5).toInt();  // ทำการตัดข้อมูลเฉพาะตัวหลักที่ 3 เป็นต้นไปถึง 5 จะได้ข้อมูล HH:MM -> MM | คือเวลาหลักนาทีที่เริ่ม
      int hour_end = timeend.substring(0, 2).toInt();        // ทำการตัดข้อมูลเฉพาะตัวหลักที่ 0 เป็นต้นไปถึง 2 จะได้ข้อมูล HH:MM -> HH | คือเวลาหลักชั่วโมงที่สิ้นสุด
      int minute_end = timeend.substring(3, 5).toInt();      // ทำการตัดข้อมูลเฉพาะตัวหลักที่ 3 เป็นต้นไปถึง 5 จะได้ข้อมูล HH:MM -> MM | คือเวลาหลักนาทีที่สิ้นสุด
      if (now.hour() >= hour_start && now.minute() >= minute_start && now.hour() <= hour_end && now.minute() <= minute_end) {
        // เงื่อนไขที่จะทำการตรวจสอบว่า เวลา ปัจจุบัน อยู่ในช่วงเวลาที่ได้มีการตั้งเวลาไว้หรือไม่ จะประกอบด้วยเงื่อนไขดังนี้
        // now.hour() >= hour_start คือ เวลาในหลักชั่วโมงปัจจุบันมากกว่าหรือตรงกับเวลาที่ได้ตั้งไว้หรือไม่
        // now.hour() <= hour_end คือ เวลาในหลักชั่วโมงปัจจุบันน้อยกว่าหรือตรงกับเวลาที่ได้ตั้งไว้หรือไม่
        // now.minute() >= minute_start คือ เวลาในหลักนาทีปัจจุบันมากกว่าหรือตรงกับเวลาที่ได้ตั้งไว้หรือไม่
        // now.minute() <= minute_end คือ เวลาในหลักนาทีปัจจุบันน้อยกว่าหรือตรงกับเวลาที่ได้ตั้งไว้หรือไม่
        digitalWrite(LED_PIN, LOW);          // ทำการสั่งงาน digital port พอร์ตที่ LED_PIN ให้มีสถานะ LOW (เปิดการทำงาน)
        Serial.println("Timer Switch ON");   // แสดง `Timer Switch ON`
      } else {                               // หากไม่เข้าเงื่อนไขที่ว่า `เวลาปัจจุบันไม่อยู่ในช่วงที่ตั้งเวลาไว้`
        digitalWrite(LED_PIN, HIGH);         // ทำการสั่งงาน digital port พอร์ตที่ LED_PIN ให้มีสถานะ HIGH (ปิดการทำงาน)
        Serial.println("Timer Switch OFF");  // แสดง `Timer Switch OFF`
      }
    }
  } else {                            //type is 0 mean weekly set
                                      //เป็นเงื่อนไขที่จะทำเมื่อตรวจสอบ doc_template["data"]["type"] แล้วมีค่า เป็น `0` หมายความว่ามีการตั้งค่าเป็นแบบกำหนดวันแบบรายสัปดาห์
    Serial.println("Get type 0 : ");  //แสดง `Get type :`
    if (doc_template["data"]["daystart"] <= now.dayOfTheWeek() && doc_template["data"]["dayend"] >= now.dayOfTheWeek()) {
      // เงื่อนไขว่าวันปัจจุบันเป็นอยู่ในช่วงวันที่ได้ตั้งค่าเวลาไว้หรือไม่ โดยมีเงื่อนไขดังนี้
      // doc_template["data"]["daystart"] <= now.dayOfTheWeek()
      // doc_template["data"]["dayend"] >= now.dayOfTheWeek()
      String timestart = doc_template["data"]["timestart"];  // ประกาศตัวแปร timestart มาเพื่อเก็บเวลาเริ่มต้นในการทำงานในวั้นนั้น โดยจะได้ข้อมูลมาจาก json และเลือกเอาเฉพาะ `doc_template["data"]["timestart"]` | จะอยู่ในรูปแบบ HH:MM (11:00)
      String timeend = doc_template["data"]["timeend"];      // ประกาศตัวแปร timeend มาเพื่อเก็บเวลาเริ่มต้นในการทำงานในวั้นนั้น โดยจะได้ข้อมูลมาจาก json และเลือกเอาเฉพาะ `doc_template["data"]["timeend"]` | จะอยู่ในรูปแบบ HH:MM (11:00)
                                                             // _____________
                                                             // |0|1|2|3|4|5|
                                                             // ----|-|---|-|
                                                             // |H|H|:|M|M| |
      int hour_start = timestart.substring(0, 2).toInt();    // ทำการตัดข้อมูลเฉพาะตัวหลักที่ 0 เป็นต้นไปถึง 2 จะได้ข้อมูล HH:MM -> HH | คือเวลาหลักชั่วโมงที่เริ่ม
      int minute_start = timestart.substring(3, 5).toInt();  // ทำการตัดข้อมูลเฉพาะตัวหลักที่ 3 เป็นต้นไปถึง 5 จะได้ข้อมูล HH:MM -> MM | คือเวลาหลักนาทีที่เริ่ม
      int hour_end = timeend.substring(0, 2).toInt();        // ทำการตัดข้อมูลเฉพาะตัวหลักที่ 0 เป็นต้นไปถึง 2 จะได้ข้อมูล HH:MM -> HH | คือเวลาหลักชั่วโมงที่สิ้นสุด
      int minute_end = timeend.substring(3, 5).toInt();      // ทำการตัดข้อมูลเฉพาะตัวหลักที่ 3 เป็นต้นไปถึง 5 จะได้ข้อมูล HH:MM -> MM | คือเวลาหลักนาทีที่สิ้นสุด
      Serial.print(timestart);                               //<-----------------check Template Get
      Serial.print("to");                                    //<-----------------check Template Get
      Serial.print(timeend);                                 //<-----------------check Template Get
                                                             //แสดง `Get type 1 : HH:MM to HH:MM`
      if (now.hour() >= hour_start && now.minute() >= minute_start && now.hour() <= hour_end && now.minute() <= minute_end) {
        // เงื่อนไขที่จะทำการตรวจสอบว่า เวลา ปัจจุบัน อยู่ในช่วงเวลาที่ได้มีการตั้งเวลาไว้หรือไม่ จะประกอบด้วยเงื่อนไขดังนี้
        // now.hour() >= hour_start คือ เวลาในหลักชั่วโมงปัจจุบันมากกว่าหรือตรงกับเวลาที่ได้ตั้งไว้หรือไม่
        // now.hour() <= hour_end คือ เวลาในหลักชั่วโมงปัจจุบันน้อยกว่าหรือตรงกับเวลาที่ได้ตั้งไว้หรือไม่
        // now.minute() >= minute_start คือ เวลาในหลักนาทีปัจจุบันมากกว่าหรือตรงกับเวลาที่ได้ตั้งไว้หรือไม่
        // now.minute() <= minute_end คือ เวลาในหลักนาทีปัจจุบันน้อยกว่าหรือตรงกับเวลาที่ได้ตั้งไว้หรือไม่
        digitalWrite(LED_PIN, LOW);          // ทำการสั่งงาน digital port พอร์ตที่ LED_PIN ให้มีสถานะ LOW (เปิดการทำงาน)
        Serial.println("Timer Switch ON");   // แสดง `Timer Switch ON`
      } else {                               // หากไม่เข้าเงื่อนไขที่ว่า `เวลาปัจจุบันไม่อยู่ในช่วงที่ตั้งเวลาไว้`
        digitalWrite(LED_PIN, HIGH);         // ทำการสั่งงาน digital port พอร์ตที่ LED_PIN ให้มีสถานะ HIGH (ปิดการทำงาน)
        Serial.println("Timer Switch OFF");  // แสดง `Timer Switch OFF`
      }
    }
  }
}


void report_Power(float power_unit, String real) {                        //ฟังก์ชันที่มีไว้สำหรับการส่งข้อมูลไปยังฐานข้อมูล โดยผ่าน API
                                                                          //เริ่มต้นด้วยการติดต่อกับ API
                                                                          //สร้าง Json File สำหรับส่งไป
                                                                          //ใส่ข้อมูลลงใน Json File
                                                                          //ส่งไฟล์ขึ้นไปที่ API
  http.begin(client, "http://ln-web.ichigozdata.win:8888/saveunit/add");  //ทำการเปิดใช้งานโปรโตคอล HTTP ไปยังปลายทาง `http://ln-web.ichigozdata.win:8888/saveunit/add`
  http.addHeader("Content-Type", "application/json");                     //ทำการเขียน Haader สำหรับส่ง API พร้อมบอกว่าเป็น Json File แนบไป
  StaticJsonDocument<200> doc;                                            // สร้าง Json ขนาด 200 byte
  doc["unit"] = power_unit;                                               // ใส่ข้อมูล [key:value] -> unit:power_unit | ตัวอย่าง (unit:1000) จะหมายถึงส่งค่า unit ไปมีค่าเป็น 1000
  doc["date"] = real;                                                     // ใส่ข้อมูล [key:value] -> date:null | Note. null หมายถึงค่าที่ว่างเปล่าไม่มีข้อมูลใดๆ สำหรับ c++ จำเป็นต้องใช้งานเป็น `nullptr` หมายถึง null pointer ซึ่งใช้งานเหมือน null ทั่วไป

  String json;               // สร้างตัวแปรชื่อ json เพื่อนำมานำข้อมูล json file มาไว้เป็นข้อความแล้วส่งไปทาง HTTP โปรโตอคล
  serializeJson(doc, json);  // ทำการนำไฟล์ json file มาเก็บไว้ในตัวแปร `json`

  int httpCode = http.POST(json);                    // ทำการใช้ method `POST` แล้วส่งข้อความ json ขึ้นไป จากนั้นนำผลลัพธ์ที่ได้มาเก็บไว้ใน httpCode ส่งที่ได้คือมา โดยทั่วไป จะเป็น `200` หมายความว่าทำงานได้ OK
  if (httpCode > 0) {                                // หากมีการส่งคืนค่า httpCode คืนมาหมายความว่าฟังก์ชันติดต่อทำงานได้และสามารถเชื่อมต่อกับ Internet ได้ ซึ่งในอีกความหมายคือสามารถส่งข้อมูลไปทาง API ได้อย่างไม่มีปัญหา
    String response = http.getString();              // ทำการ getString เพื่อมาดูว่า Response(การตอบสนองของ API) เป็นอย่างไรบ้าง
    Serial.println(response);                        //แสดง
                                                     //     `{
                                                     //     "code": 201,
                                                     //     "data": {
                                                     //         "save_uintId": **,
                                                     //         "unit": **,
                                                     //         "date": null
                                                     //     },
                                                     //     "msg": "Saveunit is Successfully Created."
                                                     // }`
  } else {                                           // หากไม่สามารถส่งค่าขึ้นไปหรือติดต่อกับ api ไม่ได้จะแจ้งเตือน
    Serial.println("Error sending data to server");  // แสดง `Error sending data to server`
  }
  http.end();  // ทำการหยุดการเชื่อมต่อโปรโตคอล http
}

bool check_safety(float power) {
  // ฟังก์ชันตรวจสอบว่ากำลังไฟที่ทำงานอยู่ในวงจรเกินกว่าค่าที่ตั้งไว้หรือไม่
  // ดึงค่าจากฐานข้อมูลผ่าน API มา
  // นำมาตรวจสอบว่าค่าได้ทำการเปิดการจำกัดกำลังไฟหรือไม่ และกำลังไฟเกินกว่าที่ตั้งไว้หรือไม่
  if (!client.connect(server_ip, SERVER_PORT)) {  //ตรวจสอบการเชื่อมต่อกับ API ว่าทำการเชื่อมต่อได้หรือไม่ โดยจะเชื่อมต่อกับ server_ip (ip หรือ domain name ที่ได้ประกาศไว้) และ SERVER_PORT (port การเชื่อมต่อ เช่น 8888)
    Serial.println("Connection API failed");      // แสดง `onnection API failed` หากเชื่อมต่อไม่ได้
    return false;                                 //ส่งคืนค่า `false` หมายถึง กำลังไฟไม่เกินกว่าที่กำหนดเนื่องจากตรวจสอบกับฐานข้อมูลไม่ได้
  }

  String url = "/savety/all";                                                                                          // ทำการเลือก method URL เพื่อทำการเรียกข้อมูลกำลังไฟและสถานะการจำกัด
  client.print(String("GET ") + url + " HTTP/1.1\r\n" + "Host: " + server_ip + "\r\n" + "Connection: close\r\n\r\n");  //format สำหรับการติดต่อกับ API

  while (!client.available())  //ตรวจสอบว่า สามารถติดต่อกับ server ได้หรือไม่ หากไม่ได้ ทำซ้ำจนกว่าจะติดต่อได้
    ;
  String response = client.readString();                           //ทำการรับค่า Response
  int bodyStart = response.indexOf("\r\n\r\n");                    //หาว่าส่วนที่เป็นค่าว่าและการเว้นบรรทัดอยู่ที่ส่วนไหนแล้วทำการบันทึกลงในตัวแปร `bodyStart`
  String body = response.substring(bodyStart + 4);                 // ทำการตัดส่วนที่เป็นค่าว่าส่วนหัวออก รวมถึงอีก 4 ตัวอักษรถัดมาด้วย
  DeserializationError error = deserializeJson(doc_savety, body);  //ทำการแปลงข้อมูล Response ที่มีแต่ส่วนที่ใช้งานได้ ให้เป็น Json

  if (error) {                                    //หากแปลงไม่ผ่านจะทำการแจ้งเตือน
    Serial.println("Parsing doc_savety failed");  //แสดง`Parsing doc_savety failed`
    return false;                                 //วนซ้ำจนกว่าจะแปลงข้อมูลสำเร็จ
  }

  if (power > doc_savety["data"]["volt"] && doc_savety["data"]["status"]) {  //เงื่อนไขสำหรับตรวจสอบว่า ค่าพลังงานไรวงจรเกินกว่าที่ตั้งค่าไว้ไหม และ ได้ทำการเปิดใช้งานการจำกัดพลังงานไหม
                                                                             //หากได้เปิดการจำกัด และพลังงานเกินกว่าที่กำหนด จะคืนค่า `TRUE` หมายความว่า เกินกว่าที่กำหนด ให้สั่งปิดการทำงานของปลั๊กไฟทั้งหมดทันที
    return true;                                                             // คืนค่า true หมายถึงเกินกว่าที่จำกัดไว้
  } else {                                                                   //หากพลังงานที่ตั้งไว้ไม่เกิน หรือไม่ได้เปิดการจำกัดไว้ จะเข้าเงื่อนไข else
    Serial.println("");
    Serial.print("Safety Over Load at : ");
    Serial.print(doc_savety["data"]["volt"] ? doc_savety["data"]["volt"] : 0);
    Serial.println(" W");  //แสดง `Safety Over Load at : ** W`
    return false;          // คืนค่า true หมายถึงไม่เกินกว่าที่กำหนด หรือไม่ได้เปิดใช้งานการจำกัด
  }
}

void checkstatus(DateTime mainTime) {
  DateTime now = mainTime;

  unsigned long long epochtime = timeClient.getEpochTime();
  unsigned long long serverMillis = epochtime*1000;

  Serial.println("Start Check Status");
  http.begin(client, "http://192.168.1.123:8888/nodemcu/update");
  http.addHeader("Content-Type", "application/json");

  StaticJsonDocument<200> doc;
  doc["millis"] = String(serverMillis);

  String json;
  serializeJson(doc, json);

  int httpCode = http.PUT(json);

  if (httpCode > 0) {
    if (httpCode == HTTP_CODE_OK) {
      String response = http.getString();
      // ดำเนินการเพิ่มเติมหลังจากส่งข้อมูลสำเร็จ
      Serial.println(response);
    } else {
      Serial.println("HTTP error: " + String(httpCode));
      // ดำเนินการจัดการข้อผิดพลาดเซิร์ฟเวอร์
    }
  } else {
    Serial.println("Error sending data to server");
    // ดำเนินการจัดการข้อผิดพลาดในการส่งข้อมูล
  }

  http.end();
}
