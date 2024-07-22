#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "soc/soc_caps.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "ntc.h"
#include "MYpwm.h"
#include <sys/param.h>
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>
#include "MYwifi.h"

#define TAG "main"

#define HOST_IP_ADDR "192.168.1.110"
#define PORT 3333
static const char *payload = "Message from Node1:   Hello!!! ";

static void udp_client_task(void *pvParameters)
{
    char rx_buffer[128];
    char host_ip[] = HOST_IP_ADDR;
    int addr_family = 0;
    int ip_protocol = 0;
    int Send_Once_Flag = 0;

    while (1)
    {

        struct sockaddr_in dest_addr;
        dest_addr.sin_addr.s_addr = inet_addr(HOST_IP_ADDR);
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_port = htons(PORT);
        addr_family = AF_INET;
        ip_protocol = IPPROTO_IP;

        int sock = socket(addr_family, SOCK_DGRAM, ip_protocol);
        if (sock < 0)
        {
            ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
            break;
        }

        // Set timeout
        struct timeval timeout;
        timeout.tv_sec = 10;
        timeout.tv_usec = 0;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof timeout);

        ESP_LOGI(TAG, "Socket created, sending to %s:%d", HOST_IP_ADDR, PORT);

        while (1)
        {

            if (!Send_Once_Flag)
            {
                // 主机不回答就一直给主机发消息
                int err = sendto(sock, payload, strlen(payload), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
                if (err < 0)
                {
                    ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
                    break;
                }
                // ESP_LOGI(TAG, "Message sent");
            }
            struct sockaddr_storage source_addr; // Large enough for both IPv4 or IPv6
            socklen_t socklen = sizeof(source_addr);
            int len = recvfrom(sock, rx_buffer, sizeof(rx_buffer) - 1, 0, (struct sockaddr *)&source_addr, &socklen);

            // Error occurred during receiving
            if (len < 0)
            {
                // ESP_LOGE(TAG, "recvfrom failed: errno %d", errno);
                // break;
            }
            // Data received
            else
            {
                rx_buffer[len] = 0; // Null-terminate whatever we received and treat like a string
                ESP_LOGI(TAG, "Received %d bytes from %s:", len, host_ip);
                ESP_LOGI(TAG, "%s", rx_buffer);

                Send_Once_Flag = 1; // 主机可得知节点ip和位置

                if (strncmp(rx_buffer, "Node1", 5) == 0)
                {
                    ESP_LOGI(TAG, "Received expected message, reconnecting");
                    if (NowState() == 0)
                        SetState(1); // 如果当前在停车状态，打开电机
                    // break;
                }
            }

            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }

        if (sock != -1)
        {
            ESP_LOGE(TAG, "Shutting down socket and restarting...");
            shutdown(sock, 0);
            close(sock);
        }
    }
    vTaskDelete(NULL);
}

void app_main(void)
{
    // 以下用于网络连接，一般不用改
    ESP_ERROR_CHECK(nvs_flash_init());
    initialise_wifi(); // 智能配网

    button_init(0); // 按键长按事件，清除WiFi信息

    ESP_ERROR_CHECK(esp_netif_init()); // 初始化UDP连接

    // 以下用于硬件初始化，一般不用改
    temp_ntc_init(); // 初始化ADC用于循迹，最多支持6路，现使用4路
    MoterInit();     // 电机初始化，State置0关闭电机

    // 创建UDP监听事件
    xTaskCreate(udp_client_task, "udp_client", 4096, NULL, 5, NULL);
    // 延时等待系统稳定
    vTaskDelay(pdMS_TO_TICKS(1000));
    // SetState(1);//打开电机
    int *Vint_pointer;
    while (1)
    {
        Vint_pointer = get_temp();
        for (int i = 0; i < 6; i++)
        {
            // ESP_LOGI(TAG,"V=%d\t%d\t%d\t%d\t%d\t%d\t",Vint_pointer[0],Vint_pointer[1],Vint_pointer[2],Vint_pointer[3],Vint_pointer[4],Vint_pointer[5]);
        }
        // SetSpeed(3000,2000);
        //  vTaskDelay(3000 / portTICK_PERIOD_MS);

        // SetSpeed(-4000,-4000);
        // vTaskDelay(3000 / portTICK_PERIOD_MS);

        vTaskDelay(pdMS_TO_TICKS(200));
    }
}