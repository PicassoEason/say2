//device A : servo, sw420, LED, record
#include <Arduino.h>
#if defined(ESP32) || defined(ARDUINO_RASPBERRY_PI_PICO_W)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#elif __has_include(<WiFiNINA.h>)
#include <WiFiNINA.h>
#elif __has_include(<WiFi101.h>)
#include <WiFi101.h>
#elif __has_include(<WiFiS3.h>)
#include <WiFiS3.h>
#endif
#include <Firebase_ESP_Client.h>
#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>
#include <ESP32Servo.h>
#define WIFI_SSID "Era"
#define WIFI_PASSWORD "123321123"
#define API_KEY "AIzaSyDOeXrq21A4-L4XrqD3owwkSY7O0AktqsE"

#define DATABASE_URL "https://home9-58f42-default-rtdb.firebaseio.com/" //<databaseName>.firebaseio.com or <databaseName>.<region>.firebasedatabase.app

/* 4. Define the user Email and password that alreadey registerd or added in your project */
#define USER_EMAIL "easonwu.student@outlook.com"
#define USER_PASSWORD "infondof5566"

// Define Firebase Data object
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

Servo myservo;
unsigned long sendDataPrevMillis = 0;

unsigned long count = 0;

#if defined(ARDUINO_RASPBERRY_PI_PICO_W)
WiFiMulti multi;
#endif
//震動+button+servo
#define buttonPin 34
#define vibrationPin 33
#define servoPin 14
#define LEDpin 13
int delayT = 1000;
int buttonState = HIGH;  // 初始狀態設為HIGH（未按下）
int lastButtonState = HIGH;
int vibrationState = 0;
bool servoState = false;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;

void setup()
{

  Serial.begin(115200);
  pinMode(buttonPin, INPUT);  // 不使用內部上拉，假設使用外部上拉電阻
  pinMode(vibrationPin, INPUT);
  myservo.attach(servoPin);
//wifi setting
#if defined(ARDUINO_RASPBERRY_PI_PICO_W)
  multi.addAP(WIFI_SSID, WIFI_PASSWORD);
  multi.run();
#else
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
#endif

  Serial.print("Connecting to Wi-Fi");
  unsigned long ms = millis();
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(300);
#if defined(ARDUINO_RASPBERRY_PI_PICO_W)
    if (millis() - ms > 10000)
      break;
#endif
  }

  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());


  Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);

  /* Assign the api key (required) */
  config.api_key = API_KEY;

  /* Assign the user sign in credentials */
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  config.database_url = DATABASE_URL;
  config.token_status_callback = tokenStatusCallback; // see addons/TokenHelper.h
  Firebase.reconnectNetwork(true);
  fbdo.setBSSLBufferSize(4096 /* Rx buffer size in bytes from 512 - 16384 */, 1024 /* Tx buffer size in bytes from 512 - 16384 */);
  fbdo.setResponseSize(2048);
  Firebase.begin(&config, &auth);
#if defined(ARDUINO_RASPBERRY_PI_PICO_W)
  config.wifi.clearAP();
  config.wifi.addAP(WIFI_SSID, WIFI_PASSWORD);
#endif

  Firebase.setDoubleDigits(5);

  config.timeout.serverResponse = 10 * 1000;
}





void loop()
{
  firebase_getdata();
  int reading = digitalRead(buttonPin);
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }
//震動感測器
  vibrationState = digitalRead(vibrationPin);
  if (vibrationState == HIGH) {//敲門 開門
    myservo.write(60);
    Serial.println("opening door");
    firebase_senddata();
    //led light
    digitalWrite(LEDpin, HIGH);
    delay(delayT);
    digitalWrite(LEDpin, LOW);
    delay(delayT);
  }

  lastButtonState = reading;
  delay(10);  // 小延遲以穩定讀取
}
void firebase_senddata(){//send open door
  if (Firebase.ready() && (millis() - sendDataPrevMillis > 15000 || sendDataPrevMillis == 0))
  {
    sendDataPrevMillis = millis();
    Serial.printf("Set string... %s\n", Firebase.RTDB.setString(&fbdo, F("/sensor/door"), F("OPEN!")) ? "ok" : fbdo.errorReason().c_str());
  }
}
void firebase_getdata(){
    if (Firebase.ready() && (millis() - sendDataPrevMillis > 15000 || sendDataPrevMillis == 0))
  {
    sendDataPrevMillis = millis();
    // 讀取 sensor/door 路徑下的字串數據
    Serial.printf("Get string... %s\n", Firebase.RTDB.getString(&fbdo, F("/sensor/door")) ? fbdo.to<const char *>() : fbdo.errorReason().c_str());
    const char *doorStatus = fbdo.to<const char *>();
    if (doorStatus != nullptr)
    {
      if (strcmp(doorStatus, "open") == 0)
      {
        Serial.println("Door is open!");
      }
      else if (strcmp(doorStatus, "closed") == 0)
      {
        // 如果門的狀態是 "closed"，執行相應的操作
        Serial.println("Door is closed!");
        myservo.write(180);
        //led light
        digitalWrite(LEDpin, HIGH);
        delay(delayT);
        digitalWrite(LEDpin, LOW);
        delay(delayT);
      }
      else
      {
        // 其他未知狀態，可以添加相應的處理邏輯
        Serial.println("Unknown door status!");
      }
    }
  }
}