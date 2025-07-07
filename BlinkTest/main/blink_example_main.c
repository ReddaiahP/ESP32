#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "LED_PATTERN";

// Define LED pins (changed GPIO 34 to GPIO 25, an output-capable pin)
#define LED1_PIN 32 // D32
#define LED2_PIN 33 // D33
#define LED3_PIN 25 // Changed from D34 to D25 (output-capable)

void led_task(void *pvParameters)
{
    // Configure LED pins as output
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LED1_PIN) | (1ULL << LED2_PIN) | (1ULL << LED3_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    esp_err_t err = gpio_config(&io_conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure GPIO pins: %s", esp_err_to_name(err));
        return;
    }

    // Turn off all LEDs initially
    err = gpio_set_level(LED1_PIN, 0);
    if (err != ESP_OK) ESP_LOGE(TAG, "Failed to set LED1 level: %s", esp_err_to_name(err));
    err = gpio_set_level(LED2_PIN, 0);
    if (err != ESP_OK) ESP_LOGE(TAG, "Failed to set LED2 level: %s", esp_err_to_name(err));
    err = gpio_set_level(LED3_PIN, 0);
    if (err != ESP_OK) ESP_LOGE(TAG, "Failed to set LED3 level: %s", esp_err_to_name(err));

    ESP_LOGI(TAG, "LED Pattern Started");

    while (1) {
        // Pattern: Turn on each LED one at a time
        err = gpio_set_level(LED1_PIN, 1); // Turn on LED1
        if (err == ESP_OK) ESP_LOGI(TAG, "LED1 ON");
        else ESP_LOGE(TAG, "Failed to turn on LED1: %s", esp_err_to_name(err));
        vTaskDelay(500 / portTICK_PERIOD_MS); // Wait 500ms
        err = gpio_set_level(LED1_PIN, 0); // Turn off LED1
        if (err == ESP_OK) ESP_LOGI(TAG, "LED1 OFF");
        else ESP_LOGE(TAG, "Failed to turn off LED1: %s", esp_err_to_name(err));

        err = gpio_set_level(LED2_PIN, 1); // Turn on LED2
        if (err == ESP_OK) ESP_LOGI(TAG, "LED2 ON");
        else ESP_LOGE(TAG, "Failed to turn on LED2: %s", esp_err_to_name(err));
        vTaskDelay(500 / portTICK_PERIOD_MS); // Wait 500ms
        err = gpio_set_level(LED2_PIN, 0); // Turn off LED2
        if (err == ESP_OK) ESP_LOGI(TAG, "LED2 OFF");
        else ESP_LOGE(TAG, "Failed to turn off LED2: %s", esp_err_to_name(err));

        err = gpio_set_level(LED3_PIN, 1); // Turn on LED3
        if (err == ESP_OK) ESP_LOGI(TAG, "LED3 ON");
        else ESP_LOGE(TAG, "Failed to turn on LED3: %s", esp_err_to_name(err));
        vTaskDelay(500 / portTICK_PERIOD_MS); // Wait 500ms
        err = gpio_set_level(LED3_PIN, 0); // Turn off LED3
        if (err == ESP_OK) ESP_LOGI(TAG, "LED3 OFF");
        else ESP_LOGE(TAG, "Failed to turn off LED3: %s", esp_err_to_name(err));

        vTaskDelay(1000 / portTICK_PERIOD_MS); // Wait 1 second before repeating
    }
}

void app_main(void)
{
    // Initialize UART for logging
    esp_log_level_set(TAG, ESP_LOG_INFO);

    // Create LED task
    xTaskCreate(led_task, "led_task", 2048, NULL, 5, NULL);
}