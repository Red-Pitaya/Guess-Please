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
sbit Key2=P3^3;           //
sbit Key3=P1^7;           //
sbit Sel0=P2^0;
sbit Sel1=P2^1;
sbit Sel2=P2^2;
sbit Buzzer=P3^4;			  //蜂鸣器
sbit M485_TRN=P3^7;   		//MAX485使能引脚，1发送，0接收
bit isSend;                 	//为1时发送数据，为0时停止
bit Start;			//开始标志
bit End;			//结束标志
bit BeepFlag;		//
bit ScoreFlag;		//分数面板
bit GetFlag;

unsigned int T0count;
unsigned int Count;		//记录猜测多少次
unsigned char GetData;          //接收的数据
unsigned char SendData;         //发送的数据
unsigned char LSegSelectState;         //左数段选
unsigned char RSegSelectState;         //右数段选
unsigned char DigSelectState;         //位选标志
unsigned char SegSelect[]={0x3f, 0x06, 0x5b, 0x4f, 0x66,0x6d, 0x7d, 0x07, 0x7f, 0x6f,0x77, 0x7c, 0x39, 0x5e, 0x79,0x71, 0x40, 0x00};//段选0-f
unsigned char DigSelect[]={0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07};	//位选
unsigned char arrRoll[]={0x3d,0x3e,0x79,0x6d,0x6d,0x00,0x73,0x38,0x79,0x77,0x6d,0x79,0x00,0x00};
unsigned char arrRollsuccess[]={0x6d,0x3e,0x39,0x39,0x79,0x6d,0x6d,0x00,0x00};
unsigned char arrRollfail[]={0x71,0x77,0x30,0x38,0x00,0x00};
unsigned char flag;				//数码管扫描
unsigned char startFlag;
unsigned char startCount;
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
unsigned char TimerH,TimerL;   //定义定时器的重装值
unsigned char code Musicsmall[] =     //音乐代码 格式为: 音符, 节拍
{0x31,0x08,0x25,0x08,0x21,0x08,0x00};
unsigned char code Musicbig[] =
{0x21,0x08,0x25,0x08,0x31,0x08,0x00};
unsigned char code arrMusicToTimerNum[] =  
{
    //此数组数据为各个音符在定时器中的重装值，第一列是高位，第二列是低位
    0xf8, 0x8c,   //低八度，低1
    0xf9, 0x5b,
    0xfa, 0x15,   //低3
    0xfa, 0x67,
    0xfb, 0x04,   //低5
    0xfb, 0x90,
    0xfc, 0x0c,   //低7
    0xfc, 0x44,   //中央C调
    0xfc, 0xac,   //中2
    0xfd, 0x09,
    0xfd, 0x34,   //中4
    0xfd, 0x82,
    0xfd, 0xc8,   //中6
    0xfe, 0x06,
    0xfe, 0x22,   //高八度，高1
    0xfe, 0x56,
    0xfe, 0x6e,   //高3
    0xfe, 0x9a,
    0xfe, 0xc1,   //高5
    0xfe, 0xe4,
    0xff, 0x03    //高7
};

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

void DelayMs( unsigned int xms )
{
    unsigned int i, j;
    for( i = xms; i > 0; i-- )
        for( j = 124; j > 0; j-- );
}

unsigned char GetPosition( unsigned char tem ) 
{
    unsigned char ucBase, ucOffset, ucPosition;     //定义曲调，音符和位置
    ucBase = tem / 16;            //高4位是曲调值,基址
    ucOffset = tem % 16;          //低4位是音符，偏移量
    if( ucBase == 1 )              //当曲调值为1时，即是低八度，基址为0
        ucBase = 0;
    else if( ucBase == 2 )          //当曲调值为2时，即是中八度，基址为14
        ucBase = 14;
    else if( ucBase == 3 )          //当曲调值为3时，即是高八度，基址为28
        ucBase = 28;
    //通过基址加上偏移量，即可定位此音符在arrMusicToTimerNum数组中的位置
	ucPosition = ucBase + ( ucOffset - 1 ) * 2; 
    return ucPosition;            //返回这一个位置值
}

void PlayMusicsmall()
{
    unsigned char ucNoteTmp, ucRhythmTmp, tem; // ucNoteTmp为音符，ucRhythmTmp为节拍
    unsigned char i = 0;
	unsigned char aaa=0;
    for(aaa=0;aaa<3;aaa++)
    {
        ucNoteTmp = Musicsmall[i];    //如果碰到结束符,延时1秒,回到开始再来一遍
            //取出当前音符在arrMusicToTimerNum数组中的位置值
			tem = GetPosition( Musicsmall[i] );              
			//把音符相应的计时器重装载值赋予ucTimerH和ucTimerL
			TimerH = arrMusicToTimerNum[tem];  
            TimerL = arrMusicToTimerNum[tem + 1];
            i++;
            TH1 = TimerH;           //把ucTimerH和ucTimerL赋予计时器
            TL1 = TimerL;
            ucRhythmTmp = Musicsmall[i];      //取得节拍
            i++;
        TR1 = 1;                          //开定时器1
        DelayMs( ucRhythmTmp * 180 );      //等待节拍完成, 通过P3^4口输出音频
        TR1 = 0;                          //关定时器1

    }
}

void PlayMusicbig()
{
    unsigned char ucNoteTmp, ucRhythmTmp, tem; // ucNoteTmp为音符，ucRhythmTmp为节拍
    unsigned char i = 0;
	unsigned char aaa=0;
    for(aaa=0;aaa<3;aaa++)
    {
        ucNoteTmp = Musicbig[i];    //如果碰到结束符,延时1秒,回到开始再来一遍
            //取出当前音符在arrMusicToTimerNum数组中的位置值
			tem = GetPosition( Musicbig[i] );              
			//把音符相应的计时器重装载值赋予ucTimerH和ucTimerL
			TimerH = arrMusicToTimerNum[tem];  
            TimerL = arrMusicToTimerNum[tem + 1];
            i++;
            TH1 = TimerH;           //把ucTimerH和ucTimerL赋予计时器
            TL1 = TimerL;
            ucRhythmTmp = Musicbig[i];      //取得节拍
            i++;
        TR1 = 1;                          //开定时器1
        DelayMs( ucRhythmTmp * 180 );      //等待节拍完成, 通过P3^4口输出音频
        TR1 = 0;                          //关定时器1

    }
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
	TH0=(65535-1000)/256; //定时器0的高八位设置
    TL0=(65535-1000)%256; //定时器0的低八位设置
}

void Init()
{
	P0M0 = 0xff ;       
    P0M1 = 0x00 ;
    P2M0 = 0x08 ;       
    P2M1 = 0x00 ;
    P3M0 = 0x10 ;		//P3.4推挽
    P3M1 = 0x00 ;
    P1M0 = 0x00 ;
    P1M1 = 0x00 ;       //P1准双向口
	
	P23=0;				
	Sel0 = 1 ;
    Sel1 = 1 ;
    Sel2 = 1 ;           //选择第八位数码管显示
	P0 = 0;
	
	DigSelectState = 0;
    LSegSelectState = 0;
	RSegSelectState = 0;
    SendData = 0 ;
	GetData = 0;
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
	Buzzer=0;
	BeepFlag = 0;	
	GetFlag=0;
	
	//ADC初始化
	P1ASF = 0x80;       //P1.7作为模拟功能A/D使用
    ADC_RES = 0;        //转换结果清零
    ADC_CONTR = 0x8F;   //ADC_POWER = 1
    CLK_DIV = 0X00;     //ADRJ = 0    ADC_RES存放高八位结果
	//外部中断0
    IT0 = 1 ;           //0下降沿
	IT1 = 0;			//1上升沿
    EX0 = 1 ;           //允许外部中断0
    PX0 = 0 ;           //外部中断为低优先级
	//定时器0中断
	Timer0Init();
	TF0 = 0;		
	TR0 = 1;		
	ET0 = 1;			//开启定时器中断
	//定时器1
    TH1 = 0xD8;
    TL1 = 0xEF;
    ET1 = 1;
    TR1 = 0;						//关闭状态，当播放音乐时开启
	PT1=0;
    EA=1;
	//485初始化
    M485_TRN = 0 ;      //接收
    P_SW2 |= 0x01 ;     //切换串口2管脚：P4.6,P4.7
	Uart2Init();
	isSend = 1 ;	//在发
	IE2 |= 0x01 ;       //开串行口2中断
    IP2 |= 0x01 ;       //串行口中断为高优先级
    EA = 1 ;            
	
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
			case 0x03: 				//向里：开始游戏
				Start=1;
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
			P0=0x00;   break;
        case 5:
            P0=0x00;   break;
        case 6:
            P0=0x00;   break;
        default:
            P0=SegSelect[GetData];   break;
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

void Timer1_Routine() interrupt 3
{
	TH0 = TimerH;
    TL0 = TimerL;
    Buzzer = ~Buzzer;
}

void Uart2_Routine() interrupt 8 using 1
{
	if(S2RI&S2CON)				//S2CON寄存器不可位寻址
	{
		GetData=S2BUF;
		//LSegSelectState=GetData/16;
		//RSegSelectState=GetData%16;		//显示读过来的数字
		GetFlag=1;

		S2CON&=~S2RI;					//清0
	}
	if(S2TI&S2CON)
	{
		isSend = 0 ;            	//清除发送信号
		S2CON&=~S2TI;					//清0
	}
}

void Int0_Routine() interrupt 0
{
	M485_TRN = 1;		//MAX485使能引脚为1时发送
	SendData=LSegSelectState*16+RSegSelectState;
	S2BUF = SendData;
	while(isSend);	//如果在发送就一直发
	isSend = 1;
	M485_TRN=0;
}

void main()
{
	Init();
	P0=0x00;
	//WriteAddr=0x00;		// 只用0x00地址记录
	while(1)
	{
		if(GetData==0&&GetFlag==1)
		{
			PlayMusicsmall();
			GetFlag=0;
		}
		if(GetData==1&&GetFlag==1)
		{
			PlayMusicbig();
			GetFlag=0;
		}
		if(GetData==2||Count>7)
		{
			End=1;
		}
		else
		{
			Key_Process();         //获取按键按下情况
			
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
// 			if(Key3==0)    //ADC中用K3   
// 			{
// 				Delay(5);
// 				if(Key3==0)
// 				{
// 					while(!Key3);
// 					LSegSelectState++;
// 					WriteDataL=LSegSelectState;
// 					WriteDataR=RSegSelectState;
// 					//WriteByte(WriteAddr,WriteData); //写入地址0x00中
// 					
// 					Delay(5);
// 				}
// 			}
		}
		
		
	}
}
