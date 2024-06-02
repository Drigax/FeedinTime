#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <WiFi.h>

// ESP32 Specific includes, consider refactoring to header with platform specific defines.
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"
#include <ESPAsyncWebServer.h>
#include "SPIFFS.h"

//TODO: move versioning info to a config file.
const int versionMajor = 0;
const int versionMinor = 1;
const int versionHotfix = 0;

//TODO: move ssid and password to a config file.
const char* ssid = "SSID"; //replace with actual ssid and password
const char* password = "PASSWORD";
int webserverPort = 80;

void restart(){
    for (int i = 10; i >= 0; i--) {
        printf("Restarting in %d seconds...\n", i);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    printf("Restarting now.\n");
    fflush(stdout);
    esp_restart();
}

void printSystemInfo(){
    /* Print chip information */
    esp_chip_info_t chip_info;
    uint32_t flash_size;
    esp_chip_info(&chip_info);
    printf("This is %s chip with %d CPU core(s), %s%s%s%s, ",
           CONFIG_IDF_TARGET,
           chip_info.cores,
           (chip_info.features & CHIP_FEATURE_WIFI_BGN) ? "WiFi/" : "",
           (chip_info.features & CHIP_FEATURE_BT) ? "BT" : "",
           (chip_info.features & CHIP_FEATURE_BLE) ? "BLE" : "",
           (chip_info.features & CHIP_FEATURE_IEEE802154) ? ", 802.15.4 (Zigbee/Thread)" : "");

    unsigned major_rev = chip_info.revision / 100;
    unsigned minor_rev = chip_info.revision % 100;
    printf("silicon revision v%d.%d, ", major_rev, minor_rev);
    if(esp_flash_get_size(NULL, &flash_size) != ESP_OK) {
        printf("Get flash size failed");
        return;
    }

    printf("%" PRIu32 "MB %s flash\n", flash_size / (uint32_t)(1024 * 1024),
           (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

    printf("Minimum free heap size: %" PRIu32 " bytes\n", esp_get_minimum_free_heap_size());
}

void notFound(AsyncWebServerRequest *request)
{
    if (request->method() == HTTP_OPTIONS)
    {
        request->send(200);
    }
    else
    {
        request->send(404, "application/json", "{\"message\":\"Not found\"}");
    }
}

void app_main() {
    printf("Initializing FeedinTime v{}");
    printSystemInfo();

    // Connect to wifi network as client.
    WiFi.mode(WIFI_STA); //Optional
    WiFi.begin(ssid, password);
    printf("\nConnecting to %s\n", ssid);

    while(WiFi.status() != WL_CONNECTED){
        Serial.print(".");
        delay(100);
    }
    printf("\nConnected to %s.\n", ssid);
    printf("Local ESP32 IP: ");
    printf(WiFi.localIP());
    printf("\n");

    // Initialize Web server.
    printf("Initializing web server on port %d.\n", webserverPort);
    AsyncWebServer server(webserverPort);

    // Initialize file system
    printf("\nInitializing SPIFFS file system.\n");
    if (!SPIFFS.begin(true))
    {
        printf("An Error has occurred while mounting SPIFFS. Restarting...\n");
        restart();
        return;
    }

    // Allow Any URL to make calls to endpoints.
    printf("\nInitializing webserver default headers.\n");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, POST, PUT");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Content-Type");

    // Serve the filesystem
    printf("Serving contents of FeedinTime/data/ at / and /static/ .\n");
    server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");
    server.serveStatic("/static/", SPIFFS, "/");

    // /schedule -> send json document of feeding schedule.
    printf("Setting up callback for /schedule .\n");
    server.on("/schedule", HTTP_GET, [](AsyncWebServerRequest *request)
        {
        String out;
        serializeJson(feedingSchedule, out);
        request->send(200, "text/json", out);
    });

    // in case we have a request that we cannot serve.
    server.onNotFound(notFound);

    // Finally, start the webserver.
    server.begin();
}

