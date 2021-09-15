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
sbit Key2=P3^3;           //
sbit Key3=P1^7;           //
sbit Sel0=P2^0;
sbit Sel1=P2^1;
sbit Sel2=P2^2;
sbit Buzzer=P3^4;			  //������
sbit M485_TRN=P3^7;   		//MAX485ʹ�����ţ�1���ͣ�0����
bit isSend;                 	//Ϊ1ʱ�������ݣ�Ϊ0ʱֹͣ
bit Start;			//��ʼ��־
bit End;			//������־
bit BeepFlag;		//
bit ScoreFlag;		//�������
bit GetFlag;

unsigned int T0count;
unsigned int Count;		//��¼�²���ٴ�
unsigned char GetData;          //���յ�����
unsigned char SendData;         //���͵�����
unsigned char LSegSelectState;         //������ѡ
unsigned char RSegSelectState;         //������ѡ
unsigned char DigSelectState;         //λѡ��־
unsigned char SegSelect[]={0x3f, 0x06, 0x5b, 0x4f, 0x66,0x6d, 0x7d, 0x07, 0x7f, 0x6f,0x77, 0x7c, 0x39, 0x5e, 0x79,0x71, 0x40, 0x00};//��ѡ0-f
unsigned char DigSelect[]={0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07};	//λѡ
unsigned char arrRoll[]={0x3d,0x3e,0x79,0x6d,0x6d,0x00,0x73,0x38,0x79,0x77,0x6d,0x79,0x00,0x00};
unsigned char arrRollsuccess[]={0x6d,0x3e,0x39,0x39,0x79,0x6d,0x6d,0x00,0x00};
unsigned char arrRollfail[]={0x71,0x77,0x30,0x38,0x00,0x00};
unsigned char flag;				//�����ɨ��
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
unsigned char TimerH,TimerL;   //���嶨ʱ������װֵ
unsigned char code Musicsmall[] =     //���ִ��� ��ʽΪ: ����, ����
{0x31,0x08,0x25,0x08,0x21,0x08,0x00};
unsigned char code Musicbig[] =
{0x21,0x08,0x25,0x08,0x31,0x08,0x00};
unsigned char code arrMusicToTimerNum[] =  
{
    //����������Ϊ���������ڶ�ʱ���е���װֵ����һ���Ǹ�λ���ڶ����ǵ�λ
    0xf8, 0x8c,   //�Ͱ˶ȣ���1
    0xf9, 0x5b,
    0xfa, 0x15,   //��3
    0xfa, 0x67,
    0xfb, 0x04,   //��5
    0xfb, 0x90,
    0xfc, 0x0c,   //��7
    0xfc, 0x44,   //����C��
    0xfc, 0xac,   //��2
    0xfd, 0x09,
    0xfd, 0x34,   //��4
    0xfd, 0x82,
    0xfd, 0xc8,   //��6
    0xfe, 0x06,
    0xfe, 0x22,   //�߰˶ȣ���1
    0xfe, 0x56,
    0xfe, 0x6e,   //��3
    0xfe, 0x9a,
    0xfe, 0xc1,   //��5
    0xfe, 0xe4,
    0xff, 0x03    //��7
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
    unsigned char ucBase, ucOffset, ucPosition;     //����������������λ��
    ucBase = tem / 16;            //��4λ������ֵ,��ַ
    ucOffset = tem % 16;          //��4λ��������ƫ����
    if( ucBase == 1 )              //������ֵΪ1ʱ�����ǵͰ˶ȣ���ַΪ0
        ucBase = 0;
    else if( ucBase == 2 )          //������ֵΪ2ʱ�������а˶ȣ���ַΪ14
        ucBase = 14;
    else if( ucBase == 3 )          //������ֵΪ3ʱ�����Ǹ߰˶ȣ���ַΪ28
        ucBase = 28;
    //ͨ����ַ����ƫ���������ɶ�λ��������arrMusicToTimerNum�����е�λ��
	ucPosition = ucBase + ( ucOffset - 1 ) * 2; 
    return ucPosition;            //������һ��λ��ֵ
}

void PlayMusicsmall()
{
    unsigned char ucNoteTmp, ucRhythmTmp, tem; // ucNoteTmpΪ������ucRhythmTmpΪ����
    unsigned char i = 0;
	unsigned char aaa=0;
    for(aaa=0;aaa<3;aaa++)
    {
        ucNoteTmp = Musicsmall[i];    //�������������,��ʱ1��,�ص���ʼ����һ��
            //ȡ����ǰ������arrMusicToTimerNum�����е�λ��ֵ
			tem = GetPosition( Musicsmall[i] );              
			//��������Ӧ�ļ�ʱ����װ��ֵ����ucTimerH��ucTimerL
			TimerH = arrMusicToTimerNum[tem];  
            TimerL = arrMusicToTimerNum[tem + 1];
            i++;
            TH1 = TimerH;           //��ucTimerH��ucTimerL�����ʱ��
            TL1 = TimerL;
            ucRhythmTmp = Musicsmall[i];      //ȡ�ý���
            i++;
        TR1 = 1;                          //����ʱ��1
        DelayMs( ucRhythmTmp * 180 );      //�ȴ��������, ͨ��P3^4�������Ƶ
        TR1 = 0;                          //�ض�ʱ��1

    }
}

void PlayMusicbig()
{
    unsigned char ucNoteTmp, ucRhythmTmp, tem; // ucNoteTmpΪ������ucRhythmTmpΪ����
    unsigned char i = 0;
	unsigned char aaa=0;
    for(aaa=0;aaa<3;aaa++)
    {
        ucNoteTmp = Musicbig[i];    //�������������,��ʱ1��,�ص���ʼ����һ��
            //ȡ����ǰ������arrMusicToTimerNum�����е�λ��ֵ
			tem = GetPosition( Musicbig[i] );              
			//��������Ӧ�ļ�ʱ����װ��ֵ����ucTimerH��ucTimerL
			TimerH = arrMusicToTimerNum[tem];  
            TimerL = arrMusicToTimerNum[tem + 1];
            i++;
            TH1 = TimerH;           //��ucTimerH��ucTimerL�����ʱ��
            TL1 = TimerL;
            ucRhythmTmp = Musicbig[i];      //ȡ�ý���
            i++;
        TR1 = 1;                          //����ʱ��1
        DelayMs( ucRhythmTmp * 180 );      //�ȴ��������, ͨ��P3^4�������Ƶ
        TR1 = 0;                          //�ض�ʱ��1

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
	S2CON = 0x10 ;      //������У��λ�������п�2����
	AUXR |= 0x14 ;      //T2Ϊ1Tģʽ����������ʱ��2
	T2L=(65536-(FOSC/4/BAUD));
	T2H=(65536-(FOSC/4/BAUD))>>8;
}

void Timer0Init(void)		//1ms@12.000MHz
{	
	AUXR &= 0x7F;		
	TMOD &= 0xF0;             //��ʱ��0����ʽ1
	TH0=(65535-1000)/256; //��ʱ��0�ĸ߰�λ����
    TL0=(65535-1000)%256; //��ʱ��0�ĵͰ�λ����
}

void Init()
{
	P0M0 = 0xff ;       
    P0M1 = 0x00 ;
    P2M0 = 0x08 ;       
    P2M1 = 0x00 ;
    P3M0 = 0x10 ;		//P3.4����
    P3M1 = 0x00 ;
    P1M0 = 0x00 ;
    P1M1 = 0x00 ;       //P1׼˫���
	
	P23=0;				
	Sel0 = 1 ;
    Sel1 = 1 ;
    Sel2 = 1 ;           //ѡ��ڰ�λ�������ʾ
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
	
	//ADC��ʼ��
	P1ASF = 0x80;       //P1.7��Ϊģ�⹦��A/Dʹ��
    ADC_RES = 0;        //ת���������
    ADC_CONTR = 0x8F;   //ADC_POWER = 1
    CLK_DIV = 0X00;     //ADRJ = 0    ADC_RES��Ÿ߰�λ���
	//�ⲿ�ж�0
    IT0 = 1 ;           //0�½���
	IT1 = 0;			//1������
    EX0 = 1 ;           //�����ⲿ�ж�0
    PX0 = 0 ;           //�ⲿ�ж�Ϊ�����ȼ�
	//��ʱ��0�ж�
	Timer0Init();
	TF0 = 0;		
	TR0 = 1;		
	ET0 = 1;			//������ʱ���ж�
	//��ʱ��1
    TH1 = 0xD8;
    TL1 = 0xEF;
    ET1 = 1;
    TR1 = 0;						//�ر�״̬������������ʱ����
	PT1=0;
    EA=1;
	//485��ʼ��
    M485_TRN = 0 ;      //����
    P_SW2 |= 0x01 ;     //�л�����2�ܽţ�P4.6,P4.7
	Uart2Init();
	isSend = 1 ;	//�ڷ�
	IE2 |= 0x01 ;       //�����п�2�ж�
    IP2 |= 0x01 ;       //���п��ж�Ϊ�����ȼ�
    EA = 1 ;            
	
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
			case 0x03: 				//�����ʼ��Ϸ
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
	if(S2RI&S2CON)				//S2CON�Ĵ�������λѰַ
	{
		GetData=S2BUF;
		//LSegSelectState=GetData/16;
		//RSegSelectState=GetData%16;		//��ʾ������������
		GetFlag=1;

		S2CON&=~S2RI;					//��0
	}
	if(S2TI&S2CON)
	{
		isSend = 0 ;            	//��������ź�
		S2CON&=~S2TI;					//��0
	}
}

void Int0_Routine() interrupt 0
{
	M485_TRN = 1;		//MAX485ʹ������Ϊ1ʱ����
	SendData=LSegSelectState*16+RSegSelectState;
	S2BUF = SendData;
	while(isSend);	//����ڷ��;�һֱ��
	isSend = 1;
	M485_TRN=0;
}

void main()
{
	Init();
	P0=0x00;
	//WriteAddr=0x00;		// ֻ��0x00��ַ��¼
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
			Key_Process();         //��ȡ�����������
			
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
// 			if(Key3==0)    //ADC����K3   
// 			{
// 				Delay(5);
// 				if(Key3==0)
// 				{
// 					while(!Key3);
// 					LSegSelectState++;
// 					WriteDataL=LSegSelectState;
// 					WriteDataR=RSegSelectState;
// 					//WriteByte(WriteAddr,WriteData); //д���ַ0x00��
// 					
// 					Delay(5);
// 				}
// 			}
		}
		
		
	}
}
