/* Copyright 2019 Motea Marius

  This example code will create a webserver that will provide basic control to AC units using the web application
  build with javascript/css. User config zone need to be updated if a different class than Collix need to be used.
  Javasctipt file may also require minor changes as in current version it will not allow to set fan speed if Auto mode
  is selected (required for Coolix).

*/
#include "Web-AC-control.h"
#if defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ESP8266WebServer.h>
#endif  // ESP8266
#if defined(ESP32)
#include <ESPmDNS.h>
#include <WebServer.h>
#include <WiFi.h>
#include <Update.h>
#endif  // ESP32
#include <WiFiUdp.h>
#include <WiFiManager.h>
#include <ArduinoJson.h>
#include <ArduinoOTA.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>


//// ###### User configuration space for AC library classes ##########

// #include <ir_Haier.h>  //  replace library based on your AC unit model, check https://github.com/crankyoldgit/IRremoteESP8266
#include "IRHaierAC160.h"

#define AUTO_MODE kHaierAcAuto
#define COOL_MODE kHaierAcCool 
#define DRY_MODE kHaierAcDry
#define HEAT_MODE kHaierAcHeat 
#define FAN_MODE kHaierAcFan

#define FAN_AUTO kHaierAcFanAuto
#define FAN_MIN kHaierAcFanLow
#define FAN_MED kHaierAcFanMed
#define FAN_HI kHaierAcFanHigh 

#define SWING_OFF kHaierAcSwingVOff
#define SWING_UP kHaierAcSwingVOff
#define SWING_DOWN kHaierAcSwingVOff
#define SWING_CHG kHaierAcSwingVOff

// ESP8266 GPIO pin to use for IR blaster.
const uint16_t kIrLed = 3;
// Library initialization, change it according to the imported library file.
//IRHaierAC ac(kIrLed);
//IRHaierAC176 ac(kIrLed);
// IRHaierACYRW02 ac(kIrLed);
IRHaierAC160 ac(kIrLed);


IRsend irsend(kIrLed);
MDNSResponder mdns;

/// ##### End user configuration ######

struct state {
  bool powerStatus = false;
  uint8_t temperature = 26;
  uint8_t swingV = 1;
  bool health = true;
  bool quiet = false;
  bool turbo = false;
  uint8_t fanSpeed = 0;
  uint8_t  climate = 1;
  bool toggleDisp = false;
};

File fsFileHandle;
String configFilename = "acStateConf.json";
// core
state acState;
// settings
char deviceName[] = "Haier AC Remote Control";
#if defined(ESP8266)
ESP8266WebServer server(80);
ESP8266HTTPUpdateServer httpUpdateServer;
WiFiManager wifiManager;
#undef HOSTNAME
#define HOSTNAME "ac"
#endif  // ESP8266
#if defined(ESP32)
WebServer server(80);
#undef HOSTNAME
#define HOSTNAME "esp32"
#endif  // ESP32

String loadFromFile(String file_name) {
  String result = "";
  fsFileHandle = FILESYSTEM.open(file_name, "r");
  if (!fsFileHandle) { // failed to open the file, retrn empty result
    return result;
  }
  while (fsFileHandle.available()) {
      result += (char)fsFileHandle.read();
  }
  fsFileHandle.close();
  return result;
}

bool writeToFile(String file_name, String contents) {  
  fsFileHandle = FILESYSTEM.open(file_name, "w");
  if (!fsFileHandle) { // failed to open the file, return false
    return false;
  }
  int bytesWritten = fsFileHandle.print(contents);
  if (bytesWritten == 0) { // write failed
      return false;
  }
  fsFileHandle.close();
  return true;
}

void loadConfig(){    
  if(FILESYSTEM.exists(configFilename)){
    String jsn = loadFromFile(configFilename);
    DynamicJsonDocument root(1024);
    deserializeJson(root,jsn);
    if (root.containsKey("temp")) {
        acState.temperature = (uint8_t) root["temp"];
    }
    if (root.containsKey("fan")) {
      acState.fanSpeed = (uint8_t) root["fan"];
    }
    if (root.containsKey("power")) {
      acState.powerStatus = (bool)root["power"];
    }
    if (root.containsKey("mode")) {
      acState.climate = (uint8_t)root["mode"];
    }
    if (root.containsKey("swingV")) {
      acState.swingV = (uint8_t)root["swingV"];
    }
    if (root.containsKey("health")) {
      acState.health = (bool)root["health"];
    }
    if (root.containsKey("quiet")) {
      acState.quiet = (bool)root["quiet"];
    }
    if (root.containsKey("turbo")) {
      acState.turbo = (bool)root["turbo"];
    }
  }

}

bool handleFileRead(String path) {
  //  send the right file to the client (if it exists)
  // Serial.println("handleFileRead: " + path);
  if (path.endsWith("/")) path += "index.html";
  // If a folder is requested, send the index file
  String contentType = mime::getContentType(path);
  // Get the MIME type
  String pathWithGz = path + ".gz";
  if (FILESYSTEM.exists(pathWithGz) || FILESYSTEM.exists(path)) {
    // If the file exists, either as a compressed archive, or normal
    // If there's a compressed version available
    if (FILESYSTEM.exists(pathWithGz))
      path += ".gz";  // Use the compressed verion
    File file = FILESYSTEM.open(path, "r");
    //  Open the file
    server.streamFile(file, contentType);
    //  Send it to the client
    file.close();
    // Close the file again
    // Serial.println(String("\tSent file: ") + path);
    return true;
  }
  // Serial.println(String("\tFile Not Found: ") + path);
  // If the file doesn't exist, return false
  return false;
}

String getContentType(String filename) {
  // convert the file extension to the MIME type
  if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  else if (filename.endsWith(".gz")) return "application/x-gzip";
  return "text/plain";
}

void handleFileUpload() {  // upload a new file to the FILESYSTEM
  HTTPUpload& upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    String filename = upload.filename;
    if (!filename.startsWith("/")) filename = "/" + filename;
    // Serial.print("handleFileUpload Name: "); //Serial.println(filename);
    fsFileHandle = FILESYSTEM.open(filename, "w");
    // Open the file for writing in FILESYSTEM (create if it doesn't exist)
    filename = String();
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (fsFileHandle)
      fsFileHandle.write(upload.buf, upload.currentSize);
      // Write the received bytes to the file
  } else if (upload.status == UPLOAD_FILE_END) {
    if (fsFileHandle) {
      // If the file was successfully created
      fsFileHandle.close();
      // Close the file again
      // Serial.print("handleFileUpload Size: ");
      // Serial.println(upload.totalSize);
      server.sendHeader("Location", "/ui.html");
      // Redirect the client to the ui page
      server.send(303);
    } else {
      server.send(500, "text/plain", "500: couldn't create file");
    }
  }
}


void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

void handleIr() {
  for (uint8_t i = 0; i < server.args(); i++) {
    if (server.argName(i) == "code") {
      uint32_t code = strtoul(server.arg(i).c_str(), NULL, 10);
#if SEND_NEC
      irsend.sendNEC(code, 32);
#endif  // SEND_NEC
    }
  }
  server.send(200);
}

void deleteRecursive(String path) {
  File file = FILESYSTEM.open(path, "r");
  bool isDir = file.isDirectory();
  file.close();

  // If it's a plain file, delete it
  if (!isDir) {
    FILESYSTEM.remove(path);
    return;
  }

  // Otherwise delete its contents first
  Dir dir = FILESYSTEM.openDir(path);

  while (dir.next()) {
    deleteRecursive(path + '/' + dir.fileName());
  }

  // Then delete the folder itself
  FILESYSTEM.rmdir(path);
}

void setup() {
  // Serial.begin(115200);
  // Serial.println();
  ac.begin();
  irsend.begin();


  delay(1000);

  Serial.println("mounting " FILESYSTEMSTR "...");

  if (!FILESYSTEM.begin()) {
    // Serial.println("Failed to mount file system");
    return;
  }

  wifiManager.setConnectTimeout(60*2);
  if (!wifiManager.autoConnect(deviceName)) {
    delay(3000);
    ESP.restart();
    delay(5000);
  }


#if defined(ESP8266)
  httpUpdateServer.setup(&server);
#endif  // ESP8266

#if defined(ESP8266)
  if (mdns.begin(HOSTNAME, WiFi.localIP())) {
#else  // ESP8266
  if (mdns.begin(HOSTNAME)) {
#endif  // ESP8266
    Serial.println("MDNS responder started");
    // Announce http tcp service on port 80
    mdns.addService("http", "tcp", 80);
  }

  server.on("/ir", handleIr);

  server.on("/disp", HTTP_PUT, []() {
    ac.toggleDisplayLED();
    ac.send();
    server.send(200);
  });

  server.on("/state", HTTP_PUT, []() {
    DynamicJsonDocument root(1024);
    DeserializationError error = deserializeJson(root, server.arg("plain"));
    if (error) {
      server.send(404, "text/plain", "FAIL. " + server.arg("plain"));
    } else {
      if (root.containsKey("temp")) {
        acState.temperature = (uint8_t) root["temp"];
      }
      if (root.containsKey("fan")) {
        acState.fanSpeed = (uint8_t) root["fan"];
      }
      if (root.containsKey("power")) {
        acState.powerStatus = (bool)root["power"];
      }
      if (root.containsKey("mode")) {
        acState.climate = (uint8_t)root["mode"];
      }
      if (root.containsKey("swingV")) {
        acState.swingV = (uint8_t)root["swingV"];
      }
      if (root.containsKey("health")) {
        acState.health = (bool)root["health"];
      }
      if (root.containsKey("quiet")) {
        acState.quiet = (bool)root["quiet"];
      }
      if (root.containsKey("turbo")) {
        acState.turbo = (bool)root["turbo"];
      }
      if (root.containsKey("disp")) {
        acState.toggleDisp = (bool)root["disp"];
      }

      //---------------------------------------------

      delay(200);

      if (acState.powerStatus) {
        ac.on();
        ac.setTemp(acState.temperature);

        if (acState.climate == 0) {
          ac.setMode(kHaierAc160Auto);
          ac.setFan(kHaierAc160FanAuto);
          acState.fanSpeed = 0;
        } else if (acState.climate == 1) {
          ac.setMode(kHaierAc160Cool);
        } else if (acState.climate == 2) {
          ac.setMode(kHaierAc160Dry);
        } else if (acState.climate == 3) {
          ac.setMode(kHaierAc160Heat);
        } else if (acState.climate == 4) {
          ac.setMode(kHaierAc160Fan);
        }

//        if (acState.climate != 0) {
          if (acState.fanSpeed == 0) {
            ac.setFan(kHaierAc160FanAuto);
          } else if (acState.fanSpeed == 1) {
            ac.setFan(kHaierAc160FanLow);
          } else if (acState.fanSpeed == 2) {
            ac.setFan(kHaierAc160FanMed);
          } else if (acState.fanSpeed == 3) {
            ac.setFan(kHaierAc160FanHigh);
          }
//        }

        // if (acState.swingV != 0) {
          if (acState.swingV == 0) {
            ac.setSwingV(kHaierAc160SwingVOff);
          } else if (acState.swingV == 1) {
            ac.setSwingV(kHaierAc160SwingVAuto);
          } else if (acState.swingV == 2) {
            ac.setSwingV(kHaierAc160SwingVTop);
          } else if (acState.swingV == 3) {
            ac.setSwingV(kHaierAc160SwingVFront);
          } else if (acState.swingV == 4) {
            ac.setSwingV(kHaierAc160SwingVBottom);
          } else if (acState.swingV == 5) {
            ac.setSwingV(kHaierAc160SwingVDown);
          } 
        // }

        ac.setHealth(acState.health);
        ac.setTurbo(acState.turbo);
        ac.setQuiet(acState.quiet);

        if(acState.toggleDisp){
          ac.toggleDisplayLED();
        }

      } else {
        ac.off();
      }
      acState.toggleDisp = false;

      String output;
      serializeJson(root, output);

      if(server.hasArg("save")){
          if(!writeToFile(configFilename,output)){
            output+="Error While Writing Configuration file.";
          }
      }else{
        ac.send();
      }

      server.send(200, "text/plain", output);
    }
  });

  server.on("/file-upload", HTTP_POST,
  // if the client posts to the upload page
  []() {
    // Send status 200 (OK) to tell the client we are ready to receive
    server.send(200);
  },
  handleFileUpload);  // Receive and save the file

  server.on("/file-upload", HTTP_GET, []() {
    // if the client requests the upload page

    String html = "<form method=\"post\" enctype=\"multipart/form-data\">";
    html += "<input type=\"file\" name=\"name\">";
    html += "<input class=\"button\" type=\"submit\" value=\"Upload\">";
    html += "</form>";
    server.send(200, "text/html", html);
  });

  server.on("/", []() {
    server.sendHeader("Location", String("ui.html"), true);
    server.send(302, "text/plain", "");
  });

  server.on("/state", HTTP_GET, []() {
    DynamicJsonDocument root(1024);
    root["mode"] = acState.climate;
    root["fan"] = acState.fanSpeed;
    root["swingV"] = acState.swingV;
    root["temp"] = acState.temperature;
    root["power"] = acState.powerStatus;
    root["health"] = acState.health;
    root["quiet"] = acState.quiet;
    root["turbo"] = acState.turbo;
    String output;
    serializeJson(root, output);
    server.send(200, "text/plain", output);
  });


  server.on("/reset", []() {
    server.send(200, "text/html", "reset");
    delay(100);
    wifiManager.resetSettings();
    ESP.restart();
  });

  server.on("/reboot", []() {
    server.send(200, "text/html", "reboot");
    delay(100);
    ESP.restart();
  });

  server.on("/fsinfo", []() {
    FSInfo fs_info;
    Dir dir = FILESYSTEM.openDir("/");
    String json;
    // json.reserve(128);
    json = "{\"type\":\"";
    json += FILESYSTEMSTR;
    json += "\", \"isOk\":";
    FILESYSTEM.info(fs_info);
    json += F("\"true\", \"totalBytes\":\"");
    json += fs_info.totalBytes;
    json += F("\", \"usedBytes\":\"");
    json += fs_info.usedBytes;
    json += "\", \"files\":\"[";
    while (dir.next()) {
        json += F("[\"");
        json += dir.fileName();
        json += F("\",\"");
        if(dir.fileSize()) {
            File f = dir.openFile("r");
            json +=f.size();
        }
        json += "\"],";
    }
    json += "]}";
    server.send(200, "application/json", json);
  });


  server.on("/delAll", []() {
    server.send(200, "text/html", "Clearing FS.");
    deleteRecursive("/");
  });

  server.on("/raw", []() {
    String msgInt = "kHaierACStateLength(INT): ";
    String msgHex = "kHaierACStateLength(HEX): ";
    char buffer [4];
    uint8_t * raw = ac.getRaw();
    for(int i=0 ;i<kHaierACStateLength;i++){
    itoa ((unsigned int)raw[i],buffer,10);
      msgInt += buffer;
    itoa ((unsigned int)raw[i],buffer,16);
      msgHex += buffer;
    }
    msgInt += "\nkHaierAC176StateLength(INT): ";
    msgHex += "\nkHaierAC176StateLength(HEX): ";
    for(int i=0 ;i<kHaierAC176StateLength;i++){
    itoa ((unsigned int)raw[i],buffer,10);
      msgInt += buffer;
    itoa ((unsigned int)raw[i],buffer,16);
      msgHex += buffer;
    }
    msgInt += "\nkHaierACYRW02StateLength(INT): ";
    msgHex += "\nkHaierACYRW02StateLength(HEX): ";
    for(int i=0 ;i<kHaierACYRW02StateLength;i++){
    itoa ((unsigned int)raw[i],buffer,10);
      msgInt += buffer;
    itoa ((unsigned int)raw[i],buffer,16);
      msgHex += buffer;;
    }
    server.send(200, "text/html", msgInt + "\n" + msgHex);
  });


  server.serveStatic("/", FILESYSTEM, "/", "max-age=86400");

  server.onNotFound(handleNotFound);

  server.begin();
  ArduinoOTA.begin();
  loadConfig();
}


void loop() {
  #if defined(ESP8266)
  mdns.update();
  #endif
  server.handleClient();
  ArduinoOTA.handle();
}
