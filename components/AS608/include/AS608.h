#ifndef __AS608_H
#define __AS608_H
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>
#define AS608_USER     1
#define PS_Sta   PAin(6)//读指纹模块状态引脚
#define CharBuffer1 0x01
#define CharBuffer2 0x02
#define BUF_SIZE (1024)
#define RD_BUF_SIZE (BUF_SIZE)

static QueueHandle_t uart2_queue;
#define EX_UART_NUM UART_NUM_2

typedef struct  
{
    uint16_t pageID;//指纹ID
    uint16_t mathscore;//匹配得分
}SearchResult;
typedef struct
{
    uint16_t PS_max;//指纹最大容量
    uint8_t  PS_level;//安全等级
    unsigned int PS_addr;
    uint8_t  PS_size;//通讯数据包大小
    uint8_t  PS_N;//波特率基数N
}SysPara;
void USART_init(void);
static void MYUSART_SendData(uint8_t   data);
//void PS_StaGPIO_Init(void);//初始化PA6读状态引脚
uint8_t PS_GetImage(void); //录入图像 
uint8_t PS_GenChar(uint8_t BufferID);//生成特征 
uint8_t PS_Match(void);//精确比对两枚指纹特征 
uint8_t PS_Search(uint8_t BufferID,uint16_t StartPage,uint16_t PageNum,SearchResult *p);//搜索指纹 
uint8_t PS_RegModel(void);//合并特征（生成模板） 
 
uint8_t PS_StoreChar(uint8_t BufferID,uint16_t PageID);//储存模板 
uint8_t PS_DeletChar(uint16_t PageID,uint16_t N);//删除模板 
uint8_t PS_Empty(void);//清空指纹库 

uint8_t PS_WriteReg(uint8_t RegNum,uint8_t DATA);//写系统寄存器 

uint8_t PS_ReadSysPara(SysPara *p); //读系统基本参数 

uint8_t PS_SetAddr(uint32_t addr);  //设置模块地址 

uint8_t PS_WriteNotepad(uint8_t NotePageNum,uint8_t *content);//写记事本 

uint8_t PS_ReadNotepad(uint8_t NotePageNum,uint8_t *note);//读记事 

uint8_t PS_HighSpeedSearch(uint8_t BufferID,uint16_t StartPage,uint16_t PageNum,SearchResult *p);//高速搜索 
  
uint8_t PS_ValidTempleteNum(uint16_t *ValidN);//读有效模板个数 

uint8_t PS_HandShake(uint32_t *PS_Addr); //与AS608模块握手

const char *EnsureMessage(uint8_t ensure);//确认码错误信息解析


int add(void);
int press_FR(int *k);
#endif
