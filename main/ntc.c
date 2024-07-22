#include <string.h>
#include <stdio.h>
#include "ntc.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_adc/adc_continuous.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "FreeRTOSConfig.h"

#include "MYpwm.h"

#define TAG "adc"

/*
GPIO32  ADC1_CH4
GPIO33  ADC1_CH5
GPIO34  ADC1_CH6
GPIO35  ADC1_CH7
GPIO36  ADC1_CH0
GPIO37  ADC1_CH1
GPIO38  ADC1_CH2
GPIO39  ADC1_CH3
*/
#define TEMP_ADC_CHANNEL7 ADC_CHANNEL_0 // ADC转换通道(ADC1有8个通道，对应gpio32 - gpio39)
#define TEMP_ADC_CHANNEL6 ADC_CHANNEL_3 // ADC转换通道(ADC1有8个通道，对应gpio32 - gpio39)
#define TEMP_ADC_CHANNEL5 ADC_CHANNEL_6 // ADC转换通道(ADC1有8个通道，对应gpio32 - gpio39)
#define TEMP_ADC_CHANNEL4 ADC_CHANNEL_7 // ADC转换通道(ADC1有8个通道，对应gpio32 - gpio39)
#define TEMP_ADC_CHANNEL3 ADC_CHANNEL_4 // ADC转换通道(ADC1有8个通道，对应gpio32 - gpio39)
#define TEMP_ADC_CHANNEL2 ADC_CHANNEL_5 // ADC转换通道(ADC1有8个通道，对应gpio32 - gpio39)
#define Channel_Num 6

#define NTC_RES 10000  // NTC电阻标称值(在电路中和NTC一起串进电路的那个电阻,一般是10K，100K)
#define ADC_V_MAX 3300 // 最大接入电压值

#define ADC_VALUE_NUM 5 // 每次采样的电压

static bool do_calibration1 = false; // 是否需要校准

static volatile int Final_value[Channel_Num] = {0}; // 电压
static volatile int Final_Char[Channel_Num] = {0};  // 电压运算完

static int s_adc_raw[Channel_Num][ADC_VALUE_NUM];     // ADC采样值
static int s_voltage_raw[Channel_Num][ADC_VALUE_NUM]; // 转换后的电压值

// ADC操作句柄
static adc_oneshot_unit_handle_t s_adc_handle = NULL;

// 转换句柄
static adc_cali_handle_t adc1_cali_handle = NULL;

// NTC温度采集任务
static void temp_adc_task(void *);

static float get_ntc_temp(uint32_t res);

/**
 * 温度检测初始化
 * @param 无
 * @return 无
 */
void temp_ntc_init(void)
{
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1, // WIFI和ADC2无法同时启用，这里选择ADC1
    };

    // 启用单次转换模式
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &s_adc_handle));

    //-------------ADC1 Config---------------//
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_12, // 分辨率
        .atten = ADC_ATTEN_DB_12,
        // 衰减倍数，ESP32设计的ADC参考电压为1100mV,只能测量0-1100mV之间的电压，如果要测量更大范围的电压
        // 需要设置衰减倍数
        /*以下是对应可测量范围
        ADC_ATTEN_DB_0	    100 mV ~ 950 mV
        ADC_ATTEN_DB_2_5	100 mV ~ 1250 mV
        ADC_ATTEN_DB_6	    150 mV ~ 1750 mV
        ADC_ATTEN_DB_12	    150 mV ~ 2450 mV
        */
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(s_adc_handle, TEMP_ADC_CHANNEL7, &config));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(s_adc_handle, TEMP_ADC_CHANNEL6, &config));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(s_adc_handle, TEMP_ADC_CHANNEL5, &config));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(s_adc_handle, TEMP_ADC_CHANNEL4, &config));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(s_adc_handle, TEMP_ADC_CHANNEL3, &config));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(s_adc_handle, TEMP_ADC_CHANNEL2, &config));
    //-------------ADC1 Calibration Init---------------//
    // do_calibration1 = example_adc_calibration_init(ADC_UNIT_1, ADC_ATTEN_DB_12, &adc1_cali_handle);

    // 新建一个任务，不断地进行ADC和温度计算
    xTaskCreatePinnedToCore(temp_adc_task, "adc_task", 2048, NULL, 2, NULL, 1);
}

static void temp_adc_task(void *param)
{
    uint16_t adc_cnt = 0;
    while (1)
    {
        adc_oneshot_read(s_adc_handle, TEMP_ADC_CHANNEL7, &s_adc_raw[0][adc_cnt]);
        adc_oneshot_read(s_adc_handle, TEMP_ADC_CHANNEL6, &s_adc_raw[1][adc_cnt]);
        adc_oneshot_read(s_adc_handle, TEMP_ADC_CHANNEL5, &s_adc_raw[2][adc_cnt]);
        adc_oneshot_read(s_adc_handle, TEMP_ADC_CHANNEL4, &s_adc_raw[3][adc_cnt]);
        adc_oneshot_read(s_adc_handle, TEMP_ADC_CHANNEL3, &s_adc_raw[4][adc_cnt]);
        adc_oneshot_read(s_adc_handle, TEMP_ADC_CHANNEL2, &s_adc_raw[5][adc_cnt]);

        adc_cnt++;
        if (adc_cnt >= ADC_VALUE_NUM)
        {
            int i = 0, j = 0;
            // 用平均值进行滤波
            uint32_t voltage[Channel_Num] = {0};

            for (i = 0; i < ADC_VALUE_NUM; i++)
            {
                for (j = 0; j < Channel_Num; j++)
                {
                    voltage[j] += s_adc_raw[j][i];
                }
            }
            for (j = 0; j < Channel_Num; j++)
            {
                Final_value[j] = voltage[j] / ADC_VALUE_NUM; // 更新检测电压值
            }

            adc_cnt = 0;
            My_Fix_line(); // 动态确定判断阈值

            int CNT = 0;
            for (int i = 0; i < Channel_Num; i++)
            {
                CNT += Final_Char[i]; // 检测到几根黑线
            }

            

            if (NowState() == 1)//是否使能电机
            {              
                if(CNT>=3)
                {
                    SetSpeed(0, 0);
                    SetState(2);//准备进入180度转弯
                }
                    
                else if (Final_Char[0]) // 偏右，左轮加速
                {
                    SetSpeed(3000, 3200);
                }
                else if (Final_Char[1])
                {
                    SetSpeed(3000, 2500);
                }
                else if (Final_Char[4]) // 偏左，右轮加速
                {
                    SetSpeed(4000, 2000);
                }
                else if (Final_Char[5]) // 偏左，右轮加速
                {
                    SetSpeed(6000, 2000);
                }
                else
                {
                    SetSpeed(3000, 2000);
                }
            }

            else if(NowState()==2)
            {
                SetSpeed(3000, -2000);//原地旋转
                vTaskDelay(pdMS_TO_TICKS(1000));
                ESP_LOGI(TAG, "Start Roll");
                SetState(3);//等待再次到达黑线
            }
            else if(NowState()==3)
            {
                if(Final_Char[4])
                {
                    SetState(0);//等待下一次udp指令
                    SetSpeed(0, 0);
                    ESP_LOGI(TAG, "Finish Roll");
                }
                    
                else
                    SetSpeed(3000, -2000);//原地旋转
            }




        }

        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

/**
 * 获取温度值
 * @param 无
 * @return 温度
 */
int *get_temp(void)
{
    //return Final_value;
    return Final_Char;
}

/*
自适应巡线阈值
*/
int My_Fix_line(void)
{
    int MAX = 0;
    int MIN = 3300;
    int Middle = 0;
    // for(int i = 0;i<Channel_Num;i++)
    // {
    //     //寻找最大最小值
    //     if(Final_value[i] > MAX)
    //     {
    //         MAX = Final_value[i];
    //     }
    //     if(Final_value[i] < MIN)
    //     {
    //         MIN = Final_value[i];
    //     }
    // }
    ////阈值限定，防止所有数据太高
    // if(MIN>MAX-100)
    //     MIN=MAX-100;

    // Middle=(MAX+MIN)/2;

    for (int i = 0; i < Channel_Num; i++)
    {
        Final_Char[i] = Final_value[i] > 20 ? 1 : 0; // 动态寻找中值，然后判断是否找到黑线
    }
    //这一版中间没有焊接，数据无效
    Final_Char[2]=0;
    Final_Char[3]=0;

    return 1;
}
