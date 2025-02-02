/*******************************************************************************
** 文件名: 		ymodem.c
** 版本：  		1.0
** 工作环境: 	RealView MDK-ARM 4.14
** 作者: 		wuguoyana
** 生成日期: 	2011-04-29
** 功能:		和Ymodem.c的相关的协议文件
                负责从超级终端接收数据(使用Ymodem协议)，并将数据加载到内部RAM中。
                如果接收数据正常，则将数据编程到Flash中；如果发生错误，则提示出错。
** 相关文件:	stm32f10x.h
** 修改日志：	2011-04-29   创建文档
*******************************************************************************/

/* 包含头文件 *****************************************************************/

#include "common.h"
#include "stm32f10x_flash.h"

/* 变量声明 -----------------------------------------------------------------*/
uint8_t file_name[FILE_NAME_LENGTH];//接收的二进制文件文件的文件名
//用户程序Flash偏移
uint32_t FlashDestination = ApplicationAddress;//用户程序起始下载存储地址
uint16_t PageSize = PAGE_SIZE;//包大小
uint32_t EraseCounter = 0x0;
uint32_t NbrOfPage = 0;       //flash是按页来存储的 起始地址到文件大小需要所需的flash页数
FLASH_Status FLASHStatus = FLASH_COMPLETE;
uint32_t RamSource;
extern uint8_t tab_1024[1024];

/*******************************************************************************
  * @函数名称	Receive_Byte
  * @函数说明   从发送端接收一个字节
  * @输入参数   c: 接收字符
                timeout: 超时时间
  * @输出参数   无
  * @返回参数   接收的结果
                0：成功接收
                1：时间超时
*******************************************************************************/
static  int32_t Receive_Byte (uint8_t *c, uint32_t timeout)
{
    while (timeout-- > 0)
    {
        if (SerialKeyPressed(c) == 1)//串口接收一个字节数据
        {
            return 0;
        }
    }
    return -1;
}

/*******************************************************************************
  * @函数名称	Send_Byte
  * @函数说明   发送一个字符
  * @输入参数   c: 发送的字符
  * @输出参数   无
  * @返回参数   发送的结果
                0：成功发送
*******************************************************************************/
static uint32_t Send_Byte (uint8_t c)
{
    SerialPutChar(c);// 串口发送一个字节
    return 0;
}

/*******************************************************************************
  * @函数名称	Receive_Packet
  * @函数说明   从发送端接收一个数据包
  * @输入参数   data ：存储数据指针           首字节定义数据包长度 + 数据包头信息(5字节？) + 1024/128字节数据
                length：本包数据长度 128/1024
                timeout ：超时时间
  * @输出参数   返回数据包的大小 
  * @返回参数   接收的结果
                0: 正常返回
                -1: 超时或者数据包错误
                1: 用户取消
*******************************************************************************/
static int32_t Receive_Packet (uint8_t *data, int32_t *length, uint32_t timeout)
{
    uint16_t i, packet_size;
    uint8_t c; //接收的一个字节数据
    *length = 0;

///////判断第一字节数据////////////	
	//每个数据包的 第一个字节数据 为标志 存在c中
    if (Receive_Byte(&c, timeout) != 0)// 接收第一个字节数据 用户传输大小 状况等  串口接收一个数据  RS485   CAN 等也可以
    {
        return -1;
    }
    switch (c)
    {
    case SOH:  // #define SOH   (0x01)  //128字节数据包开始
        packet_size = PACKET_SIZE;// 数据传输端  定义数据包的大小 128
        break;
    case STX:  // #define STX   (0x02)  //1024字节的数据包开始
        packet_size = PACKET_1K_SIZE;// 数据包大小为  1024
        break;
    case EOT:  // #define EOT                     (0x04)  //结束传输  end of transmit
        return 0;
    case CA:   // #define CA                      (0x18)  //这两个相继中止转移 cancel
        if ((Receive_Byte(&c, timeout) == 0) && (c == CA))
        {
            *length = -1;   //发送端终止    两个  连续两个  CA=(0x18)  表示发端取消传输
            return 0;     
        }
        else
        {
            return -1; 
        }
    case ABORT1:  // #define ABORT1   0x41)  //'A' == 0x41, 用户终止 
    case ABORT2:  // #define ABORT2   (0x61)  //'a' == 0x61, 用户终止
        return 1;
    default:
        return -1;
    }
		
//////接收后面的数据/////////////////
    *data = c;// 存放第一个字节数据
		// 按照数据包第一个字节定义的 数据包大小接收数据  注意还有一个数据包头信息
    for (i = 1; i < (packet_size + PACKET_OVERHEAD); i++)
    {
        if (Receive_Byte(data + i, timeout) != 0)//接收后面的数据  存入到 data内
        {
            return -1;
        }
    }
    if (data[PACKET_SEQNO_INDEX] != ((data[PACKET_SEQNO_COMP_INDEX] ^ 0xff) & 0xff))
    {
        return -1;
    }
    *length = packet_size;//返回接受的数据包长度  128或者 1024
    return 0;
}

/*******************************************************************************
  * @函数名称	Ymodem_Receive
  * @函数说明   通过 ymodem协议接收一个文件
  * @输入参数   文件存储数组 buf: 首地址指针
  * @输出参数   无
  * @返回参数   文件长度

发送方:
等待接收到 C  等待发送文件标志   
第0个数据包为 3字节 数据包大小标志 + 编号00+编号反码FF  + 128字节/1024字节 文件属性数据包(文件名字  +  文件大小 不足补零) + 两字节 CRC校验码
              等待回应ACK
其后为数据包  3字节 数据包大小标志 + 编号01+编号反码FE  + 128字节/1024字节 内容(不足补充 换行符) + 两字节 CRC校验码
              等待回应ACK
最后发送 EOT结束发送标志
              等待回应ACK
再发送一包全为零的数据包 3字节 数据包大小标志 + 编号00+编号反码FF + 128字节 0数据  + CRC校验码
              等待回应ACK
最后再一次发送 EOT结束发送标志，结束发送
              等待回应ACK

接收方：
发送'C',等待发送发送方响应
接收数据包   第一个字为 为数据包大小标志  根据标志接收字符并回应


*******************************************************************************/
int32_t Ymodem_Receive (uint8_t *buf)
{
	  // 定义一个数据包大小  文件大小
    uint8_t packet_data[PACKET_1K_SIZE + PACKET_OVERHEAD], file_size[FILE_SIZE_LENGTH], *file_ptr, *buf_ptr;
	// 数据包长度
    int32_t i, j, packet_length, session_done, file_done, packets_received, errors, session_begin, size = 0;

    //初始化Flash地址变量
    FlashDestination = ApplicationAddress;//用户程序下载的初始地址

    for (session_done = 0, errors = 0, session_begin = 0; ;)
    {
        for (packets_received = 0, file_done = 0, buf_ptr = buf; ;)
        {
					  // 接收一个数据包 数据存放在 packet_data内  数据包有效数据大小 128/1024 存放在packet_length内
            switch (Receive_Packet(packet_data, &packet_length, NAK_TIMEOUT))//接收一个数据包 数据包首地址 超时
            {
            case 0://接收数据正常
                errors = 0;
                switch (packet_length)//数据包长度
                {
                    //发送端终止
                case - 1:
                    Send_Byte(ACK); // #define ACK                     (0x06)  //回应上位机
                    return 0;
                    //结束传输
                case 0:
                    Send_Byte(ACK); // #define ACK                     (0x06)  //回应上位机
                    file_done = 1;  // 数据接收完成
                    break;
                    //正常的数据包
                default:
                    if ((packet_data[PACKET_SEQNO_INDEX] & 0xff) != (packets_received & 0xff))
                    {
                        Send_Byte(NAK); // #define NAK    (0x15)  //没回应
                    }
                    else
                    {
///////////////////////第0个数据包   为文件名+文件大小数据包/////////////////////////
                        if (packets_received == 0)
                        {
                            // 文件名数据包
													  // 擦除FLASH空间  packet_data[PACKET_HEADER]数据内容开始
                            if (packet_data[PACKET_HEADER] != 0)
                            {
                                //文件名数据包有效数据区域  从PACKET_HEADER下标开始为数据
															  //注意发送文件名后 紧接着会有一个0 以划分 文件名区域和 文件大小区域
                                for (i = 0, file_ptr = packet_data + PACKET_HEADER; (*file_ptr != 0) && (i < FILE_NAME_LENGTH);)
                                {
                                    file_name[i++] = *file_ptr++; // 文件名区域 到 *file_ptr = 0 的地方
                                }
                                file_name[i++] = '\0';//添加结束符
                                for (i = 0, file_ptr ++; (*file_ptr != ' ') && (i < FILE_SIZE_LENGTH);)
                                {
                                    file_size[i++] = *file_ptr++;// 文件大小区域 到 file_ptr = ' '的地方
                                }
                                file_size[i++] = '\0';//添加结束符
                                Str2Int(file_size, &size);// 字符串大小转换成   数字大小 size

                                //测试数据包是否过大
                                if (size > (FLASH_SIZE - ApplicationAddress_offset - 1)) 
																	// 有点问题FLASH_SIZE = (0x20000)  /* 128 KBytes */ 为全部大小 应该要减去 IAP程序大小 ApplicationAddress_offset
                                {
                                    //结束
                                    Send_Byte(CA);
                                    Send_Byte(CA);
                                    return -1; //文件超过FLASH 用户区域大小
                                }

                                //计算需要擦除Flash的页
                                NbrOfPage = FLASH_PagesMask(size);
      /////////// 直接 擦除Flash是不是有问题  应该先判断一下 接受的文件是否有效 再擦除？？？？
								//判断 crc校验码是否正确
                 ////////////////////////擦除Flash//////////////////////////
                                for (EraseCounter = 0; (EraseCounter < NbrOfPage) && (FLASHStatus == FLASH_COMPLETE); EraseCounter++)
                                {
                                    FLASHStatus = FLASH_ErasePage(FlashDestination + (PageSize * EraseCounter));
                                }
                                Send_Byte(ACK);//回应
                                Send_Byte(CRC16);//再发送 'C'
                            }
                            //文件名数据包空，结束传输
                            else  // 数据内容为 0 已经发送完成
                            {
                                Send_Byte(ACK);
                                file_done = 1;
                                session_done = 1;
                                break;
                            }
                        }
												
//////////////////////接收实际文件数据 包
                        else
                        {
                            memcpy(buf_ptr, packet_data + PACKET_HEADER, packet_length);//直接拷贝 数据包的 有效区域
                            RamSource = (uint32_t)buf;
                            for (j = 0; (j < packet_length) && (FlashDestination <  ApplicationAddress + size); j += 4)
                            {
                                //把接收到的数据编写到Flash中
                                FLASH_ProgramWord(FlashDestination, *(uint32_t*)RamSource);

                                if (*(uint32_t*)FlashDestination != *(uint32_t*)RamSource)
                                {
                                    //结束
                                    Send_Byte(CA); // 发送
                                    Send_Byte(CA);
                                    return -2;
                                }
                                FlashDestination += 4;// flash地址按4字节增长 且为偶数
                                RamSource += 4;       // 需要写入的二进制文件地址也同增长
                            }
                            Send_Byte(ACK);//回应
                        }
                        packets_received ++;//数据包编号+1
                        session_begin = 1;
                    }
                }
                break;
            case 1://接收数据终止
                Send_Byte(CA);
                Send_Byte(CA);
                return -3;
            default:
                if (session_begin > 0)
                {
                    errors ++;
                }
                if (errors > MAX_ERRORS)
                {
                    Send_Byte(CA);
                    Send_Byte(CA);
                    return 0;
                }
                Send_Byte(CRC16);
                break;
            }
            if (file_done != 0)
            {
                break;
            }
        }
        if (session_done != 0)
        {
            break;
        }
    }
    return (int32_t)size;
}

/*******************************************************************************
  * @函数名称	Ymodem_CheckResponse
  * @函数说明   通过 ymodem协议检测响应
  * @输入参数   c
  * @输出参数   无
  * @返回参数   0
*******************************************************************************/
int32_t Ymodem_CheckResponse(uint8_t c)
{
    return 0;
}

/*******************************************************************************
  * @函数名称	Ymodem_PrepareIntialPacket
  * @函数说明   准备第一个数据包     第一个数据包 的数据区域为   文件名 和文件大小 不包含数据
  * @输入参数   data：数据包起始地址
                fileName ：文件名
                length ：文件大小
  * @输出参数   无
  * @返回参数   无
*******************************************************************************/
void Ymodem_PrepareIntialPacket(uint8_t *data, const uint8_t* fileName, uint32_t *length)
{
    uint16_t i, j;
    uint8_t file_ptr[10];

    //制作头3个数据包
    data[0] = SOH;//标识数据包长度128
    data[1] = 0x00;
    data[2] = 0xff;
    //文件名数据包有效数据
	  //从第三字节开始存储文件名
    for (i = 0; (fileName[i] != '\0') && (i < FILE_NAME_LENGTH); i++)
    {
        data[i + PACKET_HEADER] = fileName[i];//文件名 最长为 256字节
    }
		//此时i为文件名存储后的偏移量
    //接着文件名存储后的地址开始 存储 文件大小
    data[i + PACKET_HEADER] = 0x00;//  在文件名后最后 补0  用于接收时区分

    Int2Str (file_ptr, *length);//文件大小 转成 字符串存放到 file_ptr中
    for (j =0, i = i + PACKET_HEADER + 1; file_ptr[j] != '\0' ; )
    {
        data[i++] = file_ptr[j++];//存放文件大小
    }
		// 后面是不是缺少  ' ' 空格符号？？
///////////////////////////////////////////////////////////////
		//按接收时的格式 看应该缺少一个 空格符 用以区分 后面为零的区域
		data[i + PACKET_HEADER] = ' ';//  在文件名后最后 补0  用于接收时区�
    for (j = i+1; j < PACKET_SIZE + PACKET_HEADER; j++)
    {
        data[j] = 0;//第一个数据包 数据位 在 存放 文件名和文件大小后如果不满，就补 0
    }
////////////////////////////////////////////////////////////
/*		
    for (j = i; j < PACKET_SIZE + PACKET_HEADER; j++)
    {
        data[j] = 0;//第一个数据包 数据位 在 存放 文件名和文件大小后如果不满，就补 0
    }
*/		 
}

/*******************************************************************************
  * @函数名称	Ymodem_PreparePacket
  * @函数说明   准备数据包
  * @输入参数   SourceBuf：数据源缓冲
                data：数据包
                pktNo ：数据包编号
                sizeBlk ：数据包长度
  * @输出参数   无
  * @返回参数   无
*******************************************************************************/
void Ymodem_PreparePacket(uint8_t *SourceBuf, uint8_t *data, uint8_t pktNo, uint32_t sizeBlk)
{
    uint16_t i, size, packetSize;
    uint8_t* file_ptr;

    //制作头3个字节的 数据包头
    packetSize = sizeBlk >= PACKET_1K_SIZE ? PACKET_1K_SIZE : PACKET_SIZE;
    size = sizeBlk < packetSize ? sizeBlk :packetSize;
	// 第一个字节 为数据区域大小 表示 128/1024
    if (packetSize == PACKET_1K_SIZE)
    {
        data[0] = STX;
    }
    else
    {
        data[0] = SOH;
    }
// 第二个字节
    data[1] = pktNo;   //数据包 编码 记录发送次数
// 第三个字节
    data[2] = (~pktNo);//编码的反码
    file_ptr = SourceBuf;//文件数据

//////数据包内的有效数据 从第三个字节开始 存入数据
    for (i = PACKET_HEADER; i < size + PACKET_HEADER; i++)
    {
        data[i] = *file_ptr++;//写入文件数据
    }
		// 应对最后一个数据包 可能数据大小小与 一包的数据大小
    if ( size  <= packetSize) //数据不够的话   补换行
    {
        for (i = size + PACKET_HEADER; i < packetSize + PACKET_HEADER; i++)
        {
            data[i] = 0x1A; //结束   
        }
    }
}

/*******************************************************************************
  * @函数名称	UpdateCRC16
  * @函数说明   更新输入数据的ＣＲＣ校验
  * @输入参数   crcIn
                byte
  * @输出参数   无
  * @返回参数   ＣＲＣ校验值
*******************************************************************************/
uint16_t UpdateCRC16(uint16_t crcIn, uint8_t byte)
{
    uint32_t crc = crcIn;
    uint32_t in = byte|0x100;
    do
    {
        crc <<= 1;
        in <<= 1;
        if (in&0x100)
            ++crc;
        if (crc&0x10000)
            crc ^= 0x1021;
    }
    while (!(in&0x10000));
    return crc&0xffffu;
}

/*******************************************************************************
  * @函数名称	  Cal_CRC16
  * @函数说明  计算输入数据的CRC校验码
  * @输入参数   data ：数据
                size ：长度
  * @输出参数   无
  * @返回参数   CRC 校验值
*******************************************************************************/
uint16_t Cal_CRC16(const uint8_t* data, uint32_t size)
{
    uint32_t crc = 0;
    const uint8_t* dataEnd = data+size;
    while (data<dataEnd)
        crc = UpdateCRC16(crc,*data++);

    crc = UpdateCRC16(crc,0);
    crc = UpdateCRC16(crc,0);
    return crc&0xffffu;
}


/*******************************************************************************
  * @函数名称	CalChecksum
  * @函数说明   计算YModem数据包的总大小
  * @输入参数   data ：数据
                size ：长度
  * @输出参数   无
  * @返回参数   数据包的总大小
*******************************************************************************/
uint8_t CalChecksum(const uint8_t* data, uint32_t size)
{
    uint32_t sum = 0;
    const uint8_t* dataEnd = data+size;
    while (data < dataEnd )
        sum += *data++;
    return sum&0xffu;
}

/*******************************************************************************
  * @函数名称	Ymodem_SendPacket
  * @函数说明   通过ymodem协议传输一个数据包
#  串口传输 数据
  * @输入参数   data ：数据地址指针
                length：长度
  * @输出参数   无
  * @返回参数   无
*******************************************************************************/
void Ymodem_SendPacket(uint8_t *data, uint16_t length)
{
    uint16_t i;
    i = 0;
    while (i < length)
    {
        Send_Byte(data[i]);
        i++;
    }
}

/*******************************************************************************
  * @函数名称	Ymodem_Transmit
  * @函数说明   通过ymodem协议传输一个文件
  * @输入参数   buf ：数据地址指针
                sendFileName ：文件名
                sizeFile：文件长度
  * @输出参数   无
  * @返回参数   是否成功
                0：成功
*******************************************************************************/
uint8_t Ymodem_Transmit (uint8_t *buf, const uint8_t* sendFileName, uint32_t sizeFile)
{

    uint8_t packet_data[PACKET_1K_SIZE + PACKET_OVERHEAD];//正常数据包长度
    uint8_t FileName[FILE_NAME_LENGTH];//文件名
    uint8_t *buf_ptr, tempCheckSum ;
    uint16_t tempCRC, blkNumber;
    uint8_t receivedC[2], CRC16_F = 0, i;
    uint32_t errors, ackReceived, size = 0, pktSize;

    errors = 0;
    ackReceived = 0;
    for (i = 0; i < (FILE_NAME_LENGTH - 1); i++)
    {
        FileName[i] = sendFileName[i];//保存文件名
    }
    CRC16_F = 1;// 需要校验码

    //准备第一个数据包           数据包起始地址   文件名   文件大小
    Ymodem_PrepareIntialPacket(&packet_data[0], FileName, &sizeFile);

    do
    {
///////发送第一个数据包 文件名   文件大小/////////////////////////////////////////////////////////////////////
        Ymodem_SendPacket(packet_data, PACKET_SIZE + PACKET_HEADER);//128 大小+3
        //发送两个字节的 CRC校验码  高八位在前  低八位在后
			  // 数据包后再发送  两个字节的  CRC校验码  或者 仅仅是 数据区域的和的后八位
        if (CRC16_F)
        {
            tempCRC = Cal_CRC16(&packet_data[3], PACKET_SIZE);//从数据区域开始产出 两个字节的 CRC校验码
            Send_Byte(tempCRC >> 8);//先发送高八位
            Send_Byte(tempCRC & 0xFF);//再发送低八位
        }
        else //发送 数据区域的和的后八位
        {
            tempCheckSum = CalChecksum (&packet_data[3], PACKET_SIZE);
            Send_Byte(tempCheckSum);
        }

        //等待数据接收端 响应  ACK
        if (Receive_Byte(&receivedC[0], 10000) == 0)
        {
            if (receivedC[0] == ACK)
            {
                //第一个 数据包正确传输
                ackReceived = 1;
            }
        }
        else
        {
            errors++;
        }
    } while (!ackReceived && (errors < 0x0A)); //第一个数据包传输正确 后 以及 错误次数过多后 就不发送第一个数据包

		// 如果是 第一个数据包发送错误次数过多  就取消发送文件  并返回错误次数
    if (errors >=  0x0A)
    {
        return errors;
    }
		
///////第一个数据包发送完成之后  开始传输后面的 文件数据包
    buf_ptr = buf; //文件存放的 首地址
    size = sizeFile;//文件大小
    blkNumber = 0x01;//数据包的编号
		
////////////////////////////////////////////////////////////////////////////////
////发送后面的数据 1024字节的数据包发送///////////////////
    while (size) 
    {
        //每次发送前 做的准备工作
	//准备下一个数据包   收据首地址    生成的一包数据   数据不够的话 数据区域补充 换行符
        Ymodem_PreparePacket(buf_ptr, &packet_data[0], blkNumber, size);
        ackReceived = 0;//数据回应标志 清零
        receivedC[0]= 0;//回应内容清理
        errors = 0;     //错误次数清零
        do
        {
        //发送下一个数据包
		//确定数据包大小
            if (size >= PACKET_1K_SIZE)//数据超过 1024大小 就按 1024的数据包发送
            {
                pktSize = PACKET_1K_SIZE;

            }
            else
            {
                pktSize = PACKET_SIZE;//否者按128大小发送
            }
		//通过数据接口(串口等)发送准备好的一包数据
            Ymodem_SendPacket(packet_data, pktSize + PACKET_HEADER);
            //发送CRC校验
            if (CRC16_F)
            {
                tempCRC = Cal_CRC16(&packet_data[3], pktSize);
                Send_Byte(tempCRC >> 8);   //高八位
                Send_Byte(tempCRC & 0xFF); //低八位
            }
            else
            {
                tempCheckSum = CalChecksum (&packet_data[3], pktSize);
                Send_Byte(tempCheckSum);//数据和低八位
            }

            //等待接收端回应
            if ((Receive_Byte(&receivedC[0], 100000) == 0)  && (receivedC[0] == ACK))
            {
                ackReceived = 1; //已回应
                if (size > pktSize) //剩余未发送数据 超过一个数据包大小吗
                {
                    buf_ptr += pktSize;// 数据地址偏移
                    size -= pktSize;   // 未发送数据大小相应减小
                    if (blkNumber == (FLASH_IMAGE_SIZE/1024))//如果超过可用flash地址大小返回错误
                    {
                        return 0xFF; //错误
                    }
                    else
                    {
                        blkNumber++;//数据包 编号+1
                    }
                }
                else//数据已经发送完了  因为生成数据包在前  数据包大小减小在后
                {
                    buf_ptr += pktSize;
                    size = 0; // 数据大小置0 结束数据的发送
                }
            }
            else
            {
                errors++;//未回应  错误次数+1
            }
        } while (!ackReceived && (errors < 0x0A));
				
        //如果没响应10次就返回错误
        if (errors >=  0x0A)
        {
            return errors;
        }

    }
//////////////////////////////////////////		
    ackReceived = 0;//回应标志清零
    receivedC[0] = 0x00;//回应内容清理
    errors = 0;         //未回应错误次数清零
    do
    {
        Send_Byte(EOT);//发送结束发送标志
        //发送 (EOT);
        //等待回应
        if ((Receive_Byte(&receivedC[0], 10000) == 0)  && receivedC[0] == ACK)
        {
            ackReceived = 1;//已回应标志
        }
        else
        {
            errors++;//未回应错误次数+1
        }
    } while (!ackReceived && (errors < 0x0A));

    if (errors >=  0x0A)
    {
        return errors;
    }
		
////////////////////////////////////////////////////////////////////////
////  发送 结束发送之后 需要发送一个全为0的数据包
    ackReceived = 0;
    receivedC[0] = 0x00;
    errors = 0;
////数据包前三个字节
    packet_data[0] = SOH;//128字节大小
    packet_data[1] = 0;
    packet_data [2] = 0xFF;
//准备数据区域 
    for (i = PACKET_HEADER; i < (PACKET_SIZE + PACKET_HEADER); i++)
    {
        packet_data [i] = 0x00;//全为 0
    }

    do
    {
      	//通过数据接口(串口等)发送准备好的一包数据
        Ymodem_SendPacket(packet_data, PACKET_SIZE + PACKET_HEADER);
			
        //发送CRC校验
        tempCRC = Cal_CRC16(&packet_data[3], PACKET_SIZE);
        Send_Byte(tempCRC >> 8);
        Send_Byte(tempCRC & 0xFF);

        //等待响应
        if (Receive_Byte(&receivedC[0], 10000) == 0)
        {
            if (receivedC[0] == ACK)
            {
                //包传输正确
                ackReceived = 1;
            }
        }
        else
        {
            errors++;
        }

    } while (!ackReceived && (errors < 0x0A));
    //如果没响应10次就返回错误
    if (errors >=  0x0A)
    {
        return errors;
    }

		ackReceived = 0;
    receivedC[0] = 0x00;
    errors = 0;
		//最后再一次发送结束发送标志
    do
    {
        Send_Byte(EOT);
        //发送 (EOT);
        //等待回应
        if ((Receive_Byte(&receivedC[0], 10000) == 0)  && receivedC[0] == ACK)
        {
            ackReceived = 1;
        }
        else
        {
            errors++;
        }
    } while (!ackReceived && (errors < 0x0A));

    if (errors >=  0x0A)
    {
        return errors;
    }
    return 0;//文件传输成功
}

/*******************************文件结束***************************************/
