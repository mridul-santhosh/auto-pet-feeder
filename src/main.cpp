#include <WiFi.h>
#include <FirebaseESP32.h>
#include <ESP32Servo.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include <NTPClient.h>
#include <RTClib.h>
#include <vector>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <Wire.h>
#include "esp_camera.h"
#include <LiquidCrystal_I2C.h>

int lcdColumns = 16;
int lcdRows = 2;

LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);  

// #include "AsyncTCP.h"
// #include "ESPAsyncWebServer.h"
#include "esp_http_server.h"
#include "img_converters.h"
#include "fb_gfx.h"
#include "soc/soc.h" //disable brownout problems
#include "soc/rtc_cntl_reg.h"  //disable brownout 

using namespace std;
// Firebase credentials
#define FIREBASE_HOST ""
#define FIREBASE_AUTH ""
// Servo pin
#define FEED_SERVO 12
#define CLEAN_SERVO 13

Servo feeder;  // create servo object to control a servo
Servo cleaner;  // create servo object to control a servo
// Servo angles for feeding and not feeding
#define FEEDING_ANGLE 180
#define CLEANING_ANGLE 180

#define PART_BOUNDARY "123456789000000000000987654321"
TaskHandle_t Task2;


#define CAMERA_MODEL_AI_THINKER

// Camera frame size and quality settings
#define CAM_WIDTH 640
#define CAM_HEIGHT 480
#define CAM_QUALITY 10

#if defined(CAMERA_MODEL_WROVER_KIT)
  #define PWDN_GPIO_NUM    -1
  #define RESET_GPIO_NUM   -1
  #define XCLK_GPIO_NUM    21
  #define SIOD_GPIO_NUM    26
  #define SIOC_GPIO_NUM    27
  
  #define Y9_GPIO_NUM      35
  #define Y8_GPIO_NUM      34
  #define Y7_GPIO_NUM      39
  #define Y6_GPIO_NUM      36
  #define Y5_GPIO_NUM      19
  #define Y4_GPIO_NUM      18
  #define Y3_GPIO_NUM       5
  #define Y2_GPIO_NUM       4
  #define VSYNC_GPIO_NUM   25
  #define HREF_GPIO_NUM    23
  #define PCLK_GPIO_NUM    22

#elif defined(CAMERA_MODEL_M5STACK_PSRAM)
  #define PWDN_GPIO_NUM     -1
  #define RESET_GPIO_NUM    15
  #define XCLK_GPIO_NUM     27
  #define SIOD_GPIO_NUM     25
  #define SIOC_GPIO_NUM     23
  
  #define Y9_GPIO_NUM       19
  #define Y8_GPIO_NUM       36
  #define Y7_GPIO_NUM       18
  #define Y6_GPIO_NUM       39
  #define Y5_GPIO_NUM        5
  #define Y4_GPIO_NUM       34
  #define Y3_GPIO_NUM       35
  #define Y2_GPIO_NUM       32
  #define VSYNC_GPIO_NUM    22
  #define HREF_GPIO_NUM     26
  #define PCLK_GPIO_NUM     21

#elif defined(CAMERA_MODEL_M5STACK_WITHOUT_PSRAM)
  #define PWDN_GPIO_NUM     -1
  #define RESET_GPIO_NUM    15
  #define XCLK_GPIO_NUM     27
  #define SIOD_GPIO_NUM     25
  #define SIOC_GPIO_NUM     23
  
  #define Y9_GPIO_NUM       19
  #define Y8_GPIO_NUM       36
  #define Y7_GPIO_NUM       18
  #define Y6_GPIO_NUM       39
  #define Y5_GPIO_NUM        5
  #define Y4_GPIO_NUM       34
  #define Y3_GPIO_NUM       35
  #define Y2_GPIO_NUM       17
  #define VSYNC_GPIO_NUM    22
  #define HREF_GPIO_NUM     26
  #define PCLK_GPIO_NUM     21

#elif defined(CAMERA_MODEL_AI_THINKER)
  #define PWDN_GPIO_NUM     32
  #define RESET_GPIO_NUM    -1
  #define XCLK_GPIO_NUM      0
  #define SIOD_GPIO_NUM     26
  #define SIOC_GPIO_NUM     27
  
  #define Y9_GPIO_NUM       35
  #define Y8_GPIO_NUM       34
  #define Y7_GPIO_NUM       39
  #define Y6_GPIO_NUM       36
  #define Y5_GPIO_NUM       21
  #define Y4_GPIO_NUM       19
  #define Y3_GPIO_NUM       18
  #define Y2_GPIO_NUM        5
  #define VSYNC_GPIO_NUM    25
  #define HREF_GPIO_NUM     23
  #define PCLK_GPIO_NUM     22

#elif defined(CAMERA_MODEL_M5STACK_PSRAM_B)
  #define PWDN_GPIO_NUM     -1
  #define RESET_GPIO_NUM    15
  #define XCLK_GPIO_NUM     27
  #define SIOD_GPIO_NUM     22
  #define SIOC_GPIO_NUM     23
  
  #define Y9_GPIO_NUM       19
  #define Y8_GPIO_NUM       36
  #define Y7_GPIO_NUM       18
  #define Y6_GPIO_NUM       39
  #define Y5_GPIO_NUM        5
  #define Y4_GPIO_NUM       34
  #define Y3_GPIO_NUM       35
  #define Y2_GPIO_NUM       32
  #define VSYNC_GPIO_NUM    25
  #define HREF_GPIO_NUM     26
  #define PCLK_GPIO_NUM     21

#else
  #error "Camera model not selected"
#endif

const char* ntpServerName = "pool.ntp.org";

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, ntpServerName);
RTC_DS3231 rtc;

std::vector<String> Timez;
DateTime Start_Date = "";
DateTime End_Date = "";

FirebaseData firebaseData;
FirebaseJson firebaseJson;

static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* _STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

httpd_handle_t camera_httpd = NULL;
httpd_handle_t stream_httpd = NULL;

static const char PROGMEM INDEX_HTML[] = R"rawliteral(
<html>
  <head>
    <title>Feeder/title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
      img {  width: auto ;
        max-width: 100% ;
        height: auto ; 
      }
    </style>
  </head>
  <body>
    <h1>Feeder</h1>
    <img src="" id="photo" >
   <script>
   function toggleCheckbox(x) {
     var xhr = new XMLHttpRequest();
     xhr.open("GET", "/action?go=" + x, true);
     xhr.send();
   }
   window.onload = document.getElementById("photo").src = window.location.href.slice(0, -1) + ":81/stream";
  </script>
  </body>
</html>
)rawliteral";

static esp_err_t index_handler(httpd_req_t *req){
  httpd_resp_set_type(req, "text/html");
  return httpd_resp_send(req, (const char *)INDEX_HTML, strlen(INDEX_HTML));
}

static esp_err_t stream_handler(httpd_req_t *req){
  camera_fb_t * fb = NULL;
  esp_err_t res = ESP_OK;
  size_t _jpg_buf_len = 0;
  uint8_t * _jpg_buf = NULL;
  char * part_buf[64];

  res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
  if(res != ESP_OK){
    return res;
  }

  while(true){
    fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Camera capture failed");
      res = ESP_FAIL;
    } else {
      if(fb->width > 400){
        if(fb->format != PIXFORMAT_JPEG){
          bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
          esp_camera_fb_return(fb);
          fb = NULL;
          if(!jpeg_converted){
            Serial.println("JPEG compression failed");
            res = ESP_FAIL;
          }
        } else {
          _jpg_buf_len = fb->len;
          _jpg_buf = fb->buf;
        }
      }
    }
    if(res == ESP_OK){
      size_t hlen = snprintf((char *)part_buf, 64, _STREAM_PART, _jpg_buf_len);
      res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
    }
    if(res == ESP_OK){
      res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
    }
    if(res == ESP_OK){
      res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
    }
    if(fb){
      esp_camera_fb_return(fb);
      fb = NULL;
      _jpg_buf = NULL;
    } else if(_jpg_buf){
      free(_jpg_buf);
      _jpg_buf = NULL;
    }
    if(res != ESP_OK){
      break;
    }  digitalWrite(FLAS
    //Serial.printf("MJPG: %uB\n",(uint32_t)(_jpg_buf_len));
  }
  return res;
}




void startCameraServer(){
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.server_port = 80;
  httpd_uri_t index_uri = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = index_handler,
    .user_ctx  = NULL
  };

  httpd_uri_t stream_uri = {
    .uri       = "/stream",
    .method    = HTTP_GET,
    .handler   = stream_handler,
    .user_ctx  = NULL
  };
  if (httpd_start(&camera_httpd, &config) == ESP_OK) {
    httpd_register_uri_handler(camera_httpd, &index_uri);
  }
  config.server_port += 1;
  config.ctrl_port += 1;
  if (httpd_start(&stream_httpd, &config) == ESP_OK) {
    httpd_register_uri_handler(stream_httpd, &stream_uri);
  }
}








TimeSpan parse_time(String time_str) {
  int hours = time_str.substring(0, 2).toInt();
  int minutes = time_str.substring(3, 5).toInt();
  return TimeSpan(hours, minutes, 0, 0);
}

DateTime parse_date(String date_str) {
  int day = date_str.substring(0, 2).toInt();
  int month = date_str.substring(3, 5).toInt();
  int year = date_str.substring(6, 10).toInt();
  return DateTime(year, month, day, 0, 0, 0);
}

bool timeComparator(const String& a, const String& b) {
    // Parse the hour and minute values from the time strings
    int a_hour, a_min, b_hour, b_min;
    char separator;

    sscanf(a.c_str(), "%d%c%d", &a_hour, &separator, &a_min);
    sscanf(b.c_str(), "%d%c%d", &b_hour, &separator, &b_min);

    // Compare the hour values first
    if (a_hour != b_hour) {
        return a_hour < b_hour;
    }
    // If the hour values are the same, compare the minute values
    else {
        return a_min < b_min;
    }
}



void feed()
{

  Serial.println("-------------------------Feeding-----------------------------------");
  for(uint8_t i = 0; i <= FEEDING_ANGLE; i++ )
  {
    feeder.write(i);
    cleaner.write(i);
    delay(10);
  }
    lcd.setCursor(0, 0);
  // print message
  lcd.print("Hello, World!");
  delay(3000);
    lcd.clear();

}

void Task2code( void * pvParameters ){
    startCameraServer();

}
void setup() {
  // Initialize serial communication
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector

  Serial.begin(115200);

    camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG; 
  
  if(psramFound()){
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }
  
  // Camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  // initialize LCD
  lcd.init();
  // turn on LCD backlight                      
  lcd.backlight();
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);
  feeder.setPeriodHertz(50);    // standard 50 hz servo
  cleaner.setPeriodHertz(50);    // standard 50 hz servo 
  Wire.begin(14, 15); // Set I2C pins to 14 (SDA) and 15 (SCL)
  feeder.attach(FEED_SERVO);
  cleaner.attach(CLEAN_SERVO);

  // Connect to WiFi
  WiFi.begin("cipherm", "11111111");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  rtc.begin();
  if (!rtc.lostPower()) {
    Serial.println("RTC is NOT running!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
}
  delay(2000);
  // Initialize Firebase
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  timeClient.update();
  DateTime now = DateTime(timeClient.getEpochTime());
  rtc.adjust(now);
  // xTaskCreate(cameraTask, "Camera Task", 8192, NULL, 1, NULL);

  // Start the web server
  xTaskCreatePinnedToCore(
                    Task2code,   /* Task function. */
                    "Task2",     /* name of task. */
                    10000,       /* Stack size of task */
                    NULL,        /* parameter of the task */
                    1,           /* priority of the task */
                    &Task2,      /* Task handle to keep track of created task */
                    1);          /* pin task to core 1 */
    delay(500); 
  // Start streaming web server}
}



void loop() {

  DateTime now = rtc.now();

  const size_t MAX_JSON_SIZE = 512;

  String jsonString;
  String jsonString2;
  // Wait for the next iteration
  if (Firebase.getString(firebaseData, "/schedules"))
  {
    jsonString = firebaseData.stringData();
    Serial.println(jsonString);
  }
  if (Firebase.getString(firebaseData, "/instant"))
  {
    jsonString2 = firebaseData.stringData();
    Serial.println(jsonString2);
  }
  // Create a JSON document with the maximum size
  StaticJsonDocument<MAX_JSON_SIZE> doc;
  StaticJsonDocument<MAX_JSON_SIZE> doc2;

  // Parse the JSON string into the JSON document
  DeserializationError error = deserializeJson(doc, jsonString);
  DeserializationError error2 = deserializeJson(doc2, jsonString2);

  // Check if there was an error while parsing the JSON
  if (error)
  {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.f_str());
    return;
  }
  // Check if there was an error while parsing the JSON
  if (error2)
  {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.f_str());
    return;
  }

  // Get the "schedules" string from the "pet_feeder" object
  String schedulesString = doc["pet_feeder"]["schedules"];
  String schedulesString2 = doc2["pet_feeder"]["instant"];

  // Parse the schedules string into a JSON document
  StaticJsonDocument<MAX_JSON_SIZE> schedulesDoc;
  error = deserializeJson(schedulesDoc, schedulesString);
  StaticJsonDocument<MAX_JSON_SIZE> schedulesDoc2;
  error = deserializeJson(schedulesDoc2, schedulesString2);

  // Check if there was an error while parsing the schedules string
  if (error)
  {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.f_str());
    return;
  }

  // Check if there was an error while parsing the schedules string
  if (error2)
  {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.f_str());
    return;
  }

  Serial.println(schedulesString2);
  if (schedulesString2 == "true")
  {
    feed();
    Firebase.setBool(firebaseData, "/instant/pet_feeder/instant", false);
    if (firebaseData.httpCode() == 200)
    {
      Serial.println("Value updated successfully.");
    }
    else
    {
      Serial.println("Error updating value.");
    }
  }

  for (JsonPair pair : schedulesDoc.as<JsonObject>())
  {
    // Get the key of the first key-value pair
    String firstKey = pair.key().c_str();

    // Get the value of the first key-value pair
    JsonVariant firstValue = pair.value();
    // Print the key and value of the first key-value pair
    Serial.print("Key: ");
    Serial.println(firstKey);
    Serial.print("Value: ");
    serializeJson(firstValue, Serial);
    Serial.println();
    JsonArray scheduleArray = firstValue;
    const char* startDateStr = scheduleArray[0].as<char*>();
    Start_Date = parse_date(String(startDateStr));
    Serial.println("--------------------start date------------------------");
    Serial.println(String(startDateStr));
    startDateStr = scheduleArray[1].as<char*>();
    End_Date = parse_date(String(startDateStr));

    Serial.println("--------------------End date------------------------");
    Serial.println(String(startDateStr));

    Serial.println(scheduleArray.size());
    for (int i = 2; i < scheduleArray.size(); i++)
    {
      String value = scheduleArray[i];
      // Do something with the value, e.g. print it to the serial console
      if (value != "")
      {
        if(std::find(Timez.begin(), Timez.end(), value) == Timez.end()) {
        Timez.push_back(value);
      }
    }
    // Break out of the loop after the first key-value pair has been printed
  }

}
sort(Timez.begin(), Timez.end(), timeComparator);

for(auto iter : Timez)
{
  TimeSpan timer = parse_time(iter);

  if(now >= Start_Date && now <= End_Date)
  {
    int currentHour = now.hour();  // extract current hour
    int currentMinute = now.minute();  // extract current minute
    TimeSpan timer = parse_time(iter);  // parse the time from the iterator
    int desiredHour = timer.hours();  // extract desired hour from TimeSpan object
    int desiredMinute = timer.minutes();  // extract desired minute from TimeSpan object

    // compare current time with desired time within 2-minute window
    if (currentHour == desiredHour && abs(currentMinute - desiredMinute) <= 2) {
      // call your function here
      feed();
    }
}


}
delay(10000);

}
