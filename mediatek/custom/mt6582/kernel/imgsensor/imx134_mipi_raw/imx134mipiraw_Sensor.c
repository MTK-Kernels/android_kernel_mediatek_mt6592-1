#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <asm/atomic.h>
#include <asm/system.h>


#include "kd_camera_hw.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"

#include "imx134mipiraw_Sensor.h"
#include "imx134mipiraw_Camera_Sensor_para.h"
#include "imx134mipiraw_CameraCustomized.h"
#include <linux/videodev2.h>
#include <linux/proc_fs.h> 
#include <linux/dma-mapping.h>

kal_bool  IMX134MIPI_MPEG4_encode_mode = KAL_FALSE;
kal_bool IMX134MIPI_Auto_Flicker_mode = KAL_FALSE;


kal_uint8 IMX134MIPI_sensor_write_I2C_address = IMX134MIPI_WRITE_ID;
kal_uint8 IMX134MIPI_sensor_read_I2C_address = IMX134MIPI_READ_ID;
#define IMX134MIPI_SENSOR_ID            0x134

	
static struct IMX134MIPI_sensor_STRUCT IMX134MIPI_sensor={
	IMX134MIPI_WRITE_ID,                                              //kal_uint16 i2c_write_id;                                                                                   
	IMX134MIPI_READ_ID,                                               //kal_uint16 i2c_read_id;                                              
	KAL_TRUE,                                                         //kal_bool first_init;                                                 
	KAL_FALSE,                                                        //kal_bool fix_video_fps;                                              
	KAL_TRUE,                                                         //kal_bool pv_mode;                                                    
	KAL_FALSE,                                                        //kal_bool video_mode; 				                                         
	KAL_FALSE,                                                          //kal_bool capture_mode; 				//True: Preview Mode; False: Capture Mo
	KAL_FALSE,                                                          //kal_bool night_mode;				//True: Night Mode; False: Auto Mode     
	KAL_FALSE,                                                          //kal_uint8 mirror_flip;                                               
	232000000,//180000000,//200000000,//240000000,                                                          //kal_uint32 pv_pclk;				//Preview Pclk                                    
	232000000,                                                          //kal_uint32 video_pclk;				//video Pclk                                  
	232000000,                                                          //kal_uint32 cp_pclk;				//Capture Pclk                                    
	0,                                                                  //kal_uint32 pv_shutter;		                                                  
	0,                                                                  //kal_uint32 video_shutter;		                                                
	0,                                                                  //kal_uint32 cp_shutter;                                                      
	64,                                                                 //kal_uint32 pv_gain;                                                         
	64,                                                                 //kal_uint32 video_gain;                                                      
	64,                                                                 //kal_uint32 cp_gain;                                                         
	IMX134MIPI_PV_LINE_LENGTH_PIXELS,                                   //kal_uint32 pv_line_length;                                                  
	IMX134MIPI_PV_FRAME_LENGTH_LINES,                                   //kal_uint32 pv_frame_length;                                                 
	IMX134MIPI_VIDEO_LINE_LENGTH_PIXELS,                                //kal_uint32 video_line_length;                                               
	IMX134MIPI_VIDEO_FRAME_LENGTH_LINES,                                //kal_uint32 video_frame_length;                                              
	IMX134MIPI_FULL_LINE_LENGTH_PIXELS,                                 //kal_uint32 cp_line_length;                                                  
	IMX134MIPI_FULL_FRAME_LENGTH_LINES,                                 //kal_uint32 cp_frame_length;                                                 
	0,                                                                  //kal_uint16 pv_dummy_pixels;		   //Dummy Pixels:must be 12s                 
	0,                                                                  //kal_uint16 pv_dummy_lines;		   //Dummy Lines                              
	0,                                                                  //kal_uint16 video_dummy_pixels;		   //Dummy Pixels:must be 12s             
	0,                                                                  //kal_uint16 video_dummy_lines;		   //Dummy Lines                            
	0,                                                                  //kal_uint16 cp_dummy_pixels;		   //Dummy Pixels:must be 12s                 
	0,                                                                  //kal_uint16 cp_dummy_lines;		   //Dummy Lines			                        
	30                                                                  //kal_uint16 video_current_frame_rate;                                        
	};                                                                           
MSDK_SCENARIO_ID_ENUM IMX134CurrentScenarioId = MSDK_SCENARIO_ID_CAMERA_PREVIEW;	 
kal_uint16	IMX134MIPI_sensor_gain_base=0x0;                                 
/* MAX/MIN Explosure Lines Used By AE Algorithm */                           
kal_uint16 IMX134MIPI_MAX_EXPOSURE_LINES = IMX134MIPI_PV_FRAME_LENGTH_LINES-4;//650;
kal_uint8  IMX134MIPI_MIN_EXPOSURE_LINES = 2;                                
kal_uint32 IMX134MIPI_isp_master_clock;
static DEFINE_SPINLOCK(imx134_drv_lock);

#define SENSORDB(fmt, arg...) printk( "[IMX134MIPIRaw] "  fmt, ##arg)
#define RETAILMSG(x,...)
#define IMX134_TEST_PATTERN_CHECKSUM (0x1ec5153d)
#define TEXT
UINT8 IMX134MIPIPixelClockDivider=0;
kal_uint16 IMX134MIPI_sensor_id=0;
MSDK_SENSOR_CONFIG_STRUCT IMX134MIPISensorConfigData;
kal_uint32 IMX134MIPI_FAC_SENSOR_REG;
kal_uint16 IMX134MIPI_sensor_flip_value; 
#define IMX134MIPI_MaxGainIndex (105)
kal_uint16 IMX134MIPI_sensorGainMapping[IMX134MIPI_MaxGainIndex][2] ={
{ 64 ,0  },   
{ 68 ,12 },   
{ 71 ,23 },   
{ 74 ,33 },   
{ 77 ,42 },   
{ 81 ,52 },   
{ 84 ,59 },   
{ 87 ,66 },   
{ 90 ,73 },   
{ 93 ,79 },   
{ 96 ,85 },   
{ 100,91 },   
{ 103,96 },   
{ 106,101},   
{ 109,105},   
{ 113,110},   
{ 116,114},   
{ 120,118},   
{ 122,121},   
{ 125,125},   
{ 128,128},   
{ 132,131},   
{ 135,134},   
{ 138,137},
{ 141,139},
{ 144,142},   
{ 148,145},   
{ 151,147},   
{ 153,149}, 
{ 157,151},
{ 160,153},      
{ 164,156},   
{ 168,158},   
{ 169,159},   
{ 173,161},   
{ 176,163},   
{ 180,165}, 
{ 182,166},   
{ 187,168},
{ 189,169},
{ 193,171},
{ 196,172},
{ 200,174},
{ 203,175}, 
{ 205,176},
{ 208,177}, 
{ 213,179}, //134==>179 yyf
{ 216,180},  
{ 219,181},   
{ 222,182},
{ 225,183},  
{ 228,184},   
{ 232,185},
{ 235,186},
{ 238,187},
{ 241,188},
{ 245,189},
{ 249,190},
{ 253,191},
{ 256,192}, 
{ 260,193},
{ 265,194},
{ 269,195},
{ 274,196},   
{ 278,197},
{ 283,198},
{ 288,199},
{ 293,200},
{ 298,201},   
{ 304,202},   
{ 310,203},
{ 315,204},
{ 322,205},   
{ 328,206},   
{ 335,207},   
{ 342,208},   
{ 349,209},   
{ 357,210},   
{ 365,211},   
{ 373,212}, 
{ 381,213},
{ 400,215},      
{ 420,217},   
{ 432,218},   
{ 443,219},      
{ 468,221},   
{ 482,222},   
{ 497,223},   
{ 512,224},
{ 529,225}, 	 
{ 546,226},   
{ 566,227},   
{ 585,228}, 	 
{ 607,229},   
{ 631,230},   
{ 656,231},   
{ 683,232},
{712,233},
{745,234},
{780,235},
{819,236},
{862,237},
{910,238},
{964,239},
{1024,240}
};


/* FIXME: old factors and DIDNOT use now. s*/
SENSOR_REG_STRUCT IMX134MIPISensorCCT[]=CAMERA_SENSOR_CCT_DEFAULT_VALUE;
SENSOR_REG_STRUCT IMX134MIPISensorReg[ENGINEER_END]=CAMERA_SENSOR_REG_DEFAULT_VALUE;
/* FIXME: old factors and DIDNOT use now. e*/
extern int iReadReg(u16 a_u2Addr , u8 * a_puBuff , u16 i2cId);
extern int iWriteReg(u16 a_u2Addr , u32 a_u4Data , u32 a_u4Bytes , u16 i2cId);
extern int iMultiWriteReg(u8 *pData, u16 lens, u16 i2cId);

#define IMX134MIPI_write_cmos_sensor(addr, para) iWriteReg((u16) addr , (u32) para , 1, IMX134MIPI_WRITE_ID)
#define imx134_multi_write_cmos_sensor(pData, lens) iMultiWriteReg((u8*) pData, (u16) lens, IMX134MIPI_WRITE_ID)

static kal_uint8 imx134_init[] = {
  	0x01,0x01,0x00,
  	0x01,0x05,0x01,
  	0x01,0x10,0x00,
  	0x02,0x20,0x01,
  	0x33,0x02,0x11,
  	0x38,0x33,0x20,
  	0x38,0x93,0x00,
  	0x39,0x06,0x08,
  	0x39,0x07,0x01,
  	0x39,0x1B,0x01,
  	0x3C,0x09,0x01,
  	0x60,0x0A,0x00,
  	0x30,0x08,0xB0,
  	0x32,0x0A,0x01,
  	0x32,0x0D,0x10,
  	0x32,0x16,0x2E,
  	0x32,0x2C,0x02,
  	0x34,0x09,0x0C,
  	0x34,0x0C,0x2D,
  	0x34,0x11,0x39,
  	0x34,0x14,0x1E,
  	0x34,0x27,0x04,
  	0x34,0x80,0x1E,
  	0x34,0x84,0x1E,
  	0x34,0x88,0x1E,
  	0x34,0x8C,0x1E,
  	0x34,0x90,0x1E,
  	0x34,0x94,0x1E,
  	0x35,0x11,0x8F,
  	0x36,0x17,0x2D,
  	0x43,0x34,0x03,
  	0x43,0x35,0x20,
  	0x43,0x36,0x03,
  	0x43,0x37,0x84,
  	0x43,0x64,0x0B,
  	0x43,0x6D,0x00,
  	0x43,0x6F,0x06,
  	0x42,0x81,0x21,
  	0x42,0x83,0x04,
  	0x42,0x84,0x08,
  	0x42,0x87,0x7F,
  	0x42,0x88,0x08,
  	0x42,0x8B,0x7F,
  	0x42,0x8C,0x08,
  	0x42,0x8F,0x7F,
  	0x42,0x97,0x00,
  	0x42,0x98,0x7E,
  	0x42,0x99,0x7E,
  	0x42,0x9A,0x7E,
  	0x42,0xA4,0xFB,
  	0x42,0xA5,0x7E,
  	0x42,0xA6,0xDF,
  	0x42,0xA7,0xB7,
  	0x42,0xAF,0x03,
  	0x42,0x07,0x03,
  	0x42,0x16,0x08,
  	0x42,0x17,0x08,
  	0x42,0x1B,0x20,
  	0x42,0x2E,0x54,
  	0x42,0x30,0xFF,
  	0x42,0x31,0xFE,
  	0x42,0x32,0xFF,
  	0x42,0x35,0x58,
  	0x42,0x36,0xF7,
  	0x42,0x37,0xFD,
  	0x42,0x39,0x4E,
  	0x42,0x3A,0xFC,
  	0x42,0x3B,0xFD,
  	0x43,0x00,0x00,
  	0x43,0x16,0x12,
  	0x43,0x17,0x22,
  	0x43,0x1A,0x00,
  	0x43,0x2B,0x20,
  	0x43,0x2D,0x01,
  	0x43,0x5E,0x01,
  	0x43,0x5F,0xFF,
  	0x44,0x01,0x3F,
  	0x44,0x12,0x3F,
  	0x44,0x13,0xFF,
  	0x44,0x1D,0x40,
  	0x44,0x1E,0x1E,
  	0x44,0x1F,0x38,
  	0x44,0x46,0x1D,
  	0x44,0x47,0xF9,
  	0x44,0x52,0x00,
  	0x44,0x53,0xA0,
  	0x44,0x54,0x08,
  	0x44,0x55,0x00,
  	0x44,0x58,0x18,
  	0x44,0x59,0x18,
  	0x44,0x5A,0x3F,
  	0x44,0x5B,0x3A,
  	0x44,0x5D,0x28,
  	0x44,0x5F,0x90,
  	0x44,0x63,0x00,
	0x44,0x65,0x00,      		                              
	0x45,0x2A,0x02};

kal_uint16 IMX134MIPI_read_cmos_sensor(kal_uint32 addr)
{
	kal_uint16 get_byte=0;
    iReadReg((u16) addr ,(u8*)&get_byte,IMX134MIPI_WRITE_ID);
    return get_byte;
}

void IMX134MIPI_write_shutter(kal_uint16 shutter)
{
	kal_uint32 frame_length = 0,line_length=0;
        kal_uint32 extra_lines = 0;
	kal_uint32 max_exp_shutter = 0;
	unsigned long flags;	
	//SENSORDB("[IMX134MIPI]enter IMX134MIPI_write_shutter function\n"); 
    if (IMX134MIPI_sensor.pv_mode == KAL_TRUE) 
	 {
	   max_exp_shutter = IMX134MIPI_PV_FRAME_LENGTH_LINES + IMX134MIPI_sensor.pv_dummy_lines-4;
     }
     else if (IMX134MIPI_sensor.video_mode== KAL_TRUE) 
     {
       max_exp_shutter = IMX134MIPI_VIDEO_FRAME_LENGTH_LINES + IMX134MIPI_sensor.video_dummy_lines-4;
	 }	 
     else if (IMX134MIPI_sensor.capture_mode== KAL_TRUE) 
     {
       max_exp_shutter = IMX134MIPI_FULL_FRAME_LENGTH_LINES + IMX134MIPI_sensor.cp_dummy_lines-4;
	 }	 
	 else
	 	{
	 	
		SENSORDB("sensor mode error\n");
	 	}
	 
	 if(shutter > max_exp_shutter)
	   extra_lines = shutter - max_exp_shutter;
	 else 
	   extra_lines = 0;
	 if (IMX134MIPI_sensor.pv_mode == KAL_TRUE) 
	 {
       frame_length =IMX134MIPI_PV_FRAME_LENGTH_LINES+ IMX134MIPI_sensor.pv_dummy_lines + extra_lines;
	   line_length = IMX134MIPI_PV_LINE_LENGTH_PIXELS+ IMX134MIPI_sensor.pv_dummy_pixels;
	   spin_lock_irqsave(&imx134_drv_lock,flags);
	   IMX134MIPI_sensor.pv_line_length = line_length;
	   IMX134MIPI_sensor.pv_frame_length = frame_length;
	   spin_unlock_irqrestore(&imx134_drv_lock,flags);
	 }
	 else if (IMX134MIPI_sensor.video_mode== KAL_TRUE) 
     {
	    frame_length = IMX134MIPI_VIDEO_FRAME_LENGTH_LINES+ IMX134MIPI_sensor.video_dummy_lines + extra_lines;
		line_length =IMX134MIPI_VIDEO_LINE_LENGTH_PIXELS + IMX134MIPI_sensor.video_dummy_pixels;
		spin_lock_irqsave(&imx134_drv_lock,flags);
		IMX134MIPI_sensor.video_line_length = line_length;
	    IMX134MIPI_sensor.video_frame_length = frame_length;
		spin_unlock_irqrestore(&imx134_drv_lock,flags);
	 } 
	 else if(IMX134MIPI_sensor.capture_mode== KAL_TRUE)
	 	{
	    frame_length = IMX134MIPI_FULL_FRAME_LENGTH_LINES+ IMX134MIPI_sensor.cp_dummy_lines + extra_lines;
		line_length =IMX134MIPI_FULL_LINE_LENGTH_PIXELS + IMX134MIPI_sensor.cp_dummy_pixels;
		spin_lock_irqsave(&imx134_drv_lock,flags);
		IMX134MIPI_sensor.cp_line_length = line_length;
	    IMX134MIPI_sensor.cp_frame_length = frame_length;
		spin_unlock_irqrestore(&imx134_drv_lock,flags);
	 }
	 else
	 	{
	 	
		SENSORDB("sensor mode error\n");
	 	}
	//IMX134MIPI_write_cmos_sensor(0x0100,0x00);// STREAM STop
	    IMX134MIPI_write_cmos_sensor(0x0104, 1);        
	    IMX134MIPI_write_cmos_sensor(0x0340, (frame_length >>8) & 0xFF);
            IMX134MIPI_write_cmos_sensor(0x0341, frame_length & 0xFF);	  
            IMX134MIPI_write_cmos_sensor(0x0202, (shutter >> 8) & 0xFF);
            IMX134MIPI_write_cmos_sensor(0x0203, shutter  & 0xFF);
            IMX134MIPI_write_cmos_sensor(0x0104, 0);    
	
   // SENSORDB("[IMX134MIPI]The Register shutter read from sensor is %d \n",shutter);
   // SENSORDB("[IMX134MIPI]exit IMX134MIPI_write_shutter function\n");
}   /* write_IMX134MIPI_shutter */

static kal_uint16 IMX134MIPIReg2Gain(const kal_uint8 iReg)
{
	SENSORDB("[IMX134MIPI]enter IMX134MIPIReg2Gain function\n");
    kal_uint8 iI;
    // Range: 1x to 8x
    for (iI = 0; iI < IMX134MIPI_MaxGainIndex; iI++) 
	{
        if(iReg <=IMX134MIPI_sensorGainMapping[iI][1])
		{
            break;
        }
		if(iReg == IMX134MIPI_sensorGainMapping[iI][1])			
		{			
			return IMX134MIPI_sensorGainMapping[iI][0];
		}    
    }
	SENSORDB("[IMX134MIPI]exit IMX134MIPIReg2Gain function\n");
    return IMX134MIPI_sensorGainMapping[iI-1][0];
}
static kal_uint8 IMX134MIPIGain2Reg(const kal_uint16 iGain)
{
	kal_uint8 iI;
    SENSORDB("[IMX134MIPI]enter IMX134MIPIGain2Reg function\n");
    for (iI = 0; iI <= (IMX134MIPI_MaxGainIndex-1); iI++) 
	{
        if(iGain <IMX134MIPI_sensorGainMapping[iI][0])
		{    
            break;
        }
		if(iGain < IMX134MIPI_sensorGainMapping[iI][0])
		{                
			return IMX134MIPI_sensorGainMapping[iI][1];       
		}
			
    }
    if(iGain != IMX134MIPI_sensorGainMapping[iI][0])
    {
         printk("[IMX134MIPIGain2Reg] Gain mapping don't correctly:%d %d \n", iGain, IMX134MIPI_sensorGainMapping[iI][0]);
    }
	SENSORDB("[IMX134MIPI]exit IMX134MIPIGain2Reg function\n");
	
    return IMX134MIPI_sensorGainMapping[iI-1][1];
	//return NONE;
}

/*************************************************************************
* FUNCTION
*    IMX134MIPI_SetGain
*
* DESCRIPTION
*    This function is to set global gain to sensor.
*
* PARAMETERS
*    gain : sensor global gain(base: 0x40)
*
* RETURNS
*    the actually gain set to sensor.
*
* GLOBALS AFFECTED
*
*************************************************************************/
void IMX134MIPI_SetGain(UINT16 iGain)
{   
    kal_uint8 iReg;
	SENSORDB("[IMX134MIPI]enter IMX134MIPI_SetGain function\n");
    iReg = IMX134MIPIGain2Reg(iGain);
	IMX134MIPI_write_cmos_sensor(0x0104, 1);
    IMX134MIPI_write_cmos_sensor(0x0205, (kal_uint8)iReg);
    IMX134MIPI_write_cmos_sensor(0x0104, 0);
	
    SENSORDB("[IMX134MIPI]The Register Gain read from sensor is %d \n",iReg);
    SENSORDB("[IMX134MIPI]exit IMX134MIPI_SetGain function\n");
}   /*  IMX134MIPI_SetGain_SetGain  */


/*************************************************************************
* FUNCTION
*    read_IMX134MIPI_gain
*
* DESCRIPTION
*    This function is to set global gain to sensor.
*
* PARAMETERS
*    None
*
* RETURNS
*    gain : sensor global gain(base: 0x40)
*
* GLOBALS AFFECTED
*
*************************************************************************/
kal_uint16 read_IMX134MIPI_gain(void)
{  
	SENSORDB("[IMX134MIPI]enter read_IMX134MIPI_gain function\n");
    return (kal_uint16)((IMX134MIPI_read_cmos_sensor(0x0204)<<8) | IMX134MIPI_read_cmos_sensor(0x0205)) ;
}  /* read_IMX134MIPI_gain */

void write_IMX134MIPI_gain(kal_uint16 gain)
{
    IMX134MIPI_SetGain(gain);
}
void IMX134MIPI_camera_para_to_sensor(void)
{

	kal_uint32    i;
	SENSORDB("[IMX134MIPI]enter IMX134MIPI_camera_para_to_sensor function\n");
    for(i=0; 0xFFFFFFFF!=IMX134MIPISensorReg[i].Addr; i++)
    {
        IMX134MIPI_write_cmos_sensor(IMX134MIPISensorReg[i].Addr, IMX134MIPISensorReg[i].Para);
    }
    for(i=ENGINEER_START_ADDR; 0xFFFFFFFF!=IMX134MIPISensorReg[i].Addr; i++)
    {
        IMX134MIPI_write_cmos_sensor(IMX134MIPISensorReg[i].Addr, IMX134MIPISensorReg[i].Para);
    }
    for(i=FACTORY_START_ADDR; i<FACTORY_END_ADDR; i++)
    {
        IMX134MIPI_write_cmos_sensor(IMX134MIPISensorCCT[i].Addr, IMX134MIPISensorCCT[i].Para);
    }
	SENSORDB("[IMX134MIPI]exit IMX134MIPI_camera_para_to_sensor function\n");

}


/*************************************************************************
* FUNCTION
*    IMX134MIPI_sensor_to_camera_para
*
* DESCRIPTION
*    // update camera_para from sensor register
*
* PARAMETERS
*    None
*
* RETURNS
*    gain : sensor global gain(base: 0x40)
*
* GLOBALS AFFECTED
*
*************************************************************************/
void IMX134MIPI_sensor_to_camera_para(void)
{

	kal_uint32    i,temp_data;
	SENSORDB("[IMX134MIPI]enter IMX134MIPI_sensor_to_camera_para function\n");
    for(i=0; 0xFFFFFFFF!=IMX134MIPISensorReg[i].Addr; i++)
    {
		temp_data=IMX134MIPI_read_cmos_sensor(IMX134MIPISensorReg[i].Addr);
		spin_lock(&imx134_drv_lock);
		IMX134MIPISensorReg[i].Para = temp_data;
		spin_unlock(&imx134_drv_lock);
    }
    for(i=ENGINEER_START_ADDR; 0xFFFFFFFF!=IMX134MIPISensorReg[i].Addr; i++)
    {
    	temp_data=IMX134MIPI_read_cmos_sensor(IMX134MIPISensorReg[i].Addr);
         spin_lock(&imx134_drv_lock);
        IMX134MIPISensorReg[i].Para = temp_data;
		spin_unlock(&imx134_drv_lock);
    }
	SENSORDB("[IMX134MIPI]exit IMX134MIPI_sensor_to_camera_para function\n");

}

/*************************************************************************
* FUNCTION
*    IMX134MIPI_get_sensor_group_count
*
* DESCRIPTION
*    //
*
* PARAMETERS
*    None
*
* RETURNS
*    gain : sensor global gain(base: 0x40)
*
* GLOBALS AFFECTED
*
*************************************************************************/
kal_int32  IMX134MIPI_get_sensor_group_count(void)
{
    return GROUP_TOTAL_NUMS;
}

void IMX134MIPI_get_sensor_group_info(kal_uint16 group_idx, kal_int8* group_name_ptr, kal_int32* item_count_ptr)
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

void IMX134MIPI_get_sensor_item_info(kal_uint16 group_idx,kal_uint16 item_idx, MSDK_SENSOR_ITEM_INFO_STRUCT* info_ptr)
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
                 SENSORDB("[IMX105MIPI][Error]get_sensor_item_info error!!!\n");
          }
           	spin_lock(&imx134_drv_lock);    
            temp_para=IMX134MIPISensorCCT[temp_addr].Para;	
			spin_unlock(&imx134_drv_lock);
            temp_gain = IMX134MIPIReg2Gain(temp_para);
            temp_gain=(temp_gain*1000)/BASEGAIN;
            info_ptr->ItemValue=temp_gain;
            info_ptr->IsTrueFalse=KAL_FALSE;
            info_ptr->IsReadOnly=KAL_FALSE;
            info_ptr->IsNeedRestart=KAL_FALSE;
            info_ptr->Min=1000;
            info_ptr->Max=15875;
            break;
        case CMMCLK_CURRENT:
            switch (item_idx)
            {
                case 0:
                    sprintf((char *)info_ptr->ItemNamePtr,"Drv Cur[2,4,6,8]mA");
                
                    //temp_reg=IMX134MIPISensorReg[CMMCLK_CURRENT_INDEX].Para;
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
                    info_ptr->ItemValue=IMX134MIPI_MAX_EXPOSURE_LINES;
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
kal_bool IMX134MIPI_set_sensor_item_info(kal_uint16 group_idx, kal_uint16 item_idx, kal_int32 ItemValue)
{
   kal_uint16 temp_addr=0, temp_para=0;

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
                 SENSORDB("[IMX105MIPI][Error]set_sensor_item_info error!!!\n");
          }
            temp_para = IMX134MIPIGain2Reg(ItemValue);
            spin_lock(&imx134_drv_lock);    
            IMX134MIPISensorCCT[temp_addr].Para = temp_para;
			spin_unlock(&imx134_drv_lock);
            IMX134MIPI_write_cmos_sensor(IMX134MIPISensorCCT[temp_addr].Addr,temp_para);
			temp_para=read_IMX134MIPI_gain();	
            spin_lock(&imx134_drv_lock);    
            IMX134MIPI_sensor_gain_base=temp_para;
			spin_unlock(&imx134_drv_lock);

            break;
        case CMMCLK_CURRENT:
            switch (item_idx)
            {
                case 0:
                    if(ItemValue==2)
                    {			
                    spin_lock(&imx134_drv_lock);    
                        IMX134MIPISensorReg[CMMCLK_CURRENT_INDEX].Para = ISP_DRIVING_2MA;
					spin_unlock(&imx134_drv_lock);
                        //IMX134MIPI_set_isp_driving_current(ISP_DRIVING_2MA);
                    }
                    else if(ItemValue==3 || ItemValue==4)
                    {
                    	spin_lock(&imx134_drv_lock);    
                        IMX134MIPISensorReg[CMMCLK_CURRENT_INDEX].Para = ISP_DRIVING_4MA;
						spin_unlock(&imx134_drv_lock);
                        //IMX134MIPI_set_isp_driving_current(ISP_DRIVING_4MA);
                    }
                    else if(ItemValue==5 || ItemValue==6)
                    {
                    	spin_lock(&imx134_drv_lock);    
                        IMX134MIPISensorReg[CMMCLK_CURRENT_INDEX].Para = ISP_DRIVING_6MA;
						spin_unlock(&imx134_drv_lock);
                        //IMX134MIPI_set_isp_driving_current(ISP_DRIVING_6MA);
                    }
                    else
                    {
                    	spin_lock(&imx134_drv_lock);    
                        IMX134MIPISensorReg[CMMCLK_CURRENT_INDEX].Para = ISP_DRIVING_8MA;
						spin_unlock(&imx134_drv_lock);
                        //IMX134MIPI_set_isp_driving_current(ISP_DRIVING_8MA);
                    }
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
					spin_lock(&imx134_drv_lock);    
                    IMX134MIPI_FAC_SENSOR_REG=ItemValue;
					spin_unlock(&imx134_drv_lock);
                    break;
                case 1:
                    IMX134MIPI_write_cmos_sensor(IMX134MIPI_FAC_SENSOR_REG,ItemValue);
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
static void IMX134MIPI_SetDummy(const kal_uint16 iPixels, const kal_uint16 iLines)
{
	kal_uint32 frame_length = 0, line_length = 0;
    if(IMX134MIPI_sensor.pv_mode == KAL_TRUE)
   	{
   	 spin_lock(&imx134_drv_lock);    
   	 IMX134MIPI_sensor.pv_dummy_pixels = iPixels;
	 IMX134MIPI_sensor.pv_dummy_lines = iLines;
   	 IMX134MIPI_sensor.pv_line_length = IMX134MIPI_PV_LINE_LENGTH_PIXELS + iPixels;
	 IMX134MIPI_sensor.pv_frame_length = IMX134MIPI_PV_FRAME_LENGTH_LINES + iLines;
	 spin_unlock(&imx134_drv_lock);
	 line_length = IMX134MIPI_sensor.pv_line_length;
	 frame_length = IMX134MIPI_sensor.pv_frame_length;	 	
   	}
   else if(IMX134MIPI_sensor.video_mode== KAL_TRUE)
   	{
   	 spin_lock(&imx134_drv_lock);    
   	 IMX134MIPI_sensor.video_dummy_pixels = iPixels;
	 IMX134MIPI_sensor.video_dummy_lines = iLines;
   	 IMX134MIPI_sensor.video_line_length = IMX134MIPI_VIDEO_LINE_LENGTH_PIXELS + iPixels;
	 IMX134MIPI_sensor.video_frame_length = IMX134MIPI_VIDEO_FRAME_LENGTH_LINES + iLines;
	 spin_unlock(&imx134_drv_lock);
	 line_length = IMX134MIPI_sensor.video_line_length;
	 frame_length = IMX134MIPI_sensor.video_frame_length;
   	}
	else if(IMX134MIPI_sensor.capture_mode== KAL_TRUE) 
		{
	  spin_lock(&imx134_drv_lock);	
   	  IMX134MIPI_sensor.cp_dummy_pixels = iPixels;
	  IMX134MIPI_sensor.cp_dummy_lines = iLines;
	  IMX134MIPI_sensor.cp_line_length = IMX134MIPI_FULL_LINE_LENGTH_PIXELS + iPixels;
	  IMX134MIPI_sensor.cp_frame_length = IMX134MIPI_FULL_FRAME_LENGTH_LINES + iLines;
	   spin_unlock(&imx134_drv_lock);
	  line_length = IMX134MIPI_sensor.cp_line_length;
	  frame_length = IMX134MIPI_sensor.cp_frame_length;
    }
	else
	{
	 SENSORDB("[IMX134MIPI]%s(),sensor mode error",__FUNCTION__);
	}
      IMX134MIPI_write_cmos_sensor(0x0104, 1);        	  
      IMX134MIPI_write_cmos_sensor(0x0340, (frame_length >>8) & 0xFF);
      IMX134MIPI_write_cmos_sensor(0x0341, frame_length & 0xFF);	
      IMX134MIPI_write_cmos_sensor(0x0342, (line_length >>8) & 0xFF);
      IMX134MIPI_write_cmos_sensor(0x0343, line_length & 0xFF);
      IMX134MIPI_write_cmos_sensor(0x0104, 0);
}   /*  IMX134MIPI_SetDummy */
static void IMX134MIPI_Para_Init(void)
{
	{	 
		spin_lock(&imx134_drv_lock);	
			  IMX134MIPI_sensor.i2c_write_id=IMX134MIPI_WRITE_ID;
			  IMX134MIPI_sensor.i2c_read_id=IMX134MIPI_READ_ID;
			  IMX134MIPI_sensor.first_init=KAL_TRUE;
			  IMX134MIPI_sensor.fix_video_fps=KAL_FALSE;
			  IMX134MIPI_sensor.pv_mode=KAL_TRUE; 
			  IMX134MIPI_sensor.video_mode=KAL_FALSE;				
			  IMX134MIPI_sensor.capture_mode=KAL_FALSE;				//True: Preview Mode; False: Capture Mode
			  IMX134MIPI_sensor.night_mode=KAL_FALSE;				//True: Night Mode; False: Auto Mode
			  IMX134MIPI_sensor.mirror_flip=KAL_FALSE;
			  IMX134MIPI_sensor.pv_pclk=232000000;				//Preview Pclk
			  IMX134MIPI_sensor.video_pclk=232000000;				//video Pclk
			  IMX134MIPI_sensor.cp_pclk=232000000;				//Capture Pclk
			  IMX134MIPI_sensor.pv_shutter=0;		   
			  IMX134MIPI_sensor.video_shutter=0; 	   
			  IMX134MIPI_sensor.cp_shutter=0;
			  IMX134MIPI_sensor.pv_gain=64;
			  IMX134MIPI_sensor.video_gain=64;
			  IMX134MIPI_sensor.cp_gain=64;
			  IMX134MIPI_sensor.pv_line_length=IMX134MIPI_PV_LINE_LENGTH_PIXELS,									//kal_uint32 pv_line_length;												  
			  IMX134MIPI_sensor.pv_frame_length=IMX134MIPI_PV_FRAME_LENGTH_LINES;
			  IMX134MIPI_sensor.video_line_length=IMX134MIPI_VIDEO_LINE_LENGTH_PIXELS;
			  IMX134MIPI_sensor.video_frame_length=IMX134MIPI_VIDEO_FRAME_LENGTH_LINES;
			  IMX134MIPI_sensor.cp_line_length=IMX134MIPI_FULL_LINE_LENGTH_PIXELS;
			  IMX134MIPI_sensor.cp_frame_length=IMX134MIPI_FULL_FRAME_LENGTH_LINES;
			  IMX134MIPI_sensor.pv_dummy_pixels=0;		   //Dummy Pixels:must be 12s
			  IMX134MIPI_sensor.pv_dummy_lines=0;		   //Dummy Lines
			  IMX134MIPI_sensor.video_dummy_pixels=0;		   //Dummy Pixels:must be 12s
			  IMX134MIPI_sensor.video_dummy_lines=0; 	   //Dummy Lines
			  IMX134MIPI_sensor.cp_dummy_pixels=0;		   //Dummy Pixels:must be 12s
			  IMX134MIPI_sensor.cp_dummy_lines=0;		   //Dummy Lines			
			  IMX134MIPI_sensor.video_current_frame_rate=0;
			  
			  spin_unlock(&imx134_drv_lock);
			  SENSORDB("[IMX134MIPI]initParaCaptureMode\n",IMX134MIPI_sensor.capture_mode); 
		}

}

static void IMX134MIPI_Sensor_Init(void)
  	{
  	
  	#if 0
  	IMX134MIPI_write_cmos_sensor(0x0101, 0x00);
  	IMX134MIPI_write_cmos_sensor(0x0105, 0x01);
  	IMX134MIPI_write_cmos_sensor(0x0110, 0x00);
  	IMX134MIPI_write_cmos_sensor(0x0220, 0x01);
  	IMX134MIPI_write_cmos_sensor(0x3302, 0x11);
  	IMX134MIPI_write_cmos_sensor(0x3833, 0x20);
  	IMX134MIPI_write_cmos_sensor(0x3893, 0x00);
  	IMX134MIPI_write_cmos_sensor(0x3906, 0x08);
  	IMX134MIPI_write_cmos_sensor(0x3907, 0x01);
  	IMX134MIPI_write_cmos_sensor(0x391B, 0x01);
  	IMX134MIPI_write_cmos_sensor(0x3C09, 0x01);
  	IMX134MIPI_write_cmos_sensor(0x600A, 0x00);
  	IMX134MIPI_write_cmos_sensor(0x3008, 0xB0);
  	IMX134MIPI_write_cmos_sensor(0x320A, 0x01);
  	IMX134MIPI_write_cmos_sensor(0x320D, 0x10);
  	IMX134MIPI_write_cmos_sensor(0x3216, 0x2E);
  	IMX134MIPI_write_cmos_sensor(0x322C, 0x02);
  	IMX134MIPI_write_cmos_sensor(0x3409, 0x0C);
  	IMX134MIPI_write_cmos_sensor(0x340C, 0x2D);
  	IMX134MIPI_write_cmos_sensor(0x3411, 0x39);
  	IMX134MIPI_write_cmos_sensor(0x3414, 0x1E);
  	IMX134MIPI_write_cmos_sensor(0x3427, 0x04);
  	IMX134MIPI_write_cmos_sensor(0x3480, 0x1E);
  	IMX134MIPI_write_cmos_sensor(0x3484, 0x1E);
  	IMX134MIPI_write_cmos_sensor(0x3488, 0x1E);
  	IMX134MIPI_write_cmos_sensor(0x348C, 0x1E);
  	IMX134MIPI_write_cmos_sensor(0x3490, 0x1E);
  	IMX134MIPI_write_cmos_sensor(0x3494, 0x1E);
  	IMX134MIPI_write_cmos_sensor(0x3511, 0x8F);
  	IMX134MIPI_write_cmos_sensor(0x3617, 0x2D);
  	IMX134MIPI_write_cmos_sensor(0x4334,0x03);
  	IMX134MIPI_write_cmos_sensor(0x4335,0x20);
  	IMX134MIPI_write_cmos_sensor(0x4336,0x03);
  	IMX134MIPI_write_cmos_sensor(0x4337,0x84);
  	IMX134MIPI_write_cmos_sensor(0x4364,0x0B);
  	IMX134MIPI_write_cmos_sensor(0x436D,0x00);
  	IMX134MIPI_write_cmos_sensor(0x436F,0x06);
  	IMX134MIPI_write_cmos_sensor(0x4281,0x21);
  	IMX134MIPI_write_cmos_sensor(0x4283,0x04);
  	IMX134MIPI_write_cmos_sensor(0x4284,0x08);
  	IMX134MIPI_write_cmos_sensor(0x4287,0x7F);
  	IMX134MIPI_write_cmos_sensor(0x4288,0x08);
  	IMX134MIPI_write_cmos_sensor(0x428B,0x7F);
  	IMX134MIPI_write_cmos_sensor(0x428C,0x08);
  	IMX134MIPI_write_cmos_sensor(0x428F,0x7F);
  	IMX134MIPI_write_cmos_sensor(0x4297,0x00);
  	IMX134MIPI_write_cmos_sensor(0x4298,0x7E);
  	IMX134MIPI_write_cmos_sensor(0x4299,0x7E);
  	IMX134MIPI_write_cmos_sensor(0x429A,0x7E);
  	IMX134MIPI_write_cmos_sensor(0x42A4,0xFB);
  	IMX134MIPI_write_cmos_sensor(0x42A5,0x7E);
  	IMX134MIPI_write_cmos_sensor(0x42A6,0xDF);
  	IMX134MIPI_write_cmos_sensor(0x42A7,0xB7);
  	IMX134MIPI_write_cmos_sensor(0x42AF,0x03);

  	IMX134MIPI_write_cmos_sensor(0x4207,0x03);
  	IMX134MIPI_write_cmos_sensor(0x4216,0x08);
  	IMX134MIPI_write_cmos_sensor(0x4217,0x08);
  	IMX134MIPI_write_cmos_sensor(0x421B,0x20);
  	IMX134MIPI_write_cmos_sensor(0x422E,0x54);
  	IMX134MIPI_write_cmos_sensor(0x4230,0xFF);
  	IMX134MIPI_write_cmos_sensor(0x4231,0xFE);
  	IMX134MIPI_write_cmos_sensor(0x4232,0xFF);
  	IMX134MIPI_write_cmos_sensor(0x4235,0x58);
  	IMX134MIPI_write_cmos_sensor(0x4236,0xF7);
  	IMX134MIPI_write_cmos_sensor(0x4237,0xFD);
  	IMX134MIPI_write_cmos_sensor(0x4239,0x4E);
  	IMX134MIPI_write_cmos_sensor(0x423A,0xFC);
  	IMX134MIPI_write_cmos_sensor(0x423B,0xFD);
  	IMX134MIPI_write_cmos_sensor(0x4300,0x00);
  	IMX134MIPI_write_cmos_sensor(0x4316,0x12);
  	IMX134MIPI_write_cmos_sensor(0x4317,0x22);
  	IMX134MIPI_write_cmos_sensor(0x431A,0x00);
  	IMX134MIPI_write_cmos_sensor(0x432B,0x20);
  	IMX134MIPI_write_cmos_sensor(0x432D,0x01);
  	IMX134MIPI_write_cmos_sensor(0x435E,0x01);
  	IMX134MIPI_write_cmos_sensor(0x435F,0xFF);
  	IMX134MIPI_write_cmos_sensor(0x4401,0x3F);
  	IMX134MIPI_write_cmos_sensor(0x4412,0x3F);
  	IMX134MIPI_write_cmos_sensor(0x4413,0xFF);
  	IMX134MIPI_write_cmos_sensor(0x441D,0x40);
  	IMX134MIPI_write_cmos_sensor(0x441E,0x1E);
  	IMX134MIPI_write_cmos_sensor(0x441F,0x38);
  	IMX134MIPI_write_cmos_sensor(0x4446,0x1D);
  	IMX134MIPI_write_cmos_sensor(0x4447,0xF9);
  	IMX134MIPI_write_cmos_sensor(0x4452,0x00);
  	IMX134MIPI_write_cmos_sensor(0x4453,0xA0);
  	IMX134MIPI_write_cmos_sensor(0x4454,0x08);
  	IMX134MIPI_write_cmos_sensor(0x4455,0x00);
  	IMX134MIPI_write_cmos_sensor(0x4458,0x18);
  	IMX134MIPI_write_cmos_sensor(0x4459,0x18);
  	IMX134MIPI_write_cmos_sensor(0x445A,0x3F);
  	IMX134MIPI_write_cmos_sensor(0x445B,0x3A);
  	IMX134MIPI_write_cmos_sensor(0x445D,0x28);
  	IMX134MIPI_write_cmos_sensor(0x445F,0x90);
  	IMX134MIPI_write_cmos_sensor(0x4463,0x00);
	IMX134MIPI_write_cmos_sensor(0x4465,0x00);      		                              
	IMX134MIPI_write_cmos_sensor(0x452A,0x02);      
          
#else
	int totalCnt = 0, len = 0;
	int transfer_len, transac_len=3;
	kal_uint8* pBuf=NULL;
	dma_addr_t dmaHandle;
	pBuf = (kal_uint8*)kmalloc(1024, GFP_KERNEL);				
	totalCnt = ARRAY_SIZE(imx134_init);
	transfer_len = totalCnt / transac_len;
	len = (transfer_len<<8)|transac_len;	
	SENSORDB("Total Count = %d, Len = 0x%x\n", totalCnt,len);	  
	memcpy(pBuf, &imx134_init, totalCnt );   
	dmaHandle = dma_map_single(NULL, pBuf, 1024, DMA_TO_DEVICE);	
	imx134_multi_write_cmos_sensor(dmaHandle, len); 
	dma_unmap_single(NULL, dmaHandle, 1024, DMA_TO_DEVICE);
				
					
#endif	

	  
  
	 // IMX134MIPI_multi_write_cmos_sensor(dmaHandle, len); 
  
  
	  

  
  		SENSORDB("IMX134MIPI_globle_setting  end \n");
  	}   /*  IMX134MIPI_Sensor_Init  */


void IMX134VideoFullSizeSetting(void)//16:9   6M

{
	SENSORDB("[IMX134MIPI]enter VideoFullSizeSetting function\n");
	IMX134MIPI_write_cmos_sensor(0x41C0, 0x01);
	IMX134MIPI_write_cmos_sensor(0x0104, 0x01);//group
	IMX134MIPI_write_cmos_sensor(0x0100, 0x00);//STREAM OFF 	
#if 1
#if 0 //size 3280 X 1664	
	IMX134MIPI_write_cmos_sensor(0x011E,0x18);
	IMX134MIPI_write_cmos_sensor(0x011F,0x00);
IMX134MIPI_write_cmos_sensor(	0x0301,0x05);     
	IMX134MIPI_write_cmos_sensor(0x0303,0x01);
	IMX134MIPI_write_cmos_sensor(0x0305,0x0C);
IMX134MIPI_write_cmos_sensor(	0x0309,0x05);     
	IMX134MIPI_write_cmos_sensor(0x030B,0x01);
	IMX134MIPI_write_cmos_sensor(0x030C,0x01);
IMX134MIPI_write_cmos_sensor(	0x030D,0x22);     
	IMX134MIPI_write_cmos_sensor(0x030E,0x01);
	IMX134MIPI_write_cmos_sensor(0x3A06,0x11);
	//Address	value
IMX134MIPI_write_cmos_sensor(	0x0108,0x03);     
	IMX134MIPI_write_cmos_sensor(0x0112,0x0A);
	IMX134MIPI_write_cmos_sensor(0x0113,0x0A);
	IMX134MIPI_write_cmos_sensor(0x0381,0x01);
	IMX134MIPI_write_cmos_sensor(0x0383,0x01);
	IMX134MIPI_write_cmos_sensor(0x0385,0x01);
	IMX134MIPI_write_cmos_sensor(0x0387,0x01);
	IMX134MIPI_write_cmos_sensor(0x0390,0x00);
	IMX134MIPI_write_cmos_sensor(0x0391,0x11);
	IMX134MIPI_write_cmos_sensor(0x0392,0x00);
	IMX134MIPI_write_cmos_sensor(0x0401,0x00);
	IMX134MIPI_write_cmos_sensor(0x0404,0x00);
	IMX134MIPI_write_cmos_sensor(0x0405,0x10);
	IMX134MIPI_write_cmos_sensor(0x4082,0x01);
	IMX134MIPI_write_cmos_sensor(0x4083,0x01);
	IMX134MIPI_write_cmos_sensor(0x7006,0x04);
	//Address	value
	IMX134MIPI_write_cmos_sensor(0x0700,0x00);
	IMX134MIPI_write_cmos_sensor(0x3A63,0x00);
	IMX134MIPI_write_cmos_sensor(0x4100,0xF8);
	IMX134MIPI_write_cmos_sensor(0x4203,0xFF);
	IMX134MIPI_write_cmos_sensor(0x4344,0x00);
	IMX134MIPI_write_cmos_sensor(0x441C,0x01);
	//Address	value
IMX134MIPI_write_cmos_sensor(	0x0340,0x08);     
IMX134MIPI_write_cmos_sensor(	0x0341,0x34);     
	IMX134MIPI_write_cmos_sensor(0x0342,0x0E);
	IMX134MIPI_write_cmos_sensor(0x0343,0x10);
	IMX134MIPI_write_cmos_sensor(0x0344,0x00);
	IMX134MIPI_write_cmos_sensor(0x0345,0x00);
	IMX134MIPI_write_cmos_sensor(0x0346,0x01);
IMX134MIPI_write_cmos_sensor(	0x0347,0x90);     
	IMX134MIPI_write_cmos_sensor(0x0348,0x0C);
	IMX134MIPI_write_cmos_sensor(0x0349,0xCF);
	IMX134MIPI_write_cmos_sensor(0x034A,0x08);
IMX134MIPI_write_cmos_sensor(	0x034B,0x0F);     
	IMX134MIPI_write_cmos_sensor(0x034C,0x0C);
	IMX134MIPI_write_cmos_sensor(0x034D,0xD0);
IMX134MIPI_write_cmos_sensor(	0x034E,0x06);     
IMX134MIPI_write_cmos_sensor(	0x034F,0x80);     
	IMX134MIPI_write_cmos_sensor(0x0350,0x00);
	IMX134MIPI_write_cmos_sensor(0x0351,0x00);
	IMX134MIPI_write_cmos_sensor(0x0352,0x00);
	IMX134MIPI_write_cmos_sensor(0x0353,0x00);
	IMX134MIPI_write_cmos_sensor(0x0354,0x0C);
	IMX134MIPI_write_cmos_sensor(0x0355,0xD0);
IMX134MIPI_write_cmos_sensor(	0x0356,0x06);     
IMX134MIPI_write_cmos_sensor(	0x0357,0x80);     
	IMX134MIPI_write_cmos_sensor(0x301D,0x30);
	IMX134MIPI_write_cmos_sensor(0x3310,0x0C);
	IMX134MIPI_write_cmos_sensor(0x3311,0xD0);
IMX134MIPI_write_cmos_sensor(	0x3312,0x06);     
IMX134MIPI_write_cmos_sensor(	0x3313,0x80);     
IMX134MIPI_write_cmos_sensor(	0x331C,0x04);     
IMX134MIPI_write_cmos_sensor(	0x331D,0x1E);     
	IMX134MIPI_write_cmos_sensor(0x4084,0x00);
	IMX134MIPI_write_cmos_sensor(0x4085,0x00);
	IMX134MIPI_write_cmos_sensor(0x4086,0x00);
	IMX134MIPI_write_cmos_sensor(0x4087,0x00);
	IMX134MIPI_write_cmos_sensor(0x4400,0x00);
	//Address	value
IMX134MIPI_write_cmos_sensor(	0x0830,0x6F);     
IMX134MIPI_write_cmos_sensor(	0x0831,0x27);     
IMX134MIPI_write_cmos_sensor(	0x0832,0x4F);     
IMX134MIPI_write_cmos_sensor(	0x0833,0x2F);     
IMX134MIPI_write_cmos_sensor(	0x0834,0x2F);     
IMX134MIPI_write_cmos_sensor(	0x0835,0x2F);     
IMX134MIPI_write_cmos_sensor(	0x0836,0x9F);     
IMX134MIPI_write_cmos_sensor(	0x0837,0x37);     
	IMX134MIPI_write_cmos_sensor(0x0839,0x1F);
	IMX134MIPI_write_cmos_sensor(0x083A,0x17);
	IMX134MIPI_write_cmos_sensor(0x083B,0x02);
	//Address	value
IMX134MIPI_write_cmos_sensor(	0x0202,0x08);     
IMX134MIPI_write_cmos_sensor(	0x0203,0x30);     
	//Address	value
	IMX134MIPI_write_cmos_sensor(0x0205,0x00);
	IMX134MIPI_write_cmos_sensor(0x020E,0x01);
	IMX134MIPI_write_cmos_sensor(0x020F,0x00);
	IMX134MIPI_write_cmos_sensor(0x0210,0x01);
	IMX134MIPI_write_cmos_sensor(0x0211,0x00);
	IMX134MIPI_write_cmos_sensor(0x0212,0x01);
	IMX134MIPI_write_cmos_sensor(0x0213,0x00);
	IMX134MIPI_write_cmos_sensor(0x0214,0x01);
	IMX134MIPI_write_cmos_sensor(0x0215,0x00);
	//Address	value
	IMX134MIPI_write_cmos_sensor(0x0230,0x00);
	IMX134MIPI_write_cmos_sensor(0x0231,0x00);
	IMX134MIPI_write_cmos_sensor(0x0233,0x00);
	IMX134MIPI_write_cmos_sensor(0x0234,0x00);
	IMX134MIPI_write_cmos_sensor(0x0235,0x40);
	IMX134MIPI_write_cmos_sensor(0x0238,0x00);
	IMX134MIPI_write_cmos_sensor(0x0239,0x04);
	IMX134MIPI_write_cmos_sensor(0x023B,0x00);
	IMX134MIPI_write_cmos_sensor(0x023C,0x01);
	IMX134MIPI_write_cmos_sensor(0x33B0,0x04);
	IMX134MIPI_write_cmos_sensor(0x33B1,0x00);
	IMX134MIPI_write_cmos_sensor(0x33B3,0x00);
	IMX134MIPI_write_cmos_sensor(0x33B4,0x01);
	IMX134MIPI_write_cmos_sensor(0x3800,0x00);
IMX134MIPI_write_cmos_sensor(	0x3A43,0x01);
#else   //size :3280 X 1832
	
IMX134MIPI_write_cmos_sensor(	0x011E,0x18);  
IMX134MIPI_write_cmos_sensor(	0x011F,0x00);  
IMX134MIPI_write_cmos_sensor(	0x0301,0x05);  
IMX134MIPI_write_cmos_sensor(	0x0303,0x01);  
IMX134MIPI_write_cmos_sensor(	0x0305,0x0C);  
IMX134MIPI_write_cmos_sensor(	0x0309,0x05);  
IMX134MIPI_write_cmos_sensor(	0x030B,0x01);  
IMX134MIPI_write_cmos_sensor(	0x030C,0x01);  
IMX134MIPI_write_cmos_sensor(	0x030D,0x22);  
IMX134MIPI_write_cmos_sensor(	0x030E,0x01);  
IMX134MIPI_write_cmos_sensor(	0x3A06,0x11);  
IMX134MIPI_write_cmos_sensor(	0x0108,0x03);  
IMX134MIPI_write_cmos_sensor(	0x0112,0x0A);  
IMX134MIPI_write_cmos_sensor(	0x0113,0x0A);  
IMX134MIPI_write_cmos_sensor(	0x0381,0x01);  
IMX134MIPI_write_cmos_sensor(	0x0383,0x01);  
IMX134MIPI_write_cmos_sensor(	0x0385,0x01);  
IMX134MIPI_write_cmos_sensor(	0x0387,0x01);  
IMX134MIPI_write_cmos_sensor(	0x0390,0x00);  
IMX134MIPI_write_cmos_sensor(	0x0391,0x11);  
IMX134MIPI_write_cmos_sensor(	0x0392,0x00);  
IMX134MIPI_write_cmos_sensor(	0x0401,0x00);  
IMX134MIPI_write_cmos_sensor(	0x0404,0x00);  
IMX134MIPI_write_cmos_sensor(	0x0405,0x10);  
IMX134MIPI_write_cmos_sensor(	0x4082,0x01);  
IMX134MIPI_write_cmos_sensor(	0x4083,0x01);  
IMX134MIPI_write_cmos_sensor(	0x7006,0x04);  
IMX134MIPI_write_cmos_sensor(	0x0700,0x00);  
IMX134MIPI_write_cmos_sensor(	0x3A63,0x00);  
IMX134MIPI_write_cmos_sensor(	0x4100,0xF8);  
IMX134MIPI_write_cmos_sensor(	0x4203,0xFF);  
IMX134MIPI_write_cmos_sensor(	0x4344,0x00);  
IMX134MIPI_write_cmos_sensor(	0x441C,0x01);  
IMX134MIPI_write_cmos_sensor(	0x0340,0x08);  
IMX134MIPI_write_cmos_sensor(	0x0341,0x34);  
IMX134MIPI_write_cmos_sensor(	0x0342,0x0E);  
IMX134MIPI_write_cmos_sensor(	0x0343,0x10);  
IMX134MIPI_write_cmos_sensor(	0x0344,0x00);  
IMX134MIPI_write_cmos_sensor(	0x0345,0x00);  
IMX134MIPI_write_cmos_sensor(	0x0346,0x01);  
IMX134MIPI_write_cmos_sensor(	0x0347,0x34);  
IMX134MIPI_write_cmos_sensor(	0x0348,0x0C);  
IMX134MIPI_write_cmos_sensor(	0x0349,0xCF);  
IMX134MIPI_write_cmos_sensor(	0x034A,0x08);  
IMX134MIPI_write_cmos_sensor(	0x034B,0x6F);  
IMX134MIPI_write_cmos_sensor(	0x034C,0x0C);  
IMX134MIPI_write_cmos_sensor(	0x034D,0xD0);  
IMX134MIPI_write_cmos_sensor(	0x034E,0x07);  
IMX134MIPI_write_cmos_sensor(	0x034F,0x3C);  
IMX134MIPI_write_cmos_sensor(	0x0350,0x00);  
IMX134MIPI_write_cmos_sensor(	0x0351,0x00);  
IMX134MIPI_write_cmos_sensor(	0x0352,0x00);  
IMX134MIPI_write_cmos_sensor(	0x0353,0x00);  
IMX134MIPI_write_cmos_sensor(	0x0354,0x0C);  
IMX134MIPI_write_cmos_sensor(	0x0355,0xD0);  
IMX134MIPI_write_cmos_sensor(	0x0356,0x07);  
IMX134MIPI_write_cmos_sensor(	0x0357,0x3C);  
IMX134MIPI_write_cmos_sensor(	0x301D,0x30);  
IMX134MIPI_write_cmos_sensor(	0x3310,0x0C);  
IMX134MIPI_write_cmos_sensor(	0x3311,0xD0);  
IMX134MIPI_write_cmos_sensor(	0x3312,0x07);  
IMX134MIPI_write_cmos_sensor(	0x3313,0x3C);  
IMX134MIPI_write_cmos_sensor(	0x331C,0x04);  
IMX134MIPI_write_cmos_sensor(	0x331D,0x1E);  
IMX134MIPI_write_cmos_sensor(	0x4084,0x00);  
IMX134MIPI_write_cmos_sensor(	0x4085,0x00);  
IMX134MIPI_write_cmos_sensor(	0x4086,0x00);  
IMX134MIPI_write_cmos_sensor(	0x4087,0x00);  
IMX134MIPI_write_cmos_sensor(	0x4400,0x00);  
IMX134MIPI_write_cmos_sensor(	0x0830,0x6F);  
IMX134MIPI_write_cmos_sensor(	0x0831,0x27);  
IMX134MIPI_write_cmos_sensor(	0x0832,0x4F);  
IMX134MIPI_write_cmos_sensor(	0x0833,0x2F);  
IMX134MIPI_write_cmos_sensor(	0x0834,0x2F);  
IMX134MIPI_write_cmos_sensor(	0x0835,0x2F);  
IMX134MIPI_write_cmos_sensor(	0x0836,0x9F);  
IMX134MIPI_write_cmos_sensor(	0x0837,0x37);  
IMX134MIPI_write_cmos_sensor(	0x0839,0x1F);  
IMX134MIPI_write_cmos_sensor(	0x083A,0x17);  
IMX134MIPI_write_cmos_sensor(	0x083B,0x02);  
IMX134MIPI_write_cmos_sensor(	0x0202,0x08);  
IMX134MIPI_write_cmos_sensor(	0x0203,0x30);  
IMX134MIPI_write_cmos_sensor(	0x0205,0x00);  
IMX134MIPI_write_cmos_sensor(	0x020E,0x01);  
IMX134MIPI_write_cmos_sensor(	0x020F,0x00);  
IMX134MIPI_write_cmos_sensor(	0x0210,0x01);  
IMX134MIPI_write_cmos_sensor(	0x0211,0x00);  
IMX134MIPI_write_cmos_sensor(	0x0212,0x01);  
IMX134MIPI_write_cmos_sensor(	0x0213,0x00);  
IMX134MIPI_write_cmos_sensor(	0x0214,0x01);  
IMX134MIPI_write_cmos_sensor(	0x0215,0x00);  
IMX134MIPI_write_cmos_sensor(	0x0230,0x00);  
IMX134MIPI_write_cmos_sensor(	0x0231,0x00);  
IMX134MIPI_write_cmos_sensor(	0x0233,0x00);  
IMX134MIPI_write_cmos_sensor(	0x0234,0x00);  
IMX134MIPI_write_cmos_sensor(	0x0235,0x40);  
IMX134MIPI_write_cmos_sensor(	0x0238,0x00);  
IMX134MIPI_write_cmos_sensor(	0x0239,0x04);  
IMX134MIPI_write_cmos_sensor(	0x023B,0x00);  
IMX134MIPI_write_cmos_sensor(	0x023C,0x01);  
IMX134MIPI_write_cmos_sensor(	0x33B0,0x04);  
IMX134MIPI_write_cmos_sensor(	0x33B1,0x00);  
IMX134MIPI_write_cmos_sensor(	0x33B3,0x00);  
IMX134MIPI_write_cmos_sensor(	0x33B4,0x01);  
IMX134MIPI_write_cmos_sensor(	0x3800,0x00);  
IMX134MIPI_write_cmos_sensor(	0x3A43,0x01);
#endif  // for big FOV 3280 2 kinds     
#else   //size 1920 X 1088
IMX134MIPI_write_cmos_sensor(	0x011E,0x18);  
IMX134MIPI_write_cmos_sensor(	0x011F,0x00);  
IMX134MIPI_write_cmos_sensor(	0x0301,0x05);  
IMX134MIPI_write_cmos_sensor(	0x0303,0x01);  
IMX134MIPI_write_cmos_sensor(	0x0305,0x0C);  
IMX134MIPI_write_cmos_sensor(	0x0309,0x05);  
IMX134MIPI_write_cmos_sensor(	0x030B,0x01);  
IMX134MIPI_write_cmos_sensor(	0x030C,0x01);  
IMX134MIPI_write_cmos_sensor(	0x030D,0x22);  
IMX134MIPI_write_cmos_sensor(	0x030E,0x01);  
IMX134MIPI_write_cmos_sensor(	0x3A06,0x11);  
IMX134MIPI_write_cmos_sensor(	0x0108,0x03);  
IMX134MIPI_write_cmos_sensor(	0x0112,0x0A);  
IMX134MIPI_write_cmos_sensor(	0x0113,0x0A);  
IMX134MIPI_write_cmos_sensor(	0x0381,0x01);  
IMX134MIPI_write_cmos_sensor(	0x0383,0x01);  
IMX134MIPI_write_cmos_sensor(	0x0385,0x01);  
IMX134MIPI_write_cmos_sensor(	0x0387,0x01);  
IMX134MIPI_write_cmos_sensor(	0x0390,0x00);  
IMX134MIPI_write_cmos_sensor(	0x0391,0x11);  
IMX134MIPI_write_cmos_sensor(	0x0392,0x00);  
IMX134MIPI_write_cmos_sensor(	0x0401,0x02);  
IMX134MIPI_write_cmos_sensor(	0x0404,0x00);  
IMX134MIPI_write_cmos_sensor(	0x0405,0x1B);  
IMX134MIPI_write_cmos_sensor(	0x4082,0x00);  
IMX134MIPI_write_cmos_sensor(	0x4083,0x00);  
IMX134MIPI_write_cmos_sensor(	0x7006,0x04);  
IMX134MIPI_write_cmos_sensor(	0x0700,0x00);  
IMX134MIPI_write_cmos_sensor(	0x3A63,0x00);  
IMX134MIPI_write_cmos_sensor(	0x4100,0xF8);  
IMX134MIPI_write_cmos_sensor(	0x4203,0xFF);  
IMX134MIPI_write_cmos_sensor(	0x4344,0x00);  
IMX134MIPI_write_cmos_sensor(	0x441C,0x01);  
IMX134MIPI_write_cmos_sensor(	0x0340,0x08);  
IMX134MIPI_write_cmos_sensor(	0x0341,0x34);  
IMX134MIPI_write_cmos_sensor(	0x0342,0x0E);  
IMX134MIPI_write_cmos_sensor(	0x0343,0x10);  
IMX134MIPI_write_cmos_sensor(	0x0344,0x00);  
IMX134MIPI_write_cmos_sensor(	0x0345,0x14);  
IMX134MIPI_write_cmos_sensor(	0x0346,0x01);  
IMX134MIPI_write_cmos_sensor(	0x0347,0x3C);  
IMX134MIPI_write_cmos_sensor(	0x0348,0x0C);  
IMX134MIPI_write_cmos_sensor(	0x0349,0xBB);  
IMX134MIPI_write_cmos_sensor(	0x034A,0x08);  
IMX134MIPI_write_cmos_sensor(	0x034B,0x67);  
IMX134MIPI_write_cmos_sensor(	0x034C,0x07);  
IMX134MIPI_write_cmos_sensor(	0x034D,0x80);  
IMX134MIPI_write_cmos_sensor(	0x034E,0x04);  
IMX134MIPI_write_cmos_sensor(	0x034F,0x40);  
IMX134MIPI_write_cmos_sensor(	0x0350,0x00);  
IMX134MIPI_write_cmos_sensor(	0x0351,0x00);  
IMX134MIPI_write_cmos_sensor(	0x0352,0x00);  
IMX134MIPI_write_cmos_sensor(	0x0353,0x00);  
IMX134MIPI_write_cmos_sensor(	0x0354,0x0C);  
IMX134MIPI_write_cmos_sensor(	0x0355,0xA8);  
IMX134MIPI_write_cmos_sensor(	0x0356,0x07);  
IMX134MIPI_write_cmos_sensor(	0x0357,0x2C);  
IMX134MIPI_write_cmos_sensor(	0x301D,0x30);  
IMX134MIPI_write_cmos_sensor(	0x3310,0x07);  
IMX134MIPI_write_cmos_sensor(	0x3311,0x80);  
IMX134MIPI_write_cmos_sensor(	0x3312,0x04);  
IMX134MIPI_write_cmos_sensor(	0x3313,0x40);  
IMX134MIPI_write_cmos_sensor(	0x331C,0x04);  
IMX134MIPI_write_cmos_sensor(	0x331D,0x1E);  
IMX134MIPI_write_cmos_sensor(	0x4084,0x07);  
IMX134MIPI_write_cmos_sensor(	0x4085,0x80);  
IMX134MIPI_write_cmos_sensor(	0x4086,0x04);  
IMX134MIPI_write_cmos_sensor(	0x4087,0x40);  
IMX134MIPI_write_cmos_sensor(	0x4400,0x00);  
IMX134MIPI_write_cmos_sensor(	0x0830,0x6F);  
IMX134MIPI_write_cmos_sensor(	0x0831,0x27);  
IMX134MIPI_write_cmos_sensor(	0x0832,0x4F);  
IMX134MIPI_write_cmos_sensor(	0x0833,0x2F);  
IMX134MIPI_write_cmos_sensor(	0x0834,0x2F);  
IMX134MIPI_write_cmos_sensor(	0x0835,0x2F);  
IMX134MIPI_write_cmos_sensor(	0x0836,0x9F);  
IMX134MIPI_write_cmos_sensor(	0x0837,0x37);  
IMX134MIPI_write_cmos_sensor(	0x0839,0x1F);  
IMX134MIPI_write_cmos_sensor(	0x083A,0x17);  
IMX134MIPI_write_cmos_sensor(	0x083B,0x02);  
IMX134MIPI_write_cmos_sensor(	0x0202,0x08);  
IMX134MIPI_write_cmos_sensor(	0x0203,0x30);  
IMX134MIPI_write_cmos_sensor(	0x0205,0x00);  
IMX134MIPI_write_cmos_sensor(	0x020E,0x01);  
IMX134MIPI_write_cmos_sensor(	0x020F,0x00);  
IMX134MIPI_write_cmos_sensor(	0x0210,0x01);  
IMX134MIPI_write_cmos_sensor(	0x0211,0x00);  
IMX134MIPI_write_cmos_sensor(	0x0212,0x01);  
IMX134MIPI_write_cmos_sensor(	0x0213,0x00);  
IMX134MIPI_write_cmos_sensor(	0x0214,0x01);  
IMX134MIPI_write_cmos_sensor(	0x0215,0x00);  
IMX134MIPI_write_cmos_sensor(	0x0230,0x00);  
IMX134MIPI_write_cmos_sensor(	0x0231,0x00);  
IMX134MIPI_write_cmos_sensor(	0x0233,0x00);  
IMX134MIPI_write_cmos_sensor(	0x0234,0x00);  
IMX134MIPI_write_cmos_sensor(	0x0235,0x40);  
IMX134MIPI_write_cmos_sensor(	0x0238,0x00);  
IMX134MIPI_write_cmos_sensor(	0x0239,0x04);  
IMX134MIPI_write_cmos_sensor(	0x023B,0x00);  
IMX134MIPI_write_cmos_sensor(	0x023C,0x01);  
IMX134MIPI_write_cmos_sensor(	0x33B0,0x04);  
IMX134MIPI_write_cmos_sensor(	0x33B1,0x00);  
IMX134MIPI_write_cmos_sensor(	0x33B3,0x00);  
IMX134MIPI_write_cmos_sensor(	0x33B4,0x01);  
IMX134MIPI_write_cmos_sensor(	0x3800,0x00);  
IMX134MIPI_write_cmos_sensor(	0x3A43,0x01);  
#endif	
	IMX134MIPI_write_cmos_sensor(0x0104, 0x00);//group
	IMX134MIPI_write_cmos_sensor(0x0100, 0x01);//STREAM ON
	SENSORDB("[IMX134MIPI]exit VideoFullSizeSetting function\n");
}
void IMX134PreviewSetting(void)
{
	//SENSORDB("[IMX134MIPI]enter PreviewSetting function\n");
	IMX134MIPI_write_cmos_sensor(0x0104, 0x01);//group
	IMX134MIPI_write_cmos_sensor(0x0100, 0x00);//STREAM OFF 	
	IMX134MIPI_write_cmos_sensor(0x011E,0x18);
	IMX134MIPI_write_cmos_sensor(0x011F,0x00);
	IMX134MIPI_write_cmos_sensor(0x0301,0x05 );          //0x0301	0x05                0x0301	0x05         0x0301	0x05             0x0301	0x05                     0x0301	0x05      0x0301	0x05 
	IMX134MIPI_write_cmos_sensor(0x0303,0x01);
	IMX134MIPI_write_cmos_sensor(0x0305,0x0C);
	IMX134MIPI_write_cmos_sensor(0x0309,0x05 );          //0x0309	0x05                0x0309	0x05         0x0309	0x05             0x0309	0x05                     0x0309	0x05      0x0309	0x05 
	IMX134MIPI_write_cmos_sensor(0x030B,0x01);
	IMX134MIPI_write_cmos_sensor(0x030C,0x01);
	IMX134MIPI_write_cmos_sensor(0x030D,0x22 );          //0x030D	0x22                0x030D	0x2C         0x030D	0x44             0x030D	0xFA                     0x030D	0x22      0x030D	0x22 
	IMX134MIPI_write_cmos_sensor(0x030E,0x01);
	IMX134MIPI_write_cmos_sensor(0x3A06,0x11);
	//Address	value
	IMX134MIPI_write_cmos_sensor(0x0108,0x03 );          //0x0108	0x03                0x0108	0x03         0x0108	0x03             0x0108	0x03                     0x0108	0x03      0x0108	0x03 
	IMX134MIPI_write_cmos_sensor(0x0112,0x0A);
	IMX134MIPI_write_cmos_sensor(0x0113,0x0A);
	IMX134MIPI_write_cmos_sensor(0x0381,0x01);
	IMX134MIPI_write_cmos_sensor(0x0383,0x01);
	IMX134MIPI_write_cmos_sensor(0x0385,0x01);
	IMX134MIPI_write_cmos_sensor(0x0387,0x01);
	IMX134MIPI_write_cmos_sensor(0x0390,0x01);
	IMX134MIPI_write_cmos_sensor(0x0391,0x22);
	IMX134MIPI_write_cmos_sensor(0x0392,0x00);
	IMX134MIPI_write_cmos_sensor(0x0401,0x00);
	IMX134MIPI_write_cmos_sensor(0x0404,0x00);
	IMX134MIPI_write_cmos_sensor(0x0405,0x10);
	IMX134MIPI_write_cmos_sensor(0x4082,0x01);
	IMX134MIPI_write_cmos_sensor(0x4083,0x01);
	IMX134MIPI_write_cmos_sensor(0x7006,0x04);
	//Address	value
	IMX134MIPI_write_cmos_sensor(0x0700,0x00);
	IMX134MIPI_write_cmos_sensor(0x3A63,0x00);
	IMX134MIPI_write_cmos_sensor(0x4100,0xF8);
	IMX134MIPI_write_cmos_sensor(0x4203,0xFF);
	IMX134MIPI_write_cmos_sensor(0x4344,0x00);
	IMX134MIPI_write_cmos_sensor(0x441C,0x01);
	//Address	value
	IMX134MIPI_write_cmos_sensor(0x0340,0x07);
	IMX134MIPI_write_cmos_sensor(0x0341,0xA0 );          //0x0341	0x48                0x0341	0xD8         0x0341	0x7E             0x0341	0x70                     0x0341	0x80      0x0341	0xA0 
	IMX134MIPI_write_cmos_sensor(0x0342,0x0F );          //0x0342	0x0E                0x0342	0x0E         0x0342	0x0E             0x0342	0x0F                     0x0342	0x0F      0x0342	0x0F 
	IMX134MIPI_write_cmos_sensor(0x0343,0x50 );          //0x0343	0x10                0x0343	0x10         0x0343	0x10             0x0343	0xA0                     0x0343	0xA0      0x0343	0x50 
	IMX134MIPI_write_cmos_sensor(0x0344,0x00);
	IMX134MIPI_write_cmos_sensor(0x0345,0x00);
	IMX134MIPI_write_cmos_sensor(0x0346,0x00);
	IMX134MIPI_write_cmos_sensor(0x0347,0x00);
	IMX134MIPI_write_cmos_sensor(0x0348,0x0C);
	IMX134MIPI_write_cmos_sensor(0x0349,0xCF);
	IMX134MIPI_write_cmos_sensor(0x034A,0x09);
	IMX134MIPI_write_cmos_sensor(0x034B,0x9F);
	IMX134MIPI_write_cmos_sensor(0x034C,0x06);
	IMX134MIPI_write_cmos_sensor(0x034D,0x68);
	IMX134MIPI_write_cmos_sensor(0x034E,0x04);
	IMX134MIPI_write_cmos_sensor(0x034F,0xD0);
	IMX134MIPI_write_cmos_sensor(0x0350,0x00);
	IMX134MIPI_write_cmos_sensor(0x0351,0x00);
	IMX134MIPI_write_cmos_sensor(0x0352,0x00);
	IMX134MIPI_write_cmos_sensor(0x0353,0x00);
	IMX134MIPI_write_cmos_sensor(0x0354,0x06);
	IMX134MIPI_write_cmos_sensor(0x0355,0x68);
	IMX134MIPI_write_cmos_sensor(0x0356,0x04);
	IMX134MIPI_write_cmos_sensor(0x0357,0xD0);
	IMX134MIPI_write_cmos_sensor(0x301D,0x30);
	IMX134MIPI_write_cmos_sensor(0x3310,0x06);
	IMX134MIPI_write_cmos_sensor(0x3311,0x68);
	IMX134MIPI_write_cmos_sensor(0x3312,0x04);
	IMX134MIPI_write_cmos_sensor(0x3313,0xD0);
	IMX134MIPI_write_cmos_sensor(0x331C,0x04);
	IMX134MIPI_write_cmos_sensor(0x331D,0x06);
	IMX134MIPI_write_cmos_sensor(0x4084,0x00);
	IMX134MIPI_write_cmos_sensor(0x4085,0x00);
	IMX134MIPI_write_cmos_sensor(0x4086,0x00);
	IMX134MIPI_write_cmos_sensor(0x4087,0x00);
	IMX134MIPI_write_cmos_sensor(0x4400,0x00);
	//Address	value
	IMX134MIPI_write_cmos_sensor(0x0830,0x6F );          //0x0830	0x6F                0x0830	0x6F         0x0830	0x77             0x0830	0x67                     0x0830	0x6F      0x0830	0x6F 
	IMX134MIPI_write_cmos_sensor(0x0831,0x27 );          //0x0831	0x27                0x0831	0x27         0x0831	0x2F             0x0831	0x27                     0x0831	0x27      0x0831	0x27 
	IMX134MIPI_write_cmos_sensor(0x0832,0x4F );          //0x0832	0x4F                0x0832	0x4F         0x0832	0x4F             0x0832	0x47                     0x0832	0x4F      0x0832	0x4F 
	IMX134MIPI_write_cmos_sensor(0x0833,0x2F );          //0x0833	0x2F                0x0833	0x2F         0x0833	0x2F             0x0833	0x27                     0x0833	0x2F      0x0833	0x2F 
	IMX134MIPI_write_cmos_sensor(0x0834,0x2F );          //0x0834	0x2F                0x0834	0x2F         0x0834	0x2F             0x0834	0x27                     0x0834	0x2F      0x0834	0x2F 
	IMX134MIPI_write_cmos_sensor(0x0835,0x2F );          //0x0835	0x2F                0x0835	0x2F         0x0835	0x37             0x0835	0x1F                     0x0835	0x2F      0x0835	0x2F 
	IMX134MIPI_write_cmos_sensor(0x0836,0x9F );          //0x0836	0x9F                0x0836	0x9F         0x0836	0xA7             0x0836	0x87                     0x0836	0x9F      0x0836	0x9F 
	IMX134MIPI_write_cmos_sensor(0x0837,0x37 );          //0x0837	0x37                0x0837	0x37         0x0837	0x37             0x0837	0x2F                     0x0837	0x37      0x0837	0x37 
	IMX134MIPI_write_cmos_sensor(0x0839,0x1F);
	IMX134MIPI_write_cmos_sensor(0x083A,0x17);
	IMX134MIPI_write_cmos_sensor(0x083B,0x02);
	//Address	value
	IMX134MIPI_write_cmos_sensor(0x0202,0x07);
	IMX134MIPI_write_cmos_sensor(0x0203,0x9C );          //0x0203	0x44                0x0203	0xD4         0x0203	0x7A             0x0203	0x6C                     0x0203	0x7C      0x0203	0x9C 
	//Address	value
	IMX134MIPI_write_cmos_sensor(0x0205,0x00);
	IMX134MIPI_write_cmos_sensor(0x020E,0x01);
	IMX134MIPI_write_cmos_sensor(0x020F,0x00);
	IMX134MIPI_write_cmos_sensor(0x0210,0x01);
	IMX134MIPI_write_cmos_sensor(0x0211,0x00);
	IMX134MIPI_write_cmos_sensor(0x0212,0x01);
	IMX134MIPI_write_cmos_sensor(0x0213,0x00);
	IMX134MIPI_write_cmos_sensor(0x0214,0x01);
	IMX134MIPI_write_cmos_sensor(0x0215,0x00);
	//Address	value
	IMX134MIPI_write_cmos_sensor(0x0230,0x00);
	IMX134MIPI_write_cmos_sensor(0x0231,0x00);
	IMX134MIPI_write_cmos_sensor(0x0233,0x00);
	IMX134MIPI_write_cmos_sensor(0x0234,0x00);
	IMX134MIPI_write_cmos_sensor(0x0235,0x40);
	IMX134MIPI_write_cmos_sensor(0x0238,0x00);
	IMX134MIPI_write_cmos_sensor(0x0239,0x04);
	IMX134MIPI_write_cmos_sensor(0x023B,0x00);
	IMX134MIPI_write_cmos_sensor(0x023C,0x01);
	IMX134MIPI_write_cmos_sensor(0x33B0,0x04);
	IMX134MIPI_write_cmos_sensor(0x33B1,0x00);
	IMX134MIPI_write_cmos_sensor(0x33B3,0x00);
	IMX134MIPI_write_cmos_sensor(0x33B4,0x01);
	IMX134MIPI_write_cmos_sensor(0x3800,0x00);
	IMX134MIPI_write_cmos_sensor(	0x3A43,0x01);   
	IMX134MIPI_write_cmos_sensor(0x0104, 0x00);//group
	IMX134MIPI_write_cmos_sensor(0x0100, 0x01);//STREAM ON

	spin_lock(&imx134_drv_lock);  
	IMX134MIPI_Auto_Flicker_mode = KAL_FALSE;	  // reset the flicker status	 
	spin_unlock(&imx134_drv_lock);
	SENSORDB("[IMX134MIPI]exit PreviewSetting function\n");
}
  
void IMX134MIPI_set_8M(void)
{
	SENSORDB("[IMX134MIPI]enter IMX134MIPI_set_8M function\n");
	IMX134MIPI_write_cmos_sensor(0x0104, 0x01);//group
	IMX134MIPI_write_cmos_sensor(0x0100, 0x00);//STREAM OFF 	
	IMX134MIPI_write_cmos_sensor(0x0101, 0x00); 
	#if 1 //3280 X 2464		 
	IMX134MIPI_write_cmos_sensor(0x011E,0x18);
	IMX134MIPI_write_cmos_sensor(0x011F,0x00);
	IMX134MIPI_write_cmos_sensor(0x0301,0x05);  
	IMX134MIPI_write_cmos_sensor(0x0303,0x01);
	IMX134MIPI_write_cmos_sensor(0x0305,0x0C);
	IMX134MIPI_write_cmos_sensor(0x0309,0x05);  
	IMX134MIPI_write_cmos_sensor(0x030B,0x01);
	IMX134MIPI_write_cmos_sensor(0x030C,0x01);
	IMX134MIPI_write_cmos_sensor(0x030D,0x22);  
	IMX134MIPI_write_cmos_sensor(0x030E,0x01);
	IMX134MIPI_write_cmos_sensor(0x3A06,0x11);
	
	//Address value
	IMX134MIPI_write_cmos_sensor(0x0108,0x03);  
	IMX134MIPI_write_cmos_sensor(0x0112,0x0A);
	IMX134MIPI_write_cmos_sensor(0x0113,0x0A);
	IMX134MIPI_write_cmos_sensor(0x0381,0x01);
	IMX134MIPI_write_cmos_sensor(0x0383,0x01);
	IMX134MIPI_write_cmos_sensor(0x0385,0x01);
	IMX134MIPI_write_cmos_sensor(0x0387,0x01);
	IMX134MIPI_write_cmos_sensor(0x0390,0x00);
	IMX134MIPI_write_cmos_sensor(0x0391,0x11);
	IMX134MIPI_write_cmos_sensor(0x0392,0x00);
	IMX134MIPI_write_cmos_sensor(0x0401,0x00);
	IMX134MIPI_write_cmos_sensor(0x0404,0x00);
	IMX134MIPI_write_cmos_sensor(0x0405,0x10);
	IMX134MIPI_write_cmos_sensor(0x4082,0x01);
	IMX134MIPI_write_cmos_sensor(0x4083,0x01);
	IMX134MIPI_write_cmos_sensor(0x7006,0x04);
	
	//Address value
	IMX134MIPI_write_cmos_sensor(0x0700,0x00);
	IMX134MIPI_write_cmos_sensor(0x3A63,0x00);
	IMX134MIPI_write_cmos_sensor(0x4100,0xF8);
	IMX134MIPI_write_cmos_sensor(0x4203,0xFF);
	IMX134MIPI_write_cmos_sensor(0x4344,0x00);
	IMX134MIPI_write_cmos_sensor(0x441C,0x01);
	
	//Address value
	IMX134MIPI_write_cmos_sensor(0x0340,0x09);
	IMX134MIPI_write_cmos_sensor(0x0341,0xEC);  
	IMX134MIPI_write_cmos_sensor(0x0342,0x0E);
	IMX134MIPI_write_cmos_sensor(0x0343,0x10);
	IMX134MIPI_write_cmos_sensor(0x0344,0x00);
	IMX134MIPI_write_cmos_sensor(0x0345,0x00);
	IMX134MIPI_write_cmos_sensor(0x0346,0x00);
	IMX134MIPI_write_cmos_sensor(0x0347,0x00);
	IMX134MIPI_write_cmos_sensor(0x0348,0x0C);
	IMX134MIPI_write_cmos_sensor(0x0349,0xCF);
	IMX134MIPI_write_cmos_sensor(0x034A,0x09);
	IMX134MIPI_write_cmos_sensor(0x034B,0x9F);
	IMX134MIPI_write_cmos_sensor(0x034C,0x0C);
	IMX134MIPI_write_cmos_sensor(0x034D,0xD0);
	IMX134MIPI_write_cmos_sensor(0x034E,0x09);
	IMX134MIPI_write_cmos_sensor(0x034F,0xA0);
	IMX134MIPI_write_cmos_sensor(0x0350,0x00);
	IMX134MIPI_write_cmos_sensor(0x0351,0x00);
	IMX134MIPI_write_cmos_sensor(0x0352,0x00);
	IMX134MIPI_write_cmos_sensor(0x0353,0x00);
	IMX134MIPI_write_cmos_sensor(0x0354,0x0C);
	IMX134MIPI_write_cmos_sensor(0x0355,0xD0);
	IMX134MIPI_write_cmos_sensor(0x0356,0x09);
	IMX134MIPI_write_cmos_sensor(0x0357,0xA0);
	IMX134MIPI_write_cmos_sensor(0x301D,0x30);
	IMX134MIPI_write_cmos_sensor(0x3310,0x0C);
	IMX134MIPI_write_cmos_sensor(0x3311,0xD0);
	IMX134MIPI_write_cmos_sensor(0x3312,0x09);
	IMX134MIPI_write_cmos_sensor(0x3313,0xA0);
	IMX134MIPI_write_cmos_sensor(0x331C,0x01);
	IMX134MIPI_write_cmos_sensor(0x331D,0xAE);
	IMX134MIPI_write_cmos_sensor(0x4084,0x00);
	IMX134MIPI_write_cmos_sensor(0x4085,0x00);
	IMX134MIPI_write_cmos_sensor(0x4086,0x00);
	IMX134MIPI_write_cmos_sensor(0x4087,0x00);
	IMX134MIPI_write_cmos_sensor(0x4400,0x00);
	
	//Address value
	IMX134MIPI_write_cmos_sensor(0x0830,0x6F);  
	IMX134MIPI_write_cmos_sensor(0x0831,0x27);  
	IMX134MIPI_write_cmos_sensor(0x0832,0x4F);  
	IMX134MIPI_write_cmos_sensor(0x0833,0x2F);  
	IMX134MIPI_write_cmos_sensor(0x0834,0x2F);  
	IMX134MIPI_write_cmos_sensor(0x0835,0x2F);  
	IMX134MIPI_write_cmos_sensor(0x0836,0x9F);  
	IMX134MIPI_write_cmos_sensor(0x0837,0x37);  
	IMX134MIPI_write_cmos_sensor(0x0839,0x1F);
	IMX134MIPI_write_cmos_sensor(0x083A,0x17);
	IMX134MIPI_write_cmos_sensor(0x083B,0x02);
	//Address value
	IMX134MIPI_write_cmos_sensor(0x0202,0x09);
	IMX134MIPI_write_cmos_sensor(0x0203,0xE8);  
	//Address value
	IMX134MIPI_write_cmos_sensor(0x0205,0x00);
	IMX134MIPI_write_cmos_sensor(0x020E,0x01);
	IMX134MIPI_write_cmos_sensor(0x020F,0x00);
	IMX134MIPI_write_cmos_sensor(0x0210,0x01);
	IMX134MIPI_write_cmos_sensor(0x0211,0x00);
	IMX134MIPI_write_cmos_sensor(0x0212,0x01);
	IMX134MIPI_write_cmos_sensor(0x0213,0x00);
	IMX134MIPI_write_cmos_sensor(0x0214,0x01);
	IMX134MIPI_write_cmos_sensor(0x0215,0x00);
	//Address value
	IMX134MIPI_write_cmos_sensor(0x0230,0x00);
	IMX134MIPI_write_cmos_sensor(0x0231,0x00);
	IMX134MIPI_write_cmos_sensor(0x0233,0x00);
	IMX134MIPI_write_cmos_sensor(0x0234,0x00);
	IMX134MIPI_write_cmos_sensor(0x0235,0x40);
	IMX134MIPI_write_cmos_sensor(0x0238,0x00);
	IMX134MIPI_write_cmos_sensor(0x0239,0x04);
	IMX134MIPI_write_cmos_sensor(0x023B,0x00);
	IMX134MIPI_write_cmos_sensor(0x023C,0x01);
	IMX134MIPI_write_cmos_sensor(0x33B0,0x04);
	IMX134MIPI_write_cmos_sensor(0x33B1,0x00);
	IMX134MIPI_write_cmos_sensor(0x33B3,0x00);
	IMX134MIPI_write_cmos_sensor(0x33B4,0x01);
	IMX134MIPI_write_cmos_sensor(0x3800,0x00);
#else //1640 X 1232
IMX134MIPI_write_cmos_sensor(0x011E,0x18);          //0x011E	0x18   
IMX134MIPI_write_cmos_sensor(0x011F,0x00);          //0x011F	0x00   
IMX134MIPI_write_cmos_sensor(0x0301,0x05);          //0x0301	0x05   
IMX134MIPI_write_cmos_sensor(0x0303,0x01);          //0x0303	0x01   
IMX134MIPI_write_cmos_sensor(0x0305,0x0C);          //0x0305	0x0C   
IMX134MIPI_write_cmos_sensor(0x0309,0x05);          //0x0309	0x05   
IMX134MIPI_write_cmos_sensor(0x030B,0x01);          //0x030B	0x01   
IMX134MIPI_write_cmos_sensor(0x030C,0x01);          //0x030C	0x01   
IMX134MIPI_write_cmos_sensor(0x030D,0x22);          //0x030D	0x22   
IMX134MIPI_write_cmos_sensor(0x030E,0x01);          //0x030E	0x01   
IMX134MIPI_write_cmos_sensor(0x3A06,0x11);          //0x3A06	0x11   
IMX134MIPI_write_cmos_sensor(0x0108,0x03);          //0x0108	0x03   
IMX134MIPI_write_cmos_sensor(0x0112,0x0A);          //0x0112	0x0A   
IMX134MIPI_write_cmos_sensor(0x0113,0x0A);          //0x0113	0x0A   
IMX134MIPI_write_cmos_sensor(0x0381,0x01);          //0x0381	0x01   
IMX134MIPI_write_cmos_sensor(0x0383,0x01);          //0x0383	0x01   
IMX134MIPI_write_cmos_sensor(0x0385,0x01);          //0x0385	0x01   
IMX134MIPI_write_cmos_sensor(0x0387,0x01);          //0x0387	0x01   
IMX134MIPI_write_cmos_sensor(0x0390,0x00);          //0x0390	0x00   
IMX134MIPI_write_cmos_sensor(0x0391,0x11);          //0x0391	0x11   
IMX134MIPI_write_cmos_sensor(0x0392,0x00);          //0x0392	0x00   
IMX134MIPI_write_cmos_sensor(0x0401,0x02);          //0x0401	0x02   
IMX134MIPI_write_cmos_sensor(0x0404,0x00);          //0x0404	0x00   
IMX134MIPI_write_cmos_sensor(0x0405,0x20);          //0x0405	0x20   
IMX134MIPI_write_cmos_sensor(0x4082,0x00);          //0x4082	0x00   
IMX134MIPI_write_cmos_sensor(0x4083,0x00);          //0x4083	0x00   
IMX134MIPI_write_cmos_sensor(0x7006,0x04);          //0x7006	0x04   
IMX134MIPI_write_cmos_sensor(0x0700,0x00);          //0x0700	0x00   
IMX134MIPI_write_cmos_sensor(0x3A63,0x00);          //0x3A63	0x00   
IMX134MIPI_write_cmos_sensor(0x4100,0xF8);          //0x4100	0xF8   
IMX134MIPI_write_cmos_sensor(0x4203,0xFF);          //0x4203	0xFF   
IMX134MIPI_write_cmos_sensor(0x4344,0x00);          //0x4344	0x00   
IMX134MIPI_write_cmos_sensor(0x441C,0x01);          //0x441C	0x01   
IMX134MIPI_write_cmos_sensor(0x0340,0x08);          //0x0340	0x08   
IMX134MIPI_write_cmos_sensor(0x0341,0x48);          //0x0341	0x48   
IMX134MIPI_write_cmos_sensor(0x0342,0x0E);          //0x0342	0x0E   
IMX134MIPI_write_cmos_sensor(0x0343,0x10);          //0x0343	0x10   
IMX134MIPI_write_cmos_sensor(0x0344,0x00);          //0x0344	0x00   
IMX134MIPI_write_cmos_sensor(0x0345,0x00);          //0x0345	0x00   
IMX134MIPI_write_cmos_sensor(0x0346,0x00);          //0x0346	0x00   
IMX134MIPI_write_cmos_sensor(0x0347,0x00);          //0x0347	0x00   
IMX134MIPI_write_cmos_sensor(0x0348,0x0C);          //0x0348	0x0C   
IMX134MIPI_write_cmos_sensor(0x0349,0xCF);          //0x0349	0xCF   
IMX134MIPI_write_cmos_sensor(0x034A,0x09);          //0x034A	0x09   
IMX134MIPI_write_cmos_sensor(0x034B,0x9F);          //0x034B	0x9F   
IMX134MIPI_write_cmos_sensor(0x034C,0x06);          //0x034C	0x06   
IMX134MIPI_write_cmos_sensor(0x034D,0x68);          //0x034D	0x68   
IMX134MIPI_write_cmos_sensor(0x034E,0x04);          //0x034E	0x04   
IMX134MIPI_write_cmos_sensor(0x034F,0xD0);          //0x034F	0xD0   
IMX134MIPI_write_cmos_sensor(0x0350,0x00);          //0x0350	0x00   
IMX134MIPI_write_cmos_sensor(0x0351,0x00);          //0x0351	0x00   
IMX134MIPI_write_cmos_sensor(0x0352,0x00);          //0x0352	0x00   
IMX134MIPI_write_cmos_sensor(0x0353,0x00);          //0x0353	0x00   
IMX134MIPI_write_cmos_sensor(0x0354,0x0C);          //0x0354	0x0C   
IMX134MIPI_write_cmos_sensor(0x0355,0xD0);          //0x0355	0xD0   
IMX134MIPI_write_cmos_sensor(0x0356,0x09);          //0x0356	0x09   
IMX134MIPI_write_cmos_sensor(0x0357,0xA0);          //0x0357	0xA0   
IMX134MIPI_write_cmos_sensor(0x301D,0x30);          //0x301D	0x30   
IMX134MIPI_write_cmos_sensor(0x3310,0x06);          //0x3310	0x06   
IMX134MIPI_write_cmos_sensor(0x3311,0x68);          //0x3311	0x68   
IMX134MIPI_write_cmos_sensor(0x3312,0x04);          //0x3312	0x04   
IMX134MIPI_write_cmos_sensor(0x3313,0xD0);          //0x3313	0xD0   
IMX134MIPI_write_cmos_sensor(0x331C,0x04);          //0x331C	0x04   
IMX134MIPI_write_cmos_sensor(0x331D,0x1E);          //0x331D	0x1E   
IMX134MIPI_write_cmos_sensor(0x4084,0x06);          //0x4084	0x06   
IMX134MIPI_write_cmos_sensor(0x4085,0x68);          //0x4085	0x68   
IMX134MIPI_write_cmos_sensor(0x4086,0x04);          //0x4086	0x04   
IMX134MIPI_write_cmos_sensor(0x4087,0xD0);          //0x4087	0xD0   
IMX134MIPI_write_cmos_sensor(0x4400,0x00);          //0x4400	0x00   
IMX134MIPI_write_cmos_sensor(0x0830,0x6F);          //0x0830	0x6F   
IMX134MIPI_write_cmos_sensor(0x0831,0x27);          //0x0831	0x27   
IMX134MIPI_write_cmos_sensor(0x0832,0x4F);          //0x0832	0x4F   
IMX134MIPI_write_cmos_sensor(0x0833,0x2F);          //0x0833	0x2F   
IMX134MIPI_write_cmos_sensor(0x0834,0x2F);          //0x0834	0x2F   
IMX134MIPI_write_cmos_sensor(0x0835,0x2F);          //0x0835	0x2F   
IMX134MIPI_write_cmos_sensor(0x0836,0x9F);          //0x0836	0x9F   
IMX134MIPI_write_cmos_sensor(0x0837,0x37);          //0x0837	0x37   
IMX134MIPI_write_cmos_sensor(0x0839,0x1F);          //0x0839	0x1F   
IMX134MIPI_write_cmos_sensor(0x083A,0x17);          //0x083A	0x17   
IMX134MIPI_write_cmos_sensor(0x083B,0x02);          //0x083B	0x02   
IMX134MIPI_write_cmos_sensor(0x0202,0x08);          //0x0202	0x08   
IMX134MIPI_write_cmos_sensor(0x0203,0x44);          //0x0203	0x44   
IMX134MIPI_write_cmos_sensor(0x0205,0x00);          //0x0205	0x00   
IMX134MIPI_write_cmos_sensor(0x020E,0x01);          //0x020E	0x01   
IMX134MIPI_write_cmos_sensor(0x020F,0x00);          //0x020F	0x00   
IMX134MIPI_write_cmos_sensor(0x0210,0x01);          //0x0210	0x01   
IMX134MIPI_write_cmos_sensor(0x0211,0x00);          //0x0211	0x00   
IMX134MIPI_write_cmos_sensor(0x0212,0x01);          //0x0212	0x01   
IMX134MIPI_write_cmos_sensor(0x0213,0x00);          //0x0213	0x00   
IMX134MIPI_write_cmos_sensor(0x0214,0x01);          //0x0214	0x01   
IMX134MIPI_write_cmos_sensor(0x0215,0x00);          //0x0215	0x00   
IMX134MIPI_write_cmos_sensor(0x0230,0x00);          //0x0230	0x00   
IMX134MIPI_write_cmos_sensor(0x0231,0x00);          //0x0231	0x00   
IMX134MIPI_write_cmos_sensor(0x0233,0x00);          //0x0233	0x00   
IMX134MIPI_write_cmos_sensor(0x0234,0x00);          //0x0234	0x00   
IMX134MIPI_write_cmos_sensor(0x0235,0x40);          //0x0235	0x40   
IMX134MIPI_write_cmos_sensor(0x0238,0x00);          //0x0238	0x00   
IMX134MIPI_write_cmos_sensor(0x0239,0x04);          //0x0239	0x04   
IMX134MIPI_write_cmos_sensor(0x023B,0x00);          //0x023B	0x00   
IMX134MIPI_write_cmos_sensor(0x023C,0x01);          //0x023C	0x01   
IMX134MIPI_write_cmos_sensor(0x33B0,0x04);          //0x33B0	0x04   
IMX134MIPI_write_cmos_sensor(0x33B1,0x00);          //0x33B1	0x00   
IMX134MIPI_write_cmos_sensor(0x33B3,0x00);          //0x33B3	0x00   
IMX134MIPI_write_cmos_sensor(0x33B4,0x01);          //0x33B4	0x01   
IMX134MIPI_write_cmos_sensor(0x3800,0x00);          //0x3800	0x00   
#endif                                             
	IMX134MIPI_write_cmos_sensor(0x3A43,0x01);  
	IMX134MIPI_write_cmos_sensor(0x0104, 0x00);//group
	IMX134MIPI_write_cmos_sensor(0x0100, 0x01);//STREAM ON
	SENSORDB("[IMX134MIPI]exit IMX134MIPI_set_8M function\n"); 
}
/*****************************************************************************/
/* Windows Mobile Sensor Interface */
/*****************************************************************************/
/*************************************************************************
* FUNCTION
*   IMX134MIPIOpen
*
* DESCRIPTION
*   This function initialize the registers of CMOS sensor
*
* PARAMETERS
*   None
*
* RETURNS
*   None
*
* GLOBALS AFFECTED
*
*************************************************************************/

UINT32 IMX134MIPIOpen(void)
{
    int  retry = 0; 
	kal_uint16 sensorid;
    // check if sensor ID correct
    retry = 3; 
	SENSORDB("[IMX134MIPI]enter IMX134MIPIOpen function\n");
    do {
	   sensorid=(kal_uint16)(((IMX134MIPI_read_cmos_sensor(0x0016)&&0x0f)<<8) | IMX134MIPI_read_cmos_sensor(0x0017));  
	   spin_lock(&imx134_drv_lock);    
	   IMX134MIPI_sensor_id =sensorid;
	   spin_unlock(&imx134_drv_lock);
		if (IMX134MIPI_sensor_id == IMX134MIPI_SENSOR_ID)
			break; 
		retry--; 
	    }
	while (retry > 0);
    SENSORDB("Read Sensor ID = 0x%04x\n", IMX134MIPI_sensor_id);
    if (IMX134MIPI_sensor_id != IMX134MIPI_SENSOR_ID)
        return ERROR_SENSOR_CONNECT_FAIL;
    IMX134MIPI_Sensor_Init();
	
    IMX134MIPI_Para_Init();
	sensorid=read_IMX134MIPI_gain();
	spin_lock(&imx134_drv_lock);	
    IMX134MIPI_sensor_gain_base = sensorid;
	spin_unlock(&imx134_drv_lock);
	SENSORDB("[IMX134MIPI]exit IMX134MIPIOpen function\n");
    return ERROR_NONE;
}

/*************************************************************************
* FUNCTION
*   IMX134MIPIGetSensorID
*
* DESCRIPTION
*   This function get the sensor ID 
*
* PARAMETERS
*   *sensorID : return the sensor ID 
*
* RETURNS
*   None
*
* GLOBALS AFFECTED
*
*************************************************************************/
UINT32 IMX134MIPIGetSensorID(UINT32 *sensorID) 
{
    int  retry = 3; 
	SENSORDB("[IMX134MIPI]enter IMX134MIPIGetSensorID function\n");
    // check if sensor ID correct
    do {		
	   *sensorID =(kal_uint16)(((IMX134MIPI_read_cmos_sensor(0x0016)&&0x0f)<<8) | IMX134MIPI_read_cmos_sensor(0x0017)); 
        if (*sensorID == IMX134MIPI_SENSOR_ID)
            break;
        retry--; 
    } while (retry > 0);

    if (*sensorID != IMX134MIPI_SENSOR_ID) {
        *sensorID = 0xFFFFFFFF; 
        return ERROR_SENSOR_CONNECT_FAIL;
    }
	SENSORDB("[IMX134MIPI]exit IMX134MIPIGetSensorID function\n");
    return ERROR_NONE;
}


/*************************************************************************
* FUNCTION
*   IMX134MIPI_SetShutter
*
* DESCRIPTION
*   This function set e-shutter of IMX134MIPI to change exposure time.
*
* PARAMETERS
*   shutter : exposured lines
*
* RETURNS
*   None
*
* GLOBALS AFFECTED
*
*************************************************************************/
void IMX134MIPI_SetShutter(kal_uint16 iShutter)
{

	//SENSORDB("[IMX134MIPI]%s():shutter=%d\n",__FUNCTION__,iShutter);
    if (iShutter < 1)
        iShutter = 1; 
	else if(iShutter > 0xffff)
		iShutter = 0xffff;
	unsigned long flags;
	spin_lock_irqsave(&imx134_drv_lock,flags);
    IMX134MIPI_sensor.pv_shutter = iShutter;	
	spin_unlock_irqrestore(&imx134_drv_lock,flags);
    IMX134MIPI_write_shutter(iShutter);
	//SENSORDB("[IMX134MIPI]exit IMX134MIPIGetSensorID function\n");
}   /*  IMX134MIPI_SetShutter   */



/*************************************************************************
* FUNCTION
*   IMX134MIPI_read_shutter
*
* DESCRIPTION
*   This function to  Get exposure time.
*
* PARAMETERS
*   None
*
* RETURNS
*   shutter : exposured lines
*
* GLOBALS AFFECTED
*
*************************************************************************/
UINT16 IMX134MIPI_read_shutter(void)
{
    return (UINT16)( (IMX134MIPI_read_cmos_sensor(0x0202)<<8) | IMX134MIPI_read_cmos_sensor(0x0203) );
}

/*************************************************************************
* FUNCTION
*   IMX134MIPI_night_mode
*
* DESCRIPTION
*   This function night mode of IMX134MIPI.
*
* PARAMETERS
*   none
*
* RETURNS
*   None
*
* GLOBALS AFFECTED
*
*************************************************************************/
void IMX134MIPI_NightMode(kal_bool bEnable)
{
	SENSORDB("[IMX134MIPI]enter IMX134MIPI_NightMode function\n");
#if 0
    /************************************************************************/
    /*                      Auto Mode: 30fps                                                                                          */
    /*                      Night Mode:15fps                                                                                          */
    /************************************************************************/
    if(bEnable)
    {
        if(OV5642_MPEG4_encode_mode==KAL_TRUE)
        {
            OV5642_MAX_EXPOSURE_LINES = (kal_uint16)((OV5642_sensor_pclk/15)/(OV5642_PV_PERIOD_PIXEL_NUMS+OV5642_PV_dummy_pixels));
            OV5642_write_cmos_sensor(0x350C, (OV5642_MAX_EXPOSURE_LINES >> 8) & 0xFF);
            OV5642_write_cmos_sensor(0x350D, OV5642_MAX_EXPOSURE_LINES & 0xFF);
            OV5642_CURRENT_FRAME_LINES = OV5642_MAX_EXPOSURE_LINES;
            OV5642_MAX_EXPOSURE_LINES = OV5642_CURRENT_FRAME_LINES - OV5642_SHUTTER_LINES_GAP;
        }
    }
    else// Fix video framerate 30 fps
    {
        if(OV5642_MPEG4_encode_mode==KAL_TRUE)
        {
            OV5642_MAX_EXPOSURE_LINES = (kal_uint16)((OV5642_sensor_pclk/30)/(OV5642_PV_PERIOD_PIXEL_NUMS+OV5642_PV_dummy_pixels));
            if(OV5642_pv_exposure_lines < (OV5642_MAX_EXPOSURE_LINES - OV5642_SHUTTER_LINES_GAP)) // for avoid the shutter > frame_lines,move the frame lines setting to shutter function
            {
                OV5642_write_cmos_sensor(0x350C, (OV5642_MAX_EXPOSURE_LINES >> 8) & 0xFF);
                OV5642_write_cmos_sensor(0x350D, OV5642_MAX_EXPOSURE_LINES & 0xFF);
                OV5642_CURRENT_FRAME_LINES = OV5642_MAX_EXPOSURE_LINES;
            }
            OV5642_MAX_EXPOSURE_LINES = OV5642_MAX_EXPOSURE_LINES - OV5642_SHUTTER_LINES_GAP;
        }
    }
	
#endif	
	SENSORDB("[IMX134MIPI]exit IMX134MIPI_NightMode function\n");
}/*	IMX134MIPI_NightMode */



/*************************************************************************
* FUNCTION
*   IMX134MIPIClose
*
* DESCRIPTION
*   This function is to turn off sensor module power.
*
* PARAMETERS
*   None
*
* RETURNS
*   None
*
* GLOBALS AFFECTED
*
*************************************************************************/
UINT32 IMX134MIPIClose(void)
{
    IMX134MIPI_write_cmos_sensor(0x0100,0x00);
    return ERROR_NONE;
}	/* IMX134MIPIClose() */

void IMX134MIPISetFlipMirror(kal_int32 imgMirror)
{
    kal_uint8  iTemp; 
	SENSORDB("[IMX134MIPI]enter IMX134MIPISetFlipMirror function\n");
    iTemp = IMX134MIPI_read_cmos_sensor(0x0101) & 0x03;	//Clear the mirror and flip bits.
    switch (imgMirror)
    {
        case IMAGE_NORMAL:
            IMX134MIPI_write_cmos_sensor(0x0101, 0x03);	//Set normal
            break;
        case IMAGE_V_MIRROR:
            IMX134MIPI_write_cmos_sensor(0x0101, iTemp | 0x01);	//Set flip
            break;
        case IMAGE_H_MIRROR:
            IMX134MIPI_write_cmos_sensor(0x0101, iTemp | 0x02);	//Set mirror
            break;
        case IMAGE_HV_MIRROR:
            IMX134MIPI_write_cmos_sensor(0x0101, 0x00);	//Set mirror and flip
            break;
    }
	SENSORDB("[IMX134MIPI]exit IMX134MIPISetFlipMirror function\n");
}


/*************************************************************************
* FUNCTION
*   IMX134MIPIPreview
*
* DESCRIPTION
*   This function start the sensor preview.
*
* PARAMETERS
*   *image_window : address pointer of pixel numbers in one period of HSYNC
*  *sensor_config_data : address pointer of line numbers in one period of VSYNC
*
* RETURNS
*   None
*
* GLOBALS AFFECTED
*
*************************************************************************/
UINT32 IMX134MIPIPreview(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                                                MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	kal_uint16 iStartX = 0, iStartY = 0;	
	SENSORDB("[IMX134MIPI]enter IMX134MIPIPreview function\n");
	spin_lock(&imx134_drv_lock);    
	IMX134MIPI_MPEG4_encode_mode = KAL_FALSE;
	IMX134MIPI_sensor.video_mode=KAL_FALSE;
	IMX134MIPI_sensor.pv_mode=KAL_TRUE;
	IMX134MIPI_sensor.capture_mode=KAL_FALSE;
	spin_unlock(&imx134_drv_lock);
	IMX134PreviewSetting();
	iStartX += IMX134MIPI_IMAGE_SENSOR_PV_STARTX;
	iStartY += IMX134MIPI_IMAGE_SENSOR_PV_STARTY;
	spin_lock(&imx134_drv_lock);	
	IMX134MIPI_sensor.cp_dummy_pixels = 0;
	IMX134MIPI_sensor.cp_dummy_lines = 0;
	IMX134MIPI_sensor.pv_dummy_pixels = 0;
	IMX134MIPI_sensor.pv_dummy_lines = 0;
	IMX134MIPI_sensor.video_dummy_pixels = 0;
	IMX134MIPI_sensor.video_dummy_lines = 0;
	IMX134MIPI_sensor.pv_line_length = IMX134MIPI_PV_LINE_LENGTH_PIXELS+IMX134MIPI_sensor.pv_dummy_pixels; 
	IMX134MIPI_sensor.pv_frame_length = IMX134MIPI_PV_FRAME_LENGTH_LINES+IMX134MIPI_sensor.pv_dummy_lines;
	spin_unlock(&imx134_drv_lock);

	IMX134MIPI_SetDummy(IMX134MIPI_sensor.pv_dummy_pixels,IMX134MIPI_sensor.pv_dummy_lines);
	//IMX134MIPI_SetShutter(IMX134MIPI_sensor.pv_shutter);
	spin_lock(&imx134_drv_lock);	
	memcpy(&IMX134MIPISensorConfigData, sensor_config_data, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
	spin_unlock(&imx134_drv_lock);
	image_window->GrabStartX= iStartX;
	image_window->GrabStartY= iStartY;
	image_window->ExposureWindowWidth= IMX134MIPI_REAL_PV_WIDTH ;
	image_window->ExposureWindowHeight= IMX134MIPI_REAL_PV_HEIGHT ; 
	SENSORDB("[IMX134MIPI]eXIT IMX134MIPIPreview function\n"); 
	return ERROR_NONE;
	}	/* IMX134MIPIPreview() */

/*************************************************************************
* FUNCTION
* RETURNS
*   None
*
* GLOBALS AFFECTED
*
*************************************************************************/
UINT32 IMX134MIPIVideo(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                                                MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	kal_uint16 iStartX = 0, iStartY = 0;
	SENSORDB("[IMX134MIPI]enter IMX134MIPIVideo function\n"); 
	spin_lock(&imx134_drv_lock);    
    IMX134MIPI_MPEG4_encode_mode = KAL_TRUE;  
	IMX134MIPI_sensor.video_mode=KAL_TRUE;
	IMX134MIPI_sensor.pv_mode=KAL_FALSE;
	IMX134MIPI_sensor.capture_mode=KAL_FALSE;
	spin_unlock(&imx134_drv_lock);
	
	IMX134VideoFullSizeSetting();
	//PreviewSetting();  //modify by yyf for video ==> preview
	
	iStartX += IMX134MIPI_IMAGE_SENSOR_VIDEO_STARTX;
	iStartY += IMX134MIPI_IMAGE_SENSOR_VIDEO_STARTY;
	spin_lock(&imx134_drv_lock);	
	IMX134MIPI_sensor.cp_dummy_pixels = 0;
	IMX134MIPI_sensor.cp_dummy_lines = 0;
	IMX134MIPI_sensor.pv_dummy_pixels = 0;
	IMX134MIPI_sensor.pv_dummy_lines = 0;
	IMX134MIPI_sensor.video_dummy_pixels = 0;
	IMX134MIPI_sensor.video_dummy_lines = 0;
	IMX134MIPI_sensor.video_line_length = IMX134MIPI_VIDEO_LINE_LENGTH_PIXELS+IMX134MIPI_sensor.video_dummy_pixels; 
	IMX134MIPI_sensor.video_frame_length = IMX134MIPI_VIDEO_FRAME_LENGTH_LINES+IMX134MIPI_sensor.video_dummy_lines;
	spin_unlock(&imx134_drv_lock);
	
	IMX134MIPI_SetDummy(IMX134MIPI_sensor.video_dummy_pixels,IMX134MIPI_sensor.video_dummy_lines);
	//IMX134MIPI_SetShutter(IMX134MIPI_sensor.video_shutter);
	spin_lock(&imx134_drv_lock);	
	memcpy(&IMX134MIPISensorConfigData, sensor_config_data, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
	spin_unlock(&imx134_drv_lock);
	image_window->GrabStartX= iStartX;
	image_window->GrabStartY= iStartY;    
    SENSORDB("[IMX134MIPI]eXIT IMX134MIPIVideo function\n"); 
	return ERROR_NONE;
}	/* IMX134MIPIPreview() */

UINT32 IMX134MIPICapture(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                                                MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	kal_uint16 iStartX = 0, iStartY = 0;
	SENSORDB("[IMX134MIPI]enter IMX134MIPICapture function\n");
	if(IMX134MIPI_sensor.capture_mode==KAL_TRUE)//ZSD mode Return deretly
	return ERROR_NONE;
	spin_lock(&imx134_drv_lock);	
	IMX134MIPI_sensor.video_mode=KAL_FALSE;
	IMX134MIPI_sensor.pv_mode=KAL_FALSE;
	IMX134MIPI_sensor.capture_mode=KAL_TRUE;
	IMX134MIPI_MPEG4_encode_mode = KAL_FALSE; 
	IMX134MIPI_Auto_Flicker_mode = KAL_FALSE;       
	IMX134MIPI_sensor.cp_dummy_pixels = 0;
	IMX134MIPI_sensor.cp_dummy_lines = 0;
	spin_unlock(&imx134_drv_lock);
	IMX134MIPI_set_8M();
	//IMX134MIPISetFlipMirror(sensor_config_data->SensorImageMirror); 
	spin_lock(&imx134_drv_lock);    
	IMX134MIPI_sensor.cp_line_length=IMX134MIPI_FULL_LINE_LENGTH_PIXELS+IMX134MIPI_sensor.cp_dummy_pixels;
	IMX134MIPI_sensor.cp_frame_length=IMX134MIPI_FULL_FRAME_LENGTH_LINES+IMX134MIPI_sensor.cp_dummy_lines;
	spin_unlock(&imx134_drv_lock);
	iStartX = IMX134MIPI_IMAGE_SENSOR_CAP_STARTX;
	iStartY = IMX134MIPI_IMAGE_SENSOR_CAP_STARTY;
	image_window->GrabStartX=iStartX;
	image_window->GrabStartY=iStartY;
	image_window->ExposureWindowWidth=IMX134MIPI_REAL_CAP_WIDTH ;
	image_window->ExposureWindowHeight=IMX134MIPI_REAL_CAP_HEIGHT;
	IMX134MIPI_SetDummy(IMX134MIPI_sensor.cp_dummy_pixels, IMX134MIPI_sensor.cp_dummy_lines);   
	spin_lock(&imx134_drv_lock);	
	memcpy(&IMX134MIPISensorConfigData, sensor_config_data, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
	spin_unlock(&imx134_drv_lock);
	SENSORDB("[IMX134MIPI]exit IMX134MIPICapture function\n");
	return ERROR_NONE;
}	/* IMX134MIPICapture() */

UINT32 IMX134MIPIGetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution)
{
    SENSORDB("[IMX134MIPI]eXIT IMX134MIPIGetResolution function\n");
    pSensorResolution->SensorPreviewWidth	= IMX134MIPI_REAL_PV_WIDTH;
    pSensorResolution->SensorPreviewHeight	= IMX134MIPI_REAL_PV_HEIGHT;
    pSensorResolution->SensorFullWidth		= IMX134MIPI_REAL_CAP_WIDTH;
    pSensorResolution->SensorFullHeight		= IMX134MIPI_REAL_CAP_HEIGHT;
    pSensorResolution->SensorVideoWidth		= IMX134MIPI_REAL_VIDEO_WIDTH;
    pSensorResolution->SensorVideoHeight    = IMX134MIPI_REAL_VIDEO_HEIGHT;
    SENSORDB("IMX134MIPIGetResolution :8-14");    

    return ERROR_NONE;
}   /* IMX134MIPIGetResolution() */

UINT32 IMX134MIPIGetInfo(MSDK_SCENARIO_ID_ENUM ScenarioId,
                                                MSDK_SENSOR_INFO_STRUCT *pSensorInfo,
                                                MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{ 
	SENSORDB("[IMX134MIPI]enter IMX134MIPIGetInfo function\n");
	switch(ScenarioId){
			case MSDK_SCENARIO_ID_CAMERA_ZSD:
			case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG://hhl 2-28
				pSensorInfo->SensorFullResolutionX=IMX134MIPI_REAL_CAP_WIDTH;
				pSensorInfo->SensorFullResolutionY=IMX134MIPI_REAL_CAP_HEIGHT;
				pSensorInfo->SensorStillCaptureFrameRate=25;

			break;//hhl 2-28
			case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
				pSensorInfo->SensorPreviewResolutionX=IMX134MIPI_REAL_VIDEO_WIDTH;
				pSensorInfo->SensorPreviewResolutionY=IMX134MIPI_REAL_VIDEO_HEIGHT;
				pSensorInfo->SensorCameraPreviewFrameRate=30;
			break;
		default:
        pSensorInfo->SensorPreviewResolutionX=IMX134MIPI_REAL_PV_WIDTH;
        pSensorInfo->SensorPreviewResolutionY=IMX134MIPI_REAL_PV_HEIGHT;
				pSensorInfo->SensorCameraPreviewFrameRate=30;
			break;
	}
    pSensorInfo->SensorVideoFrameRate=30;	
    pSensorInfo->SensorStillCaptureFrameRate=24;
    pSensorInfo->SensorWebCamCaptureFrameRate=24;
    pSensorInfo->SensorResetActiveHigh=FALSE;
    pSensorInfo->SensorResetDelayCount=5;
    pSensorInfo->SensorOutputDataFormat=SENSOR_OUTPUT_FORMAT_RAW_R;
    pSensorInfo->SensorClockPolarity=SENSOR_CLOCK_POLARITY_LOW; /*??? */
    pSensorInfo->SensorClockFallingPolarity=SENSOR_CLOCK_POLARITY_LOW;
    pSensorInfo->SensorHsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
    pSensorInfo->SensorVsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
    pSensorInfo->SensorInterruptDelayLines = 1;
    pSensorInfo->SensroInterfaceType=SENSOR_INTERFACE_TYPE_MIPI;

    pSensorInfo->CaptureDelayFrame = 2; 
    pSensorInfo->PreviewDelayFrame = 2; 
    pSensorInfo->VideoDelayFrame = 2; 
    pSensorInfo->SensorMasterClockSwitch = 0; 
    pSensorInfo->SensorDrivingCurrent = ISP_DRIVING_8MA;      
    pSensorInfo->AEShutDelayFrame = 0;		    /* The frame of setting shutter default 0 for TG int */
    pSensorInfo->AESensorGainDelayFrame = 0;     /* The frame of setting sensor gain */
    pSensorInfo->AEISPGainDelayFrame = 2;	
	   
    switch (ScenarioId)
    {
        case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
            pSensorInfo->SensorClockFreq=24;
            pSensorInfo->SensorClockDividCount=	5;
            pSensorInfo->SensorClockRisingCount= 0;
            pSensorInfo->SensorClockFallingCount= 2;
            pSensorInfo->SensorPixelClockCount= 3;
            pSensorInfo->SensorDataLatchCount= 2;
            pSensorInfo->SensorGrabStartX = IMX134MIPI_IMAGE_SENSOR_PV_STARTX; 
            pSensorInfo->SensorGrabStartY = IMX134MIPI_IMAGE_SENSOR_PV_STARTY;           		
            pSensorInfo->SensorMIPILaneNumber = SENSOR_MIPI_4_LANE;			
            pSensorInfo->MIPIDataLowPwr2HighSpeedTermDelayCount = 0; 
	     	pSensorInfo->MIPIDataLowPwr2HighSpeedSettleDelayCount = 14; 
	    	pSensorInfo->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
            pSensorInfo->SensorWidthSampling = 0;  // 0 is default 1x
            pSensorInfo->SensorHightSampling = 0;   // 0 is default 1x 
            pSensorInfo->SensorPacketECCOrder = 1;
            break;	
        case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			   pSensorInfo->SensorClockFreq=24;
			   pSensorInfo->SensorClockDividCount= 5;
			   pSensorInfo->SensorClockRisingCount= 0;
			   pSensorInfo->SensorClockFallingCount= 2;
			   pSensorInfo->SensorPixelClockCount= 3;
			   pSensorInfo->SensorDataLatchCount= 2;
			   pSensorInfo->SensorGrabStartX = IMX134MIPI_IMAGE_SENSOR_VIDEO_STARTX; 
			   pSensorInfo->SensorGrabStartY = IMX134MIPI_IMAGE_SENSOR_VIDEO_STARTY;				   
			   pSensorInfo->SensorMIPILaneNumber = SENSOR_MIPI_4_LANE;		   
			   pSensorInfo->MIPIDataLowPwr2HighSpeedTermDelayCount = 0; 
			pSensorInfo->MIPIDataLowPwr2HighSpeedSettleDelayCount = 14; 
			pSensorInfo->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
			   pSensorInfo->SensorWidthSampling = 0;  // 0 is default 1x
			   pSensorInfo->SensorHightSampling = 0;   // 0 is default 1x 
			   pSensorInfo->SensorPacketECCOrder = 1;

			break;
        case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		case MSDK_SCENARIO_ID_CAMERA_ZSD:
            pSensorInfo->SensorClockFreq=24;
            pSensorInfo->SensorClockDividCount=	5;
            pSensorInfo->SensorClockRisingCount= 0;
            pSensorInfo->SensorClockFallingCount= 2;
            pSensorInfo->SensorPixelClockCount= 3;
            pSensorInfo->SensorDataLatchCount= 2;
            pSensorInfo->SensorGrabStartX = IMX134MIPI_IMAGE_SENSOR_CAP_STARTX;	//2*IMX134MIPI_IMAGE_SENSOR_PV_STARTX; 
            pSensorInfo->SensorGrabStartY = IMX134MIPI_IMAGE_SENSOR_CAP_STARTY;	//2*IMX134MIPI_IMAGE_SENSOR_PV_STARTY;          			
            pSensorInfo->SensorMIPILaneNumber = SENSOR_MIPI_4_LANE;			
            pSensorInfo->MIPIDataLowPwr2HighSpeedTermDelayCount = 0; 
            pSensorInfo->MIPIDataLowPwr2HighSpeedSettleDelayCount = 14; 
            pSensorInfo->MIPICLKLowPwr2HighSpeedTermDelayCount = 0; 
            pSensorInfo->SensorWidthSampling = 0;  // 0 is default 1x
            pSensorInfo->SensorHightSampling = 0;   // 0 is default 1x
            pSensorInfo->SensorPacketECCOrder = 1;
            break;
        default:
			 pSensorInfo->SensorClockFreq=24;
			 pSensorInfo->SensorClockDividCount= 5;
			 pSensorInfo->SensorClockRisingCount= 0;
			 pSensorInfo->SensorClockFallingCount= 2;
			 pSensorInfo->SensorPixelClockCount= 3;
			 pSensorInfo->SensorDataLatchCount= 2;
			 pSensorInfo->SensorGrabStartX = IMX134MIPI_IMAGE_SENSOR_PV_STARTX; 
			 pSensorInfo->SensorGrabStartY = IMX134MIPI_IMAGE_SENSOR_PV_STARTY; 				 
			 pSensorInfo->SensorMIPILaneNumber = SENSOR_MIPI_4_LANE;		 
			 pSensorInfo->MIPIDataLowPwr2HighSpeedTermDelayCount = 0; 
		     pSensorInfo->MIPIDataLowPwr2HighSpeedSettleDelayCount = 14; 
		  	 pSensorInfo->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
			 pSensorInfo->SensorWidthSampling = 0;	// 0 is default 1x
			 pSensorInfo->SensorHightSampling = 0;	 // 0 is default 1x 
			 pSensorInfo->SensorPacketECCOrder = 1;

            break;
    }
	spin_lock(&imx134_drv_lock);	
    IMX134MIPIPixelClockDivider=pSensorInfo->SensorPixelClockCount;
    memcpy(pSensorConfigData, &IMX134MIPISensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
	spin_unlock(&imx134_drv_lock);
    SENSORDB("[IMX134MIPI]exit IMX134MIPIGetInfo function\n");
    return ERROR_NONE;
}   /* IMX134MIPIGetInfo() */


UINT32 IMX134MIPIControl(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow,
                                                MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{    
		spin_lock(&imx134_drv_lock);	
		IMX134CurrentScenarioId = ScenarioId;
		spin_unlock(&imx134_drv_lock);
		SENSORDB("[IMX134MIPI]enter IMX134MIPIControl function\n");
    switch (ScenarioId)
    {
        case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
            IMX134MIPIPreview(pImageWindow, pSensorConfigData);
            break;
        case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			IMX134MIPIVideo(pImageWindow, pSensorConfigData);
            break;
        case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
	    case MSDK_SCENARIO_ID_CAMERA_ZSD:
            IMX134MIPICapture(pImageWindow, pSensorConfigData);//hhl 2-28
            break;
        default:
            return ERROR_INVALID_SCENARIO_ID;
    }
	SENSORDB("[IMX134MIPI]exit IMX134MIPIControl function\n");
    return ERROR_NONE;
} /* IMX134MIPIControl() */

UINT32 IMX134MIPISetVideoMode(UINT16 u2FrameRate)
{
    SENSORDB("[IMX134MIPISetVideoMode] frame rate = %d\n", u2FrameRate);
    
    SENSORDB("[IMX134MIPISetVideoMode] before u2FrameRate==0 \n");
    if(u2FrameRate==0)  return ERROR_NONE; //add by yyf for preview to video 2013.07.04
    SENSORDB("[IMX134MIPISetVideoMode] after u2FrameRate==0 \n");

	kal_uint16 IMX134MIPI_Video_Max_Expourse_Time = 0;
	SENSORDB("[IMX134MIPI]%s():fix_frame_rate=%d\n",__FUNCTION__,u2FrameRate);
	spin_lock(&imx134_drv_lock);
	IMX134MIPI_sensor.fix_video_fps = KAL_TRUE;
	spin_unlock(&imx134_drv_lock);
	u2FrameRate=u2FrameRate*10;//10*FPS
	SENSORDB("[IMX134MIPI][Enter Fix_fps func] IMX134MIPI_Fix_Video_Frame_Rate = %d\n", u2FrameRate/10);
	IMX134MIPI_Video_Max_Expourse_Time = (kal_uint16)((IMX134MIPI_sensor.video_pclk*10/u2FrameRate)/IMX134MIPI_sensor.video_line_length);
	
	if (IMX134MIPI_Video_Max_Expourse_Time > IMX134MIPI_VIDEO_FRAME_LENGTH_LINES/*IMX134MIPI_sensor.pv_frame_length*/) 
	{
		spin_lock(&imx134_drv_lock);    
		IMX134MIPI_sensor.video_frame_length = IMX134MIPI_Video_Max_Expourse_Time;
		IMX134MIPI_sensor.video_dummy_lines = IMX134MIPI_sensor.video_frame_length-IMX134MIPI_VIDEO_FRAME_LENGTH_LINES;
		spin_unlock(&imx134_drv_lock);
		SENSORDB("[IMX134MIPI]%s():frame_length=%d,dummy_lines=%d\n",__FUNCTION__,IMX134MIPI_sensor.video_frame_length,IMX134MIPI_sensor.video_dummy_lines);
		IMX134MIPI_SetDummy(IMX134MIPI_sensor.video_dummy_pixels,IMX134MIPI_sensor.video_dummy_lines);
	}
	spin_lock(&imx134_drv_lock);    
	IMX134MIPI_MPEG4_encode_mode = KAL_TRUE; 
	spin_unlock(&imx134_drv_lock);
	SENSORDB("[IMX134MIPI]exit IMX134MIPISetVideoMode function\n");
	return ERROR_NONE;
}

UINT32 IMX134MIPISetAutoFlickerMode(kal_bool bEnable, UINT16 u2FrameRate)
{
	kal_uint32 pv_max_frame_rate_lines=0;

	if(IMX134MIPI_sensor.pv_mode==TRUE)
	pv_max_frame_rate_lines=IMX134MIPI_PV_FRAME_LENGTH_LINES;
	else
    pv_max_frame_rate_lines=IMX134MIPI_VIDEO_FRAME_LENGTH_LINES	;
    SENSORDB("[IMX134MIPISetAutoFlickerMode] frame rate(10base) = %d %d\n", bEnable, u2FrameRate);
    if(bEnable) 
	{   // enable auto flicker   
    	spin_lock(&imx134_drv_lock);    
        IMX134MIPI_Auto_Flicker_mode = KAL_TRUE; 
		spin_unlock(&imx134_drv_lock);
        if(IMX134MIPI_MPEG4_encode_mode == KAL_TRUE) 
		{ // in the video mode, reset the frame rate
            pv_max_frame_rate_lines = IMX134MIPI_MAX_EXPOSURE_LINES + (IMX134MIPI_MAX_EXPOSURE_LINES>>7);            
            IMX134MIPI_write_cmos_sensor(0x0104, 1);        
            IMX134MIPI_write_cmos_sensor(0x0340, (pv_max_frame_rate_lines >>8) & 0xFF);
            IMX134MIPI_write_cmos_sensor(0x0341, pv_max_frame_rate_lines & 0xFF);	
            IMX134MIPI_write_cmos_sensor(0x0104, 0);        	
        }
    } 
	else 
	{
    	spin_lock(&imx134_drv_lock);    
        IMX134MIPI_Auto_Flicker_mode = KAL_FALSE; 
		spin_unlock(&imx134_drv_lock);
        if(IMX134MIPI_MPEG4_encode_mode == KAL_TRUE) 
		{    // in the video mode, restore the frame rate
            IMX134MIPI_write_cmos_sensor(0x0104, 1);        
            IMX134MIPI_write_cmos_sensor(0x0340, (IMX134MIPI_MAX_EXPOSURE_LINES >>8) & 0xFF);
            IMX134MIPI_write_cmos_sensor(0x0341, IMX134MIPI_MAX_EXPOSURE_LINES & 0xFF);	
            IMX134MIPI_write_cmos_sensor(0x0104, 0);        	
        }
        printk("Disable Auto flicker\n");    
    }
    return ERROR_NONE;
}
UINT32 IMX134MIPISetMaxFramerateByScenario(MSDK_SCENARIO_ID_ENUM scenarioId, MUINT32 frameRate) {
	kal_uint32 pclk;
	kal_int16 dummyLine;
	kal_uint16 lineLength,frameHeight;	
	SENSORDB("IMX134MIPISetMaxFramerateByScenario: scenarioId = %d, frame rate = %d\n",scenarioId,frameRate);
	switch (scenarioId) {
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
			pclk = 232000000;//180000000;//200000000£»//240000000;
			lineLength = IMX134MIPI_PV_LINE_LENGTH_PIXELS;
			frameHeight = (10 * pclk)/frameRate/lineLength;
			dummyLine = frameHeight - IMX134MIPI_PV_FRAME_LENGTH_LINES;

			
			if(dummyLine<0)
				dummyLine = 0;
			spin_lock(&imx134_drv_lock);	
			IMX134MIPI_sensor.pv_mode=TRUE;
			spin_unlock(&imx134_drv_lock);
			IMX134MIPI_SetDummy(0, dummyLine);			
			break;			
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			pclk = 232000000;
			lineLength = IMX134MIPI_VIDEO_LINE_LENGTH_PIXELS;
			frameHeight = (10 * pclk)/frameRate/lineLength;
			dummyLine = frameHeight - IMX134MIPI_VIDEO_FRAME_LENGTH_LINES;
			if(dummyLine<0)
				dummyLine = 0;
			spin_lock(&imx134_drv_lock);	
			IMX134MIPI_sensor.pv_mode=TRUE;
			spin_unlock(&imx134_drv_lock);
			IMX134MIPI_SetDummy(0, dummyLine);			
			break;			
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		case MSDK_SCENARIO_ID_CAMERA_ZSD:			
			pclk = 232000000;
			lineLength = IMX134MIPI_FULL_LINE_LENGTH_PIXELS;
			frameHeight = (10 * pclk)/frameRate/lineLength;
			dummyLine = frameHeight - IMX134MIPI_FULL_FRAME_LENGTH_LINES;
			if(dummyLine<0)
				dummyLine = 0;
			
			spin_lock(&imx134_drv_lock);	
			IMX134MIPI_sensor.pv_mode=FALSE;
			spin_unlock(&imx134_drv_lock);
			IMX134MIPI_SetDummy(0, dummyLine);			
			break;		
        case MSDK_SCENARIO_ID_CAMERA_3D_PREVIEW: //added
            break;
        case MSDK_SCENARIO_ID_CAMERA_3D_VIDEO:
			break;
        case MSDK_SCENARIO_ID_CAMERA_3D_CAPTURE: //added   
			break;		
		default:
			break;
	}
	SENSORDB("[IMX134MIPI]exit IMX134MIPISetMaxFramerateByScenario function\n");
	return ERROR_NONE;
}
UINT32 IMX134MIPIGetDefaultFramerateByScenario(MSDK_SCENARIO_ID_ENUM scenarioId, MUINT32 *pframeRate) 
{

	switch (scenarioId) {
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			 *pframeRate = 300;//240;
			 break;
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		case MSDK_SCENARIO_ID_CAMERA_ZSD:
			 *pframeRate = 250;//250;
			break;		//hhl 2-28
        case MSDK_SCENARIO_ID_CAMERA_3D_PREVIEW: //added
        case MSDK_SCENARIO_ID_CAMERA_3D_VIDEO:
        case MSDK_SCENARIO_ID_CAMERA_3D_CAPTURE: //added   
			 *pframeRate = 300;
			break;		
		default:
			break;
	}

	return ERROR_NONE;
}
UINT32 IMX134MIPISetTestPatternMode(kal_bool bEnable)
{
    SENSORDB("[IMX134MIPISetTestPatternMode] Test pattern enable:%d\n", bEnable);
    
    if(bEnable) {   // enable color bar   
        IMX134MIPI_write_cmos_sensor(0x30D8, 0x10);  // color bar test pattern
        IMX134MIPI_write_cmos_sensor(0x0600, 0x00);  // color bar test pattern
        IMX134MIPI_write_cmos_sensor(0x0601, 0x02);  // color bar test pattern 
    } else {
        IMX134MIPI_write_cmos_sensor(0x30D8, 0x00);  // disable color bar test pattern
    }
    return ERROR_NONE;
}

UINT32 IMX134MIPIFeatureControl(MSDK_SENSOR_FEATURE_ENUM FeatureId,
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
            *pFeatureReturnPara16++=IMX134MIPI_REAL_CAP_WIDTH;
            *pFeatureReturnPara16=IMX134MIPI_REAL_CAP_HEIGHT;
            *pFeatureParaLen=4;
            break;
        case SENSOR_FEATURE_GET_PERIOD:
        		switch(IMX134CurrentScenarioId)
        		{
        			case MSDK_SCENARIO_ID_CAMERA_ZSD:
        		    case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
 		            *pFeatureReturnPara16++=IMX134MIPI_sensor.cp_line_length;  
 		            *pFeatureReturnPara16=IMX134MIPI_sensor.cp_frame_length;
		            SENSORDB("Sensor period:%d %d\n",IMX134MIPI_sensor.cp_line_length, IMX134MIPI_sensor.cp_frame_length); 
		            *pFeatureParaLen=4;        				
        				break;
        			case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
					*pFeatureReturnPara16++=IMX134MIPI_sensor.video_line_length;  
					*pFeatureReturnPara16=IMX134MIPI_sensor.video_frame_length;
					 SENSORDB("Sensor period:%d %d\n", IMX134MIPI_sensor.video_line_length, IMX134MIPI_sensor.video_frame_length); 
					 *pFeatureParaLen=4;
						break;
        			default:	
					*pFeatureReturnPara16++=IMX134MIPI_sensor.pv_line_length;  
					*pFeatureReturnPara16=IMX134MIPI_sensor.pv_frame_length;
		            SENSORDB("Sensor period:%d %d\n", IMX134MIPI_sensor.pv_line_length, IMX134MIPI_sensor.pv_frame_length); 
		            *pFeatureParaLen=4;
	            break;
          	}
          	break;
        case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ:
        		switch(IMX134CurrentScenarioId)
        		{
        			case MSDK_SCENARIO_ID_CAMERA_ZSD:
        			case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		            *pFeatureReturnPara32 = IMX134MIPI_sensor.cp_pclk; 
		            *pFeatureParaLen=4;		         	
					
		            SENSORDB("Sensor CPCLK:%dn",IMX134MIPI_sensor.cp_pclk); 
		         		break; //hhl 2-28
					case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
						*pFeatureReturnPara32 = IMX134MIPI_sensor.video_pclk;
						*pFeatureParaLen=4;
						SENSORDB("Sensor videoCLK:%d\n",IMX134MIPI_sensor.video_pclk); 
						break;
		         		default:
		            *pFeatureReturnPara32 = IMX134MIPI_sensor.pv_pclk;
		            *pFeatureParaLen=4;
					SENSORDB("Sensor pvclk:%d\n",IMX134MIPI_sensor.pv_pclk); 
		            break;
		         }
		         break;
        case SENSOR_FEATURE_SET_ESHUTTER:
            IMX134MIPI_SetShutter(*pFeatureData16); 
            break;
		case SENSOR_FEATURE_SET_SENSOR_SYNC: 
			break;
        case SENSOR_FEATURE_SET_NIGHTMODE:
            IMX134MIPI_NightMode((BOOL) *pFeatureData16);
            break;
        case SENSOR_FEATURE_SET_GAIN:
           IMX134MIPI_SetGain((UINT16) *pFeatureData16); 
            break;
        case SENSOR_FEATURE_SET_FLASHLIGHT:
            break;
        case SENSOR_FEATURE_SET_ISP_MASTER_CLOCK_FREQ:
			spin_lock(&imx134_drv_lock);    
            IMX134MIPI_isp_master_clock=*pFeatureData32;
			spin_unlock(&imx134_drv_lock);
            break;
        case SENSOR_FEATURE_SET_REGISTER:
			IMX134MIPI_write_cmos_sensor(pSensorRegData->RegAddr, pSensorRegData->RegData);
            break;
        case SENSOR_FEATURE_GET_REGISTER:
            pSensorRegData->RegData = IMX134MIPI_read_cmos_sensor(pSensorRegData->RegAddr);
            break;
        case SENSOR_FEATURE_SET_CCT_REGISTER:
            SensorRegNumber=FACTORY_END_ADDR;
            for (i=0;i<SensorRegNumber;i++)
            {
            	spin_lock(&imx134_drv_lock);    
                IMX134MIPISensorCCT[i].Addr=*pFeatureData32++;
                IMX134MIPISensorCCT[i].Para=*pFeatureData32++; 
				spin_unlock(&imx134_drv_lock);
            }
            break;
        case SENSOR_FEATURE_GET_CCT_REGISTER:
            SensorRegNumber=FACTORY_END_ADDR;
            if (*pFeatureParaLen<(SensorRegNumber*sizeof(SENSOR_REG_STRUCT)+4))
                return FALSE;
            *pFeatureData32++=SensorRegNumber;
            for (i=0;i<SensorRegNumber;i++)
            {
                *pFeatureData32++=IMX134MIPISensorCCT[i].Addr;
                *pFeatureData32++=IMX134MIPISensorCCT[i].Para; 
            }
            break;
        case SENSOR_FEATURE_SET_ENG_REGISTER:
            SensorRegNumber=ENGINEER_END;
            for (i=0;i<SensorRegNumber;i++)
            {	spin_lock(&imx134_drv_lock);    
                IMX134MIPISensorReg[i].Addr=*pFeatureData32++;
                IMX134MIPISensorReg[i].Para=*pFeatureData32++;
				spin_unlock(&imx134_drv_lock);
            }
            break;
        case SENSOR_FEATURE_GET_ENG_REGISTER:
            SensorRegNumber=ENGINEER_END;
            if (*pFeatureParaLen<(SensorRegNumber*sizeof(SENSOR_REG_STRUCT)+4))
                return FALSE;
            *pFeatureData32++=SensorRegNumber;
            for (i=0;i<SensorRegNumber;i++)
            {
                *pFeatureData32++=IMX134MIPISensorReg[i].Addr;
                *pFeatureData32++=IMX134MIPISensorReg[i].Para;
            }
            break;
        case SENSOR_FEATURE_GET_REGISTER_DEFAULT:
            if (*pFeatureParaLen>=sizeof(NVRAM_SENSOR_DATA_STRUCT))
            {
                pSensorDefaultData->Version=NVRAM_CAMERA_SENSOR_FILE_VERSION;
                pSensorDefaultData->SensorId=IMX134MIPI_SENSOR_ID;
                memcpy(pSensorDefaultData->SensorEngReg, IMX134MIPISensorReg, sizeof(SENSOR_REG_STRUCT)*ENGINEER_END);
                memcpy(pSensorDefaultData->SensorCCTReg, IMX134MIPISensorCCT, sizeof(SENSOR_REG_STRUCT)*FACTORY_END_ADDR);
            }
            else
                return FALSE;
            *pFeatureParaLen=sizeof(NVRAM_SENSOR_DATA_STRUCT);
            break;
        case SENSOR_FEATURE_GET_CONFIG_PARA:
            memcpy(pSensorConfigData, &IMX134MIPISensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
            *pFeatureParaLen=sizeof(MSDK_SENSOR_CONFIG_STRUCT);
            break;
        case SENSOR_FEATURE_CAMERA_PARA_TO_SENSOR:
            IMX134MIPI_camera_para_to_sensor();
            break;

        case SENSOR_FEATURE_SENSOR_TO_CAMERA_PARA:
            IMX134MIPI_sensor_to_camera_para();
            break;
        case SENSOR_FEATURE_GET_GROUP_COUNT:
            *pFeatureReturnPara32++=IMX134MIPI_get_sensor_group_count();
            *pFeatureParaLen=4;
            break;
        case SENSOR_FEATURE_GET_GROUP_INFO:
            IMX134MIPI_get_sensor_group_info(pSensorGroupInfo->GroupIdx, pSensorGroupInfo->GroupNamePtr, &pSensorGroupInfo->ItemCount);
            *pFeatureParaLen=sizeof(MSDK_SENSOR_GROUP_INFO_STRUCT);
            break;
        case SENSOR_FEATURE_GET_ITEM_INFO:
            IMX134MIPI_get_sensor_item_info(pSensorItemInfo->GroupIdx,pSensorItemInfo->ItemIdx, pSensorItemInfo);
            *pFeatureParaLen=sizeof(MSDK_SENSOR_ITEM_INFO_STRUCT);
            break;

        case SENSOR_FEATURE_SET_ITEM_INFO:
            IMX134MIPI_set_sensor_item_info(pSensorItemInfo->GroupIdx, pSensorItemInfo->ItemIdx, pSensorItemInfo->ItemValue);
            *pFeatureParaLen=sizeof(MSDK_SENSOR_ITEM_INFO_STRUCT);
            break;

        case SENSOR_FEATURE_GET_ENG_INFO:
            pSensorEngInfo->SensorId = 134;
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
            IMX134MIPISetVideoMode(*pFeatureData16);
            break;
        case SENSOR_FEATURE_CHECK_SENSOR_ID:
            IMX134MIPIGetSensorID(pFeatureReturnPara32); 
            break;             
        case SENSOR_FEATURE_SET_AUTO_FLICKER_MODE:
            IMX134MIPISetAutoFlickerMode((BOOL)*pFeatureData16, *(pFeatureData16+1));            
	        break;
        case SENSOR_FEATURE_SET_TEST_PATTERN:
            IMX134MIPISetTestPatternMode((BOOL)*pFeatureData16);        	
            break;
			
		case SENSOR_FEATURE_GET_TEST_PATTERN_CHECKSUM_VALUE:
			*pFeatureReturnPara32 = IMX134_TEST_PATTERN_CHECKSUM;
			*pFeatureParaLen=4;
					break;
		case SENSOR_FEATURE_SET_MAX_FRAME_RATE_BY_SCENARIO:
			IMX134MIPISetMaxFramerateByScenario((MSDK_SCENARIO_ID_ENUM)*pFeatureData32, *(pFeatureData32+1));
			break;
		case SENSOR_FEATURE_GET_DEFAULT_FRAME_RATE_BY_SCENARIO:
			IMX134MIPIGetDefaultFramerateByScenario((MSDK_SCENARIO_ID_ENUM)*pFeatureData32, (MUINT32 *)(*(pFeatureData32+1)));
			break;
        default:
            break;
    }
    return ERROR_NONE;
}	/* IMX134MIPIFeatureControl() */


SENSOR_FUNCTION_STRUCT	SensorFuncIMX134MIPI=
{
    IMX134MIPIOpen,
    IMX134MIPIGetInfo,
    IMX134MIPIGetResolution,
    IMX134MIPIFeatureControl,
    IMX134MIPIControl,
    IMX134MIPIClose
};

UINT32 IMX134_MIPI_RAW_SensorInit(PSENSOR_FUNCTION_STRUCT *pfFunc)
{
    /* To Do : Check Sensor status here */
    if (pfFunc!=NULL)
        *pfFunc=&SensorFuncIMX134MIPI;
    return ERROR_NONE;
}   /* SensorInit() */

