#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_http_server.h"
#include "nvs_flash.h"

static const char *TAG = "OTA_SERVER";

// HTML page for OTA update (same as Arduino version)
static const char *server_index = 
"<!DOCTYPE html>"
"<html>"
"<head>"
  "<title>ESP32 OTA Update</title>"
  "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
  "<style>"
    "body {"
      "font-family: Arial, sans-serif;"
      "text-align: center;"
      "margin: 0;"
      "padding: 20px;"
      "background-color: #f5f5f5;"
    "}"
    ".container {"
      "max-width: 500px;"
      "margin: 0 auto;"
      "background: white;"
      "padding: 20px;"
      "border-radius: 10px;"
      "box-shadow: 0 0 10px rgba(0,0,0,0.1);"
    "}"
    "h1 {"
      "color: #2c3e50;"
    "}"
    ".upload-btn {"
      "background-color: #3498db;"
      "color: white;"
      "padding: 10px 20px;"
      "border: none;"
      "border-radius: 5px;"
      "cursor: pointer;"
      "font-size: 16px;"
      "margin-top: 20px;"
    "}"
    ".upload-btn:hover {"
      "background-color: #2980b9;"
    "}"
    ".progress-container {"
      "width: 100%;"
      "background-color: #ecf0f1;"
      "border-radius: 5px;"
      "margin: 20px 0;"
      "display: none;"
    "}"
    ".progress-bar {"
      "width: 0%;"
      "height: 30px;"
      "background-color: #2ecc71;"
      "border-radius: 5px;"
      "text-align: center;"
      "line-height: 30px;"
      "color: white;"
    "}"
    "#status {"
      "margin-top: 10px;"
      "font-weight: bold;"
    "}"
    "#filename {"
      "margin-top: 10px;"
      "color: #7f8c8d;"
    "}"
  "</style>"
"</head>"
"<body>"
  "<div class=\"container\">"
    "<h1>ESP32 Firmware Update</h1>"
    "<p>Upload a new firmware file (.bin) to update the device</p>"
    "<form id=\"uploadForm\">"
      "<input type=\"file\" id=\"firmwareFile\" accept=\".bin\" style=\"display: none;\">"
      "<button type=\"button\" class=\"upload-btn\" onclick=\"document.getElementById('firmwareFile').click()\">Select Firmware File</button>"
      "<div id=\"filename\"></div>"
      "<button type=\"button\" class=\"upload-btn\" onclick=\"uploadFile()\" id=\"uploadBtn\" disabled>Upload Firmware</button>"
    "</form>"
    "<div class=\"progress-container\" id=\"progressContainer\">"
      "<div class=\"progress-bar\" id=\"progressBar\">0%</div>"
    "</div>"
    "<div id=\"status\"></div>"
  "</div>"
  "<script>"
    "document.getElementById('firmwareFile').addEventListener('change', function(e) {"
      "if (this.files.length > 0) {"
        "document.getElementById('filename').innerHTML = 'Selected: ' + this.files[0].name;"
        "document.getElementById('uploadBtn').disabled = false;"
      "}"
    "});"
    "function uploadFile() {"
      "const fileInput = document.getElementById('firmwareFile');"
      "const file = fileInput.files[0];"
      "const progressContainer = document.getElementById('progressContainer');"
      "const progressBar = document.getElementById('progressBar');"
      "const statusDiv = document.getElementById('status');"
      "if (!file) {"
        "statusDiv.innerHTML = 'Please select a file first';"
        "statusDiv.style.color = 'red';"
        "return;"
      "}"
      "const xhr = new XMLHttpRequest();"
      "const formData = new FormData();"
      "formData.append('update', file);"
      "xhr.open('POST', '/update', true);"
      "progressContainer.style.display = 'block';"
      "statusDiv.innerHTML = 'Uploading...';"
      "statusDiv.style.color = 'black';"
      "xhr.upload.onprogress = function(e) {"
        "if (e.lengthComputable) {"
          "const percent = Math.round((e.loaded / e.total) * 100);"
          "progressBar.style.width = percent + '%';"
          "progressBar.innerHTML = percent + '%';"
        "}"
      "};"
      "xhr.onload = function() {"
        "if (xhr.status == 200) {"
          "statusDiv.innerHTML = xhr.responseText;"
          "statusDiv.style.color = 'green';"
          "if (xhr.responseText.indexOf('OK') !== -1) {"
            "setTimeout(function() {"
              "statusDiv.innerHTML += '<br>Device will restart shortly...';"
            "}, 1000);"
          "}"
        "} else {"
          "statusDiv.innerHTML = 'Error: ' + xhr.responseText;"
          "statusDiv.style.color = 'red';"
        "}"
      "};"
      "xhr.onerror = function() {"
        "statusDiv.innerHTML = 'Upload failed';"
        "statusDiv.style.color = 'red';"
      "};"
      "xhr.send(formData);"
    "}"
  "</script>"
"</body>"
"</html>";

// OTA handle
static esp_ota_handle_t ota_handle;
static bool ota_in_progress = false;

static esp_err_t index_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, server_index, strlen(server_index));
    return ESP_OK;
}

static esp_err_t update_handler(httpd_req_t *req)
{
    char buf[1000];
    int recv_len;
    esp_err_t err;

    if (!ota_in_progress) {
        esp_ota_handle_t update_handle = 0;
        const esp_partition_t *update_partition = esp_ota_get_next_update_partition(NULL);
        err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &update_handle);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "OTA Begin failed (%s)", esp_err_to_name(err));
            httpd_resp_send_500(req);
            return err;
        }
        ota_handle = update_handle;
        ota_in_progress = true;
        ESP_LOGI(TAG, "OTA Update started");
    }

    while ((recv_len = httpd_req_recv(req, buf, sizeof(buf))) > 0) {
        err = esp_ota_write(ota_handle, (const void *)buf, recv_len);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "OTA Write failed (%s)", esp_err_to_name(err));
            ota_in_progress = false;
            esp_ota_end(ota_handle);
            httpd_resp_send_500(req);
            return err;
        }
    }

    if (recv_len == 0) {
        err = esp_ota_end(ota_handle);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "OTA End failed (%s)", esp_err_to_name(err));
            ota_in_progress = false;
            httpd_resp_send_500(req);
            return err;
        }

        err = esp_ota_set_boot_partition(esp_ota_get_next_update_partition(NULL));
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "OTA Set boot partition failed (%s)", esp_err_to_name(err));
            ota_in_progress = false;
            httpd_resp_send_500(req);
            return err;
        }

        httpd_resp_sendstr(req, "OK");
        ESP_LOGI(TAG, "OTA Update successful, restarting...");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        esp_restart();
    }

    return ESP_OK;
}

static void start_webserver(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_uri_t index_uri = {
            .uri       = "/",
            .method    = HTTP_GET,
            .handler   = index_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &index_uri);

        httpd_uri_t update_uri = {
            .uri       = "/update",
            .method    = HTTP_POST,
            .handler   = update_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &update_uri);
    }
}

static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        ESP_LOGI(TAG, "Station connected to AP");
    }
}

static void wifi_init_softap(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = "ESP32-OTA-Update",
            .ssid_len = strlen("ESP32-OTA-Update"),
            .password = "update1234",
            .max_connection = 4,
            .authmode = WIFI_AUTH_WPA2_PSK
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    esp_netif_ip_info_t ip_info;
    esp_netif_t *ap_netif = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
    esp_netif_get_ip_info(ap_netif, &ip_info);
    ESP_LOGI(TAG, "AP IP address: " IPSTR, IP2STR(&ip_info.ip));
}

void app_main(void)
{
    ESP_LOGI(TAG, "Starting OTA Update Server");

    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Initialize WiFi and start webserver
    wifi_init_softap();
    start_webserver();
}