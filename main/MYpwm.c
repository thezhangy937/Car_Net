#include "MYpwm.h"

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/ledc.h"
#include "esp_err.h"
#include "driver/gpio.h"

#define LEDC_TEST_CH_NUM (4)

#define LEDC_HS_CH0_GPIO (18)
#define LEDC_HS_CH1_GPIO (19)
#define LEDC_HS_CH2_GPIO (4)
#define LEDC_HS_CH3_GPIO (5)

static const char *TAG = "Task";
static int LeftSpeed = 0;
static int RightSpeed = 0;

static int Car_State = 1; // 小车状态

static void PWM_TASK(void *param) // 传入空指针方便后期传入参数:
{
    while (1)
    {

        if (RightSpeed > 0)
        {
            ledc_set_duty(LEDC_HIGH_SPEED_MODE, 0, RightSpeed);
            ledc_set_duty(LEDC_HIGH_SPEED_MODE, 1, 0);
        }
        else
        {
            ledc_set_duty(LEDC_HIGH_SPEED_MODE, 0, 0);
            ledc_set_duty(LEDC_HIGH_SPEED_MODE, 1, -RightSpeed);
        }

        if (LeftSpeed > 0)
        {
            ledc_set_duty(LEDC_HIGH_SPEED_MODE, 2, LeftSpeed);
            ledc_set_duty(LEDC_HIGH_SPEED_MODE, 3, 0);
        }
        else
        {
            ledc_set_duty(LEDC_HIGH_SPEED_MODE, 2, 0);
            ledc_set_duty(LEDC_HIGH_SPEED_MODE, 3, -LeftSpeed);
        }
        for (int i = 0; i < 4; i++)
        {
            ledc_update_duty(LEDC_HIGH_SPEED_MODE, i);
        }

        // printf("Hello Task!\n");//打印Hello Task!
        vTaskDelay(20 / portTICK_PERIOD_MS); // 延时1000ms=1s,使系统执行其他任务
    }
}

int MoterInit(void)
{
    /*
     * Prepare and set configuration of timers
     * that will be used by LED Controller
     */
    ledc_timer_config_t ledc_timer = {
        .duty_resolution = LEDC_TIMER_13_BIT, // resolution of PWM duty
        .freq_hz = 4000,                      // frequency of PWM signal
        .speed_mode = LEDC_HIGH_SPEED_MODE,   // timer mode
        .timer_num = LEDC_TIMER_0,            // timer index
        .clk_cfg = LEDC_AUTO_CLK,             // Auto select the source clock
    };
    // Set configuration of timer0 for high speed channels
    ledc_timer_config(&ledc_timer);

    ledc_channel_config_t ledc_channel[LEDC_TEST_CH_NUM] = {

        {.channel = LEDC_CHANNEL_0,
         .duty = 0,
         .gpio_num = LEDC_HS_CH0_GPIO,
         .speed_mode = LEDC_HIGH_SPEED_MODE,
         .hpoint = 0,
         .timer_sel = LEDC_TIMER_0,
         .flags.output_invert = 0},
        {.channel = LEDC_CHANNEL_1,
         .duty = 0,
         .gpio_num = LEDC_HS_CH1_GPIO,
         .speed_mode = LEDC_HIGH_SPEED_MODE,
         .hpoint = 0,
         .timer_sel = LEDC_TIMER_0,
         .flags.output_invert = 0},
        {.channel = LEDC_CHANNEL_2,
         .duty = 0,
         .gpio_num = LEDC_HS_CH2_GPIO,
         .speed_mode = LEDC_HIGH_SPEED_MODE,
         .hpoint = 0,
         .timer_sel = LEDC_TIMER_0,
         .flags.output_invert = 1},
        {.channel = LEDC_CHANNEL_3,
         .duty = 0,
         .gpio_num = LEDC_HS_CH3_GPIO,
         .speed_mode = LEDC_HIGH_SPEED_MODE,
         .hpoint = 0,
         .timer_sel = LEDC_TIMER_0,
         .flags.output_invert = 1},
    };

    // Set LED Controller with previously prepared configuration
    for (int ch = 0; ch < LEDC_TEST_CH_NUM; ch++)
    {
        ledc_channel_config(&ledc_channel[ch]);
    }

    // 注意我这里将堆栈大小修改为了2048，因为使用了ESP_LOGI()函数将花费更多CPU资源
    // xTaskCreate(PWM_TASK,"PWM_TASK",2048,NULL,1,NULL);//创建任务函数
    
    SetState(0);//关闭电机
    return 1;
}

// 设置并更新速度
int SetSpeed(int Left, int Right)
{
    if (Right > 0)
    {
        ledc_set_duty(LEDC_HIGH_SPEED_MODE, 0, Right);
        ledc_set_duty(LEDC_HIGH_SPEED_MODE, 1, 0);
    }
    else
    {
        ledc_set_duty(LEDC_HIGH_SPEED_MODE, 0, 0);
        ledc_set_duty(LEDC_HIGH_SPEED_MODE, 1, -Right);
    }

    if (Left > 0)
    {
        ledc_set_duty(LEDC_HIGH_SPEED_MODE, 2, Left);
        ledc_set_duty(LEDC_HIGH_SPEED_MODE, 3, 0);
    }
    else
    {
        ledc_set_duty(LEDC_HIGH_SPEED_MODE, 2, 0);
        ledc_set_duty(LEDC_HIGH_SPEED_MODE, 3, -Left);
    }
    for (int i = 0; i < 4; i++)
    {
        ledc_update_duty(LEDC_HIGH_SPEED_MODE, i);
    }

    return 1;
}

int NowState(void)
{
    return Car_State; // 返回当前状态
}

int SetState(int state)
{
    return Car_State=state;//设置状态，打开电机
    
}