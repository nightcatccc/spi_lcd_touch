#include <stdio.h>
#include <string.h>
#include "AS608.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "driver/gpio.h"
extern int count_people;
uint32_t AS608Addr = 0XFFFFFFFF;//模块地址
static const char *TAG = "UART TEST";
int flag_rx_uart2;//串口接收标志位

uint8_t uart2_recive_data[RD_BUF_SIZE];//串口接收缓冲区

uart_event_t event;//事件变量

uint16_t ID;

static void uart_event_task(void *arg)
{
	
    size_t buffered_size;//缓存区大小
    for (;;) {
        //Waiting for UART event.
        if (xQueueReceive(uart2_queue, (void *)&event, (TickType_t)portMAX_DELAY)) {//从队列中接收数据，储存在event缓存区
            bzero(uart2_recive_data, RD_BUF_SIZE);//将区域dtmp开始的RD_BUF_SIZE个字节全部清零
            //ESP_LOGI(TAG, "uart[%d] event:", EX_UART_NUM);
            switch (event.type) {
            //Event of UART receving data
            /*We'd better handler data event fast, there would be much more data events than
            other types of events. If we take too much time on data event, the queue might
            be full.（尽量快速处理时间）*/
            case UART_DATA://收到数据
                //ESP_LOGI(TAG, "[UART DATA]: %d", event.size);
                //uart_read_bytes(EX_UART_NUM, dtmp, event.size, portMAX_DELAY);//接收数据
                //ESP_LOGI(TAG, "[DATA EVT]:");
                //uart_write_bytes(EX_UART_NUM, (const char*) dtmp, event.size);//发回数据
				//ESP_LOGI(TAG, "recive data");
				uart_read_bytes(EX_UART_NUM, uart2_recive_data, event.size, portMAX_DELAY);//接收数据
				
				flag_rx_uart2=1;
                break;
            //Event of HW FIFO overflow detected
            case UART_FIFO_OVF://缓冲区溢出
                ESP_LOGI(TAG, "hw fifo overflow");
                // If fifo overflow happened, you should consider adding flow control for your application.
                // The ISR has already reset the rx FIFO,
                // As an example, we directly flush the rx buffer here in order to read more data.（此处直接刷新缓存区读取新数据，放弃旧数据）
                uart_flush_input(EX_UART_NUM);
                xQueueReset(uart2_queue);
                break;
            //Event of UART ring buffer full
            case UART_BUFFER_FULL://接收数据溢出，与UART_FIFO_OVF相似，不过有软硬件层的区别
                ESP_LOGI(TAG, "ring buffer full");
                // If buffer full happened, you should consider increasing your buffer size
                // As an example, we directly flush the rx buffer here in order to read more data.（刷新缓存区）
                uart_flush_input(EX_UART_NUM);
                xQueueReset(uart2_queue);
                break;
            //Event of UART RX break detected
            case UART_BREAK:
                ESP_LOGI(TAG, "uart rx break");
                break;
            //Event of UART parity check error
            case UART_PARITY_ERR:
                ESP_LOGI(TAG, "uart parity error");
                break;
            //Event of UART frame error
            case UART_FRAME_ERR:
                ESP_LOGI(TAG, "uart frame error");
                break;
            //UART_PATTERN_DET
            case UART_PATTERN_DET:
                uart_get_buffered_data_len(EX_UART_NUM, &buffered_size);
                int pos = uart_pattern_pop_pos(EX_UART_NUM);
                ESP_LOGI(TAG, "[UART PATTERN DETECTED] pos: %d, buffered size: %d", pos, buffered_size);
                if (pos == -1) {
                    // There used to be a UART_PATTERN_DET event, but the pattern position queue is full so that it can not
                    // record the position. We should set a larger queue size.
                    // As an example, we directly flush the rx buffer here.
                    uart_flush_input(EX_UART_NUM);
                } else {
                    uart_read_bytes(EX_UART_NUM, uart2_recive_data, pos, 100 / portTICK_PERIOD_MS);
                    uint8_t pat[3 + 1];
                    memset(pat, 0, sizeof(pat));
                    uart_read_bytes(EX_UART_NUM, pat, 3, 100 / portTICK_PERIOD_MS);
                    ESP_LOGI(TAG, "read data: %s", uart2_recive_data);
                    ESP_LOGI(TAG, "read pat : %s", pat);
                }
                break;
            //Others
            default:
                ESP_LOGI(TAG, "uart event type: %d", event.type);
                break;
            }
        }
		vTaskDelay(50/portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
	
}


void USART_init(void)
{
    uart_config_t uart_config = {
        .baud_rate = 57600,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    int intr_alloc_flags = 0;

#if CONFIG_UART_ISR_IN_IRAM
    intr_alloc_flags = ESP_INTR_FLAG_IRAM;
#endif

    uart_driver_install(EX_UART_NUM, BUF_SIZE * 2, BUF_SIZE * 2, 20, &uart2_queue, 0);
    uart_param_config(UART_NUM_2, &uart_config);
    uart_set_pin(UART_NUM_2, GPIO_NUM_10, GPIO_NUM_11, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    uart_pattern_queue_reset(EX_UART_NUM, 20);//重置队列，最多纪录20条
    xTaskCreate(uart_event_task, "uart_event_task", 2048, NULL, 9, NULL);//创建任务句柄
    PS_Empty();
    ID=0;
    count_people=0;
}

#include <string.h>
#include "AS608.h"

//uint32_t AS608Addr = 0XFFFFFFFF; //默认


//串口发送一个字节
static void MYUSART_SendData(uint8_t data)
{
	uart_write_bytes(UART_NUM_2, &data,sizeof(data));
}
//发送包头
static void SendHead(void)
{
	MYUSART_SendData(0xEF);
	MYUSART_SendData(0x01);
}
//发送地址
static void SendAddr(void)
{
	MYUSART_SendData(AS608Addr>>24);
	MYUSART_SendData(AS608Addr>>16);
	MYUSART_SendData(AS608Addr>>8);
	MYUSART_SendData(AS608Addr);
}
//发送包标识,
static void SendFlag(uint8_t flag)
{
	MYUSART_SendData(flag);
}
//发送包长度
static void SendLength(int length)
{
	MYUSART_SendData(length>>8);
	MYUSART_SendData(length);
}
//发送指令码
static void Sendcmd(uint8_t cmd)
{
	MYUSART_SendData(cmd);
}
//发送校验和
static void SendCheck(uint16_t check)
{
	MYUSART_SendData(check>>8);
	MYUSART_SendData(check);
}
//判断中断接收的数组有没有应答包
//waittime为等待中断接收数据的时间（单位1ms）
//返回值：数据包首地址
static uint8_t *JudgeStr(uint16_t waittime)
{
	char *data;
	uint8_t str[8];
	str[0]=0xef;str[1]=0x01;str[2]=AS608Addr>>24;
	str[3]=AS608Addr>>16;str[4]=AS608Addr>>8;
	str[5]=AS608Addr;str[6]=0x07;str[7]='\0';
	while(--waittime)
	{
		vTaskDelay(1 / portTICK_PERIOD_MS);
		if(flag_rx_uart2==1)//接收到一次数据
		{
            data=strstr((const char*)uart2_recive_data,(const char*)str);
			//printf("*********************");
			/*for(int i=0;i<event.size;i++){
				printf("data:%d\n",data[i]);
			}*/
			if(data){
				return (uint8_t*)data;	
				}
		}
		flag_rx_uart2=0;
	}
	
	return 0;
}
//录入图像 PS_GetImage
//功能:探测手指，探测到后录入指纹图像存于ImageBuffer。 
//模块返回确认字
uint8_t PS_GetImage(void)
{
  uint16_t temp;
  uint8_t  ensure;
 
	uint8_t  *data_get;
	 
	SendHead();
	SendAddr();
	SendFlag(0x01);//命令包标识
	SendLength(0x03);
	Sendcmd(0x01);
    
 	temp =  0x01+0x03+0x01;
	SendCheck(temp);
	data_get=JudgeStr(2000);
	
	if(data_get){
		ensure=data_get[9];
		//printf("ensure:%d\n",ensure);
	}
	else
		ensure=0xff;
	return ensure;
}
//生成特征 PS_GenChar
//功能:将ImageBuffer中的原始图像生成指纹特征文件存于CharBuffer1或CharBuffer2			 
//参数:BufferID --> charBuffer1:0x01	charBuffer1:0x02												
//模块返回确认字
static const char *TAG_add = "add zw";
uint8_t PS_GenChar(uint8_t BufferID)
{
	uint16_t temp;
  uint8_t  ensure;
	uint8_t  *data;
	
	SendHead();
	SendAddr();
	SendFlag(0x01);//命令包标识
	SendLength(0x04);
	Sendcmd(0x02);
	MYUSART_SendData(BufferID);
	temp = 0x01+0x04+0x02+BufferID;
	SendCheck(temp);
	
	data=JudgeStr(2000);
	
	if(data)
		{ensure=data[9];
		
		}
	else
		ensure=0xff;
	return ensure;
}
//精确比对两枚指纹特征 PS_Match
//功能:精确比对CharBuffer1 与CharBuffer2 中的特征文件 
//模块返回确认字
uint8_t PS_Match(void)
{
	uint16_t temp;
  uint8_t  ensure;
	uint8_t  *data;
	SendHead();
	SendAddr();
	SendFlag(0x01);//命令包标识
	SendLength(0x03);
	Sendcmd(0x03);
	temp = 0x01+0x03+0x03;
	SendCheck(temp);
	data=JudgeStr(2000);
	if(data)
		ensure=data[9];
	else
		ensure=0xff;
	return ensure;
}
//搜索指纹 PS_Search
//功能:以CharBuffer1或CharBuffer2中的特征文件搜索整个或部分指纹库.若搜索到，则返回页码。			
//参数:  BufferID @ref CharBuffer1	CharBuffer2
//说明:  模块返回确认字，页码（相配指纹模板）
uint8_t PS_Search(uint8_t BufferID,uint16_t StartPage,uint16_t PageNum,SearchResult *p)
{
	uint16_t temp;
  uint8_t  ensure;
	uint8_t  *data;
	SendHead();
	SendAddr();
	SendFlag(0x01);//命令包标识
	SendLength(0x08);
	Sendcmd(0x04);
	MYUSART_SendData(BufferID);
	MYUSART_SendData(StartPage>>8);
	MYUSART_SendData(StartPage);
	MYUSART_SendData(PageNum>>8);
	MYUSART_SendData(PageNum);
	temp = 0x01+0x08+0x04+BufferID
	+(StartPage>>8)+(uint8_t)StartPage
	+(PageNum>>8)+(uint8_t)PageNum;
	SendCheck(temp);
	data=JudgeStr(2000);
	if(data)
	{
		ensure = data[9];
		p->pageID   =(data[10]<<8)+data[11];
		p->mathscore=(data[12]<<8)+data[13];	
	}
	else
		ensure = 0xff;
	return ensure;	
}
//合并特征（生成模板）PS_RegModel
//功能:将CharBuffer1与CharBuffer2中的特征文件合并生成 模板,结果存于CharBuffer1与CharBuffer2	
//说明:  模块返回确认字
uint8_t PS_RegModel(void)
{
	uint16_t temp;
  uint8_t  ensure;
	uint8_t  *data;
	SendHead();
	SendAddr();
	SendFlag(0x01);//命令包标识
	SendLength(0x03);
	Sendcmd(0x05);
	temp = 0x01+0x03+0x05;
	SendCheck(temp);
	data=JudgeStr(2000);
	if(data)
		ensure=data[9];
	else
		ensure=0xff;
	return ensure;		
}
//储存模板 PS_StoreChar
//功能:将 CharBuffer1 或 CharBuffer2 中的模板文件存到 PageID 号flash数据库位置。			
//参数:  BufferID @ref charBuffer1:0x01	charBuffer1:0x02
//       PageID（指纹库位置号）
//说明:  模块返回确认字
uint8_t PS_StoreChar(uint8_t BufferID,uint16_t PageID)
{
	uint16_t temp;
  uint8_t  ensure;
	uint8_t  *data;
	SendHead();
	SendAddr();
	SendFlag(0x01);//命令包标识
	SendLength(0x06);
	Sendcmd(0x06);
	MYUSART_SendData(BufferID);
	MYUSART_SendData(PageID>>8);
	MYUSART_SendData(PageID);
	temp = 0x01+0x06+0x06+BufferID
	+(PageID>>8)+(uint8_t)PageID;
	SendCheck(temp);
	data=JudgeStr(2000);
	if(data)
		ensure=data[9];
	else
		ensure=0xff;
	return ensure;	
}
//删除模板 PS_DeletChar
//功能:  删除flash数据库中指定ID号开始的N个指纹模板
//参数:  PageID(指纹库模板号)，N删除的模板个数。
//说明:  模块返回确认字
uint8_t PS_DeletChar(uint16_t PageID,uint16_t N)
{
	uint16_t temp;
  uint8_t  ensure=0;
	uint8_t  *data;
	SendHead();
	SendAddr();
	SendFlag(0x01);//命令包标识
	SendLength(0x07);
	Sendcmd(0x0C);
	MYUSART_SendData(PageID>>8);
	MYUSART_SendData(PageID);
	MYUSART_SendData(N>>8);
	MYUSART_SendData(N);
	temp = 0x01+0x07+0x0C
	+(PageID>>8)+(uint8_t)PageID
	+(N>>8)+(uint8_t)N;
	SendCheck(temp);
	data=JudgeStr(2000);
	//printf("ensure:%d\n",data[9]);
	if(data)
		{ensure=data[9];
		printf("\ndelete OK\n");}
	else
		ensure=0xff;
	return ensure;
}
//清空指纹库 PS_Empty
//功能:  删除flash数据库中所有指纹模板
//参数:  无
//说明:  模块返回确认字
uint8_t PS_Empty(void)
{
	uint16_t temp;
  uint8_t  ensure;
	uint8_t  *data;
	SendHead();
	SendAddr();
	SendFlag(0x01);//命令包标识
	SendLength(0x03);
	Sendcmd(0x0D);
	temp = 0x01+0x03+0x0D;
	SendCheck(temp);
	data=JudgeStr(2000);
	if(data)
		ensure=data[9];
	else
		ensure=0xff;
	return ensure;
}
//写系统寄存器 PS_WriteReg
//功能:  写模块寄存器
//参数:  寄存器序号RegNum:4\5\6
//说明:  模块返回确认字
uint8_t PS_WriteReg(uint8_t RegNum,uint8_t DATA)
{
	uint16_t temp;
  uint8_t  ensure;
	uint8_t  *data;
	SendHead();
	SendAddr();
	SendFlag(0x01);//命令包标识
	SendLength(0x05);
	Sendcmd(0x0E);
	MYUSART_SendData(RegNum);
	MYUSART_SendData(DATA);
	temp = RegNum+DATA+0x01+0x05+0x0E;
	SendCheck(temp);
	data=JudgeStr(2000);
	if(data)
		ensure=data[9];
	else
		ensure=0xff;
	if(ensure==0)
		printf("\r\n设置参数成功！");
	else
		printf("\r\n%s",EnsureMessage(ensure));
	return ensure;
}
//读系统基本参数 PS_ReadSysPara
//功能:  读取模块的基本参数（波特率，包大小等)
//参数:  无
//说明:  模块返回确认字 + 基本参数（16bytes）
uint8_t PS_ReadSysPara(SysPara *p)
{
	uint16_t temp;
  uint8_t  ensure;
	uint8_t  *data;
	SendHead();
	SendAddr();
	SendFlag(0x01);//命令包标识
	SendLength(0x03);
	Sendcmd(0x0F);
	temp = 0x01+0x03+0x0F;
	SendCheck(temp);
	data=JudgeStr(1000);
	if(data)
	{
		ensure = data[9];
		p->PS_max = (data[14]<<8)+data[15];
		p->PS_level = data[17];
		p->PS_addr=(data[18]<<24)+(data[19]<<16)+(data[20]<<8)+data[21];
		p->PS_size = data[23];
		p->PS_N = data[25];
	}		
	else
		ensure=0xff;
	if(ensure==0x00)
	{
		printf("\r\n模块最大指纹容量=%d",p->PS_max);
		printf("\r\n对比等级=%d",p->PS_level);
		printf("\r\n地址=%x",p->PS_addr);
		printf("\r\n波特率=%d",p->PS_N*9600);
	}
	else 
			printf("\r\n%s",EnsureMessage(ensure));
	return ensure;
}
//设置模块地址 PS_SetAddr
//功能:  设置模块地址
//参数:  PS_addr
//说明:  模块返回确认字
uint8_t PS_SetAddr(uint32_t PS_addr)
{
	uint16_t temp;
  uint8_t  ensure;
	uint8_t  *data;
	SendHead();
	SendAddr();
	SendFlag(0x01);//命令包标识
	SendLength(0x07);
	Sendcmd(0x15);
	MYUSART_SendData(PS_addr>>24);
	MYUSART_SendData(PS_addr>>16);
	MYUSART_SendData(PS_addr>>8);
	MYUSART_SendData(PS_addr);
	temp = 0x01+0x07+0x15
	+(uint8_t)(PS_addr>>24)+(uint8_t)(PS_addr>>16)
	+(uint8_t)(PS_addr>>8) +(uint8_t)PS_addr;				
	SendCheck(temp);
	AS608Addr=PS_addr;//发送完指令，更换地址
    data=JudgeStr(2000);
	if(data)
		ensure=data[9];
	else{
		ensure=0xff;	
		AS608Addr = PS_addr;
    }
	if(ensure==0x00)
		printf("\r\n设置地址成功!");
	else
		printf("\r\n%s",EnsureMessage(ensure));
	return ensure;
}
//功能： 模块内部为用户开辟了256bytes的FLASH空间用于存用户记事本,
//	该记事本逻辑上被分成 16 个页。
//参数:  NotePageNum(0~15),Byte32(要写入内容，32个字节)
//说明:  模块返回确认字
uint8_t PS_WriteNotepad(uint8_t NotePageNum,uint8_t *Byte32)
{
	uint16_t temp=0;
  uint8_t  ensure,i;
	uint8_t  *data;
	SendHead();
	SendAddr();
	SendFlag(0x01);//命令包标识
	SendLength(36);
	Sendcmd(0x18);
	MYUSART_SendData(NotePageNum);
	for(i=0;i<32;i++)
	 {
		 MYUSART_SendData(Byte32[i]);
		 temp += Byte32[i];
	 }
  temp =0x01+36+0x18+NotePageNum+temp;
	SendCheck(temp);
  data=JudgeStr(2000);
	if(data)
		ensure=data[9];
	else
		ensure=0xff;
	return ensure;
}
//读记事PS_ReadNotepad
//功能：  读取FLASH用户区的128bytes数据
//参数:  NotePageNum(0~15)
//说明:  模块返回确认字+用户信息
uint8_t PS_ReadNotepad(uint8_t NotePageNum,uint8_t *Byte32)
{
	uint16_t temp;
  uint8_t  ensure,i;
	uint8_t  *data;
	SendHead();
	SendAddr();
	SendFlag(0x01);//命令包标识
	SendLength(0x04);
	Sendcmd(0x19);
	MYUSART_SendData(NotePageNum);
	temp = 0x01+0x04+0x19+NotePageNum;
	SendCheck(temp);
  data=JudgeStr(2000);
	if(data)
	{
		ensure=data[9];
		for(i=0;i<32;i++)
		{
			Byte32[i]=data[10+i];
		}
	}
	else
		ensure=0xff;
	return ensure;
}
//高速搜索PS_HighSpeedSearch
//功能：以 CharBuffer1或CharBuffer2中的特征文件高速搜索整个或部分指纹库。
//		  若搜索到，则返回页码,该指令对于的确存在于指纹库中 ，且登录时质量
//		  很好的指纹，会很快给出搜索结果。
//参数:  BufferID， StartPage(起始页)，PageNum（页数）
//说明:  模块返回确认字+页码（相配指纹模板）
uint8_t PS_HighSpeedSearch(uint8_t BufferID,uint16_t StartPage,uint16_t PageNum,SearchResult *p)
{
	uint16_t temp;
  uint8_t  ensure;
	uint8_t  *data;
	SendHead();
	SendAddr();
	SendFlag(0x01);//命令包标识
	SendLength(0x08);
	Sendcmd(0x1b);
	MYUSART_SendData(BufferID);
	MYUSART_SendData(StartPage>>8);
	MYUSART_SendData(StartPage);
	MYUSART_SendData(PageNum>>8);
	MYUSART_SendData(PageNum);
	temp = 0x01+0x08+0x1b+BufferID
	+(StartPage>>8)+(uint8_t)StartPage
	+(PageNum>>8)+(uint8_t)PageNum;
	SendCheck(temp);
	data=JudgeStr(2000);
 	if(data)
	{
		ensure=data[9];
		p->pageID 	=(data[10]<<8) +data[11];
		p->mathscore=(data[12]<<8) +data[13];
	}
	else
		ensure=0xff;
	return ensure;
}
//读有效模板个数 PS_ValidTempleteNum
//功能：读有效模板个数
//参数: 无
//说明: 模块返回确认字+有效模板个数ValidN
uint8_t PS_ValidTempleteNum(uint16_t *ValidN)
{
	uint16_t temp;
  uint8_t  ensure;
	uint8_t  *data;
	SendHead();
	SendAddr();
	SendFlag(0x01);//命令包标识
	SendLength(0x03);
	Sendcmd(0x1d);
	temp = 0x01+0x03+0x1d;
	SendCheck(temp);
  data=JudgeStr(2000);
	if(data)
	{
		ensure=data[9];
		*ValidN = (data[10]<<8) +data[11];
	}		
	else
		ensure=0xff;
	
	if(ensure==0x00)
	{
		//printf("\r\n有效指纹个数=%d",(data[10]<<8)+data[11]);
		ESP_LOGI(TAG_add, "有效指纹数量：%d\n",(data[10]<<8)+data[11]);
	}
	else
		printf("\r\n%s",EnsureMessage(ensure));
	return ensure;
}
//与AS608握手 PS_HandShake
//参数: PS_Addr地址指针
//说明: 模块返新地址（正确地址）	
/*uint8_t PS_HandShake(uint32_t *PS_Addr)
{	
	SendHead();
	SendAddr();
	MYUSART_SendData(0X01);
	MYUSART_SendData(0X00);
	MYUSART_SendData(0X00);
	vTaskDelay(200 / portTICK_PERIOD_MS);
	if(UART_2_FLAG&0X8000)//接收到数据
	{		
        uint8_t *data = (uint8_t *) malloc(BUF_SIZE);
        uart_read_bytes(UART_NUM_2, data, BUF_SIZE, 20 / portTICK_PERIOD_MS);
		if(//判断是不是模块返回的应答包				
                data[0]==0XEF
				&&data[1]==0X01
				&&data[6]==0X07
			)
			{
				*PS_Addr=(data[2]<<24) + (data[3]<<16)
								+(data[4]<<8) + (data[5]);
                                UART_2_FLAG=0;
				return 0;
			}
            UART_2_FLAG=0;					
	}
	return 1;		
} */
//模块应答包确认码信息解析
//功能：解析确认码错误信息返回信息
//参数: ensure
const char *EnsureMessage(uint8_t ensure) 
{
	const char *p;
	switch(ensure)
	{
		case  0x00:
			p="OK";break;		
		case  0x01:
			p="数据包接收错误";break;
		case  0x02:
			p="传感器上没有手指";break;
		case  0x03:
			p="录入指纹图像失败";break;
		case  0x04:
			p="指纹图像太干、太淡而生不成特征";break;
		case  0x05:
			p="指纹图像太湿、太糊而生不成特征";break;
		case  0x06:
			p="指纹图像太乱而生不成特征";break;
		case  0x07:
			p="指纹图像正常，但特征点太少（或面积太小）而生不成特征";break;
		case  0x08:
			p="指纹不匹配";break;
		case  0x09:
			p="没搜索到指纹";break;
		case  0x0a:
			p="特征合并失败";break;
		case  0x0b:
			p="访问指纹库时地址序号超出指纹库范围";
		case  0x10:
			p="删除模板失败";break;
		case  0x11:
			p="清空指纹库失败";break;	
		case  0x15:
			p="缓冲区内没有有效原始图而生不成图像";break;
		case  0x18:
			p="读写 FLASH 出错";break;
		case  0x19:
			p="未定义错误";break;
		case  0x1a:
			p="无效寄存器号";break;
		case  0x1b:
			p="寄存器设定内容错误";break;
		case  0x1c:
			p="记事本页码指定错误";break;
		case  0x1f:
			p="指纹库满";break;
		case  0x20:
			p="地址错误";break;
		default :
			p="模块返回确认码有误";break;
	}
 return p;	
}


int add(void){//录入指纹
    uint8_t i=0,ensure=0 ,processnum=0;
    int flag=0;
    while(1){
    switch (processnum)
		{
			case 0:
				i++;
				ESP_LOGE(TAG_add,"请按手指");
				ensure=PS_GetImage();
				//printf("ensure:%d\n",ensure);
				if(ensure==0x00) 
				{
					ESP_LOGI(TAG_add, "录取成功");
					ensure=PS_GenChar(CharBuffer1);//生成特征
					if(ensure==0x00)
					{
						i=0;
						ESP_LOGI(TAG_add, "指纹正常");
						processnum=1;//跳到第二步						
					}			
				}						
				break;
			
			case 1:
				i++;
				ensure=PS_GetImage();
				if(ensure==0x00) 
				{
					ensure=PS_GenChar(CharBuffer2);//生成特征			
					if(ensure==0x00)
					{
						i=0;
						ESP_LOGI(TAG_add, "生成特征正常");
						processnum=2;//跳到第三步
					}
				}	
				break;

			case 2:
				ensure=PS_Match();
				if(ensure==0x00) 
				{
					ESP_LOGI(TAG_add, "对比成功");
					processnum=3;//跳到第四步
				}
				else 
				{
					i=0;
					processnum=0;//跳回第一步		
				}
				vTaskDelay(1000/portTICK_PERIOD_MS);
				break;

			case 3:

				ensure=PS_RegModel();
				if(ensure==0x00) 
				{
					ESP_LOGI(TAG_add, "生成模板成功");
					processnum=4;//跳到第五步
				}else {processnum=0;}
				vTaskDelay(1000/portTICK_PERIOD_MS);
				break;
				
			case 4:	
		  		ID++;
				ensure=PS_StoreChar(CharBuffer2,ID);//储存模板
				if(ensure==0x00) 
				{			
					ESP_LOGE(TAG_add,"录入成功");
					uint16_t ValidN;
					PS_ValidTempleteNum(&ValidN);//读库指纹个数
					vTaskDelay(1500/portTICK_PERIOD_MS);	
					flag=1;
                    return flag;
				}else {processnum=0;}					
				break;				
		}
		vTaskDelay(400/portTICK_PERIOD_MS);	
		if(i==10)//超过10次没有按手指则退出
		{
			ESP_LOGE(TAG_add,"录入失败");
			break;	
		}				
	}return flag;
}

//刷指纹
int press_FR(int *k)
{
	SearchResult seach;
	uint8_t ensure;
	char *str;
    int flag=0;
    uint8_t i=0;
    while(1){
        ESP_LOGE(TAG_add,"请刷指纹");
        i++;
        ensure=PS_GetImage();
        if(ensure==0x00)//获取图像成功 
        {	
            ensure=PS_GenChar(CharBuffer1);
            if(ensure==0x00) //生成特征成功
            {		
                ensure=PS_Search(CharBuffer1,0,300,&seach);
                if(ensure==0x00)//搜索成功
                {	
                    if(seach.mathscore>100)
                        {
                            ESP_LOGE(TAG_add,"配对成功！指纹ID：%d",seach.pageID);
                            *k=seach.pageID;
                            flag=1;
                            return flag;
                            vTaskDelay(5000/portTICK_PERIOD_MS);
                        }else
                        {
                            ESP_LOGE(TAG_add,"匹配度不够，无法确认身份");
                        }

                }else
                {
                            ESP_LOGE(TAG_add,"未能搜索到匹配的指纹");
                }

            }
        }
        vTaskDelay(700/portTICK_PERIOD_MS);	
		if(i==10)//超过10次没有按手指则退出
		{
			ESP_LOGE(TAG_add,"录入失败");
			break;	
		}		
    }
	
		return flag;
}






