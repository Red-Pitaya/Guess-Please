#include <STC15F2K60S2.H>
#include <intrins.h>
#define S2RI  0x01           //接收中断请求标志位
#define S2TI  0x02           //发送中断请求标志位
#define ADC_POWER 0X80     //ADC电源开关
#define ADC_FLAG 0X10        //当A/D转换完成后，ADC_FLAG要软件清零
#define ADC_START 0X08       //当A/D转换完成后，ADC_START会自动清零，所以要开始下一次转换，则需要置位
#define cstAdcSpeed90 0X60   /*ADC转换速度 90个时钟周期转换一次*/
#define cstAdcChs17 0X07     /*选择P1.7作为A/D输入*/
#define FOSC 11059200L     //晶振频率
#define BAUD 9600         //串口波特率

sbit Key1=P3^2;           //启动发送
sbit Key2=P3^3;           //显示存储数据
sbit Key3=P1^7;           //存储数据至0x00
sbit Sel0=P2^0;
sbit Sel1=P2^1;
sbit Sel2=P2^2;
sbit Buzzer=P3^4;			  //蜂鸣器
sbit I2C_SDA=P4^0;      //I2C总线的数据线
sbit I2C_SCL=P5^5;      //I2C总线的时钟线
sbit M485_TRN=P3^7;   		//MAX485使能引脚，1发送，0接收
bit isSend;                 	//为1时发送数据，为0时停止
bit guessflag;		//开始猜测flag
bit Start;			//开始标志
bit End;			//结束标志
bit BeepFlag;		//
bit ScoreFlag;		//分数面板

unsigned int T0count;
unsigned int Count;		//记录猜测多少次
unsigned char GetData;          //接收的数据
unsigned char SendData;         //发送的数据
unsigned char alreadySendData;		//已发送的数据
unsigned char LSegSelectState;         //左数段选
unsigned char RSegSelectState;         //右数段选
unsigned char DigSelectState;         //位选标志
unsigned char SegSelect[]={0x3f, 0x06, 0x5b, 0x4f, 0x66,0x6d, 0x7d, 0x07, 0x7f, 0x6f,0x77, 0x7c, 0x39, 0x5e, 0x79,0x71, 0x40, 0x00};//段选0-f
unsigned char DigSelect[]={0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07};
unsigned char arrRoll[]={0x3d,0x3e,0x79,0x6d,0x6d,0x00,0x73,0x38,0x79,0x77,0x6d,0x79,0x00,0x00};//位选
unsigned char arrRollsuccess[]={0x6d,0x3e,0x39,0x39,0x79,0x6d,0x6d,0x00,0x00};
unsigned char arrRollfail[]={0x71,0x77,0x30,0x38,0x00,0x00};
unsigned char WriteAddr;			//写入地址
unsigned char WriteData;
unsigned char WriteDataL;			//写入数据
unsigned char WriteDataR;
unsigned char ReadData;				//读出数据
unsigned char flag;				//数码管扫描
unsigned char startFlag;		//start位选
unsigned char startCount;		//start分频
unsigned char Dig1Tmp;
unsigned char Dig2Tmp;
unsigned char Dig3Tmp;
unsigned char Dig4Tmp;
unsigned char Dig5Tmp;
unsigned char Dig6Tmp;
unsigned char Dig7Tmp;
unsigned char Dig8Tmp;
unsigned char endFlag;		//end位选
unsigned char endCount;		//end分频
unsigned char endDig1Tmp;
unsigned char endDig2Tmp;
unsigned char endDig3Tmp;
unsigned char endDig4Tmp;
unsigned char endDig5Tmp;
unsigned char endDig6Tmp;
unsigned char endDig7Tmp;
unsigned char endDig8Tmp;

void Delay(unsigned int ms)		//@11.0592MHz
{
	unsigned char i, j;

	while(ms>0)
	{
		_nop_();
		_nop_();
		_nop_();
		i = 11;
		j = 190;
		do
		{
			while (--j);
		} while (--i);
		ms--;
	}
}

void delay()        //延时4us
{
    ;;
}

void Buzzer_Time(unsigned int ms)
{
	unsigned int i;
	for(i=0;i<ms*2;i++)
	{
		Buzzer=!Buzzer;
		Delay(1);
	}
}

void Uart2Init(void)		//9600bps@11.0592MHz
{
	S2CON = 0x10 ;      //定义无校验位，允许串行口2接收
	AUXR |= 0x14 ;      //T2为1T模式，并启动定时器2
	T2L=(65536-(FOSC/4/BAUD));
	T2H=(65536-(FOSC/4/BAUD))>>8;
}

void Timer0Init(void)		//1ms@12.000MHz
{	
	AUXR &= 0x7F;		
	TMOD &= 0xF0;             //定时器0，方式1
	TMOD |= 0x01;		//???????
	TH0=(65535-1000)/256; //定时器0的高八位设置
    TL0=(65535-1000)%256; //定时器0的低八位设置
}

void Init()
{
	P0M0 = 0xff ;       
    P0M1 = 0x00 ;
    P2M0 = 0x08 ;       
    P2M1 = 0x00 ;
    P3M0 = 0x10 ;
    P3M1 = 0x00 ;
    P1M0 = 0x00 ;
    P1M1 = 0x00 ;       //P1、P3准双向口
	
	P23=0;				
	Sel0 = 1 ;
    Sel1 = 1 ;
    Sel2 = 1 ;           //选择第八位数码管显示
	P0 = 0;
	
	DigSelectState = 0;
    LSegSelectState = 0;
	RSegSelectState = 0;
    SendData = 1 ;
	WriteAddr = 0;      //写入地址初始化
	WriteData=0;
    WriteDataL = 0;      //写入数据初始化
	WriteDataR = 0;
	flag = 0;
	Start=0;
	End=0;
	ScoreFlag=0;
	startFlag=0;
	startCount=0;
	Dig1Tmp=0;
	Dig2Tmp=1;
	Dig3Tmp=2;
	Dig4Tmp=3;
	Dig5Tmp=4;
	Dig6Tmp=5;
	Dig7Tmp=6;
	Dig8Tmp=7;
	endFlag=0;
	endCount=0;
	endDig1Tmp=0;
	endDig2Tmp=1;
	endDig3Tmp=2;
	endDig4Tmp=3;
	endDig5Tmp=4;
	endDig6Tmp=5;
	endDig7Tmp=6;
	endDig8Tmp=7;
	Count = 0;
	guessflag=0;
	Buzzer=0;
	BeepFlag = 0;	
	
	//ADC初始化
	P1ASF = 0x80;       //P1.7作为模拟功能A/D使用
    ADC_RES = 0;        //转换结果清零
    ADC_CONTR = 0x8F;   //ADC_POWER = 1
    CLK_DIV = 0X00;     //ADRJ = 0    ADC_RES存放高八位结果
	//外部中断0
    IT0 = 1 ;           //0下降沿
	//IT1 = 1;
    EX0 = 1 ;           //允许外部中断0
    PX0 = 0 ;           //外部中断为低优先级
	//定时器0中断
	Timer0Init();
	TF0 = 0;		
	TR0 = 1;		
	ET0 = 1;			//开启定时器中断
	//定时器1
    TH1 = 0xff;
    TL1 = 0x03;
    ET1 = 1;
    TR1 = 0;						//关闭状态，当播放音乐时开启
    EA=1;
	//485初始化
    M485_TRN = 0 ;      //接收
    P_SW2 |= 0x01 ;     //切换串口2管脚：P4.6,P4.7
	Uart2Init();
	isSend = 1 ;	//在发
	IE2 |= 0x01 ;       //开串行口2中断
    IP2 |= 0x01 ;       //串行口中断为高优先级
    EA = 1 ;            
	
	I2C_SCL = 1;
	delay();
    I2C_SDA = 1;
	delay();
	
}

void I2C_Start()        //开始
{
    I2C_SDA = 1;
    delay();
    I2C_SCL = 1;
    delay();
    I2C_SDA = 0;
    delay();
}
void I2C_Stop()         //停止
{
    I2C_SDA = 0;
    delay();
    I2C_SCL = 1;
    delay();
    I2C_SDA = 1;
    delay();
}
void I2C_Response()      //回复应答
{
    unsigned char i = 0;
    I2C_SCL = 1;
    delay();
    while(I2C_SDA==1&&(i<255)) //表示若在一段时间内没有收到从器件的应答则
        i++;                //主器件默认从期间已经收到数据而不再等待应答信号。
    I2C_SCL=0;
    delay();
}
void I2C_SendByte( unsigned char Data ) //写一个字节
{
    unsigned char i, temp;
    temp = Data;
    for( i = 0; i < 8; i++ )
    {
        temp = temp << 1;
        I2C_SCL = 0;
        delay();
        I2C_SDA = CY;
        delay();
        I2C_SCL = 1;
        delay();
    }
    I2C_SCL = 0;
    delay();
    I2C_SDA = 1;
    delay();
}
unsigned char I2C_ReceiveByte()            //读一个字节
{
    unsigned char i, k;
    I2C_SCL = 0;
    delay();
    I2C_SDA = 1;
    delay();
    for( i = 0; i < 8; i++ )
    {
        I2C_SCL = 1;
        delay();
        k = ( k << 1 ) | I2C_SDA;
        delay();
        I2C_SCL = 0;
        delay();
    }
    delay();
    return k;
}
void WriteByte( unsigned char Addr, unsigned char Data ) //写入一个数据
{
    I2C_Start();
    I2C_SendByte( 0xA0 );
    I2C_Response();
    I2C_SendByte( Addr );
    I2C_Response();
    I2C_SendByte( Data );
    I2C_Response();
    I2C_Stop();
}
unsigned char ReadByte( unsigned char Addr )      //读一个字节数据
{
    unsigned char Data;
    I2C_Start();
    I2C_SendByte( 0xA0 );
    I2C_Response();
    I2C_SendByte( Addr );
    I2C_Response();
    I2C_Start();
    I2C_SendByte( 0xA0|0x01 );
    I2C_Response();
    Data = I2C_ReceiveByte();
    I2C_Stop();
    return Data;
}



unsigned char GetADC()	//获取AD值
{
    unsigned char tmp;
    ADC_CONTR=ADC_POWER|ADC_START|cstAdcSpeed90|cstAdcChs17;//没有将ADC_FLAG置1，用于判断A/D是否结束
    _nop_();	_nop_();	_nop_();	_nop_();
    while(!(ADC_CONTR&ADC_FLAG)); //等待直到A/D转换结束
    ADC_CONTR&=~ADC_FLAG;           //ADC_FLAG软件清0
    tmp=ADC_RES;                 //获取AD的值
    return tmp;
}

unsigned char KeyCheck()		//获取导航按键值
{
    unsigned char key;
    key=GetADC();     //获取AD的值
    if(key!=255)    //有按键按下时
    {
        Delay(5);
        key=GetADC();
        if(key!=255)            //消抖
        {
            key=key&0xE0;       //获取高3位，其他位清零
            key=_cror_(key,5);  //循环右移5位 获取A/D转换高三位值，减小误差
            return key;
        }
    }
    return 0x07;        //没有按键按下时返回值0x07
}

void Key_Process()	//导航按键处理
{
    unsigned char KeyCurrent;  //导航按键当前的状态
    unsigned char KeyPast;     //导航按键前一个状态

    KeyCurrent = KeyCheck();    //获取当前ADC值
    if(KeyCurrent != 0x07 )       //导航按键是否被按下 不等于0x07表示有按下
    {
        KeyPast=KeyCurrent;
        while(KeyCurrent!=0x07)        //等待导航按键松开
            KeyCurrent=KeyCheck();

        switch(KeyPast)
        {
            case 0x05:                     //上键：显示的数字加1
				if(DigSelectState==0)	//左
				{
					if(LSegSelectState==9)
					{
						LSegSelectState=0;
					}
					else
						LSegSelectState++;
				}
				if(DigSelectState==1)	//右
				{
					if(RSegSelectState==9)
					{
						RSegSelectState=0;
					}
					else
						RSegSelectState++;
				}
                break;
				
            case 0x02:                     //下键：显示的数字减1
                if(DigSelectState==0)	//左
				{
					if(LSegSelectState==0)
					{
						LSegSelectState=9;
					}
					else
						LSegSelectState--;
				}
				if(DigSelectState==1)	//右
				{
					if(RSegSelectState==0)
					{
						RSegSelectState=9;
					}
					else
						RSegSelectState--;
				}
                break;
				
			case 0x01:                     //右键：右移
				if(DigSelectState==1)	//右数
				{
                    DigSelectState=0;
				}
                else					//左数
				{
                    DigSelectState=1;
				}
				break;
			
			case 0x04:                     //左键：左移
				if(DigSelectState==1)	//右数
				{
                    DigSelectState=0;
				}
                else					//左数
				{
                    DigSelectState=1;
				}
				break;
				
			case 0x00:                     //Key3：存储数据
// 				LSegSelectState++;
// 				WriteDataL=LSegSelectState;
// 				WriteDataR=RSegSelectState;
				WriteData=LSegSelectState*16+RSegSelectState;
				WriteByte(WriteAddr,WriteData);
				guessflag=1;
			
			case 0x03:				//向里：切换01
				Start=1;
				SendData++;
				//LSegSelectState++;
				if(SendData==3)		//1为大，0为小,2相等
					SendData=0;
				
				break;
				
        }
    }

    Delay(100);
}

// void displaynum(unsigned char location, number)
// {	
// 	P0=0;
// 	P2=DigSelect[location];
// 	P0=SegSelect[number];
// 	//Delay(1);
// }

void display()
{
	flag++;
	if(flag==8)
		flag=0;
	P0=0x00;
	P2=DigSelect[flag];
	switch(flag)
	{
		case 0:
            P0=SegSelect[LSegSelectState];   break;
        case 1:
            P0=SegSelect[RSegSelectState];   break;
        case 2:
            P0=0x00;   break;
        case 3:
            P0=0x00;   break;
        case 4:
			if(guessflag==1)
				P0=SegSelect[Count];   break;
        case 5:
            P0=0x00;   break;
        case 6:
            P0=0x00;   break;
        default:
            if(guessflag==1)	
				P0=SegSelect[SendData];   break;
	}
}

void displayScore()
{
	flag++;
	if(flag==8)
		flag=0;
	P0=0x00;
	P2=DigSelect[flag];
	switch(flag)
	{
		case 0:
            P0=0x6d;   break;
        case 1:
            P0=0x39;   break;
        case 2:
            P0=0x3f;   break;
        case 3:
            P0=0x7b;   break;
        case 4:
			P0=0x79;   break;
        case 5:
            P0=0x00;   break;
        case 6:
            P0=0x00;   break;
        default:
            P0=SegSelect[7-Count];   break;
	}
}

void displayStart()
{
	startFlag++;
	if(startFlag==8)
	{
		startFlag=0;
		startCount++;
	}
	if(startCount==100)
	{
		startCount=0;
		Dig1Tmp++;     //让从左到右各个数码管上的数字都加一
        Dig2Tmp++;
        Dig3Tmp++;
        Dig4Tmp++;
        Dig5Tmp++;
        Dig6Tmp++;
        Dig7Tmp++;
        Dig8Tmp++;
	}
	P0=0;                       //让数码管显示更加好，不受上一次P0赋的值的影响
    P2=DigSelect[startFlag];   //位选
    switch(startFlag)           //每次中断显示一个数码管来显示
    {
        case 0:
            P0=arrRoll[Dig1Tmp%14];
            break;//从左到右，第一个数码管显示
        case 1:
            P0=arrRoll[Dig2Tmp%14];
            break;//从左到右，第二个数码管显示
        case 2:
            P0=arrRoll[Dig3Tmp%14];
            break;//从左到右，第三个数码管显示
        case 3:
            P0=arrRoll[Dig4Tmp%14];
            break;//从左到右，第四个数码管显示
        case 4:
            P0=arrRoll[Dig5Tmp%14];
            break;//从左到右，第五个数码管显示
        case 5:
            P0=arrRoll[Dig6Tmp%14];
            break;//从左到右，第六个数码管显示
        case 6:
            P0=arrRoll[Dig7Tmp%14];
            break;//从左到右，第七个数码管显示
        default:
            P0=arrRoll[Dig8Tmp%14];
            break;//从左到右，第八个数码管显示
    }
// 	if(Dig1Tmp==24)
// 		Start=1;
}

void displayEnd()
{
	endFlag++;
	if(endFlag==8)
	{
		endFlag=0;
		endCount++;
	}
	if(endCount==100)
	{
		endCount=0;
		endDig1Tmp++;     //让从左到右各个数码管上的数字都加一
        endDig2Tmp++;
        endDig3Tmp++;
        endDig4Tmp++;
        endDig5Tmp++;
        endDig6Tmp++;
        endDig7Tmp++;
        endDig8Tmp++;
	}
	P0=0;                       //让数码管显示更加好，不受上一次P0赋的值的影响
    P2=DigSelect[endFlag];   //位选
    switch(endFlag)           //每次中断显示一个数码管来显示
    {
        case 0:
            P0=arrRollsuccess[endDig1Tmp%9];
            break;//从左到右，第一个数码管显示
        case 1:
            P0=arrRollsuccess[endDig2Tmp%9];
            break;//从左到右，第二个数码管显示
        case 2:
            P0=arrRollsuccess[endDig3Tmp%9];
            break;//从左到右，第三个数码管显示
        case 3:
            P0=arrRollsuccess[endDig4Tmp%9];
            break;//从左到右，第四个数码管显示
        case 4:
            P0=arrRollsuccess[endDig5Tmp%9];
            break;//从左到右，第五个数码管显示
        case 5:
            P0=arrRollsuccess[endDig6Tmp%9];
            break;//从左到右，第六个数码管显示
        case 6:
            P0=arrRollsuccess[endDig7Tmp%9];
            break;//从左到右，第七个数码管显示
        default:
            P0=arrRollsuccess[endDig8Tmp%9];
            break;//从左到右，第八个数码管显示
    }
	if(endDig1Tmp==7)
 		ScoreFlag=1;
}

void displayFail()
{
	endFlag++;
	if(endFlag==8)
	{
		endFlag=0;
		endCount++;
	}
	if(endCount==100)
	{
		endCount=0;
		endDig1Tmp++;     //让从左到右各个数码管上的数字都加一
        endDig2Tmp++;
        endDig3Tmp++;
        endDig4Tmp++;
        endDig5Tmp++;
        endDig6Tmp++;
        endDig7Tmp++;
        endDig8Tmp++;
	}
	P0=0;                       //让数码管显示更加好，不受上一次P0赋的值的影响
    P2=DigSelect[endFlag];   //位选
    switch(endFlag)           //每次中断显示一个数码管来显示
    {
        case 0:
            P0=arrRollfail[endDig1Tmp%6];
            break;//从左到右，第一个数码管显示
        case 1:
            P0=arrRollfail[endDig2Tmp%6];
            break;//从左到右，第二个数码管显示
        case 2:
            P0=arrRollfail[endDig3Tmp%6];
            break;//从左到右，第三个数码管显示
        case 3:
            P0=arrRollfail[endDig4Tmp%6];
            break;//从左到右，第四个数码管显示
        case 4:
            P0=arrRollfail[endDig5Tmp%6];
            break;//从左到右，第五个数码管显示
        case 5:
            P0=arrRollfail[endDig6Tmp%6];
            break;//从左到右，第六个数码管显示
        case 6:
            P0=arrRollfail[endDig7Tmp%6];
            break;//从左到右，第七个数码管显示
        default:
            P0=arrRollfail[endDig8Tmp%6];
            break;//从左到右，第八个数码管显示
    }
}

void Timer0_Routine() interrupt 1
{
	TL0 = 0x18;		
	TH0 = 0xFC;		
	T0count++;
	
	if(T0count>=10)
	{
		T0count=0;
		
// 		if(Key2==0)    //Key2按下，存储数据显示在数码管上
// 		{
// 			Delay(5);
//             if(Key2==0)
//             {
//                 while(!Key2);
// 				ReadData=ReadByte(WriteAddr);
// 				//用LRSegSelectState读取的话，会改变当前正在输的值
// 				LSegSelectState=ReadData/10;
// 				RSegSelectState=ReadData%10;
// 				displaynum(0,LSegSelectState);
// 				Delay(10);
// 				displaynum(1,RSegSelectState);
//             }
// 		}
// 		else
// 		{
// 			displaynum(0,LSegSelectState);
// 			Delay(10);
// 			displaynum(1,RSegSelectState);
// 		}	
	}
	if(!Start)
	{
		displayStart();
	}
	if(Start==1&&End==0)
	{
		display();
	}
	if(End==1)
	{
		if(Count>7)
		{
			displayFail();
		}
		else
		{
			if(ScoreFlag==0)
				displayEnd();
			else
				displayScore();
		}
	}
}

// void Timer1_Routine() interrupt 3
// {
//     if( BeepFlag )
//     {
//         Beep = ~Beep;            //产生方波使得蜂鸣器发声
//     }
//     else
//     {
//         Beep = 0;                  //停止发声，并将Beep端口置于低电平
//     }
// }

void Uart2_Routine() interrupt 8 using 1
{
	if(S2RI&S2CON)				//S2CON寄存器不可位寻址
	{
		GetData=S2BUF;
		LSegSelectState=GetData/16;
		RSegSelectState=GetData%16;		//显示读过来的数字
		S2CON&=~S2RI;					//清0
		//countFlag1=1;
		//Count++;
	}
	if(S2TI&S2CON)
	{
		isSend = 0 ;            	//清除发送信号
		S2CON&=~S2TI;					//清0
		//countFlag2=1;
		//Count++;
	}
}
void Int0_Routine() interrupt 0
{
	M485_TRN = 1;		//MAX485使能引脚为1时发送
	S2BUF = SendData;
	while(isSend);	//如果在发送就一直发
	isSend = 1;
	alreadySendData=SendData;
	M485_TRN=0;
}

void main()
{
	Init();
	P0=0x00;
	//WriteAddr=0x00;		// 只用0x00地址记录
	while(1)
	{
		if(alreadySendData==2||Count>7)		//7次猜不中结束
		{
			End=1;
		}
		else
		{
			Key_Process();         //获取按键按下情况
			ReadData=ReadByte(WriteAddr);
			
			if(Key1==0)    //ADC中用K3   
			{
				Delay(5);
				if(Key1==0)
				{
					while(!Key1);
					Buzzer_Time(100);
					Count++;
					Delay(5);
				}
			}
			
			if(Key2==0)    //Key2按下，存储数据显示在数码管上
			{
				Delay(5);
				if(Key2==0)
				{
					while(!Key2);
					//用LRSegSelectState读取的话，会改变当前正在输的值
					LSegSelectState=ReadData/16;
					RSegSelectState=ReadData%16;
					Delay(5);
				}
			}
		}
	}
}
