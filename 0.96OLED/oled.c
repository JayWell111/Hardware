/* ============================================================================
 * 0.96寸 OLED (SSD1306, 128x64) —— 独立可移植驱动（软件模拟 I2C 接口）
 * ----------------------------------------------------------------------------
 * 依赖：仅 STM32 标准外设库（stm32f10x.h）
 * 文件：oled.h / oled.c / oledfont.h（三个文件需一起复制到目标工程）
 * 移植：见 oled.h 顶部说明。
 * 说明：所有显示内容先写入内部显存 OLED_GRAM，再由 OLED_Refresh() 统一刷屏。
 * ==========================================================================*/

#include "oled.h"
#include "oledfont.h"

/* OLED 显存：144 列(>128，预留滚动左移空间) × 8 页(每页 8 行，共 64 行) */
u8 OLED_GRAM[144][8];

/* ============================ 简易软件延时 ================================
 * 不依赖工程自带的 delay 函数，方便移植。
 * 若你的工程已有精确延时（delay_ms），可直接把下方对 OLED_DelayMs 的调用
 * 换成你的 delay_ms。
 * ========================================================================= */
static void OLED_DelayUs(u32 n)
{
    volatile u32 i;
    for(i=0;i<n;i++);
}

static void OLED_DelayMs(u16 ms)
{
    volatile u32 i;
    u16 t;
    for(t=0;t<ms;t++)
        for(i=0;i<24000;i++);   // 72MHz 主频下近似 1ms（粗略值）
}

/* ============================ 软件模拟 I2C ================================
 * 以下为底层函数，均已 static，不会与目标工程其它 I2C 函数冲突。
 * 从机地址 0x78（SSD1306 的 0x3C<<1，写方向）；0x40=数据 0x00=命令。
 * ========================================================================= */
static void IIC_delay(void)
{
    u8 t=3;
    while(t--);
}

// 起始信号
static void I2C_Start(void)
{
    OLED_SDA_Set();
    OLED_SCL_Set();
    IIC_delay();
    OLED_SDA_Clr();
    IIC_delay();
    OLED_SCL_Clr();
    IIC_delay();
}

// 结束信号
static void I2C_Stop(void)
{
    OLED_SDA_Clr();
    OLED_SCL_Set();
    IIC_delay();
    OLED_SDA_Set();
}

// 等待（本驱动不真正检测 ACK，仅按时序等待）
static void I2C_WaitAck(void)
{
    OLED_SDA_Set();
    IIC_delay();
    OLED_SCL_Set();
    IIC_delay();
    OLED_SCL_Clr();
    IIC_delay();
}

// 从最高位开始逐位发送一个字节
static void Send_Byte(u8 dat)
{
    u8 i;
    for(i=0;i<8;i++)
    {
        if(dat&0x80) OLED_SDA_Set();
        else         OLED_SDA_Clr();
        IIC_delay();
        OLED_SCL_Set();
        IIC_delay();
        OLED_SCL_Clr();
        dat<<=1;
    }
}

// 发送一个字节
// mode: 0=命令，1=数据
static void OLED_WR_Byte(u8 dat, u8 mode)
{
    I2C_Start();
    Send_Byte(0x78);          // 器件地址(写)
    I2C_WaitAck();
    if(mode) Send_Byte(0x40); // 数据
    else     Send_Byte(0x00); // 命令
    I2C_WaitAck();
    Send_Byte(dat);
    I2C_WaitAck();
    I2C_Stop();
}

/* ============================ 显示开关 / 反显 / 旋转 ===================== */
// 开启 OLED 显示
void OLED_DisPlay_On(void)
{
    OLED_WR_Byte(0x8D, OLED_CMD); // 电荷泵使能
    OLED_WR_Byte(0x14, OLED_CMD); // 开启电荷泵
    OLED_WR_Byte(0xAF, OLED_CMD); // 点亮屏幕
}

// 关闭 OLED 显示
void OLED_DisPlay_Off(void)
{
    OLED_WR_Byte(0x8D, OLED_CMD); // 电荷泵使能
    OLED_WR_Byte(0x10, OLED_CMD); // 关闭电荷泵
    OLED_WR_Byte(0xAE, OLED_CMD); // 关闭屏幕
}

// 反显：0 正常显示，1 反色显示
void OLED_ColorTurn(u8 i)
{
    if(i==0) OLED_WR_Byte(0xA6, OLED_CMD); // 正常
    if(i==1) OLED_WR_Byte(0xA7, OLED_CMD); // 反色
}

// 旋转：0 正常显示，1 屏幕翻转 180°
void OLED_DisplayTurn(u8 i)
{
    if(i==0)
    {
        OLED_WR_Byte(0xC8, OLED_CMD); // 正常
        OLED_WR_Byte(0xA1, OLED_CMD);
    }
    if(i==1)
    {
        OLED_WR_Byte(0xC0, OLED_CMD); // 反转
        OLED_WR_Byte(0xA0, OLED_CMD);
    }
}

/* ============================ 显存刷新 / 清屏 ============================ */
// 把显存 OLED_GRAM 刷新到 OLED（按 8 页逐页发送）
void OLED_Refresh(void)
{
    u8 i,n;
    for(i=0;i<8;i++)
    {
        OLED_WR_Byte(0xb0+i, OLED_CMD); // 设置行起始地址
        OLED_WR_Byte(0x00, OLED_CMD);   // 设置低列起始地址
        OLED_WR_Byte(0x10, OLED_CMD);   // 设置高列起始地址
        I2C_Start();
        Send_Byte(0x78);
        I2C_WaitAck();
        Send_Byte(0x40);                // 数据模式
        I2C_WaitAck();
        for(n=0;n<128;n++)
        {
            Send_Byte(OLED_GRAM[n][i]);
            I2C_WaitAck();
        }
        I2C_Stop();
    }
}

// 清屏：清显存并刷新
void OLED_Clear(void)
{
    u8 i,n;
    for(i=0;i<8;i++)
        for(n=0;n<128;n++)
            OLED_GRAM[n][i]=0;
    OLED_Refresh();
}

/* ============================ 基础绘图 =================================== */
// 画点：x:0~127  y:0~63  t:1 点亮，0 熄灭
void OLED_DrawPoint(u8 x, u8 y, u8 t)
{
    u8 i,m,n;
    i=y/8;
    m=y%8;
    n=1<<m;
    if(t){ OLED_GRAM[x][i]|=n; }
    else
    {
        OLED_GRAM[x][i]=~OLED_GRAM[x][i];
        OLED_GRAM[x][i]|=n;
        OLED_GRAM[x][i]=~OLED_GRAM[x][i];
    }
}

// 画线（Bresenham 算法）：x1,y1 起点；x2,y2 终点；mode:0 熄灭，1 点亮
void OLED_DrawLine(u8 x1, u8 y1, u8 x2, u8 y2, u8 mode)
{
    u16 t;
    int xerr=0,yerr=0,delta_x,delta_y,distance;
    int incx,incy,uRow,uCol;

    delta_x=x2-x1;  // 计算坐标增量
    delta_y=y2-y1;
    uRow=x1;        // 画线起点坐标
    uCol=y1;
    if(delta_x>0) incx=1;                 // 设置单步方向
    else if(delta_x==0) incx=0;           // 垂直线
    else { incx=-1; delta_x=-delta_x; }
    if(delta_y>0) incy=1;
    else if(delta_y==0) incy=0;           // 水平线
    else { incy=-1; delta_y=-delta_y; }
    if(delta_x>delta_y) distance=delta_x; // 选取基本增量坐标轴
    else distance=delta_y;
    for(t=0;t<distance+1;t++)
    {
        OLED_DrawPoint(uRow,uCol,mode);
        xerr+=delta_x;
        yerr+=delta_y;
        if(xerr>distance){ xerr-=distance; uRow+=incx; }
        if(yerr>distance){ yerr-=distance; uCol+=incy; }
    }
}

// 画圆：x,y 圆心；r 半径
void OLED_DrawCircle(u8 x, u8 y, u8 r)
{
    int a,b,num;
    a=0;
    b=r;
    while(2*b*b >= r*r)
    {
        OLED_DrawPoint(x+a, y-b, 1);
        OLED_DrawPoint(x-a, y-b, 1);
        OLED_DrawPoint(x-a, y+b, 1);
        OLED_DrawPoint(x+a, y+b, 1);

        OLED_DrawPoint(x+b, y+a, 1);
        OLED_DrawPoint(x+b, y-a, 1);
        OLED_DrawPoint(x-b, y-a, 1);
        OLED_DrawPoint(x-b, y+a, 1);

        a++;
        num=(a*a+b*b)-r*r;   // 计算画的点离圆心的距离
        if(num>0){ b--; a--; }
    }
}

/* ============================ 字符 / 数字显示 ============================ */
// 显示一个字符：x,y 起点；size1:8/12/16/24；mode:0 反色，1 正常
void OLED_ShowChar(u8 x, u8 y, u8 chr, u8 size1, u8 mode)
{
    u8 i,m,temp,size2,chr1;
    u8 x0=x,y0=y;
    if(size1==8) size2=6;
    else size2=(size1/8+((size1%8)?1:0))*(size1/2); // 一个字符点阵所占字节数
    chr1=chr-' ';                                    // 计算偏移后的值
    for(i=0;i<size2;i++)
    {
        if(size1==8)      temp=asc2_0806[chr1][i];   // 6x8 字体
        else if(size1==12) temp=asc2_1206[chr1][i];  // 6x12 字体
        else if(size1==16) temp=asc2_1608[chr1][i];  // 8x16 字体
        else if(size1==24) temp=asc2_2412[chr1][i];  // 12x24 字体
        else return;
        for(m=0;m<8;m++)
        {
            if(temp&0x01) OLED_DrawPoint(x,y,mode);
            else          OLED_DrawPoint(x,y,!mode);
            temp>>=1;
            y++;
        }
        x++;
        if((size1!=8)&&((x-x0)==size1/2)){ x=x0; y0=y0+8; }
        y=y0;
    }
}

// 显示字符串：*chr 以空格~波浪号之间的字符组成
void OLED_ShowString(u8 x, u8 y, u8 *chr, u8 size1, u8 mode)
{
    while((*chr>=' ')&&(*chr<='~'))  // 判断是不是非法字符
    {
        OLED_ShowChar(x,y,*chr,size1,mode);
        if(size1==8) x+=6;
        else x+=size1/2;
        chr++;
    }
}

// m^n
static u32 OLED_Pow(u8 m, u8 n)
{
    u32 result=1;
    while(n--) result*=m;
    return result;
}

// 显示数字：num 数值；len 位数；若高位为 0 也显示 '0'
void OLED_ShowNum(u8 x, u8 y, u32 num, u8 len, u8 size1, u8 mode)
{
    u8 t,temp,m=0;
    if(size1==8) m=2;
    for(t=0;t<len;t++)
    {
        temp=(num/OLED_Pow(10,len-t-1))%10;
        if(temp==0) OLED_ShowChar(x+(size1/2+m)*t, y, '0', size1, mode);
        else        OLED_ShowChar(x+(size1/2+m)*t, y, temp+'0', size1, mode);
    }
}

/* ============================ 汉字显示 =================================== */
// 显示汉字：num 为 oledfont.h 中 Hzk 数组的序号；size1:16/24/32/64
void OLED_ShowChinese(u8 x, u8 y, u8 num, u8 size1, u8 mode)
{
    u8 m,temp;
    u8 x0=x,y0=y;
    u16 i,size3=(size1/8+((size1%8)?1:0))*size1;  // 一个汉字点阵所占字节数
    for(i=0;i<size3;i++)
    {
        if(size1==16)      temp=Hzk1[num][i];  // 16x16 字体
        else if(size1==24) temp=Hzk2[num][i];  // 24x24 字体
        else if(size1==32) temp=Hzk3[num][i];  // 32x32 字体
        else if(size1==64) temp=Hzk4[num][i];  // 64x64 字体
        else return;
        for(m=0;m<8;m++)
        {
            if(temp&0x01) OLED_DrawPoint(x,y,mode);
            else          OLED_DrawPoint(x,y,!mode);
            temp>>=1;
            y++;
        }
        x++;
        if((x-x0)==size1){ x=x0; y0=y0+8; }
        y=y0;
    }
}

/* ============================ 滚动显示 =================================== */
// 汉字循环左移滚动（死循环，不会返回）
// num:显示汉字的个数；space:每一遍显示间隔；mode:0 反色，1 正常
void OLED_ScrollDisplay(u8 num, u8 space, u8 mode)
{
    u8 i,n,t=0,m=0,r;
    while(1)
    {
        if(m==0)
        {
            OLED_ShowChinese(128,24,t,16,mode); // 写入一个汉字保存在显存
            t++;
        }
        if(t==num)
        {
            for(r=0;r<16*space;r++)            // 显示间隔
            {
                for(i=1;i<144;i++)
                    for(n=0;n<8;n++)
                        OLED_GRAM[i-1][n]=OLED_GRAM[i][n];
                OLED_Refresh();
            }
            t=0;
        }
        m++;
        if(m==16) m=0;
        for(i=1;i<144;i++)                     // 实现左移
            for(n=0;n<8;n++)
                OLED_GRAM[i-1][n]=OLED_GRAM[i][n];
        OLED_Refresh();
    }
}

/* ============================ 图片显示 =================================== */
// 显示图片：x,y 起点；sizex,sizey 图片宽高；BMP 图片点阵数组；mode:0 反色，1 正常
void OLED_ShowPicture(u8 x, u8 y, u8 sizex, u8 sizey, u8 *BMP, u8 mode)
{
    u16 j=0;
    u8 i,n,temp,m;
    u8 x0=x,y0=y;
    sizey=sizey/8+((sizey%8)?1:0);
    for(n=0;n<sizey;n++)
    {
        for(i=0;i<sizex;i++)
        {
            temp=BMP[j];
            j++;
            for(m=0;m<8;m++)
            {
                if(temp&0x01) OLED_DrawPoint(x,y,mode);
                else          OLED_DrawPoint(x,y,!mode);
                temp>>=1;
                y++;
            }
            x++;
            if((x-x0)==sizex)
            {
                x=x0;
                y0=y0+8;
            }
            y=y0;
        }
    }
}

/* ============================ 初始化 ===================================== */
// OLED 初始化：GPIO 配置 + SSD1306 命令序列
void OLED_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    /* 使能 GPIO 时钟：SCL/SDA 在 GPIOG/GPIOD（若你改了引脚，这里也要对应加上对应端口的时钟） */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOG|RCC_APB2Periph_GPIOD, ENABLE);

    /* SCL：开漏输出 + 上拉（软件模拟 I2C 用） */
    GPIO_InitStructure.GPIO_Pin  = OLED_SCL_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(OLED_SCL_PORT, &GPIO_InitStructure);
    GPIO_SetBits(OLED_SCL_PORT, OLED_SCL_PIN);

    /* SDA：开漏输出 + 上拉 */
    GPIO_InitStructure.GPIO_Pin  = OLED_SDA_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(OLED_SDA_PORT, &GPIO_InitStructure);
    GPIO_SetBits(OLED_SDA_PORT, OLED_SDA_PIN);

    /* RES：推挽输出 */
    GPIO_InitStructure.GPIO_Pin  = OLED_RES_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(OLED_RES_PORT, &GPIO_InitStructure);
    GPIO_SetBits(OLED_RES_PORT, OLED_RES_PIN);

    /* 复位时序 */
    OLED_RES_Clr();
    OLED_DelayMs(200);
    OLED_RES_Set();

    /* SSD1306 初始化命令序列 */
    OLED_WR_Byte(0xAE, OLED_CMD); // 关闭显示
    OLED_WR_Byte(0x00, OLED_CMD); // 设置低列地址
    OLED_WR_Byte(0x10, OLED_CMD); // 设置高列地址
    OLED_WR_Byte(0x40, OLED_CMD); // 设置起始行地址
    OLED_WR_Byte(0x81, OLED_CMD); // 设置对比度
    OLED_WR_Byte(0xCF, OLED_CMD); // 对比度值
    OLED_WR_Byte(0xA1, OLED_CMD); // SEG 左右映射
    OLED_WR_Byte(0xC8, OLED_CMD); // COM 上下扫描方向
    OLED_WR_Byte(0xA6, OLED_CMD); // 正常显示
    OLED_WR_Byte(0xA8, OLED_CMD); // 设置复用比
    OLED_WR_Byte(0x3F, OLED_CMD); // 1/64 duty
    OLED_WR_Byte(0xD3, OLED_CMD); // 设置显示偏移
    OLED_WR_Byte(0x00, OLED_CMD); // 无偏移
    OLED_WR_Byte(0xD5, OLED_CMD); // 设置显示时钟分频/振荡频率
    OLED_WR_Byte(0x80, OLED_CMD); // 约 100 帧/秒
    OLED_WR_Byte(0xD9, OLED_CMD); // 设置预充电周期
    OLED_WR_Byte(0xF1, OLED_CMD); // 预充电 15 时钟
    OLED_WR_Byte(0xDA, OLED_CMD); // 设置 COM 引脚硬件配置
    OLED_WR_Byte(0x12, OLED_CMD);
    OLED_WR_Byte(0xDB, OLED_CMD); // 设置 VCOMH
    OLED_WR_Byte(0x40, OLED_CMD);
    OLED_WR_Byte(0x20, OLED_CMD); // 设置页寻址模式
    OLED_WR_Byte(0x02, OLED_CMD);
    OLED_WR_Byte(0x8D, OLED_CMD); // 电荷泵使能
    OLED_WR_Byte(0x14, OLED_CMD); // 开启电荷泵
    OLED_WR_Byte(0xA4, OLED_CMD); // 关闭整体显示
    OLED_WR_Byte(0xA6, OLED_CMD); // 关闭反显

    OLED_Clear();                 // 清屏
    OLED_WR_Byte(0xAF, OLED_CMD); // 点亮屏幕
}
