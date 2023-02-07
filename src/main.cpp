#include <Arduino.h>
#include <WiFi.h>
#include "reset.h"
#include <StreamString.h>
#include <LEAmDNS.h>
#include <pico/cyw43_arch.h>
#include <AsyncWebServer_RP2040W.h>

const char *app_version = "0.0.1"; // версия
const char *ssid = "xxxxxxxxxxxxxxxx"; // имя сети
const char *password = "xxxxxxxx"; // пароль wifi

const int led = LED_BUILTIN; // светодиод который будет моргать при активности

AsyncWebServer server(80);

// Функция которая выводит главную страницу http://soler-exporter.local/
void handlerRoot(AsyncWebServerRequest *request) {
  digitalWrite(led, 1);
  StreamString temp; 
  temp.reserve(500); // Preallocate a large chunk to avoid memory fragmentation
  temp.printf("<html>\
  <head>\
    <title>Solar Inverter SmartWatt ECO Exporter v0.0.1</title>\
  </head>\
  <body>\
    <h1>Solar Inverter SmartWatt ECO Exporter v0.0.1</h1>\
    <a href=\"/metrics\">Metrics</a></br>\
    <a href=\"/update\">Update</a></br>\
  </body>\
</html>");
  request->send(200, "text/html", temp);
  digitalWrite(led, 0);
}

// Функция которая выводит страницу c метриками http://solar-exporter.local/metrics
void handlerMetrics(AsyncWebServerRequest *request) {
  digitalWrite(led, 1);
  StreamString temp;
  temp.reserve(500);
  temp.printf("<html>\
  <head>\
    <title>Solar Inverter SmartWatt ECO Exporter v0.0.1</title>\
  </head>\
  <body>\
  # HELP solar_exporter_version_info is a version of solar exporter appplication started in raspbery pico w micro controller\
  # TYPE solar_exporter_version_info gauge\
  solar_exporter_version_info{version=\"%s\"} 1\
  # HELP solar_exporter_board_temp is temperature of Raspbery Pico W board from analogReadTemp() function\
  # TYPE solar_exporter_board_temp gauge\
  solar_exporter_board_temp %s\
  </body>\
</html>", app_version, analogReadTemp());
  request->send(200, "text/html", temp);
  digitalWrite(led, 0);
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.printf("Starting Firmware version %s\n", app_version);
  Serial.print("Wifi init");
  
  // инициализация wifi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  // в процессе инициализцаии wifi ресуем иочку каждые 500 милисекунд
  while (WiFi.status() == WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  // Если wifi успешно подключен то сообщение
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("\n");
    Serial.printf("Board successfully connected to %s\n", ssid);
  }

  // если Wifi не подключился к точке доступа то перезагружаем плату и всё сначала
  if (WiFi.status() != WL_CONNECTED) {
    Serial.printf("Unable to connect to %s\n", ssid);
    delay(500);
    Serial.println("Reboot...!");
    delay(1000);
    resetFunc();
  }

  // Выводим ip вдррес который получили по DHCP
  Serial.printf("Board IP address: %s\n", WiFi.localIP());

  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    handlerRoot(request);
  });
  server.on("/metrics", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    handlerMetrics(request);
  });
  server.begin();

}

void loop() {
  
}