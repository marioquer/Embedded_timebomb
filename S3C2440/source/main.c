
#include "config.h"
#include <stdlib.h>

#include <stdio.h> 

//=======================================
#include "app_cfg.h"
#include "Printf.h"
#include "ucos_ii.h"

#include "gui.h"
#include "math.h"
#include "GUI_Protected.h"
#include "GUIDEMO.H"
#include "WM.h"
#include "Dialog.h"
#include "LISTBOX.h"
#include "EDIT.h"
#include "SLIDER.h"
#include "FRAMEWIN.h"

//=========================================
OS_STK  MainTaskStk[MainTaskStkLengh];
int number=0,passwordLen=6;
int passwordarray[6]={0,0,0,0,0,0};
int tt;

/*******************************************************************
*
*      Structure containing information for drawing routine
*
*******************************************************************
*/

typedef struct
{
	I16 *aY;
}PARAM;

/*******************************************************************
*
*                    Defines
*
********************************************************************
*/

//#define YSIZE (LCD_YSIZE - 100)
//#define DEG2RAD (3.1415926f / 180)


//条件预编译
#if LCD_BITSPERPIXEL == 1
  #define COLOR_GRAPH0 GUI_WHITE
  #define COLOR_GRAPH1 GUI_WHITE
#else
  #define COLOR_GRAPH0 GUI_GREEN
  #define COLOR_GRAPH1 GUI_YELLOW
#endif


///////////////////////////////////////////////////////////////////
void (*FunP[3][3])(void);
void reset(void);


int defuse=111111,state,timeout,msec;   //state: 0 未设置状态   1 倒数状态   2 爆炸/清除
int dismantled=0;

void addcode(void)
{
	int i;
	if(passwordarray[number]>=9)
		passwordarray[number]=0;
	else
		passwordarray[number]++;

	for(i=0;i<passwordLen;i++){
		GUI_DispCharAt(passwordarray[0]+48,20*i+20,140);
	}
}


//展示剩余秒数
void show(int t)
{
	int fir=t/10;
	int sec=t%10;
	GUI_SetColor(GUI_BLACK);
	GUI_SetBkColor(GUI_WHITE);
	if(fir!=0)
		GUI_DispCharAt('0'+fir,20,60);
	else
    	GUI_DispStringAt("     ",20,60);
    GUI_DispCharAt('0'+sec,40,60);
}

//Setting
void s_up(void)
{
	timeout++;
	if(timeout>99){
		GUI_SetColor(GUI_RED);
		GUI_DispStringAt("It's the maximum value!",20,60);
		GUI_Delay(200);
		GUI_DispCEOL(); // clean this line
		timeout=99;
	}
	show(timeout);
}

void s_down(void)
{
	timeout--;
	if(timeout<0){
		GUI_SetColor(GUI_RED);
		GUI_DispStringAt("It's the minimum value!",20,60);
		GUI_Delay(200);
		GUI_DispCEOL(); // clean this line
		timeout=0;
	}
	show(timeout);
}

void s_arm(void)
{
	GUI_SetColor(GUI_BLACK);
	GUI_SetBkColor(GUI_WHITE);
	GUI_SetFont(&GUI_Font32_ASCII); 
	GUI_DispStringAt("Bomb Counting down...",20,220); 
	GUI_DispStringAt("Please enter the password",20,100);	
	state=1;
}

//Timing
void t_up(void)
{
	addcode();	
}

//切换密码位数
void t_down()
{
	int i;
	if(number!=(passwordLen-1)){
		number++;
	}else{
		number = 0;
	}
	GUI_SetColor(GUI_BLACK);
	GUI_SetBkColor(GUI_WHITE);
	GUI_SetFont(&GUI_Font32_ASCII); 
	// GUI_DispCharAt('*',20*(number+1),140);
	// GUI_DispCharAt(passwordarray[1-number]+'0',(2-number)*20,140); 
	for(i=0;i<passwordLen;i++){
		if(i!=number){
			GUI_DispCharAt(passwordarray[i]+'0',(i+1)*20,140); 
		}else{
			GUI_DispCharAt('*',(i+1)*20,140);
		}
	}
	Uart_Printf("password:%d\n",number);
}

//确认密码
void t_arm(void)
{
	int i;
	int temp = 0, mul = 1; //mul 为倍数
	for(i = passwordLen-1;i>=0;i--){
		temp+=passwordarray[i]*mul;
		mul*=10;
	}

    // int temp=passwordarray[0]*10+passwordarray[1];
    
	if(temp==defuse)
	{
		Uart_Printf("Correct Password\n"); 
		GUI_ClearRect(0,0,LCD_XSIZE,LCD_YSIZE);
		GUI_SetColor(GUI_BLACK);
		GUI_SetBkColor(GUI_WHITE);
		GUI_SetFont(&GUI_Font32_ASCII); 
		GUI_DispStringAt("Bomb cleared!",20,10);
		GUI_DispStringAt("Press any button to retry",20,60);
		state=2;
	}
	else{
		GUI_SetColor(GUI_RED);
		GUI_DispStringAt("Incorrect Password!!",20,180);
		GUI_Delay(200);
		GUI_DispCEOL(); 
		Uart_Printf("Incorrect Password\n"); 
	}
}  

//Get input , return key
U8 Key_Scan( void )
{
	Delay( 80 ) ;
	rGPBDAT &=~(1<<6);
	rGPBDAT |=1<<7;

	if(      (rGPFDAT&(1<< 0)) == 0 )		return 1;
	else if( (rGPFDAT&(1<< 2)) == 0 )		return 3;
	else if( (rGPGDAT&(1<< 3)) == 0 )		return 5;
	else if( (rGPGDAT&(1<<11)) == 0 )		return 7;

	rGPBDAT &=~(1<<7);
	rGPBDAT |=1<<6;
	if(      (rGPFDAT&(1<< 0)) == 0 )		return 2;
	else if( (rGPFDAT&(1<< 2)) == 0 )		return 4;
	else if( (rGPGDAT&(1<< 3)) == 0 )		return 6;
	else if( (rGPGDAT&(1<<11)) == 0 )		return 8;
	else return 0xff;	
}



void explode(void)
{
	GUI_SetColor(GUI_BLACK);
	GUI_SetBkColor(GUI_RED);
	GUI_ClearRect(0,0,LCD_XSIZE,LCD_YSIZE);
	GUI_SetFont(&GUI_Font32_ASCII); 
	GUI_DispStringAt("The bomb has exploded...",20,10);
	GUI_DispStringAt("================================================",20,60);
	GUI_DispStringAt("Press any button to retry",20,110);
	state=2;
}


//根据按键和state调用方法
void Key_ISR()
{
	int key;
	key = Key_Scan();
	while(Key_Scan()==key)			//放置按着不放
		;
	if(state<=2)
	{
		if(key % 2 == 1 && key<7)
			FunP[state][(key-1)/2]();
		if(key==7)
			reset();
		if(rEINTPEND & 1<<11)
			rEINTPEND |= 1<<11;
		if(rEINTPEND & 1<<19)
			rEINTPEND |= 1<<19;
	}
	//Clear Rs, SRCPND INTPND
	ClearPending(BIT_EINT8_23);
	ClearPending(BIT_EINT0);
	ClearPending(BIT_EINT2);
}

void KeyInit()
{
	MMU_Init();
	
	//Set GPGCON interrupt
	rGPGCON |= (1<<1 | 1<<7 | 1<<11 | 1<<13 );
	rEXTINT1 = 0;
	
	//Interrupt entrance
	pISR_EINT0 = pISR_EINT2 = pISR_EINT8_23 = (U32)Key_ISR;
	
	//Clear Rs
	rEINTPEND |= (1<<11 | 1<<19);
	ClearPending(BIT_EINT0 | BIT_EINT2 | BIT_EINT8_23);
	
	//
	rEINTMASK &= ~(1<<11 | 1<<19);
	
	EnableIrq(BIT_EINT0 | BIT_EINT2 | BIT_EINT8_23);
}

static void Wait(void)
{
	int gct=GUI_GetTime();

	//秒数后面的变化
	if(state == 1){
		int tmsec = msec - (gct-tt)/2;
		GUI_DispCharAt(':',60,60);
		GUI_DispCharAt(tmsec/10+'0',80,60);
		GUI_DispCharAt(tmsec%10+'0',100,60);
	}

	if(gct-tt>200)				//每隔一段时间对state进行检查，150？？？
	{
		tt=gct;

		if(state==1)
		{
			if(timeout==0)    //state为1时状态转换条件的轮询
				explode();
			else
			{
				timeout--;
				show(timeout);
			}
		}
	}
}

void reset()
{
    GUI_SetColor(GUI_BLACK);
 	GUI_SetBkColor(GUI_WHITE);
    GUI_SetFont(&GUI_Font32_ASCII); 
 
  	GUI_ClearRect(0,0,LCD_XSIZE,LCD_YSIZE);
	state=0;
	timeout=0;
	msec=99;
	passwordarray[0]=passwordarray[1]=0;
	GUI_DispStringAt("Set time:",20,10);
	show(timeout);
}

int num=0;

int Main(void)
{
	//密码定义
	defuse=111111;
	
	//表驱动初始化函数
	FunP[0][0]=s_down;
	FunP[0][1]=s_up;
	FunP[0][2]=s_arm;
	
    FunP[1][0]=t_down;
	FunP[1][1]=t_up;
	FunP[1][2]=t_arm;
	
	FunP[2][0]=FunP[2][1]=FunP[2][2]=reset;
	
	//初始化目标板硬件
	TargetInit(); 
	
	//初始化us-OS II 系统组件 
   	OSInit();	 
   	
   	//设置系统时间为0（ticks）
   	OSTimeSet(0);
   
   	//返回当前时间（与OSTimeGet等效）
    tt=GUI_GetTime();
   	
   	//创建timebomb任务，并将其置于Task栈栈顶，优先级置为最高
  	OSTaskCreate (timeBomb,(void *)0, &MainTaskStk[MainTaskStkLengh - 1], MainTaskPrio);
  	
  	//启动操作系统，找到最高优先级的任务并运行
	OSStart();	
	return 0;
}




//还需要在app_cfg.h中声明任务
void timeBomb(void *pdata) //Main Task create taks0 and task1
{
#if OS_CRITICAL_METHOD == 3// Allocate storage for CPU status register 
	OS_CPU_SR  cpu_sr;
#endif

	OS_ENTER_CRITICAL();//保存SR中的状态位，且关任务中断
		Timer0Init();	//initial timer0 for ucos time tick
		ISRInit();   	//initial interrupt prio or enable or disable
		KeyInit();
	OS_EXIT_CRITICAL();//恢复SR中的状态位，重新开启任务中断
	
	//print massage to Uart
	OSPrintfInit(); 
	OSStatInit();
	
	GUI_Init();   
  
	reset();
	
	while(1)
	{  
		Wait();
	}	
}
