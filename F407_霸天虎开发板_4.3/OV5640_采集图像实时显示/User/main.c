/**
  ******************************************************************************
  * @file    main.c
  * @author  fire
  * @version V1.0
  * @date    2015-xx-xx
  * @brief   OV5640����ͷ��ʾ����
  ******************************************************************************
  * @attention
  *
  * ʵ��ƽ̨:Ұ�� STM32  F407������ 
  * ��̳    :http://www.firebbs.cn
  * �Ա�    :https://fire-stm32.taobao.com
  *
  ******************************************************************************
  */
  
#include "stm32f4xx.h"
#include "./usart/bsp_debug_usart.h"
#include "./lcd/bsp_nt35510_lcd.h"
#include "./camera/bsp_ov5640.h"
#include "./camera/ov5640_AF.h"
#include "./systick/bsp_SysTick.h"
#include "./sram/bsp_sram.h"	  
#include "./key/bsp_key.h" 
#include "./led/bsp_led.h"   



/*���������*/
uint32_t Task_Delay[NumOfTask];

char dispBuf[100];
OV5640_IDTypeDef OV5640_Camera_ID;

uint8_t fps=0;



/**
  * @brief  Һ����������ʹ������ͷ���ݲɼ�
  * @param  ��
  * @retval ��
  */
void ImagDisp(void)
{
		//ɨ��ģʽ������
    NT35510_GramScan(cam_mode.lcd_scan);
    LCD_SetFont(&Font16x32);
		LCD_SetColors(RED,BLACK);
	
    NT35510_Clear(0,0,LCD_X_LENGTH,LCD_Y_LENGTH);	/* ��������ʾȫ�� */

	/*DMA������ݴ��䵽Һ���������������ݰ��������� */
    NT35510_OpenWindow(cam_mode.lcd_sx,
													cam_mode.lcd_sy,
													cam_mode.cam_out_width,
													cam_mode.cam_out_height);	
	
		OV5640_Capture_Control(ENABLE);
}


//�޸�cam_mode�������޸ĺ���Ҫ���µ������ú�������Ч
void Camera_Mode_Reconfig(void)
{
	cam_mode.frame_rate = FRAME_RATE_30FPS,	
	
	//ISP����
	cam_mode.cam_isp_sx = 0;
	cam_mode.cam_isp_sy = 0;	
	
	cam_mode.cam_isp_width = 1920;
	cam_mode.cam_isp_height = 1080;
	
	//�������
	cam_mode.scaling = 1;      //ʹ���Զ�����
	cam_mode.cam_out_sx = 16;	//ʹ���Զ����ź�һ�����ó�16����
	cam_mode.cam_out_sy = 4;	  //ʹ���Զ����ź�һ�����ó�4����
	cam_mode.cam_out_width = 320;
	cam_mode.cam_out_height = 240;
	
	//LCDλ��
	cam_mode.lcd_sx = 270;
	cam_mode.lcd_sy = 120;
	cam_mode.lcd_scan = 5; //LCDɨ��ģʽ��
	
	//���¿ɸ����Լ�����Ҫ������������Χ���ṹ�����Ͷ���	
	cam_mode.light_mode = 0;//�Զ�����ģʽ
	cam_mode.saturation = 0;	
	cam_mode.brightness = 0;
	cam_mode.contrast = 0;
	cam_mode.effect = 0;		//����ģʽ
	cam_mode.exposure = 0;		

	cam_mode.auto_focus = 1;//�Զ��Խ�
}


//��*��ע�����
//��������ʱ��֧������ͷ�ӳ��ߣ�����
/**
  * @brief  ������
  * @param  ��
  * @retval ��
  */
int main(void)
{	
  uint8_t focus_status = 0;
	
	NT35510_Init ();         //LCD ��ʼ��
	
	LCD_SetFont(&Font16x32);
	LCD_SetColors(RED,BLACK);

  NT35510_Clear(0,0,LCD_X_LENGTH,LCD_Y_LENGTH);	/* ��������ʾȫ�� */

  Debug_USART_Config();   
	
	/* ����SysTick Ϊ1ms�ж�һ��,ʱ�䵽�󴥷���ʱ�жϣ�
	*����stm32fxx_it.c�ļ���SysTick_Handler����ͨ�����жϴ�����ʱ
	*/
	SysTick_Init();
	
	Key_GPIO_Config();

	//Һ��ɨ�跽��
	NT35510_GramScan(5);
	
  CAMERA_DEBUG("STM32F407 DCMI ����OV5640����");

  /* ��ʼ������ͷGPIO��IIC */
  OV5640_HW_Init();   

  /* ��ȡ����ͷоƬID��ȷ������ͷ�������� */
  OV5640_ReadID(&OV5640_Camera_ID);

   if(OV5640_Camera_ID.PIDH  == 0x56)
  {
    sprintf((char*)dispBuf, "OV5640 Camera ID:0x%x,initializing... ", OV5640_Camera_ID.PIDH);
		NT35510_DispStringLine_EN(LINE(0),dispBuf);
    CAMERA_DEBUG("��⵽����ͷ %x %x",OV5640_Camera_ID.PIDH ,OV5640_Camera_ID.PIDL);

  }
  else
  {
    LCD_SetTextColor(RED);
    NT35510_DispString_EN(10,10, "Can not detect OV5640 module,please check the connection!");
    CAMERA_DEBUG("û�м�⵽OV5640����ͷ�������¼�����ӡ�");

    while(1);  
  }

  
  OV5640_Init();
	
  OV5640_RGB565_Default_Config();
	
	OV5640_USER_Config();

	OV5640_FOCUS_AD5820_Init();
	

	if(cam_mode.auto_focus ==1)
	{
		OV5640_FOCUS_AD5820_Constant_Focus();
		focus_status = 1;
	}
    
	/*DMAֱ�Ӵ�������ͷ���ݵ�LCD��Ļ��ʾ*/
	ImagDisp();
	
  while(1)
	{		
		//����ʱ��̬�޸�����ͷ����ʾ��
		if( Key_Scan(KEY1_GPIO_PORT,KEY1_PIN) == KEY_ON  )
		{
			//�رղɼ�
			OV5640_Capture_Control(DISABLE);
			
			//�޸�Cam mode ����
			Camera_Mode_Reconfig();
			
			OV5640_USER_Config();

			if(cam_mode.auto_focus ==1)
			{
				OV5640_FOCUS_AD5820_Constant_Focus();
				focus_status = 1;
			}
				
			/*DMAֱ�Ӵ�������ͷ���ݵ�LCD��Ļ��ʾ*/
			ImagDisp();			
			
		}   
		
		//��ת�Խ�״̬����ԭ���ڳ����Խ�����ͣ��������
		if( Key_Scan(KEY2_GPIO_PORT,KEY2_PIN) == KEY_ON  )
		{
			if(focus_status == 1)
			{
				//��ͣ�Խ�
				OV5640_FOCUS_AD5820_Pause_Focus();
				focus_status = 0 ;
			}
			else
			{
				//�Զ��Խ�
				OV5640_FOCUS_AD5820_Constant_Focus();
				focus_status = 1 ;
			}
		}   


		
		//ʹ�ô������֡��
		if(Task_Delay[0]==0)
		{			
			/*���֡��*/
			CAMERA_DEBUG("\r\n֡��:%.1f/s \r\n", (double)fps/5.0);
			//����
			fps =0;			
			
			Task_Delay[0]=5000; //��ֵÿ1ms���1������0�ſ������½�������

		}		
	}
}



/*********************************************END OF FILE**********************/

