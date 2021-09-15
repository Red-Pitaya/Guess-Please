#include <STC15F2K60S2.H>
#include <intrins.h>
#define S2RI  0x01           //�����ж������־λ
#define S2TI  0x02           //�����ж������־λ
#define ADC_POWER 0X80     //ADC��Դ����
#define ADC_FLAG 0X10        //��A/Dת����ɺ�ADC_FLAGҪ�������
#define ADC_START 0X08       //��A/Dת����ɺ�ADC_START���Զ����㣬����Ҫ��ʼ��һ��ת��������Ҫ��λ
#define cstAdcSpeed90 0X60   /*ADCת���ٶ� 90��ʱ������ת��һ��*/
#define cstAdcChs17 0X07     /*ѡ��P1.7��ΪA/D����*/
#define FOSC 11059200L     //����Ƶ��
#define BAUD 9600         //���ڲ�����

sbit Key1=P3^2;           //��������
sbit Key2=P3^3;           //��ʾ�洢����
sbit Key3=P1^7;           //�洢������0x00
sbit Sel0=P2^0;
sbit Sel1=P2^1;
sbit Sel2=P2^2;
sbit Buzzer=P3^4;			  //������
sbit I2C_SDA=P4^0;      //I2C���ߵ�������
sbit I2C_SCL=P5^5;      //I2C���ߵ�ʱ����
sbit M485_TRN=P3^7;   		//MAX485ʹ�����ţ�1���ͣ�0����
bit isSend;                 	//Ϊ1ʱ�������ݣ�Ϊ0ʱֹͣ
bit guessflag;		//��ʼ�²�flag
bit Start;			//��ʼ��־
bit End;			//������־
bit BeepFlag;		//
bit ScoreFlag;		//�������

unsigned int T0count;
unsigned int Count;		//��¼�²���ٴ�
unsigned char GetData;          //���յ�����
unsigned char SendData;         //���͵�����
unsigned char alreadySendData;		//�ѷ��͵�����
unsigned char LSegSelectState;         //������ѡ
unsigned char RSegSelectState;         //������ѡ
unsigned char DigSelectState;         //λѡ��־
unsigned char SegSelect[]={0x3f, 0x06, 0x5b, 0x4f, 0x66,0x6d, 0x7d, 0x07, 0x7f, 0x6f,0x77, 0x7c, 0x39, 0x5e, 0x79,0x71, 0x40, 0x00};//��ѡ0-f
unsigned char DigSelect[]={0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07};
unsigned char arrRoll[]={0x3d,0x3e,0x79,0x6d,0x6d,0x00,0x73,0x38,0x79,0x77,0x6d,0x79,0x00,0x00};//λѡ
unsigned char arrRollsuccess[]={0x6d,0x3e,0x39,0x39,0x79,0x6d,0x6d,0x00,0x00};
unsigned char arrRollfail[]={0x71,0x77,0x30,0x38,0x00,0x00};
unsigned char WriteAddr;			//д���ַ
unsigned char WriteData;
unsigned char WriteDataL;			//д������
unsigned char WriteDataR;
unsigned char ReadData;				//��������
unsigned char flag;				//�����ɨ��
unsigned char startFlag;		//startλѡ
unsigned char startCount;		//start��Ƶ
unsigned char Dig1Tmp;
unsigned char Dig2Tmp;
unsigned char Dig3Tmp;
unsigned char Dig4Tmp;
unsigned char Dig5Tmp;
unsigned char Dig6Tmp;
unsigned char Dig7Tmp;
unsigned char Dig8Tmp;
unsigned char endFlag;		//endλѡ
unsigned char endCount;		//end��Ƶ
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

void delay()        //��ʱ4us
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
	S2CON = 0x10 ;      //������У��λ�������п�2����
	AUXR |= 0x14 ;      //T2Ϊ1Tģʽ����������ʱ��2
	T2L=(65536-(FOSC/4/BAUD));
	T2H=(65536-(FOSC/4/BAUD))>>8;
}

void Timer0Init(void)		//1ms@12.000MHz
{	
	AUXR &= 0x7F;		
	TMOD &= 0xF0;             //��ʱ��0����ʽ1
	TMOD |= 0x01;		//???????
	TH0=(65535-1000)/256; //��ʱ��0�ĸ߰�λ����
    TL0=(65535-1000)%256; //��ʱ��0�ĵͰ�λ����
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
    P1M1 = 0x00 ;       //P1��P3׼˫���
	
	P23=0;				
	Sel0 = 1 ;
    Sel1 = 1 ;
    Sel2 = 1 ;           //ѡ��ڰ�λ�������ʾ
	P0 = 0;
	
	DigSelectState = 0;
    LSegSelectState = 0;
	RSegSelectState = 0;
    SendData = 1 ;
	WriteAddr = 0;      //д���ַ��ʼ��
	WriteData=0;
    WriteDataL = 0;      //д�����ݳ�ʼ��
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
	
	//ADC��ʼ��
	P1ASF = 0x80;       //P1.7��Ϊģ�⹦��A/Dʹ��
    ADC_RES = 0;        //ת���������
    ADC_CONTR = 0x8F;   //ADC_POWER = 1
    CLK_DIV = 0X00;     //ADRJ = 0    ADC_RES��Ÿ߰�λ���
	//�ⲿ�ж�0
    IT0 = 1 ;           //0�½���
	//IT1 = 1;
    EX0 = 1 ;           //�����ⲿ�ж�0
    PX0 = 0 ;           //�ⲿ�ж�Ϊ�����ȼ�
	//��ʱ��0�ж�
	Timer0Init();
	TF0 = 0;		
	TR0 = 1;		
	ET0 = 1;			//������ʱ���ж�
	//��ʱ��1
    TH1 = 0xff;
    TL1 = 0x03;
    ET1 = 1;
    TR1 = 0;						//�ر�״̬������������ʱ����
    EA=1;
	//485��ʼ��
    M485_TRN = 0 ;      //����
    P_SW2 |= 0x01 ;     //�л�����2�ܽţ�P4.6,P4.7
	Uart2Init();
	isSend = 1 ;	//�ڷ�
	IE2 |= 0x01 ;       //�����п�2�ж�
    IP2 |= 0x01 ;       //���п��ж�Ϊ�����ȼ�
    EA = 1 ;            
	
	I2C_SCL = 1;
	delay();
    I2C_SDA = 1;
	delay();
	
}

void I2C_Start()        //��ʼ
{
    I2C_SDA = 1;
    delay();
    I2C_SCL = 1;
    delay();
    I2C_SDA = 0;
    delay();
}
void I2C_Stop()         //ֹͣ
{
    I2C_SDA = 0;
    delay();
    I2C_SCL = 1;
    delay();
    I2C_SDA = 1;
    delay();
}
void I2C_Response()      //�ظ�Ӧ��
{
    unsigned char i = 0;
    I2C_SCL = 1;
    delay();
    while(I2C_SDA==1&&(i<255)) //��ʾ����һ��ʱ����û���յ���������Ӧ����
        i++;                //������Ĭ�ϴ��ڼ��Ѿ��յ����ݶ����ٵȴ�Ӧ���źš�
    I2C_SCL=0;
    delay();
}
void I2C_SendByte( unsigned char Data ) //дһ���ֽ�
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
unsigned char I2C_ReceiveByte()            //��һ���ֽ�
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
void WriteByte( unsigned char Addr, unsigned char Data ) //д��һ������
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
unsigned char ReadByte( unsigned char Addr )      //��һ���ֽ�����
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



unsigned char GetADC()	//��ȡADֵ
{
    unsigned char tmp;
    ADC_CONTR=ADC_POWER|ADC_START|cstAdcSpeed90|cstAdcChs17;//û�н�ADC_FLAG��1�������ж�A/D�Ƿ����
    _nop_();	_nop_();	_nop_();	_nop_();
    while(!(ADC_CONTR&ADC_FLAG)); //�ȴ�ֱ��A/Dת������
    ADC_CONTR&=~ADC_FLAG;           //ADC_FLAG�����0
    tmp=ADC_RES;                 //��ȡAD��ֵ
    return tmp;
}

unsigned char KeyCheck()		//��ȡ��������ֵ
{
    unsigned char key;
    key=GetADC();     //��ȡAD��ֵ
    if(key!=255)    //�а�������ʱ
    {
        Delay(5);
        key=GetADC();
        if(key!=255)            //����
        {
            key=key&0xE0;       //��ȡ��3λ������λ����
            key=_cror_(key,5);  //ѭ������5λ ��ȡA/Dת������λֵ����С���
            return key;
        }
    }
    return 0x07;        //û�а�������ʱ����ֵ0x07
}

void Key_Process()	//������������
{
    unsigned char KeyCurrent;  //����������ǰ��״̬
    unsigned char KeyPast;     //��������ǰһ��״̬

    KeyCurrent = KeyCheck();    //��ȡ��ǰADCֵ
    if(KeyCurrent != 0x07 )       //���������Ƿ񱻰��� ������0x07��ʾ�а���
    {
        KeyPast=KeyCurrent;
        while(KeyCurrent!=0x07)        //�ȴ����������ɿ�
            KeyCurrent=KeyCheck();

        switch(KeyPast)
        {
            case 0x05:                     //�ϼ�����ʾ�����ּ�1
				if(DigSelectState==0)	//��
				{
					if(LSegSelectState==9)
					{
						LSegSelectState=0;
					}
					else
						LSegSelectState++;
				}
				if(DigSelectState==1)	//��
				{
					if(RSegSelectState==9)
					{
						RSegSelectState=0;
					}
					else
						RSegSelectState++;
				}
                break;
				
            case 0x02:                     //�¼�����ʾ�����ּ�1
                if(DigSelectState==0)	//��
				{
					if(LSegSelectState==0)
					{
						LSegSelectState=9;
					}
					else
						LSegSelectState--;
				}
				if(DigSelectState==1)	//��
				{
					if(RSegSelectState==0)
					{
						RSegSelectState=9;
					}
					else
						RSegSelectState--;
				}
                break;
				
			case 0x01:                     //�Ҽ�������
				if(DigSelectState==1)	//����
				{
                    DigSelectState=0;
				}
                else					//����
				{
                    DigSelectState=1;
				}
				break;
			
			case 0x04:                     //���������
				if(DigSelectState==1)	//����
				{
                    DigSelectState=0;
				}
                else					//����
				{
                    DigSelectState=1;
				}
				break;
				
			case 0x00:                     //Key3���洢����
// 				LSegSelectState++;
// 				WriteDataL=LSegSelectState;
// 				WriteDataR=RSegSelectState;
				WriteData=LSegSelectState*16+RSegSelectState;
				WriteByte(WriteAddr,WriteData);
				guessflag=1;
			
			case 0x03:				//����л�01
				Start=1;
				SendData++;
				//LSegSelectState++;
				if(SendData==3)		//1Ϊ��0ΪС,2���
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
		Dig1Tmp++;     //�ô����Ҹ���������ϵ����ֶ���һ
        Dig2Tmp++;
        Dig3Tmp++;
        Dig4Tmp++;
        Dig5Tmp++;
        Dig6Tmp++;
        Dig7Tmp++;
        Dig8Tmp++;
	}
	P0=0;                       //���������ʾ���Ӻã�������һ��P0����ֵ��Ӱ��
    P2=DigSelect[startFlag];   //λѡ
    switch(startFlag)           //ÿ���ж���ʾһ�����������ʾ
    {
        case 0:
            P0=arrRoll[Dig1Tmp%14];
            break;//�����ң���һ���������ʾ
        case 1:
            P0=arrRoll[Dig2Tmp%14];
            break;//�����ң��ڶ����������ʾ
        case 2:
            P0=arrRoll[Dig3Tmp%14];
            break;//�����ң��������������ʾ
        case 3:
            P0=arrRoll[Dig4Tmp%14];
            break;//�����ң����ĸ��������ʾ
        case 4:
            P0=arrRoll[Dig5Tmp%14];
            break;//�����ң�������������ʾ
        case 5:
            P0=arrRoll[Dig6Tmp%14];
            break;//�����ң��������������ʾ
        case 6:
            P0=arrRoll[Dig7Tmp%14];
            break;//�����ң����߸��������ʾ
        default:
            P0=arrRoll[Dig8Tmp%14];
            break;//�����ң��ڰ˸��������ʾ
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
		endDig1Tmp++;     //�ô����Ҹ���������ϵ����ֶ���һ
        endDig2Tmp++;
        endDig3Tmp++;
        endDig4Tmp++;
        endDig5Tmp++;
        endDig6Tmp++;
        endDig7Tmp++;
        endDig8Tmp++;
	}
	P0=0;                       //���������ʾ���Ӻã�������һ��P0����ֵ��Ӱ��
    P2=DigSelect[endFlag];   //λѡ
    switch(endFlag)           //ÿ���ж���ʾһ�����������ʾ
    {
        case 0:
            P0=arrRollsuccess[endDig1Tmp%9];
            break;//�����ң���һ���������ʾ
        case 1:
            P0=arrRollsuccess[endDig2Tmp%9];
            break;//�����ң��ڶ����������ʾ
        case 2:
            P0=arrRollsuccess[endDig3Tmp%9];
            break;//�����ң��������������ʾ
        case 3:
            P0=arrRollsuccess[endDig4Tmp%9];
            break;//�����ң����ĸ��������ʾ
        case 4:
            P0=arrRollsuccess[endDig5Tmp%9];
            break;//�����ң�������������ʾ
        case 5:
            P0=arrRollsuccess[endDig6Tmp%9];
            break;//�����ң��������������ʾ
        case 6:
            P0=arrRollsuccess[endDig7Tmp%9];
            break;//�����ң����߸��������ʾ
        default:
            P0=arrRollsuccess[endDig8Tmp%9];
            break;//�����ң��ڰ˸��������ʾ
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
		endDig1Tmp++;     //�ô����Ҹ���������ϵ����ֶ���һ
        endDig2Tmp++;
        endDig3Tmp++;
        endDig4Tmp++;
        endDig5Tmp++;
        endDig6Tmp++;
        endDig7Tmp++;
        endDig8Tmp++;
	}
	P0=0;                       //���������ʾ���Ӻã�������һ��P0����ֵ��Ӱ��
    P2=DigSelect[endFlag];   //λѡ
    switch(endFlag)           //ÿ���ж���ʾһ�����������ʾ
    {
        case 0:
            P0=arrRollfail[endDig1Tmp%6];
            break;//�����ң���һ���������ʾ
        case 1:
            P0=arrRollfail[endDig2Tmp%6];
            break;//�����ң��ڶ����������ʾ
        case 2:
            P0=arrRollfail[endDig3Tmp%6];
            break;//�����ң��������������ʾ
        case 3:
            P0=arrRollfail[endDig4Tmp%6];
            break;//�����ң����ĸ��������ʾ
        case 4:
            P0=arrRollfail[endDig5Tmp%6];
            break;//�����ң�������������ʾ
        case 5:
            P0=arrRollfail[endDig6Tmp%6];
            break;//�����ң��������������ʾ
        case 6:
            P0=arrRollfail[endDig7Tmp%6];
            break;//�����ң����߸��������ʾ
        default:
            P0=arrRollfail[endDig8Tmp%6];
            break;//�����ң��ڰ˸��������ʾ
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
		
// 		if(Key2==0)    //Key2���£��洢������ʾ���������
// 		{
// 			Delay(5);
//             if(Key2==0)
//             {
//                 while(!Key2);
// 				ReadData=ReadByte(WriteAddr);
// 				//��LRSegSelectState��ȡ�Ļ�����ı䵱ǰ�������ֵ
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
//         Beep = ~Beep;            //��������ʹ�÷���������
//     }
//     else
//     {
//         Beep = 0;                  //ֹͣ����������Beep�˿����ڵ͵�ƽ
//     }
// }

void Uart2_Routine() interrupt 8 using 1
{
	if(S2RI&S2CON)				//S2CON�Ĵ�������λѰַ
	{
		GetData=S2BUF;
		LSegSelectState=GetData/16;
		RSegSelectState=GetData%16;		//��ʾ������������
		S2CON&=~S2RI;					//��0
		//countFlag1=1;
		//Count++;
	}
	if(S2TI&S2CON)
	{
		isSend = 0 ;            	//��������ź�
		S2CON&=~S2TI;					//��0
		//countFlag2=1;
		//Count++;
	}
}
void Int0_Routine() interrupt 0
{
	M485_TRN = 1;		//MAX485ʹ������Ϊ1ʱ����
	S2BUF = SendData;
	while(isSend);	//����ڷ��;�һֱ��
	isSend = 1;
	alreadySendData=SendData;
	M485_TRN=0;
}

void main()
{
	Init();
	P0=0x00;
	//WriteAddr=0x00;		// ֻ��0x00��ַ��¼
	while(1)
	{
		if(alreadySendData==2||Count>7)		//7�β²��н���
		{
			End=1;
		}
		else
		{
			Key_Process();         //��ȡ�����������
			ReadData=ReadByte(WriteAddr);
			
			if(Key1==0)    //ADC����K3   
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
			
			if(Key2==0)    //Key2���£��洢������ʾ���������
			{
				Delay(5);
				if(Key2==0)
				{
					while(!Key2);
					//��LRSegSelectState��ȡ�Ļ�����ı䵱ǰ�������ֵ
					LSegSelectState=ReadData/16;
					RSegSelectState=ReadData%16;
					Delay(5);
				}
			}
		}
	}
}
