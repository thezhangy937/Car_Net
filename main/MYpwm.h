/*
 * @Author: ZY
 * @Date: 2024-07-22 00:13:45
 * @LastEditors: thezhangy937 2549867105@qq.com
 * @LastEditTime: 2024-07-22 22:44:43
 * @FilePath: \Total\main\MYpwm.h
 * @Description: 
 * 
 * Copyright (c) 2024 by ${git_name_email}, All Rights Reserved. 
 */
#pragma once
#ifdef __cplusplus
extern "C" {
#endif

int MoterInit(void);//电机初始化

int SetSpeed(int Left,int Right);//设置左右轮速度
//!!!!!!!由于没有加入速度控制，目前大概测得左3000右2000能直走，但后续可能修改 

//状态说明
//  0:电机停止  收到正确的udp包 ->1
//  1:正常巡线  遇到横着的黑线  ->2
//  2：原地旋转半秒            ->3
//  3:继续旋转直到检测到黑线    ->1
int NowState(void);//返回当前状态
int SetState(int state);//设置当前状态


#ifdef __cplusplus
}
#endif