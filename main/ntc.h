/*
 * @Author: ZY
 * @Date: 2024-07-22 00:13:45
 * @LastEditors: thezhangy937 2549867105@qq.com
 * @LastEditTime: 2024-07-22 22:51:28
 * @FilePath: \Total\main\ntc.h
 * @Description: 
 * 
 * Copyright (c) 2024 by ${git_name_email}, All Rights Reserved. 
 */
#ifndef _NTC_H_
#define _NTC_H_


void temp_ntc_init(void);//ADC初始化，用于巡线，此函数会创建一个Task不断读取ADC的结果，一般不用修改

int My_Fix_line(void);//将读取的ADC结果进行判断，是否为黑线，目前使用的阈值为20，大于20认为是黑线

int* get_temp(void);//返回ADC的识别结果，通过修改return返回的数组，查看原始数据和黑线判断数据，调试用

#endif
