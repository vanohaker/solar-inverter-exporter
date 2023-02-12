// #include <Arduino.h>
#include <WiFi.h>
#include <StreamString.h>
#include <LEAmDNS.h>
#include <pico.h>
#include <AsyncWebServer_RP2040W.h>
#include <PicoOTA.h>
#include <LittleFS.h>

const char *app_version = "0.0.2"; // версия
const char *ssid = "laborotory"; // имя сети
const char *password = "Dabg3h9h"; // пароль wifi
const char *dnsname = "solar-exporter"; // DNS имя экспортера
const char *firmwareFilename = "firmware.bin";
boolean fwFileExist = false;

File uploadFile;

const int led = LED_BUILTIN; // светодиод который будет моргать при активности

AsyncWebServer server(80);
size_t content_len;

void listAllFilesInDir(String dir_path)
{
	Dir dir = LittleFS.openDir(dir_path);
	while(dir.next()) {
		if (dir.isFile()) {
			// print file names
			Serial.print("File: ");
			Serial.println(dir_path + dir.fileName());
		}
		if (dir.isDirectory()) {
			// print directory names
			Serial.print("Dir: ");
			Serial.println(dir_path + dir.fileName() + "/");
			// recursive file listing inside new directory
			listAllFilesInDir(dir_path + dir.fileName() + "/");
		}
	}
}

size_t LittleFSFilesize(const char* filename) {
  auto file = LittleFS.open(filename, "r");
  size_t filesize = file.size();
  // Don't forget to clean up!
  file.close();
  return filesize;
}

void handlerReboot(AsyncWebServerRequest *request) {
  digitalWrite(led, 1);
  StreamString temp;
  temp.reserve(500);
  temp.printf("<html><head><title>Reboot</title>\
        <link rel=\"stylesheet\" href=\"./style.mini.css\">\
        <script src=\"./script.mini.js\"></script>\
    </head>\
    <body>\
        Reboot after <div id=\"counter\">5</div> secends!\
    </body>\
  </html>");
  AsyncWebServerResponse *response = request->beginResponse(200, "text/html", temp);
  request->send(response);
  digitalWrite(led, 0);
}

void handlerDoReboot(AsyncWebServerRequest *request) {
  digitalWrite(led, 1);
  request->send(200, "text/html", "OK");
  rp2040.reboot();
  digitalWrite(led, 0);
}

// Функция которая выводит главную страницу http://soler-exporter.local/
void handlerRoot(AsyncWebServerRequest *request) {
  digitalWrite(led, 1);
  StreamString temp;
  temp.reserve(500); // Preallocate a large chunk to avoid memory fragmentation
  temp.printf("<html>\
  <head>\
    <title>Solar Inverter SmartWatt ECO Exporter v%s</title>\
  </head>\
  <body>\
    <h1>Solar Inverter SmartWatt ECO Exporter v%s</h1>\
    <a href=\"/metrics\">Metrics</a></br>\
    <a href=\"/update\">Update</a></br>\
  </body>\
</html>", app_version, app_version);
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
    <title>Solar Inverter SmartWatt ECO Exporter v%s</title>\n\
  </head>\n\
  <body>\n\
  # HELP solar_exporter_version_info is a version of solar exporter appplication started in raspbery pico w micro controller</br>\n\
  # TYPE solar_exporter_version_info gauge</br>\n", app_version);
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

void handleUpdate(AsyncWebServerRequest *request) {
  digitalWrite(led, 1);
  StreamString temp;
  temp.reserve(500);
  temp.printf("<form method='POST' action='/upload' enctype='multipart/form-data'><input type='file' name='upload'><input type='submit' value='Upload'></form>");
  AsyncWebServerResponse *response = request->beginResponse(200, "text/html", temp);
  request->send(response);
  digitalWrite(led, 0);
}

// GET REQUEST
// Страница обновлений /update
void handleUpdateStatus(AsyncWebServerRequest *request) {
  digitalWrite(led, 1);
  AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", "OK");
  response->addHeader("X-debug", "handleUpdateStatus-end");
  response->addHeader("Connection", "close");
  request->send(response);
  digitalWrite(led, 0);
}

// POST REQUEST
// url страници /update
// Upload file and flash it to FLASH
void handleUpload(AsyncWebServerRequest * request, String filename,size_t index, uint8_t *data, size_t len, bool final) {
  int httpStatus = 200;
  String message;
  boolean error = false;

  // Загрузка файла началась.
  if (!index) {
    // Если файл не начинается с / то добавляем его
    if (!filename.startsWith("/")){ filename = "/" + filename; }
    // Сравниваем имя файла чтобы понять что мы загружаем
    if (filename == "/firmware.bin") {
      Serial.printf("Uploading firmwire file : %s\n\r", filename.c_str());
      uploadFile = LittleFS.open(filename, "w");
    } else {
      error = true;
      final = true;
      Serial.printf("Illegal upload filename : %s\n\r", filename.c_str());
    }
  }
  if (!error) {
    for (size_t i = 0; i < len; i++){
      uploadFile.write(data[i]);
    }
  }
  // Если больше нет данных то final становится true
  if (final) {
    if (!error) {
      uploadFile.close();
      // Закрываем файл в который записывали загружаемый файл
      Serial.printf("Writed file size is : %i\n\r", LittleFSFilesize(filename.c_str()));
      Serial.printf("Upload file size is : %i\n\r", len+index);
      picoOTA.begin();
      picoOTA.addFile("/firmware.bin");
      picoOTA.commit();
      message = "All OK. Board reboot.";
    } else {
      httpStatus = 500;
      message = "Illegal file";
    }

    request->redirect("/reboot");
  }
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
  Serial.print("\" ");
  Serial.print("\n\r");
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  delay(2000);
  // clean screen
  Serial.write(27);       // ESC command
  Serial.print("[2J");    // clear screen command
  Serial.write(27);
  Serial.print("[H");     // cursor to home command

  Serial.printf("Starting Firmware version: %s\n\r", app_version);
  Serial.print("Wifi Firmware Version: "); 
  Serial.println(WiFi.firmwareVersion());
  Serial.print("Wifi init...");
  
  // инициализация wifi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  delay(2000);

  // в процессе инициализцаии wifi ресуем иочку каждые 500 милисекунд
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  // Если wifi успешно подключен то сообщение
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("OK");
  }

  // если Wifi не подключился к точке доступа то перезагружаем плату и всё сначала
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("ERROR");
    Serial.println("Reboot...!");
    delay(2000);
    rp2040.reboot();
  }

  Serial.print("MDNS init...");

  bool mdnsstatus = MDNS.begin(dnsname);

  if (mdnsstatus) {
    Serial.println("OK");
  } else {
    Serial.println("ERROR");
  }

  //Littlafs initial
  Serial.print("Littlefs init...");

  if (LittleFS.begin()){
    Serial.println("OK");
    listAllFilesInDir("/");
  } else {
    Serial.println("ERROR");
    delay(2000);
    rp2040.reboot();
  }

  // Выводим ip вдррес который получили по DHCP
  Serial.printf("IP address : %d.%d.%d.%d\n\r", WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3]);
  Serial.printf("Netmask    : %d.%d.%d.%d\n\r", WiFi.subnetMask()[0], WiFi.subnetMask()[1], WiFi.subnetMask()[2], WiFi.subnetMask()[3]);
  Serial.printf("Gateway    : %d.%d.%d.%d\n\r", WiFi.gatewayIP()[0], WiFi.gatewayIP()[1], WiFi.gatewayIP()[2], WiFi.gatewayIP()[3]);

  Serial.printf("LittleFS partition size : %i\n\r", lfs_fs_size);
  Serial.printf("LittleFS free space : %i\n\r", lfs_fs_traverse);

  if (LittleFS.exists("/firmware.bin")){
    Serial.printf("Remove firmware.bin!\n\r");
    LittleFS.remove("/firmware.bin");
  }

  // GET methods
  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request){
    handlerRoot(request);
    printrequestdata(request);
  });
  server.on("/metrics", HTTP_GET, [](AsyncWebServerRequest * request){
    handlerMetrics(request);
    printrequestdata(request);
  });
  server.on("/update", HTTP_GET, [](AsyncWebServerRequest *request){
    handleUpdate(request);
    printrequestdata(request);
  });
  server.on("/reboot", HTTP_GET, [](AsyncWebServerRequest *request){
    handlerReboot(request);
  });
    server.on("/doreboot", HTTP_GET, [](AsyncWebServerRequest *request){
    handlerDoReboot(request);
  });
  server.on("/firmware.bin", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/firmware.bin", "application/octet-stream");
  });
  server.on("/otacommand.bin", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/otacommand.bin", "application/octet-stream");
  });
  server.on("/script.mini.js", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/script.mini.js", "text/javascript");
  });
  server.on("/style.mini.css", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/style.mini.css", "text/css");
  });

  server.on("/upload", HTTP_POST, [](AsyncWebServerRequest *request){
    handleUpdateStatus(request);
    printrequestdata(request);
  },[](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final){
    handleUpload(request, filename, index, data, len, final);
  });

  // HEAD methods
  server.on("/metrics", HTTP_HEAD, [](AsyncWebServerRequest * request){
    digitalWrite(led, 1);
    AsyncWebServerResponse *response = request->beginResponse(200, "text/plan", "OK");
    request->send(response);
    printrequestdata(request);
    digitalWrite(led, 0);
  });
  server.onNotFound([](AsyncWebServerRequest * request){
    digitalWrite(led, 1);
    request->send(404, "text/html", "Not Found");
    printrequestdata(request);
    digitalWrite(led, 0);
  });
  server.begin();

}

void loop() {
  
}
