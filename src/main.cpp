#include <Arduino.h>
#include <WiFi.h>
#include "reset.h"
#include <StreamString.h>
#include <LEAmDNS.h>
#include <pico/cyw43_arch.h>
#include <AsyncWebServer_RP2040W.h>
// #include <ArduinoOTA.h>

const char *app_version = "0.0.1"; // версия
const char *ssid = "laborotory"; // имя сети
const char *password = "Dabg3h9h"; // пароль wifi
const char *dnsname = "solar-exporter"; // DNS имя экспортера

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
  temp.printf("<html>\n\
  <head>\n\
    <title>Solar Inverter SmartWatt ECO Exporter v0.0.1</title>\n\
  </head>\n\
  <body>\n\
  # HELP solar_exporter_version_info is a version of solar exporter appplication started in raspbery pico w micro controller</br>\n\
  # TYPE solar_exporter_version_info gauge</br>\n");
  temp.printf("solar_exporter_version_info{version=\"%s\"} 1</br>\n", app_version);
  temp.printf("# HELP solar_exporter_wifi_firmwire is version of the firmwire that was used when compiling the exporte</br>\n\
  # TYPE solar_exporter_wifi_firmwire gauge</br>\n");
  temp.printf("solar_exporter_wifi_firmwire{version=\"%s\", wifi_driver_version=\"%s\"} 1</br>\n", app_version, WiFi.firmwareVersion());
  temp.printf("# HELP solar_exporter_wifi_signal_strength is a strength of wifi signal</br>\n\
  # TYPE solar_exporter_wifi_signal_strength gauge</br>\n");
  temp.printf("solar_exporter_wifi_signal_strength{version=\"%s\"} %i</br>\n", app_version, WiFi.RSSI());
  temp.printf("# HELP solar_exporter_board_temp is temperature of Raspbery Pico W board from analogReadTemp() function</br>\n\
  # TYPE solar_exporter_board_temp gauge</br>\n");
  temp.printf("solar_exporter_board_temp %.2f</br>\n", floorf(analogReadTemp() * 100) / 100);
  temp.printf("</body>\n\
</html>", floorf(analogReadTemp() * 100) / 100);
  AsyncWebServerResponse *response = request->beginResponse(200, "text/html", temp);
  request->send(response);
  digitalWrite(led, 0);
}

void printrequestdata(AsyncWebServerRequest *request) {
  Serial.printf("%d.%d.%d.%d:", request->client()->remoteIP()[0], request->client()->remoteIP()[1], request->client()->remoteIP()[2], request->client()->remoteIP()[3]);
  Serial.printf("%i -> ", request->client()->remotePort());
  Serial.printf("%d.%d.%d.%d:", request->client()->localIP()[0], request->client()->localIP()[1], request->client()->localIP()[2], request->client()->localIP()[3]);
  Serial.printf("%i ", request->client()->localPort());
  switch (request->version())
  {
  case 0:
    Serial.print("HTTP/1.0 ");
    break;
  case 1:
    Serial.print("HTTP/1.1 ");
    break;
  default:
    Serial.print("HTTP/X.X ");
    break;
  }
  Serial.printf("%s ", request->methodToString());
  Serial.printf("%s \"", request->url().c_str());
  Serial.print(request->header("User-Agent"));
  Serial.print("\"");
  Serial.print("\n");
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  delay(2000);
  Serial.printf("Starting Firmware version: %s\n", app_version);
  Serial.print("Wifi Firmware Version: "); 
  Serial.println(WiFi.firmwareVersion());
  Serial.print("Wifi init...");
  
  // инициализация wifi
  // WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  delay(2000);

  // в процессе инициализцаии wifi ресуем иочку каждые 500 милисекунд
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  // Если wifi успешно подключен то сообщение
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("OK\n");
  }

  // если Wifi не подключился к точке доступа то перезагружаем плату и всё сначала
  if (WiFi.status() != WL_CONNECTED) {
    Serial.print("ERROR\n");
    Serial.println("Reboot...!");
    delay(2000);
    rp2040.reboot();
  }

  Serial.print("MDNS init...");

  bool mdnsstatus = MDNS.begin(dnsname);

  if (mdnsstatus) {
    Serial.println("OK");
  } else {
    Serial.print("ERROR");
  }

  // Выводим ip вдррес который получили по DHCP
  Serial.printf("Board IP address: %d.%d.%d.%d\n", WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3]);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    handlerRoot(request);
  });
  server.on("/metrics", HTTP_GET, [](AsyncWebServerRequest * request){
    handlerMetrics(request);
    printrequestdata(request);
  });
  server.onNotFound([](AsyncWebServerRequest * request){
    request->send(404, "text/html", "Not Found");
  });
  server.begin();

}

void loop() {
  
}
