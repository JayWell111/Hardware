#ifndef __OLED_H
#define __OLED_H

#include "stm32f10x.h"   // STM32 标准外设库（提供 GPIO 操作与 u8/u16/u32 类型）

/* ============================================================================
 * 0.96寸 OLED (SSD1306, 128x64) —— 独立可移植驱动（软件模拟 I2C 接口）
 * ----------------------------------------------------------------------------
 * 移植步骤（复制到新工程）：
 *   1. 把 oled.h / oled.c / oledfont.h 三个文件加入工程并添加头文件路径；
 *   2. 修改下方「引脚配置」为你目标板的实际引脚（注意 SCL/SDA 用开漏输出，
 *      板上有上拉电阻；若没有需在外部加上拉）；
 *   3. 使用流程：OLED_Init() 初始化 → 调用 OLED_Show* 系列写显存
 *                → OLED_Refresh() 一次性刷到屏幕。
 *   4. 显示中文用 OLED_ShowChinese()，num 为 oledfont.h 中汉字的序号
 *      （Hzk1[0] 是"中"，Hzk1[1] 是"景"……）；如需更多汉字需自行取模添加。
 * ==========================================================================*/

/* ---------------------------- 引脚配置（移植时改这里）---------------------- */
#define OLED_SCL_PORT   GPIOG          // D0 / SCL  时钟线
#define OLED_SCL_PIN    GPIO_Pin_12
#define OLED_SDA_PORT   GPIOD          // D1 / SDA  数据线
#define OLED_SDA_PIN    GPIO_Pin_5
#define OLED_RES_PORT   GPIOD          // RES 复位线
#define OLED_RES_PIN    GPIO_Pin_4

/* 引脚操作宏（一般不用改） */
#define OLED_SCL_Clr()  GPIO_ResetBits(OLED_SCL_PORT, OLED_SCL_PIN)
#define OLED_SCL_Set()  GPIO_SetBits(OLED_SCL_PORT, OLED_SCL_PIN)
#define OLED_SDA_Clr()  GPIO_ResetBits(OLED_SDA_PORT, OLED_SDA_PIN)
#define OLED_SDA_Set()  GPIO_SetBits(OLED_SDA_PORT, OLED_SDA_PIN)
#define OLED_RES_Clr()  GPIO_ResetBits(OLED_RES_PORT, OLED_RES_PIN)
#define OLED_RES_Set()  GPIO_SetBits(OLED_RES_PORT, OLED_RES_PIN)

/* 命令/数据标志 */
#define OLED_CMD   0   // 写命令
#define OLED_DATA  1   // 写数据

/* ------------------------------ 对外函数声明 ------------------------------ */
void OLED_Init(void);                 // 初始化（含 GPIO 配置与 SSD1306 命令序列）
void OLED_Refresh(void);              // 把显存 OLED_GRAM 刷新到屏幕
void OLED_Clear(void);                // 清屏（清显存并刷新）

void OLED_ColorTurn(u8 i);            // 反显：0 正常，1 反色
void OLED_DisplayTurn(u8 i);          // 旋转：0 正常，1 翻转 180°
void OLED_DisPlay_On(void);           // 开显示
void OLED_DisPlay_Off(void);          // 关显示

/* 绘图（写入显存，需调用 OLED_Refresh 才显示） */
void OLED_DrawPoint(u8 x, u8 y, u8 t);                       // 画点 t:1点亮 0熄灭
void OLED_DrawLine(u8 x1, u8 y1, u8 x2, u8 y2, u8 mode);     // 画线
void OLED_DrawCircle(u8 x, u8 y, u8 r);                      // 画圆

/* 字符/数字/汉字/图片（写入显存，需调用 OLED_Refresh 才显示）
 * size1: 字体大小 8(6x8) / 12(6x12) / 16(8x16) / 24(12x24)
 * mode : 0 反色，1 正常
 */
void OLED_ShowChar(u8 x, u8 y, u8 chr, u8 size1, u8 mode);
void OLED_ShowString(u8 x, u8 y, u8 *chr, u8 size1, u8 mode);
void OLED_ShowNum(u8 x, u8 y, u32 num, u8 len, u8 size1, u8 mode);
void OLED_ShowChinese(u8 x, u8 y, u8 num, u8 size1, u8 mode); // size1:16/24/32/64
void OLED_ScrollDisplay(u8 num, u8 space, u8 mode);           // 汉字滚动（死循环）
void OLED_ShowPicture(u8 x, u8 y, u8 sizex, u8 sizey, u8 *BMP, u8 mode);

#endif
