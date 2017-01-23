/*******************************************************************************************/
     

/*******************************************************************************************/
#include <linux/videodev2.h>    
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <asm/atomic.h>
#include <linux/xlog.h>
#include <asm/system.h>

#include <linux/proc_fs.h> 
#include <linux/dma-mapping.h>

#include "kd_camera_hw.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"

#include "ov9724mipiraw_Sensor.h"
#include "ov9724mipiraw_Camera_Sensor_para.h"
#include "ov9724mipiraw_CameraCustomized.h"

static DEFINE_SPINLOCK(OV9724mipiraw_drv_lock);

#define OV9724_DEBUG
#ifdef OV9724_DEBUG
	#define OV9724DB(fmt, arg...) xlog_printk(ANDROID_LOG_DEBUG, "[OV9724Raw] ",  fmt, ##arg)
#else
	#define OV9724DB(fmt, arg...)
#endif

kal_uint32 OV9724_FeatureControl_PERIOD_PixelNum=OV9724_PV_PERIOD_PIXEL_NUMS;
kal_uint32 OV9724_FeatureControl_PERIOD_LineNum=OV9724_PV_PERIOD_LINE_NUMS;

UINT16 OV9724_VIDEO_MODE_TARGET_FPS = 30;

MSDK_SCENARIO_ID_ENUM OV9724CurrentScenarioId = MSDK_SCENARIO_ID_CAMERA_PREVIEW;
MSDK_SENSOR_CONFIG_STRUCT OV9724SensorConfigData;
static OV9724_PARA_STRUCT OV9724;
kal_uint32 OV9724_FAC_SENSOR_REG;


SENSOR_REG_STRUCT OV9724SensorCCT[]=CAMERA_SENSOR_CCT_DEFAULT_VALUE;
SENSOR_REG_STRUCT OV9724SensorReg[ENGINEER_END]=CAMERA_SENSOR_REG_DEFAULT_VALUE;


#define OV9724_TEST_PATTERN_CHECKSUM 0x8790037C
kal_bool OV9724_During_testpattern = KAL_FALSE;

extern int iReadReg(u16 a_u2Addr , u8 * a_puBuff , u16 i2cId);
extern int iWriteReg(u16 a_u2Addr , u32 a_u4Data , u32 a_u4Bytes , u16 i2cId);
extern int iMultiWriteReg(u8 *pData, u16 lens, u16 i2cId);

#define OV9724_write_cmos_sensor(addr, para) iWriteReg((u16) addr , (u32) para , 1, OV9724MIPI_WRITE_ID)
#define OV9724_multi_write_cmos_sensor(pData, lens) iMultiWriteReg((u8*) pData, (u16) lens, OV9724MIPI_WRITE_ID)


kal_uint16 OV9724_read_cmos_sensor(kal_uint32 addr)
{
kal_uint16 get_byte=0;
    iReadReg((u16) addr ,(u8*)&get_byte,OV9724MIPI_WRITE_ID);
    return get_byte;
}


void OV9724_Init_Para(void)
{

	spin_lock(&OV9724mipiraw_drv_lock);
	OV9724.sensorMode = SENSOR_MODE_INIT;
	OV9724.OV9724AutoFlickerMode = KAL_FALSE;
	OV9724.OV9724VideoMode = KAL_FALSE;
	OV9724.DummyLines= 0;
	OV9724.DummyPixels= 0;
	OV9724.pvPclk =  (3660); 
	OV9724.videoPclk = (3660);
	OV9724.capPclk = (3660);

	OV9724.shutter = 0x02f0;
	OV9724.ispBaseGain = BASEGAIN;
	OV9724.sensorGlobalGain = 0x0200;
	spin_unlock(&OV9724mipiraw_drv_lock);
}

kal_uint32 GetOV9724LineLength(void)
{
	kal_uint32 OV9724_line_length = 0;
	
	if ( SENSOR_MODE_PREVIEW == OV9724.sensorMode )  
	{
		OV9724_line_length = OV9724_PV_PERIOD_PIXEL_NUMS + OV9724.DummyPixels;
	}
	else if( SENSOR_MODE_VIDEO == OV9724.sensorMode ) 
	{
		OV9724_line_length = OV9724_VIDEO_PERIOD_PIXEL_NUMS + OV9724.DummyPixels;
	}
	else
	{
		OV9724_line_length = OV9724_FULL_PERIOD_PIXEL_NUMS + OV9724.DummyPixels;
	}

    return OV9724_line_length;

}


kal_uint32 GetOV9724FrameLength(void)
{
	kal_uint32 OV9724_frame_length = 0;

	if ( SENSOR_MODE_PREVIEW == OV9724.sensorMode )  
	{
		OV9724_frame_length = OV9724_PV_PERIOD_LINE_NUMS + OV9724.DummyLines ;
	}
	else if( SENSOR_MODE_VIDEO == OV9724.sensorMode ) 
	{
		OV9724_frame_length = OV9724_VIDEO_PERIOD_LINE_NUMS + OV9724.DummyLines ;
	}
	else
	{
		OV9724_frame_length = OV9724_FULL_PERIOD_LINE_NUMS + OV9724.DummyLines ;
	}

	return OV9724_frame_length;
}


kal_uint32 OV9724_CalcExtra_For_ShutterMargin(kal_uint32 shutter_value,kal_uint32 shutterLimitation)
{
    kal_uint32 extra_lines = 0;

	
	if (shutter_value <4 ){
		shutter_value = 4;
	}

	
	if (shutter_value > shutterLimitation)
	{
		extra_lines = shutter_value - shutterLimitation;
    }
	else
		extra_lines = 0;

    return extra_lines;

}


kal_uint32 OV9724_CalcFrameLength_For_AutoFlicker(void)
{

    kal_uint32 AutoFlicker_min_framelength = 0;

	switch(OV9724CurrentScenarioId)
	{
		case MSDK_SCENARIO_ID_CAMERA_ZSD:
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			AutoFlicker_min_framelength = (OV9724.capPclk*10000) /(OV9724_FULL_PERIOD_PIXEL_NUMS + OV9724.DummyPixels)/OV9724_AUTOFLICKER_OFFSET_30*10 ;
			break;
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			if(OV9724_VIDEO_MODE_TARGET_FPS==30)
			{
				AutoFlicker_min_framelength = (OV9724.videoPclk*10000) /(OV9724_VIDEO_PERIOD_PIXEL_NUMS + OV9724.DummyPixels)/OV9724_AUTOFLICKER_OFFSET_30*10 ;
			}
			else if(OV9724_VIDEO_MODE_TARGET_FPS==15)
			{
				AutoFlicker_min_framelength = (OV9724.videoPclk*10000) /(OV9724_VIDEO_PERIOD_PIXEL_NUMS + OV9724.DummyPixels)/OV9724_AUTOFLICKER_OFFSET_15*10 ;
			}
			else
			{
				AutoFlicker_min_framelength = OV9724_VIDEO_PERIOD_LINE_NUMS + OV9724.DummyLines;
			}
			break;
			
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		default:
			AutoFlicker_min_framelength = (OV9724.pvPclk*10000) /(OV9724_PV_PERIOD_PIXEL_NUMS + OV9724.DummyPixels)/OV9724_AUTOFLICKER_OFFSET_30*10 ;
			break;
	}

	//OV9724DB("AutoFlicker_min_framelength =%d,OV9724CurrentScenarioId =%d\n", AutoFlicker_min_framelength,OV9724CurrentScenarioId);

	return AutoFlicker_min_framelength;

}


void OV9724_write_shutter(kal_uint32 shutter)
{
	kal_uint32 min_framelength = OV9724_PV_PERIOD_LINE_NUMS, max_shutter=0;
	kal_uint32 line_length = 0;
	kal_uint32 frame_length = 0;
	unsigned long flags;

	kal_uint32 read_shutter_1 = 0;
	kal_uint32 read_shutter_2 = 0;
	kal_uint32 read_shutter_3 = 0;

   // if(shutter > 0x73c8)//400ms for capture SaturationGain
	//	shutter = 0x73c8;
	
    line_length  = GetOV9724LineLength();
	frame_length = GetOV9724FrameLength();
	
	max_shutter  = frame_length-OV9724_SHUTTER_MARGIN;

    frame_length = frame_length + OV9724_CalcExtra_For_ShutterMargin(shutter,max_shutter);

	if(OV9724.OV9724AutoFlickerMode == KAL_TRUE)
	{
        min_framelength = OV9724_CalcFrameLength_For_AutoFlicker();

        if(frame_length < min_framelength)
			frame_length = min_framelength;
	}
	
	spin_lock_irqsave(&OV9724mipiraw_drv_lock,flags);
	OV9724_FeatureControl_PERIOD_PixelNum = line_length;
	OV9724_FeatureControl_PERIOD_LineNum = frame_length;
	spin_unlock_irqrestore(&OV9724mipiraw_drv_lock,flags);

	//Set total frame length
	OV9724_write_cmos_sensor(0x0340, (frame_length >> 8) & 0xFF);
	OV9724_write_cmos_sensor(0x0341, frame_length & 0xFF);
	
	//Set shutter 
	OV9724_write_cmos_sensor(0x0202, (shutter>>8) & 0xFF);
	OV9724_write_cmos_sensor(0x0203,  shutter & 0xFF);

	OV9724DB("ov9724 write shutter=%x, line_length=%x, frame_length=%x\n", shutter, line_length, frame_length);

}


static kal_uint16 OV9724Reg2Gain(const kal_uint16 iReg)
{
    kal_uint32 iGain =0; 


    iGain = ((iReg>>4)+1)*(16+(iReg&0x0f));
	iGain = iGain*BASEGAIN/16;

	return iGain;
	
}

static kal_uint16 OV9724Gain2Reg(const kal_uint32 Gain)
{
    kal_uint32 iReg = 0x0000;

    if (Gain < 2 * BASEGAIN) {
        iReg = 16 * (Gain - BASEGAIN) / BASEGAIN;
    }else if (Gain < 4 * BASEGAIN) {
        iReg |= 0x10;
        iReg |= 8 * (Gain - 2 * BASEGAIN) / BASEGAIN;
    }else if (Gain < 8 * BASEGAIN) {
        iReg |= 0x30;
        iReg |= 4 * (Gain - 4 * BASEGAIN) / BASEGAIN;
    }else if (Gain < 16 * BASEGAIN) {
        iReg |= 0x70;
        iReg |= 2 * (Gain - 8 * BASEGAIN) / BASEGAIN;
    }else if (Gain < 32 * BASEGAIN) {
        iReg |= 0xF0;
        iReg |= (Gain - 16 * BASEGAIN) / BASEGAIN;
	    OV9724DB("OV9724_SetGain >16x,shouldn't reach this conditon,check it ");
    }else {
	    OV9724DB("OV9724_SetGain error =>ASSERT ");
        ASSERT(0);
    }

    return iReg;

}

void write_OV9724_gain(kal_uint16 gain)
{

	OV9724_write_cmos_sensor(0x0205,gain);
	return;
}

void OV9724_SetGain(UINT16 iGain)
{
	unsigned long flags;
	
	spin_lock_irqsave(&OV9724mipiraw_drv_lock,flags);
	OV9724.realGain = iGain;
	OV9724.sensorGlobalGain = OV9724Gain2Reg(iGain);
	spin_unlock_irqrestore(&OV9724mipiraw_drv_lock,flags);

	write_OV9724_gain(OV9724.sensorGlobalGain);
	
	OV9724DB("[OV9724_SetGain]:BB_setValue =%d,Sensor_reg_value=0x%x\n",OV9724.realGain,OV9724.sensorGlobalGain); 

}   

kal_uint16 read_OV9724_gain(void)
{
	unsigned long flags;
	kal_uint16 read_gain=0;
	kal_uint16 ov9724_real_gain=0;

	read_gain= OV9724_read_cmos_sensor(0x0205);	
	ov9724_real_gain= OV9724Reg2Gain(read_gain);

	spin_lock_irqsave(&OV9724mipiraw_drv_lock,flags);
	OV9724.sensorGlobalGain = read_gain;
	OV9724.realGain = ov9724_real_gain;
	spin_unlock_irqrestore(&OV9724mipiraw_drv_lock,flags);

	OV9724DB("[OV9724_readGain]Sensor_reg_value=0x%x,BB_value=%d\n",OV9724.sensorGlobalGain,OV9724.realGain);

	return OV9724.sensorGlobalGain;
}  


#if 1
void OV9724_camera_para_to_sensor(void)
{
    kal_uint32    i;
    for(i=0; 0xFFFFFFFF!=OV9724SensorReg[i].Addr; i++)
    {
        OV9724_write_cmos_sensor(OV9724SensorReg[i].Addr, OV9724SensorReg[i].Para);
    }
    for(i=ENGINEER_START_ADDR; 0xFFFFFFFF!=OV9724SensorReg[i].Addr; i++)
    {
        OV9724_write_cmos_sensor(OV9724SensorReg[i].Addr, OV9724SensorReg[i].Para);
    }
    for(i=FACTORY_START_ADDR; i<FACTORY_END_ADDR; i++)
    {
        OV9724_write_cmos_sensor(OV9724SensorCCT[i].Addr, OV9724SensorCCT[i].Para);
    }
}

void OV9724_sensor_to_camera_para(void)
{
    kal_uint32    i, temp_data;
    for(i=0; 0xFFFFFFFF!=OV9724SensorReg[i].Addr; i++)
    {
         temp_data = OV9724_read_cmos_sensor(OV9724SensorReg[i].Addr);
		 spin_lock(&OV9724mipiraw_drv_lock);
		 OV9724SensorReg[i].Para =temp_data;
		 spin_unlock(&OV9724mipiraw_drv_lock);
    }
    for(i=ENGINEER_START_ADDR; 0xFFFFFFFF!=OV9724SensorReg[i].Addr; i++)
    {
        temp_data = OV9724_read_cmos_sensor(OV9724SensorReg[i].Addr);
		spin_lock(&OV9724mipiraw_drv_lock);
		OV9724SensorReg[i].Para = temp_data;
		spin_unlock(&OV9724mipiraw_drv_lock);
    }
}

kal_int32  OV9724_get_sensor_group_count(void)
{
    return GROUP_TOTAL_NUMS;
}

void OV9724_get_sensor_group_info(kal_uint16 group_idx, kal_int8* group_name_ptr, kal_int32* item_count_ptr)
{
   switch (group_idx)
   {
        case PRE_GAIN:
            sprintf((char *)group_name_ptr, "CCT");
            *item_count_ptr = 2;
            break;
        case CMMCLK_CURRENT:
            sprintf((char *)group_name_ptr, "CMMCLK Current");
            *item_count_ptr = 1;
            break;
        case FRAME_RATE_LIMITATION:
            sprintf((char *)group_name_ptr, "Frame Rate Limitation");
            *item_count_ptr = 2;
            break;
        case REGISTER_EDITOR:
            sprintf((char *)group_name_ptr, "Register Editor");
            *item_count_ptr = 2;
            break;
        default:
            ASSERT(0);
}
}

void OV9724_get_sensor_item_info(kal_uint16 group_idx,kal_uint16 item_idx, MSDK_SENSOR_ITEM_INFO_STRUCT* info_ptr)
{
    kal_int16 temp_reg=0;
    kal_uint16 temp_gain=0, temp_addr=0, temp_para=0;

    switch (group_idx)
    {
        case PRE_GAIN:
           switch (item_idx)
          {
              case 0:
                sprintf((char *)info_ptr->ItemNamePtr,"Pregain-R");
                  temp_addr = PRE_GAIN_R_INDEX;
              break;
              case 1:
                sprintf((char *)info_ptr->ItemNamePtr,"Pregain-Gr");
                  temp_addr = PRE_GAIN_Gr_INDEX;
              break;
              case 2:
                sprintf((char *)info_ptr->ItemNamePtr,"Pregain-Gb");
                  temp_addr = PRE_GAIN_Gb_INDEX;
              break;
              case 3:
                sprintf((char *)info_ptr->ItemNamePtr,"Pregain-B");
                  temp_addr = PRE_GAIN_B_INDEX;
              break;
              case 4:
                 sprintf((char *)info_ptr->ItemNamePtr,"SENSOR_BASEGAIN");
                 temp_addr = SENSOR_BASEGAIN;
              break;
              default:
                 ASSERT(0);
          }

            temp_para= OV9724SensorCCT[temp_addr].Para;
			//temp_gain= (temp_para/OV9724.sensorBaseGain) * 1000;

            info_ptr->ItemValue=temp_gain;
            info_ptr->IsTrueFalse=KAL_FALSE;
            info_ptr->IsReadOnly=KAL_FALSE;
            info_ptr->IsNeedRestart=KAL_FALSE;
            info_ptr->Min= OV9724_MIN_ANALOG_GAIN * 1000;
            info_ptr->Max= OV9724_MAX_ANALOG_GAIN * 1000;
            break;
        case CMMCLK_CURRENT:
            switch (item_idx)
            {
                case 0:
                    sprintf((char *)info_ptr->ItemNamePtr,"Drv Cur[2,4,6,8]mA");

                    //temp_reg=MT9P017SensorReg[CMMCLK_CURRENT_INDEX].Para;
                    temp_reg = ISP_DRIVING_2MA;
                    if(temp_reg==ISP_DRIVING_2MA)
                    {
                        info_ptr->ItemValue=2;
                    }
                    else if(temp_reg==ISP_DRIVING_4MA)
                    {
                        info_ptr->ItemValue=4;
                    }
                    else if(temp_reg==ISP_DRIVING_6MA)
                    {
                        info_ptr->ItemValue=6;
                    }
                    else if(temp_reg==ISP_DRIVING_8MA)
                    {
                        info_ptr->ItemValue=8;
                    }

                    info_ptr->IsTrueFalse=KAL_FALSE;
                    info_ptr->IsReadOnly=KAL_FALSE;
                    info_ptr->IsNeedRestart=KAL_TRUE;
                    info_ptr->Min=2;
                    info_ptr->Max=8;
                    break;
                default:
                    ASSERT(0);
            }
            break;
        case FRAME_RATE_LIMITATION:
            switch (item_idx)
            {
                case 0:
                    sprintf((char *)info_ptr->ItemNamePtr,"Max Exposure Lines");
                    info_ptr->ItemValue=    111;  
                    info_ptr->IsTrueFalse=KAL_FALSE;
                    info_ptr->IsReadOnly=KAL_TRUE;
                    info_ptr->IsNeedRestart=KAL_FALSE;
                    info_ptr->Min=0;
                    info_ptr->Max=0;
                    break;
                case 1:
                    sprintf((char *)info_ptr->ItemNamePtr,"Min Frame Rate");
                    info_ptr->ItemValue=12;
                    info_ptr->IsTrueFalse=KAL_FALSE;
                    info_ptr->IsReadOnly=KAL_TRUE;
                    info_ptr->IsNeedRestart=KAL_FALSE;
                    info_ptr->Min=0;
                    info_ptr->Max=0;
                    break;
                default:
                    ASSERT(0);
            }
            break;
        case REGISTER_EDITOR:
            switch (item_idx)
            {
                case 0:
                    sprintf((char *)info_ptr->ItemNamePtr,"REG Addr.");
                    info_ptr->ItemValue=0;
                    info_ptr->IsTrueFalse=KAL_FALSE;
                    info_ptr->IsReadOnly=KAL_FALSE;
                    info_ptr->IsNeedRestart=KAL_FALSE;
                    info_ptr->Min=0;
                    info_ptr->Max=0xFFFF;
                    break;
                case 1:
                    sprintf((char *)info_ptr->ItemNamePtr,"REG Value");
                    info_ptr->ItemValue=0;
                    info_ptr->IsTrueFalse=KAL_FALSE;
                    info_ptr->IsReadOnly=KAL_FALSE;
                    info_ptr->IsNeedRestart=KAL_FALSE;
                    info_ptr->Min=0;
                    info_ptr->Max=0xFFFF;
                    break;
                default:
                ASSERT(0);
            }
            break;
        default:
            ASSERT(0);
    }
}



kal_bool OV9724_set_sensor_item_info(kal_uint16 group_idx, kal_uint16 item_idx, kal_int32 ItemValue)
{
   kal_uint16  temp_gain=0,temp_addr=0, temp_para=0;

   switch (group_idx)
    {
        case PRE_GAIN:
            switch (item_idx)
            {
              case 0:
                temp_addr = PRE_GAIN_R_INDEX;
              break;
              case 1:
                temp_addr = PRE_GAIN_Gr_INDEX;
              break;
              case 2:
                temp_addr = PRE_GAIN_Gb_INDEX;
              break;
              case 3:
                temp_addr = PRE_GAIN_B_INDEX;
              break;
              case 4:
                temp_addr = SENSOR_BASEGAIN;
              break;
              default:
                 ASSERT(0);
          }

		 temp_gain=((ItemValue*BASEGAIN+500)/1000);			//+500:get closed integer value

		  if(temp_gain>=1*BASEGAIN && temp_gain<=16*BASEGAIN)
          {
//             temp_para=(temp_gain * OV9724.sensorBaseGain + BASEGAIN/2)/BASEGAIN;
          }
          else
			  ASSERT(0);

		  spin_lock(&OV9724mipiraw_drv_lock);
          OV9724SensorCCT[temp_addr].Para = temp_para;
		  spin_unlock(&OV9724mipiraw_drv_lock);
          OV9724_write_cmos_sensor(OV9724SensorCCT[temp_addr].Addr,temp_para);

            break;
        case CMMCLK_CURRENT:
            switch (item_idx)
            {
                case 0:
                    //no need to apply this item for driving current
                    break;
                default:
                    ASSERT(0);
            }
            break;
        case FRAME_RATE_LIMITATION:
            ASSERT(0);
            break;
        case REGISTER_EDITOR:
            switch (item_idx)
            {
                case 0:
					spin_lock(&OV9724mipiraw_drv_lock);
                    OV9724_FAC_SENSOR_REG=ItemValue;
					spin_unlock(&OV9724mipiraw_drv_lock);
                    break;
                case 1:
                    OV9724_write_cmos_sensor(OV9724_FAC_SENSOR_REG,ItemValue);
                    break;
                default:
                    ASSERT(0);
            }
            break;
        default:
            ASSERT(0);
    }
    return KAL_TRUE;
}
#endif


static void OV9724_SetDummy( const kal_uint32 iPixels, const kal_uint32 iLines )
{
	kal_uint32 line_length = 0;
	kal_uint32 frame_length = 0;

	if ( SENSOR_MODE_PREVIEW == OV9724.sensorMode )
	{
		line_length = OV9724_PV_PERIOD_PIXEL_NUMS + iPixels;
		frame_length = OV9724_PV_PERIOD_LINE_NUMS + iLines;
	}
	else if( SENSOR_MODE_VIDEO== OV9724.sensorMode )
	{
		line_length = OV9724_VIDEO_PERIOD_PIXEL_NUMS + iPixels;
		frame_length = OV9724_VIDEO_PERIOD_LINE_NUMS + iLines;
	}
	else
	{
		line_length = OV9724_FULL_PERIOD_PIXEL_NUMS + iPixels;
		frame_length = OV9724_FULL_PERIOD_LINE_NUMS + iLines;
	}

	spin_lock(&OV9724mipiraw_drv_lock);
	OV9724_FeatureControl_PERIOD_PixelNum = line_length;
	OV9724_FeatureControl_PERIOD_LineNum = frame_length;
	spin_unlock(&OV9724mipiraw_drv_lock);

	//Set total frame length
	OV9724_write_cmos_sensor(0x0340, (frame_length >> 8) & 0xFF);
	OV9724_write_cmos_sensor(0x0341, frame_length & 0xFF);
	//Set total line length
	OV9724_write_cmos_sensor(0x0342, (line_length >> 8) & 0xFF);
	OV9724_write_cmos_sensor(0x0343, line_length & 0xFF);

}   


void OV9724PreviewSetting(void)
{
	OV9724DB(" OV9724PreviewSetting enter \n");
	OV9724DB(" dummy setting for 720p Resolution\n");

}


void OV9724VideoSetting(void)
{
	OV9724DB(" OV9724VideoSetting enter \n");
	OV9724DB(" dummy setting for 720p Resolution\n");
	
}


void OV9724CaptureSetting(void)
{
	OV9724DB(" OV9724CaptureSetting enter \n");
	OV9724DB(" dummy setting for 720p Resolution\n");
}


static kal_uint8 ov9724_init[] = {
0x01,0x03, 0x01,//pll
0x32,0x10, 0x43,//
0x03,0x07, 0x3d,//pll
0x57,0x80, 0x3e,//
0x36,0x06, 0x75,//
0x37,0x05, 0x41,//
0x36,0x01, 0x34,//
0x36,0x07, 0x94,//
0x36,0x08, 0x38,//
0x37,0x12, 0xb4,//
0x37,0x0d, 0xcc,//
0x40,0x10, 0x08,//
0x40,0x00, 0x01,//
0x03,0x40, 0x02,//frame length	760
0x03,0x41, 0xf8,//
0x03,0x42, 0x06,//line length 1576
0x03,0x43, 0x28,//
0x02,0x02, 0x02,// exposure high bit
0x02,0x03, 0xf0,// exposure low bit
0x48,0x01, 0x0f,//mipi control
0x48,0x01, 0x8f,//
0x48,0x14, 0x2b,//mipi
0x01,0x01, 0x01,//mirror & flip
0x51,0x10, 0x09,//
0x43,0x07, 0x3a,//
0x50,0x00, 0x00,//BPC off;	on:0x06
0x50,0x01, 0x73,//blc & manual wb gain
0x02,0x05, 0x3f,// gain control
0x01,0x00, 0x01};

static void OV9724_Sensor_Init(void)
{
	OV9724DB("OV9724_Sensor_Init enter:720p Resolution\n");
#if 0
	OV9724_write_cmos_sensor(0x0103, 0x01);//pll
	OV9724_write_cmos_sensor(0x3210, 0x43);//
	OV9724_write_cmos_sensor(0x0307, 0x3d);//pll
	OV9724_write_cmos_sensor(0x5780, 0x3e);//
	OV9724_write_cmos_sensor(0x3606, 0x75);//
	OV9724_write_cmos_sensor(0x3705, 0x41);//
	OV9724_write_cmos_sensor(0x3601, 0x34);//
	OV9724_write_cmos_sensor(0x3607, 0x94);//
	OV9724_write_cmos_sensor(0x3608, 0x38);//
	OV9724_write_cmos_sensor(0x3712, 0xb4);//
	OV9724_write_cmos_sensor(0x370d, 0xcc);//
	OV9724_write_cmos_sensor(0x4010, 0x08);//
	OV9724_write_cmos_sensor(0x4000, 0x01);//
	
	OV9724_write_cmos_sensor(0x0340, 0x02);//frame length  760
	OV9724_write_cmos_sensor(0x0341, 0xf8);//
	
	OV9724_write_cmos_sensor(0x0342, 0x06);//line length 1576
	OV9724_write_cmos_sensor(0x0343, 0x28);//
	
	OV9724_write_cmos_sensor(0x0202, 0x02);// exposure high bit
	OV9724_write_cmos_sensor(0x0203, 0xf0);// exposure low bit
	
	OV9724_write_cmos_sensor(0x4801, 0x0f);//mipi control
	OV9724_write_cmos_sensor(0x4801, 0x8f);//
	OV9724_write_cmos_sensor(0x4814, 0x2b);//mipi
	OV9724_write_cmos_sensor(0x0101, 0x01);//mirror & flip
	OV9724_write_cmos_sensor(0x5110, 0x09);//
	OV9724_write_cmos_sensor(0x4307, 0x3a);//
	OV9724_write_cmos_sensor(0x5000, 0x00);//BPC off;  on:0x06
	
	OV9724_write_cmos_sensor(0x5001, 0x73);//blc & manual wb gain
	
	OV9724_write_cmos_sensor(0x0205, 0x3f);// gain control
	
	OV9724_write_cmos_sensor(0x0100, 0x01);//
#else
	int totalCnt = 0, len = 0;
	int transfer_len, transac_len=3;
	kal_uint8* pBuf=NULL;
	dma_addr_t dmaHandle;
	pBuf = (kal_uint8*)kmalloc(1024, GFP_KERNEL);


	totalCnt = ARRAY_SIZE(ov9724_init);
	transfer_len = totalCnt / transac_len;
	len = (transfer_len<<8)|transac_len;	
	OV9724DB("Total Count = %d, Len = 0x%x\n", totalCnt,len);	  
	memcpy(pBuf, &ov9724_init, totalCnt );   
	dmaHandle = dma_map_single(NULL, pBuf, 1024, DMA_TO_DEVICE);	
	OV9724_multi_write_cmos_sensor(dmaHandle, len); 

	dma_unmap_single(NULL, dmaHandle, 1024, DMA_TO_DEVICE);
#endif

}


UINT32 OV9724Open(void)
{

	volatile signed int i;
	kal_uint16 sensor_id = 0;

	OV9724DB("OV9724 Open enter :\n ");
    mdelay(2);

	for(i=0;i<2;i++)
	{
		sensor_id = (OV9724_read_cmos_sensor(0x300A)<<8)|OV9724_read_cmos_sensor(0x300B);
		OV9724DB("OV9724 READ ID :%x",sensor_id);
		if(sensor_id != OV9724_SENSOR_ID)
		{
			return ERROR_SENSOR_CONNECT_FAIL;
		}else
			break;
	}
	
	OV9724_Sensor_Init();
    OV9724_Init_Para();
	
	OV9724DB("OV9724Open exit :\n ");

    return ERROR_NONE;
}


UINT32 OV9724GetSensorID(UINT32 *sensorID)
{
    int  retry = 2;

	OV9724DB("OV9724GetSensorID enter :\n ");
    mdelay(5);

    do {
        *sensorID = (OV9724_read_cmos_sensor(0x300A)<<8)|OV9724_read_cmos_sensor(0x300B);

        if (*sensorID == OV9724_SENSOR_ID)
        	{
        		OV9724DB("Sensor ID = 0x%04x\n", *sensorID);
            	break;
        	}
        OV9724DB("Read Sensor ID Fail = 0x%04x\n", *sensorID);
        retry--;
    } while (retry > 0);

    if (*sensorID != OV9724_SENSOR_ID) {
        *sensorID = 0xFFFFFFFF;
        return ERROR_SENSOR_CONNECT_FAIL;
    }
    return ERROR_NONE;
}


void OV9724_SetShutter(kal_uint32 iShutter)
{

   spin_lock(&OV9724mipiraw_drv_lock);
   OV9724.shutter= iShutter;
   spin_unlock(&OV9724mipiraw_drv_lock);

   OV9724_write_shutter(iShutter);
   return;
}


UINT32 OV9724_read_shutter(void)
{

	kal_uint16 temp_reg1=0, temp_reg2=0;
	kal_uint16 shutter =0;

	temp_reg1=OV9724_read_cmos_sensor(0x0202);
	temp_reg2=OV9724_read_cmos_sensor(0x0203);

    shutter = ((temp_reg1<<8)&0xf0)|temp_reg2;

	return shutter;
}

void OV9724_NightMode(kal_bool bEnable)
{

}

UINT32 OV9724Close(void)
{

    return ERROR_NONE;
}


void OV9724SetFlipMirror(kal_int32 imgMirror)
{

	OV9724DB("OV9724SetFlipMirror :%d\n",imgMirror);
    switch (imgMirror)
    {
		case IMAGE_NORMAL:
			OV9724_write_cmos_sensor(0x0101,0x00);
			OV9724_write_cmos_sensor(0x0347,0x00);
			OV9724_write_cmos_sensor(0x034b,0x27);
			break;
		case IMAGE_H_MIRROR:
			OV9724_write_cmos_sensor(0x0101,0x01);
			OV9724_write_cmos_sensor(0x0347,0x00);
			OV9724_write_cmos_sensor(0x034b,0x27);
			break;
		case IMAGE_V_MIRROR:
			OV9724_write_cmos_sensor(0x0101,0x02);
			OV9724_write_cmos_sensor(0x0347,0x01);
			OV9724_write_cmos_sensor(0x034b,0x27);

			break;
		case IMAGE_HV_MIRROR:
			OV9724_write_cmos_sensor(0x0101,0x03);
			OV9724_write_cmos_sensor(0x0347,0x01);
			OV9724_write_cmos_sensor(0x034b,0x27);
			break;
		default:
			break;
    }

}


UINT32 OV9724Preview(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                                                MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{

	OV9724DB("OV9724Preview enter:");

	OV9724PreviewSetting();

	spin_lock(&OV9724mipiraw_drv_lock);
	OV9724.sensorMode = SENSOR_MODE_PREVIEW; 
	OV9724.DummyPixels = 0;
	OV9724.DummyLines = 0 ;
	OV9724_FeatureControl_PERIOD_PixelNum=OV9724_PV_PERIOD_PIXEL_NUMS+ OV9724.DummyPixels;
	OV9724_FeatureControl_PERIOD_LineNum=OV9724_PV_PERIOD_LINE_NUMS+OV9724.DummyLines;
	OV9724.imgMirror = sensor_config_data->SensorImageMirror;
	spin_unlock(&OV9724mipiraw_drv_lock);
	
	//OV9724SetFlipMirror(sensor_config_data->SensorImageMirror);

    mdelay(40);//THIS DELAY SHOULD BE NEED BY CTS OR MONKEY
	OV9724DB("OV9724Preview exit:\n");

    return ERROR_NONE;
}


UINT32 OV9724Video(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                                                MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{

	OV9724DB("OV9724Video enter:");

	OV9724VideoSetting();

	spin_lock(&OV9724mipiraw_drv_lock);
	OV9724.sensorMode = SENSOR_MODE_VIDEO;
	OV9724_FeatureControl_PERIOD_PixelNum=OV9724_VIDEO_PERIOD_PIXEL_NUMS+ OV9724.DummyPixels;
	OV9724_FeatureControl_PERIOD_LineNum=OV9724_VIDEO_PERIOD_LINE_NUMS+OV9724.DummyLines;
	OV9724.imgMirror = sensor_config_data->SensorImageMirror;
	spin_unlock(&OV9724mipiraw_drv_lock);
	
	//OV9724SetFlipMirror(sensor_config_data->SensorImageMirror);

    mdelay(40);//THIS DELAY SHOULD BE NEED BY CTS OR MONKEY
	OV9724DB("OV9724Video exit:\n");
    return ERROR_NONE;
}


UINT32 OV9724Capture(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                                                MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{

	if( SENSOR_MODE_CAPTURE== OV9724.sensorMode)
	{
		OV9724DB("OV9724Capture BusrtShot / ZSD!!!\n");
	}
	else
	{
		OV9724DB("OV9724Capture enter:\n");

		OV9724CaptureSetting();

		spin_lock(&OV9724mipiraw_drv_lock);
		OV9724.sensorMode = SENSOR_MODE_CAPTURE;
		OV9724.imgMirror = sensor_config_data->SensorImageMirror;
		OV9724.DummyPixels = 0;
		OV9724.DummyLines = 0 ;
		OV9724_FeatureControl_PERIOD_PixelNum = OV9724_FULL_PERIOD_PIXEL_NUMS + OV9724.DummyPixels;
		OV9724_FeatureControl_PERIOD_LineNum = OV9724_FULL_PERIOD_LINE_NUMS + OV9724.DummyLines;
		spin_unlock(&OV9724mipiraw_drv_lock);

		//OV9724SetFlipMirror(sensor_config_data->SensorImageMirror);

	    mdelay(40);//THIS DELAY SHOULD BE NEED BY CTS OR MONKEY
		OV9724DB("OV9724Capture exit:\n");
	}
	
#if 0//check with captureSetting, no need here for OV9724;
	if(OV9724_During_testpattern == KAL_TRUE)
	{
		OV9724_write_cmos_sensor(0x0601,0x02);
	}
#endif

    return ERROR_NONE;
}	



UINT32 OV9724GetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution)
{

    OV9724DB("OV9724GetResolution!!\n");

	pSensorResolution->SensorPreviewWidth	= OV9724_IMAGE_SENSOR_PV_WIDTH;
    pSensorResolution->SensorPreviewHeight	= OV9724_IMAGE_SENSOR_PV_HEIGHT;
	
    pSensorResolution->SensorFullWidth		= OV9724_IMAGE_SENSOR_FULL_WIDTH;
    pSensorResolution->SensorFullHeight		= OV9724_IMAGE_SENSOR_FULL_HEIGHT;
	
    pSensorResolution->SensorVideoWidth		= OV9724_IMAGE_SENSOR_VIDEO_WIDTH;
    pSensorResolution->SensorVideoHeight    = OV9724_IMAGE_SENSOR_VIDEO_HEIGHT;
    return ERROR_NONE;
}   

UINT32 OV9724GetInfo(MSDK_SCENARIO_ID_ENUM ScenarioId,
                                                MSDK_SENSOR_INFO_STRUCT *pSensorInfo,
                                                MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{

	spin_lock(&OV9724mipiraw_drv_lock);
	OV9724.imgMirror = pSensorConfigData->SensorImageMirror ;
	spin_unlock(&OV9724mipiraw_drv_lock);

    pSensorInfo->SensorOutputDataFormat= SENSOR_OUTPUT_FORMAT_RAW_B;
   
    pSensorInfo->SensorClockPolarity =SENSOR_CLOCK_POLARITY_LOW;
    pSensorInfo->SensorClockFallingPolarity=SENSOR_CLOCK_POLARITY_LOW;
    pSensorInfo->SensorHsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
    pSensorInfo->SensorVsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;

    pSensorInfo->SensroInterfaceType=SENSOR_INTERFACE_TYPE_MIPI;

    pSensorInfo->CaptureDelayFrame = 2;
    pSensorInfo->PreviewDelayFrame = 2;
    pSensorInfo->VideoDelayFrame = 2;

    pSensorInfo->SensorDrivingCurrent = ISP_DRIVING_8MA;
    pSensorInfo->AEShutDelayFrame = 0;	    
    pSensorInfo->AESensorGainDelayFrame = 0;
    pSensorInfo->AEISPGainDelayFrame = 2;

    switch (ScenarioId)
    {
        case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
            pSensorInfo->SensorClockFreq=24;
            pSensorInfo->SensorClockRisingCount= 0;

            pSensorInfo->SensorGrabStartX = OV9724_PV_X_START;
            pSensorInfo->SensorGrabStartY = OV9724_PV_Y_START;

            pSensorInfo->SensorMIPILaneNumber = SENSOR_MIPI_1_LANE;
			
            pSensorInfo->MIPIDataLowPwr2HighSpeedTermDelayCount = 0;
	     	pSensorInfo->MIPIDataLowPwr2HighSpeedSettleDelayCount = 14;
	    	pSensorInfo->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
            pSensorInfo->SensorPacketECCOrder = 1;
            break;
        case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
            pSensorInfo->SensorClockFreq=24;
            pSensorInfo->SensorClockRisingCount= 0;

            pSensorInfo->SensorGrabStartX = OV9724_VIDEO_X_START;
            pSensorInfo->SensorGrabStartY = OV9724_VIDEO_Y_START;

            pSensorInfo->SensorMIPILaneNumber = SENSOR_MIPI_1_LANE;
            pSensorInfo->MIPIDataLowPwr2HighSpeedTermDelayCount = 0;
	     	pSensorInfo->MIPIDataLowPwr2HighSpeedSettleDelayCount = 14;
	    	pSensorInfo->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
            pSensorInfo->SensorPacketECCOrder = 1;
            break;
        case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		case MSDK_SCENARIO_ID_CAMERA_ZSD:
            pSensorInfo->SensorClockFreq=24;
            pSensorInfo->SensorClockRisingCount= 0;

            pSensorInfo->SensorGrabStartX = OV9724_FULL_X_START;	
            pSensorInfo->SensorGrabStartY = OV9724_FULL_Y_START;	

            pSensorInfo->SensorMIPILaneNumber = SENSOR_MIPI_1_LANE;
            pSensorInfo->MIPIDataLowPwr2HighSpeedTermDelayCount = 0;
            pSensorInfo->MIPIDataLowPwr2HighSpeedSettleDelayCount = 14;
            pSensorInfo->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
            pSensorInfo->SensorPacketECCOrder = 1;
            break;
        default:
			pSensorInfo->SensorClockFreq=24;
            pSensorInfo->SensorClockRisingCount= 0;

            pSensorInfo->SensorGrabStartX = OV9724_PV_X_START;
            pSensorInfo->SensorGrabStartY = OV9724_PV_Y_START;

            pSensorInfo->SensorMIPILaneNumber = SENSOR_MIPI_1_LANE;
            pSensorInfo->MIPIDataLowPwr2HighSpeedTermDelayCount = 0;
	     	pSensorInfo->MIPIDataLowPwr2HighSpeedSettleDelayCount = 14;
	    	pSensorInfo->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
            pSensorInfo->SensorPacketECCOrder = 1;
            break;
    }

    memcpy(pSensorConfigData, &OV9724SensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));

    return ERROR_NONE;
}   /* OV9724GetInfo() */



UINT32 OV9724Control(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow,
                                                MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
		spin_lock(&OV9724mipiraw_drv_lock);
		OV9724CurrentScenarioId = ScenarioId;
		spin_unlock(&OV9724mipiraw_drv_lock);
		
		OV9724DB("OV9724CurrentScenarioId=%d\n",OV9724CurrentScenarioId);

	switch (ScenarioId)
    {
        case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
            OV9724Preview(pImageWindow, pSensorConfigData);
            break;
        case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			OV9724Video(pImageWindow, pSensorConfigData);
			break;
        case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		case MSDK_SCENARIO_ID_CAMERA_ZSD:
            OV9724Capture(pImageWindow, pSensorConfigData);
            break;

        default:
            return ERROR_INVALID_SCENARIO_ID;

    }
    return ERROR_NONE;
} /* OV9724Control() */



kal_uint32 OV9724_SET_FrameLength_ByVideoMode(UINT16 Video_TargetFps)
{
    UINT32 frameRate = 0;
	kal_uint32 MIN_FrameLength=0;
	
	if(OV9724.OV9724AutoFlickerMode == KAL_TRUE)
	{
		if (Video_TargetFps==30)
			frameRate= OV9724_AUTOFLICKER_OFFSET_30;
		else if(Video_TargetFps==15)
			frameRate= OV9724_AUTOFLICKER_OFFSET_15;
		else
			frameRate=Video_TargetFps*10;
	
		MIN_FrameLength = (OV9724.videoPclk*10000)/(OV9724_VIDEO_PERIOD_PIXEL_NUMS + OV9724.DummyPixels)/frameRate*10;
	}
	else
		MIN_FrameLength = (OV9724.videoPclk*10000) /(OV9724_VIDEO_PERIOD_PIXEL_NUMS + OV9724.DummyPixels)/Video_TargetFps;

	//for some seldom issue while changing mode; 60hz have higher probability;
    if(MIN_FrameLength <(OV9724.shutter +OV9724_SHUTTER_MARGIN))
    {
		MIN_FrameLength = OV9724.shutter + OV9724_SHUTTER_MARGIN;

	}

     return MIN_FrameLength;

}



UINT32 OV9724SetVideoMode(UINT16 u2FrameRate)
{

    kal_uint32 MIN_Frame_length =0,frameRate=0,extralines=0;
    OV9724DB("[OV9724SetVideoMode] frame rate = %d\n", u2FrameRate);

	spin_lock(&OV9724mipiraw_drv_lock);
	OV9724_VIDEO_MODE_TARGET_FPS=u2FrameRate;
	spin_unlock(&OV9724mipiraw_drv_lock);

	if(u2FrameRate==0)
	{
		OV9724DB("Disable Video Mode or dynimac fps\n");
		return KAL_TRUE;
	}
	if(u2FrameRate >30 || u2FrameRate <5)
	    OV9724DB("abmornal frame rate seting,pay attention~\n");

    if(OV9724.sensorMode == SENSOR_MODE_VIDEO)//video ScenarioId recording
    {

        MIN_Frame_length = OV9724_SET_FrameLength_ByVideoMode(u2FrameRate);

		if((MIN_Frame_length <=OV9724_VIDEO_PERIOD_LINE_NUMS))
		{
			MIN_Frame_length = OV9724_VIDEO_PERIOD_LINE_NUMS;
			OV9724DB("[OV9724SetVideoMode]config LINE_NUMS < Org_setting ,please check it.\n");
		}
		OV9724DB("[OV9724SetVideoMode]current fps (10 base)= %d\n", (OV9724.videoPclk*10000)*10/(OV9724_VIDEO_PERIOD_PIXEL_NUMS + OV9724.DummyPixels)/MIN_Frame_length);
		extralines = MIN_Frame_length - OV9724_VIDEO_PERIOD_LINE_NUMS;
		
		spin_lock(&OV9724mipiraw_drv_lock);
		OV9724.DummyPixels = 0;//define dummy pixels and lines
		OV9724.DummyLines = extralines ;
		spin_unlock(&OV9724mipiraw_drv_lock);
		
		OV9724_SetDummy(OV9724.DummyPixels,extralines);
    }
	
	OV9724DB("[OV9724SetVideoMode]MIN_Frame_length=%d,OV9724.DummyLines=%d\n",MIN_Frame_length,OV9724.DummyLines);

    return KAL_TRUE;
}


UINT32 OV9724SetAutoFlickerMode(kal_bool bEnable, UINT16 u2FrameRate)
{

	if(bEnable) {   
		spin_lock(&OV9724mipiraw_drv_lock);
		OV9724.OV9724AutoFlickerMode = KAL_TRUE;
		spin_unlock(&OV9724mipiraw_drv_lock);
        OV9724DB("OV9724 Enable Auto flicker\n");
    } else {
    	spin_lock(&OV9724mipiraw_drv_lock);
        OV9724.OV9724AutoFlickerMode = KAL_FALSE;
		spin_unlock(&OV9724mipiraw_drv_lock);
        OV9724DB("OV9724 Disable Auto flicker\n");
    }

    return ERROR_NONE;
}


UINT32 OV9724SetTestPatternMode(kal_bool bEnable)
{
    OV9724DB("[OV9724SetTestPatternMode] Test pattern enable:%d\n", bEnable);
    if(bEnable == KAL_TRUE)
    {
        OV9724_During_testpattern = KAL_TRUE;
		OV9724_write_cmos_sensor(0x0601,0x02);
    }
	else
	{
        OV9724_During_testpattern = KAL_FALSE;
		OV9724_write_cmos_sensor(0x0601,0x00);
	}

    return ERROR_NONE;
}


/*************************************************************************
*
* DESCRIPTION:
* INTERFACE FUNCTION, FOR USER TO SET MAX  FRAMERATE;
* 
*************************************************************************/
UINT32 OV9724MIPISetMaxFramerateByScenario(MSDK_SCENARIO_ID_ENUM scenarioId, MUINT32 frameRate) {
	kal_uint32 pclk;
	kal_int16 dummyLine;
	kal_uint16 lineLength,frameHeight;
		
	OV9724DB("OV9724MIPISetMaxFramerateByScenario: scenarioId = %d, frame rate = %d\n",scenarioId,frameRate);
	switch (scenarioId) {
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
			pclk = OV9724_PREVIEW_PCLK;
			lineLength = OV9724_PV_PERIOD_PIXEL_NUMS;
			frameHeight = (10 * pclk)/frameRate/lineLength;
			dummyLine = frameHeight - OV9724_PV_PERIOD_LINE_NUMS;
			if(dummyLine<0)
				dummyLine = 0;
			spin_lock(&OV9724mipiraw_drv_lock);
			OV9724.sensorMode = SENSOR_MODE_PREVIEW;
			spin_unlock(&OV9724mipiraw_drv_lock);
			OV9724_SetDummy(0, dummyLine);			
			break;			
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			pclk = OV9724_VIDEO_PCLK;
			lineLength = OV9724_VIDEO_PERIOD_PIXEL_NUMS;
			frameHeight = (10 * pclk)/frameRate/lineLength;
			dummyLine = frameHeight - OV9724_VIDEO_PERIOD_LINE_NUMS;
			if(dummyLine<0)
				dummyLine = 0;
			spin_lock(&OV9724mipiraw_drv_lock);
			OV9724.sensorMode = SENSOR_MODE_VIDEO;
			spin_unlock(&OV9724mipiraw_drv_lock);
			OV9724_SetDummy(0, dummyLine);			
			break;			
			 break;
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		case MSDK_SCENARIO_ID_CAMERA_ZSD:			
			pclk = OV9724_CAPTURE_PCLK;
			lineLength = OV9724_FULL_PERIOD_PIXEL_NUMS;
			frameHeight = (10 * pclk)/frameRate/lineLength;
			dummyLine = frameHeight - OV9724_FULL_PERIOD_LINE_NUMS;
			if(dummyLine<0)
				dummyLine = 0;
			spin_lock(&OV9724mipiraw_drv_lock);
			OV9724.sensorMode = SENSOR_MODE_CAPTURE;
			spin_unlock(&OV9724mipiraw_drv_lock);
			OV9724_SetDummy(0, dummyLine);			
			break;		
        case MSDK_SCENARIO_ID_CAMERA_3D_PREVIEW:
            break;
        case MSDK_SCENARIO_ID_CAMERA_3D_VIDEO:
			break;
        case MSDK_SCENARIO_ID_CAMERA_3D_CAPTURE:   
			break;		
		default:
			break;
	}	
	return ERROR_NONE;
}


UINT32 OV9724MIPIGetDefaultFramerateByScenario(MSDK_SCENARIO_ID_ENUM scenarioId, MUINT32 *pframeRate) 
{

	switch (scenarioId) {
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			 *pframeRate = OV9724_MAX_FPS_PREVIEW;
			 break;
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		case MSDK_SCENARIO_ID_CAMERA_ZSD:
			 *pframeRate = OV9724_MAX_FPS_CAPTURE;
			break;		
        case MSDK_SCENARIO_ID_CAMERA_3D_PREVIEW: //added
        case MSDK_SCENARIO_ID_CAMERA_3D_VIDEO:
        case MSDK_SCENARIO_ID_CAMERA_3D_CAPTURE: //added   
			 *pframeRate = OV9724_MAX_FPS_CAPTURE;
			break;		
		default:
			break;
	}

	return ERROR_NONE;
}


UINT32 OV9724FeatureControl(MSDK_SENSOR_FEATURE_ENUM FeatureId,
                                                                UINT8 *pFeaturePara,UINT32 *pFeatureParaLen)
{
    UINT16 *pFeatureReturnPara16=(UINT16 *) pFeaturePara;
    UINT16 *pFeatureData16=(UINT16 *) pFeaturePara;
    UINT32 *pFeatureReturnPara32=(UINT32 *) pFeaturePara;
    UINT32 *pFeatureData32=(UINT32 *) pFeaturePara;
    UINT32 SensorRegNumber;
    UINT32 i;
    PNVRAM_SENSOR_DATA_STRUCT pSensorDefaultData=(PNVRAM_SENSOR_DATA_STRUCT) pFeaturePara;
    MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData=(MSDK_SENSOR_CONFIG_STRUCT *) pFeaturePara;
    MSDK_SENSOR_REG_INFO_STRUCT *pSensorRegData=(MSDK_SENSOR_REG_INFO_STRUCT *) pFeaturePara;
    MSDK_SENSOR_GROUP_INFO_STRUCT *pSensorGroupInfo=(MSDK_SENSOR_GROUP_INFO_STRUCT *) pFeaturePara;
    MSDK_SENSOR_ITEM_INFO_STRUCT *pSensorItemInfo=(MSDK_SENSOR_ITEM_INFO_STRUCT *) pFeaturePara;
    MSDK_SENSOR_ENG_INFO_STRUCT	*pSensorEngInfo=(MSDK_SENSOR_ENG_INFO_STRUCT *) pFeaturePara;

    switch (FeatureId)
    {
        case SENSOR_FEATURE_GET_RESOLUTION:
            *pFeatureReturnPara16++= OV9724_IMAGE_SENSOR_FULL_WIDTH;
            *pFeatureReturnPara16= OV9724_IMAGE_SENSOR_FULL_HEIGHT;
            *pFeatureParaLen=4;
            break;
        case SENSOR_FEATURE_GET_PERIOD:
				*pFeatureReturnPara16++= OV9724_FeatureControl_PERIOD_PixelNum;
				*pFeatureReturnPara16= OV9724_FeatureControl_PERIOD_LineNum;
				*pFeatureParaLen=4;
				break;
        case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ:
			switch(OV9724CurrentScenarioId)
			{
				case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
					*pFeatureReturnPara32 = OV9724_PREVIEW_PCLK;
					*pFeatureParaLen=4;
					break;
				case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
					*pFeatureReturnPara32 = OV9724_VIDEO_PCLK;
					*pFeatureParaLen=4;
					break;
				case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
				case MSDK_SCENARIO_ID_CAMERA_ZSD:
					*pFeatureReturnPara32 = OV9724_CAPTURE_PCLK;
					*pFeatureParaLen=4;
					break;
				default:
					*pFeatureReturnPara32 = OV9724_PREVIEW_PCLK;
					*pFeatureParaLen=4;
					break;
			}
		    break;

        case SENSOR_FEATURE_SET_ESHUTTER:
            OV9724_SetShutter(*pFeatureData16);
            break;
        case SENSOR_FEATURE_SET_NIGHTMODE:
            OV9724_NightMode((BOOL) *pFeatureData16);
            break;
        case SENSOR_FEATURE_SET_GAIN:  
           OV9724_SetGain((UINT16) *pFeatureData16);
            break;
        case SENSOR_FEATURE_SET_FLASHLIGHT:
            break;
        case SENSOR_FEATURE_SET_ISP_MASTER_CLOCK_FREQ:
            //OV9724_isp_master_clock=*pFeatureData32;
            break;
        case SENSOR_FEATURE_SET_REGISTER:
            OV9724_write_cmos_sensor(pSensorRegData->RegAddr, pSensorRegData->RegData);
            break;
        case SENSOR_FEATURE_GET_REGISTER:
            pSensorRegData->RegData = OV9724_read_cmos_sensor(pSensorRegData->RegAddr);
            break;
        case SENSOR_FEATURE_SET_CCT_REGISTER:
            SensorRegNumber=FACTORY_END_ADDR;
            for (i=0;i<SensorRegNumber;i++)
            {
            	spin_lock(&OV9724mipiraw_drv_lock);
                OV9724SensorCCT[i].Addr=*pFeatureData32++;
                OV9724SensorCCT[i].Para=*pFeatureData32++;
				spin_unlock(&OV9724mipiraw_drv_lock);
            }
            break;
        case SENSOR_FEATURE_GET_CCT_REGISTER:
            SensorRegNumber=FACTORY_END_ADDR;
            if (*pFeatureParaLen<(SensorRegNumber*sizeof(SENSOR_REG_STRUCT)+4))
                return FALSE;
            *pFeatureData32++=SensorRegNumber;
            for (i=0;i<SensorRegNumber;i++)
            {
                *pFeatureData32++=OV9724SensorCCT[i].Addr;
                *pFeatureData32++=OV9724SensorCCT[i].Para;
            }
            break;
        case SENSOR_FEATURE_SET_ENG_REGISTER:
            SensorRegNumber=ENGINEER_END;
            for (i=0;i<SensorRegNumber;i++)
            {
            	spin_lock(&OV9724mipiraw_drv_lock);
                OV9724SensorReg[i].Addr=*pFeatureData32++;
                OV9724SensorReg[i].Para=*pFeatureData32++;
				spin_unlock(&OV9724mipiraw_drv_lock);
            }
            break;
        case SENSOR_FEATURE_GET_ENG_REGISTER:
            SensorRegNumber=ENGINEER_END;
            if (*pFeatureParaLen<(SensorRegNumber*sizeof(SENSOR_REG_STRUCT)+4))
                return FALSE;
            *pFeatureData32++=SensorRegNumber;
            for (i=0;i<SensorRegNumber;i++)
            {
                *pFeatureData32++=OV9724SensorReg[i].Addr;
                *pFeatureData32++=OV9724SensorReg[i].Para;
            }
            break;
        case SENSOR_FEATURE_GET_REGISTER_DEFAULT:
            if (*pFeatureParaLen>=sizeof(NVRAM_SENSOR_DATA_STRUCT))
            {
                pSensorDefaultData->Version=NVRAM_CAMERA_SENSOR_FILE_VERSION;
                pSensorDefaultData->SensorId=OV9724_SENSOR_ID;
                memcpy(pSensorDefaultData->SensorEngReg, OV9724SensorReg, sizeof(SENSOR_REG_STRUCT)*ENGINEER_END);
                memcpy(pSensorDefaultData->SensorCCTReg, OV9724SensorCCT, sizeof(SENSOR_REG_STRUCT)*FACTORY_END_ADDR);
            }
            else
                return FALSE;
            *pFeatureParaLen=sizeof(NVRAM_SENSOR_DATA_STRUCT);
            break;
        case SENSOR_FEATURE_GET_CONFIG_PARA:
            memcpy(pSensorConfigData, &OV9724SensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
            *pFeatureParaLen=sizeof(MSDK_SENSOR_CONFIG_STRUCT);
            break;
        case SENSOR_FEATURE_CAMERA_PARA_TO_SENSOR:
            OV9724_camera_para_to_sensor();
            break;

        case SENSOR_FEATURE_SENSOR_TO_CAMERA_PARA:
            OV9724_sensor_to_camera_para();
            break;
        case SENSOR_FEATURE_GET_GROUP_COUNT:
            *pFeatureReturnPara32++=OV9724_get_sensor_group_count();
            *pFeatureParaLen=4;
            break;
        case SENSOR_FEATURE_GET_GROUP_INFO:
            OV9724_get_sensor_group_info(pSensorGroupInfo->GroupIdx, pSensorGroupInfo->GroupNamePtr, &pSensorGroupInfo->ItemCount);
            *pFeatureParaLen=sizeof(MSDK_SENSOR_GROUP_INFO_STRUCT);
            break;
        case SENSOR_FEATURE_GET_ITEM_INFO:
            OV9724_get_sensor_item_info(pSensorItemInfo->GroupIdx,pSensorItemInfo->ItemIdx, pSensorItemInfo);
            *pFeatureParaLen=sizeof(MSDK_SENSOR_ITEM_INFO_STRUCT);
            break;

        case SENSOR_FEATURE_SET_ITEM_INFO:
            OV9724_set_sensor_item_info(pSensorItemInfo->GroupIdx, pSensorItemInfo->ItemIdx, pSensorItemInfo->ItemValue);
            *pFeatureParaLen=sizeof(MSDK_SENSOR_ITEM_INFO_STRUCT);
            break;

        case SENSOR_FEATURE_GET_ENG_INFO:
            pSensorEngInfo->SensorId = OV9724_SENSOR_ID;
            pSensorEngInfo->SensorType = CMOS_SENSOR;
            pSensorEngInfo->SensorOutputDataFormat=SENSOR_OUTPUT_FORMAT_RAW_B;
            *pFeatureParaLen=sizeof(MSDK_SENSOR_ENG_INFO_STRUCT);
            break;
        case SENSOR_FEATURE_GET_LENS_DRIVER_ID:
            // get the lens driver ID from EEPROM or just return LENS_DRIVER_ID_DO_NOT_CARE
            // if EEPROM does not exist in camera module.
            *pFeatureReturnPara32=LENS_DRIVER_ID_DO_NOT_CARE;
            *pFeatureParaLen=4;
            break;

        case SENSOR_FEATURE_INITIALIZE_AF:
            break;
        case SENSOR_FEATURE_CONSTANT_AF:
            break;
        case SENSOR_FEATURE_MOVE_FOCUS_LENS:
            break;
        case SENSOR_FEATURE_SET_VIDEO_MODE:
            OV9724SetVideoMode(*pFeatureData16);
            break;
        case SENSOR_FEATURE_CHECK_SENSOR_ID:
            OV9724GetSensorID(pFeatureReturnPara32);
            break;
        case SENSOR_FEATURE_SET_AUTO_FLICKER_MODE:
            OV9724SetAutoFlickerMode((BOOL)*pFeatureData16, *(pFeatureData16+1));
	        break;
		case SENSOR_FEATURE_SET_MAX_FRAME_RATE_BY_SCENARIO:
			OV9724MIPISetMaxFramerateByScenario((MSDK_SCENARIO_ID_ENUM)*pFeatureData32, *(pFeatureData32+1));
			break;
		case SENSOR_FEATURE_GET_DEFAULT_FRAME_RATE_BY_SCENARIO:
			OV9724MIPIGetDefaultFramerateByScenario((MSDK_SCENARIO_ID_ENUM)*pFeatureData32, (MUINT32 *)(*(pFeatureData32+1)));
			break;
		case SENSOR_FEATURE_SET_TEST_PATTERN:
			OV9724SetTestPatternMode((BOOL)*pFeatureData16);
			break;
		case SENSOR_FEATURE_GET_TEST_PATTERN_CHECKSUM_VALUE://for factory mode auto testing 			
			*pFeatureReturnPara32=OV9724_TEST_PATTERN_CHECKSUM; 		  
			*pFeatureParaLen=4; 							
		     break;
        default:
            break;
    }
    return ERROR_NONE;
}	


SENSOR_FUNCTION_STRUCT	SensorFuncOV9724=
{
    OV9724Open,
    OV9724GetInfo,
    OV9724GetResolution,
    OV9724FeatureControl,
    OV9724Control,
    OV9724Close
};

UINT32 OV9724_MIPI_RAW_SensorInit(PSENSOR_FUNCTION_STRUCT *pfFunc)
{
    if (pfFunc!=NULL)
        *pfFunc=&SensorFuncOV9724;

    return ERROR_NONE;
}  

