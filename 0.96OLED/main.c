/* ============================================================================
 * 0.96寸 OLED —— 复杂功能测试程序（自动轮播多页演示）
 * ----------------------------------------------------------------------------
 * 依赖：仅 stm32f10x.h + oled.h / oled.c / oledfont.h
 * 演示内容：
 *   Page1 全 ASCII 字符表 (6x8)
 *   Page2 四种 ASCII 字号对比 (6x8 / 6x12 / 8x16 / 12x24)
 *   Page3 汉字 16x16 / 24x24 / 32x32 / 64x64 四种大小
 *   Page4 画线（星形放射线 + 网格线）
 *   Page5 画圆（同心圆 + 六宫格）
 *   Page6 图片显示（程序生成的 8x8 棋盘格位图）
 *   Page7 数字倒计时动画（12x24 字体）
 *   Page8 特效（反色 / 翻转 / 关屏 / 开屏）
 *   Scroll 汉字滚动（可选，进入后为死循环）
 * ----------------------------------------------------------------------------
 * 注意：本文件含 main 函数，与 main_example.c 二选一，勿同时加入工程。
 * ==========================================================================*/

#include "stm32f10x.h"   // STM32 标准外设库
#include "oled.h"        // OLED 驱动

/* 1=演示完所有页面后进入汉字滚动（死循环）；0=循环重播所有页面 */
#define ENABLE_SCROLL_DEMO   1

/* ----------------------------- 简易软件延时 ----------------------------- */
static void Delay_ms(u16 ms)
{
    volatile u32 i;
    u16 t;
    for(t=0;t<ms;t++)
        for(i=0;i<24000;i++);   // 72MHz 下近似 1ms
}

/* ============================ 各演示页面 ================================ */

/* Page1：6x8 字体显示全部可见 ASCII 字符 */
static void Demo_Page1_Ascii(void)
{
    u8 x=0, y=0, c;
    OLED_Clear();
    for(c=' ';c<='~';c++)
    {
        if(c==' ') OLED_ShowChar(x,y,'.',8,1);  // 空格显示为小点便于观察
        else       OLED_ShowChar(x,y,c,8,1);
        x+=6;
        if(x>126){ x=0; y+=8; }
    }
    OLED_Refresh();
    Delay_ms(2000);
}

/* Page2：四种 ASCII 字号对比 */
static void Demo_Page2_Fonts(void)
{
    OLED_Clear();
    OLED_ShowChar(0,  0, 'A', 8,  1);   // 6x8
    OLED_ShowChar(16, 0, 'A', 12, 1);   // 6x12
    OLED_ShowChar(40, 0, 'A', 16, 1);   // 8x16
    OLED_ShowChar(72, 0, 'A', 24, 1);   // 12x24
    OLED_ShowString(0, 32, "6x8 6x12 8x16 12x24", 8, 1);
    OLED_Refresh();
    Delay_ms(2000);

    /* 第二屏：三种字号整行显示 */
    OLED_Clear();
    OLED_ShowString(0, 0,  "FONT 6x8", 8, 1);
    OLED_ShowString(0, 8,  "FONT 6x12", 12, 1);
    OLED_ShowString(0, 20, "FONT 8x16", 16, 1);
    OLED_ShowString(0, 36, "FONT 12x24", 24, 1);
    OLED_Refresh();
    Delay_ms(2000);
}

/* Page3：汉字 16/24/32/64 四种大小（Hzk1[0]=中，Hzk1[1]=景，Hzk1[2]=园……） */
static void Demo_Page3_Chinese(void)
{
    /* A: 16x16 —— 中景园电子技术 */
    OLED_Clear();
    OLED_ShowChinese(0,   0, 0, 16, 1);  // 中
    OLED_ShowChinese(18,  0, 1, 16, 1);  // 景
    OLED_ShowChinese(36,  0, 2, 16, 1);  // 园
    OLED_ShowChinese(54,  0, 3, 16, 1);  // 电
    OLED_ShowChinese(72,  0, 4, 16, 1);  // 子
    OLED_ShowChinese(90,  0, 5, 16, 1);  // 技
    OLED_ShowChinese(108, 0, 6, 16, 1);  // 术
    OLED_ShowString(0, 20, "16x16 FONT", 8, 1);
    OLED_Refresh();
    Delay_ms(1200);

    /* B: 24x24 */
    OLED_Clear();
    OLED_ShowChinese(16, 8, 0, 24, 1);
    OLED_ShowString(56, 28, "24x24", 16, 1);
    OLED_Refresh();
    Delay_ms(1200);

    /* C: 32x32（居中） */
    OLED_Clear();
    OLED_ShowChinese(48, 16, 0, 32, 1);
    OLED_Refresh();
    Delay_ms(1200);

    /* D: 64x64（整屏大字） */
    OLED_Clear();
    OLED_ShowChinese(32, 0, 0, 64, 1);
    OLED_Refresh();
    Delay_ms(1500);
}

/* Page4：画线（星形放射线 + 网格） */
static void Demo_Page4_Lines(void)
{
    static const s8 dirs[8][2] = {
        {1,0},{1,1},{0,1},{-1,1},{-1,0},{-1,-1},{0,-1},{1,-1}
    };
    u8 i, r;
    OLED_Clear();
    for(i=0;i<8;i++)
        OLED_DrawLine(64, 32, 64+dirs[i][0]*28, 32+dirs[i][1]*28, 1);
    OLED_Refresh();
    Delay_ms(1500);

    /* 第二屏：网格线 */
    OLED_Clear();
    for(r=0;r<64;r+=8)  OLED_DrawLine(0, r, 127, r, 1);   // 水平
    for(r=0;r<128;r+=8) OLED_DrawLine(r, 0, r, 63, 1);    // 垂直
    OLED_Refresh();
    Delay_ms(1500);
}

/* Page5：画圆（同心圆 + 六宫格） */
static void Demo_Page5_Circles(void)
{
    u8 r;
    OLED_Clear();
    for(r=5;r<=30;r+=5) OLED_DrawCircle(64, 32, r);
    /* 外框 */
    OLED_DrawLine(0, 0, 127, 0, 1);
    OLED_DrawLine(127, 0, 127, 63, 1);
    OLED_DrawLine(127, 63, 0, 63, 1);
    OLED_DrawLine(0, 63, 0, 0, 1);
    OLED_Refresh();
    Delay_ms(1500);

    /* 第二屏：六宫格圆 */
    OLED_Clear();
    OLED_DrawCircle(20, 16, 12);
    OLED_DrawCircle(64, 16, 12);
    OLED_DrawCircle(108, 16, 12);
    OLED_DrawCircle(20, 48, 12);
    OLED_DrawCircle(64, 48, 12);
    OLED_DrawCircle(108, 48, 12);
    OLED_Refresh();
    Delay_ms(1500);
}

/* Page6：图片显示 —— 程序生成 128x64 棋盘格位图再调用 OLED_ShowPicture */
static void Demo_Page6_Image(void)
{
    static u8 img[128*8];          // 128x64 位图缓冲（放 BSS）
    u16 k=0;
    u8 x, y, yy;

    /* 生成 8x8 棋盘格。取模格式与 OLED_ShowPicture 一致：低位=上方行 */
    for(y=0;y<64;y+=8)
    {
        for(x=0;x<128;x++)
        {
            u8 v=0;
            for(yy=0;yy<8;yy++)
                if(((x/8)+((y+yy)/8))%2==0) v |= (u8)(1<<yy);
            img[k++]=v;
        }
    }

    OLED_Clear();
    OLED_ShowPicture(0, 0, 128, 64, img, 1);
    OLED_ShowString(0, 0, "IMAGE CHECKER", 8, 1);   // 叠加一行说明
    OLED_Refresh();
    Delay_ms(2000);

    /* 第二屏：同一图片反色显示（mode=0） */
    OLED_Clear();
    OLED_ShowPicture(0, 0, 128, 64, img, 0);
    OLED_ShowString(0, 0, "INVERTED", 8, 1);
    OLED_Refresh();
    Delay_ms(2000);
}

/* Page7：数字倒计时动画（12x24 字体） */
static void Demo_Page7_Countdown(void)
{
    u32 c;
    for(c=10;c>0;c--)
    {
        OLED_Clear();
        OLED_ShowString(8, 0, "COUNTDOWN:", 16, 1);
        OLED_ShowNum(48, 24, c, 2, 24, 1);   // 2 位，高位补 0
        OLED_Refresh();
        Delay_ms(200);
    }
}

/* Page8：特效 —— 反色 / 翻转 / 关屏 / 开屏 */
static void Demo_Page8_Fx(void)
{
    OLED_Clear();
    OLED_ShowString(8, 24, "INVERT TEST", 16, 1);
    OLED_Refresh();
    Delay_ms(800);

    OLED_ColorTurn(1);   // 反色
    Delay_ms(800);
    OLED_ColorTurn(0);   // 恢复
    Delay_ms(800);

    OLED_DisplayTurn(1); // 翻转 180°
    Delay_ms(800);
    OLED_DisplayTurn(0); // 恢复
    Delay_ms(800);

    OLED_DisPlay_Off();  // 关屏
    Delay_ms(800);
    OLED_DisPlay_On();   // 开屏
    Delay_ms(800);
}

/* ============================== 主函数 ================================== */
int main(void)
{
    u8 round;

    OLED_Init();          // 初始化
    OLED_ColorTurn(0);    // 正常显示
    OLED_DisplayTurn(0);  // 正常方向

    /* 整轮播放 3 遍 */
    for(round=0;round<3;round++)
    {
        Demo_Page1_Ascii();
        Demo_Page2_Fonts();
        Demo_Page3_Chinese();
        Demo_Page4_Lines();
        Demo_Page5_Circles();
        Demo_Page6_Image();
        Demo_Page7_Countdown();
        Demo_Page8_Fx();
    }

#if ENABLE_SCROLL_DEMO
    /* 汉字滚动演示（死循环，不会返回）：滚动"中景园电子" */
    OLED_ScrollDisplay(5, 4, 1);
#else
    while(1);
#endif
}
