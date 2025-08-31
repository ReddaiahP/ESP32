#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"

#define MOSI 12
#define MISO 13
#define SCLK 15
#define CS   14

void app_main(void)
{
    spi_device_handle_t handle;
    spi_bus_config_t spi_config = {
        .mosi_io_num = MOSI,
        .miso_io_num = MISO,
        .sclk_io_num = SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 0
    };

    spi_device_interface_config_t spi_device_config = {
        .clock_speed_hz = 1000000,
        .duty_cycle_pos = 128,
        .mode = 0,
        .spics_io_num = CS,
        .queue_size = 1,
        .flags = 0
    };

    
    esp_err_t ret = spi_bus_initialize(SPI2_HOST, &spi_config, 0);
    if (ret != ESP_OK) {
        printf("Failed to initialize SPI bus: %d\n", ret);
        return;
    }

    
    ret = spi_bus_add_device(SPI2_HOST, &spi_device_config, &handle);
    if (ret != ESP_OK) {
        printf("Failed to add SPI device: %d\n", ret);
        return;
    }

    char sendbuff[128] = {0};
    char recvbuff[128] = {0};

    spi_transaction_t t;
    memset(&t, 0, sizeof(t));

    while(1) {
        snprintf(sendbuff, sizeof(sendbuff), "HI hello from esp32");

        t.length = strlen(sendbuff) * 8;
        t.tx_buffer = sendbuff;
        t.rx_buffer = recvbuff;

        ret = spi_device_transmit(handle, &t);
        if (ret == ESP_OK) {
            printf("Transmitted: %s\n", sendbuff);
            printf("Received: %s\n", recvbuff);
        } else {
            printf("Failed to transmit: %d\n", ret);
        }

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
