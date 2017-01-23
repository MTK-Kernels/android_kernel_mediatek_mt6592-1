/*****************************************************************************
*  Copyright Statement:
*  --------------------
*  This software is protected by Copyright and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of MediaTek Inc. (C) 2008
*
*  BY OPENING THIS FILE, BUYER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
*  THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
*  RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO BUYER ON
*  AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
*  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
*  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
*  NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
*  SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
*  SUPPLIED WITH THE MEDIATEK SOFTWARE, AND BUYER AGREES TO LOOK ONLY TO SUCH
*  THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. MEDIATEK SHALL ALSO 
*  NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE RELEASES MADE TO BUYER'S
*  SPECIFICATION OR TO CONFORM TO A PARTICULAR STANDARD OR OPEN FORUM.
*
*  BUYER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND CUMULATIVE
*  LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
*  AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
*  OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY BUYER TO
*  MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
*
*  THE TRANSACTION CONTEMPLATED HEREUNDER SHALL BE CONSTRUED IN ACCORDANCE
*  WITH THE LAWS OF THE STATE OF CALIFORNIA, USA, EXCLUDING ITS CONFLICT OF
*  LAWS PRINCIPLES.  ANY DISPUTES, CONTROVERSIES OR CLAIMS ARISING THEREOF AND
*  RELATED THERETO SHALL BE SETTLED BY ARBITRATION IN SAN FRANCISCO, CA, UNDER
*  THE RULES OF THE INTERNATIONAL CHAMBER OF COMMERCE (ICC).
*
*****************************************************************************/
/*****************************************************************************

 ****************************************************************************/
//add liteon and sunny IMX134 module driver and para for H30
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

#include "imx134liteonmipiraw_Sensor.h"
#include "imx134liteonmipiraw_Camera_Sensor_para.h"
#include "imx134liteonmipiraw_CameraCustomized.h"

#include "cam_cal_define.h"
static kal_bool  IMX134MIPI_MPEG4_encode_mode = KAL_FALSE;
static kal_bool IMX134MIPI_Auto_Flicker_mode = KAL_FALSE;


static kal_uint8 IMX134MIPI_sensor_write_I2C_address = IMX134MIPI_WRITE_ID;
static kal_uint8 IMX134MIPI_sensor_read_I2C_address = IMX134MIPI_READ_ID;
//#define IMX134MIPI_SENSOR_ID            0x134

	
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
static MSDK_SCENARIO_ID_ENUM IMX134CurrentScenarioId = MSDK_SCENARIO_ID_CAMERA_PREVIEW;	 
static kal_uint16	IMX134MIPI_sensor_gain_base=0x0;                                 
/* MAX/MIN Explosure Lines Used By AE Algorithm */                           
static kal_uint16 IMX134MIPI_MAX_EXPOSURE_LINES = IMX134MIPI_PV_FRAME_LENGTH_LINES-4;//650;
static kal_uint8  IMX134MIPI_MIN_EXPOSURE_LINES = 2;                                
static kal_uint32 IMX134MIPI_isp_master_clock;
static DEFINE_SPINLOCK(imx134_drv_lock);

#define SENSORDB(fmt, arg...) printk( "[IMX134MIPIRaw] "  fmt, ##arg)
#define RETAILMSG(x,...)
#define TEXT
static UINT8 IMX134MIPIPixelClockDivider=0;
static kal_uint16 IMX134MIPI_sensor_id=0;
static MSDK_SENSOR_CONFIG_STRUCT IMX134MIPISensorConfigData;
static kal_uint32 IMX134MIPI_FAC_SENSOR_REG;
static kal_uint16 IMX134MIPI_sensor_flip_value; 
#define IMX134MIPI_MaxGainIndex (97)
static kal_uint16 IMX134MIPI_sensorGainMapping[IMX134MIPI_MaxGainIndex][2] ={
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
{ 683,232}
};
/* FIXME: old factors and DIDNOT use now. s*/
static SENSOR_REG_STRUCT IMX134MIPISensorCCT[]=CAMERA_SENSOR_CCT_DEFAULT_VALUE;
static SENSOR_REG_STRUCT IMX134MIPISensorReg[ENGINEER_END]=CAMERA_SENSOR_REG_DEFAULT_VALUE;
/* FIXME: old factors and DIDNOT use now. e*/
extern int iReadReg(u16 a_u2Addr , u8 * a_puBuff , u16 i2cId);
extern int iWriteReg(u16 a_u2Addr , u32 a_u4Data , u32 a_u4Bytes , u16 i2cId);
#define IMX134MIPI_write_cmos_sensor(addr, para) iWriteReg((u16) addr , (u32) para , 1, IMX134MIPI_WRITE_ID)


#define IMX134MIPI_USE_OTP

#ifdef IMX134MIPI_USE_OTP

typedef enum tag_sensor_module_type 
{
    MODULE_LITEON = 0,
    MODULE_SUNNY,
    MODULE_UNSUPPORT,
}IMX134MIPI_module_type;

static IMX134MIPI_module_type module_type = MODULE_UNSUPPORT;

//I2C address for module ID/VCM/LSC/AWB
#define OTP_ID_AWB_I2C_ADD     0xA4
#define OTP_VCM_I2C_ADD        0xA6
#define OTP_LSC_I2C_ADD_1     0xA4
#define OTP_LSC_I2C_ADD_2     0xA6
#define OTP_CHECKSUM_I2C_ADD    0xA6

#define OTP_AWB_REG    0x00
#define OTP_VCM_REG    0x40
#define OTP_CHECKSUM_REG 0x44
#define OTP_LSC_1_REG  0x0b
#define OTP_LSC_2_REG  0x00

//sum of all bytes readed
static kal_uint32 OTPSUMVAL = 0;

//7*5*4*2
#define IMX134MIPI_OTP_LSC_SIZE  280

//Lsc length in A4 add, 0xFF-0xB+1 = 245 bytes.
#define  IMX134MIPI_OTP_LSC_A4_SIZE  (0xFF-0x0B+1)
//Lsc lenght in A6 add, 0x23 = 35 bytes.
#define  IMX134MIPI_OTP_LSC_A6_SIZE  (IMX134MIPI_OTP_LSC_SIZE - IMX134MIPI_OTP_LSC_A4_SIZE)


struct IMX134MIPI_Otp_Struct {
    kal_uint32 iProduct_Year;//product year
    kal_uint32 iProduct_Month;//product month
    kal_uint32 iProduct_Date;//product data
    kal_uint32 iCamera_Id;//hw camea id
    kal_uint32 iSupplier_Version_Id;//supplier id
    kal_uint32 iWB_RG;//rg ratio
    kal_uint32 iWB_BG;//bg ratio
    kal_uint32 iWB_GbGr;//gb/ratio
    kal_uint32 iVCM_Start;//vcm start current
    kal_uint32 iVCM_End;//vcm end current
    kal_uint32 iCheckSum;//OTP checksum value
    kal_uint8  iLsc_param[IMX134MIPI_OTP_LSC_SIZE];//lens shading correction
};

static struct IMX134MIPI_Otp_Struct gcurrent_otp = { 0};

//Golden values for R/G, B/G, G/G
#define RG_GOLDEN  0x240
#define BG_GOLDEN  0x250
#define GG_GOLDEN  0x400


//OTP read indications
static  kal_uint32 IMX134MIPI_otp_read_flag = 0 ;


#define    IMX134MIPI_OTP_ID_READ                             (1 << 0)
#define    IMX134MIPI_OTP_AWB_READ                          (1 << 1)
#define    IMX134MIPI_OTP_VCM_READ                          (1 << 2)
#define    IMX134MIPI_OTP_LSC_READ                           (1 << 3)
#define    IMX134MIPI_OTP_CHECKSUM_READ                (1 << 4)

//check sum errors
#define    IMX134MIPI_OTP_CHECKSUM_ERR                        (1 << 5)

#define    IMX134MIPI_OTP_ALL_READ  (IMX134MIPI_OTP_ID_READ | IMX134MIPI_OTP_AWB_READ | IMX134MIPI_OTP_VCM_READ \
                       | IMX134MIPI_OTP_LSC_READ|IMX134MIPI_OTP_CHECKSUM_READ )

extern int iReadRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u8 * a_pRecvData, u16 a_sizeRecvData, u16 i2cId);
static kal_uint16 IMX134MIPI_read_eeprom_otp_data(u16 i2c_add, u16 addr)
{
    kal_uint8 get_byte=0;
    char puSendCmd[1] = {(char)(addr & 0xFF)};

    iReadRegI2C(puSendCmd, 1, (u8*)&get_byte, 1, i2c_add);
    //SENSORDB("IMX134MIPI_read_eeprom_otp_data, i2c_add :0x%x, addr:0x%x, get_byte  :0x%x\n", i2c_add,addr, get_byte);

    OTPSUMVAL = OTPSUMVAL + get_byte;
    return get_byte;
}
#endif


static kal_uint16 IMX134MIPI_read_cmos_sensor(kal_uint32 addr)
{
	kal_uint16 get_byte=0;
    iReadReg((u16) addr ,(u8*)&get_byte,IMX134MIPI_WRITE_ID);
    return get_byte;
}

static void IMX134MIPI_write_shutter(kal_uint16 shutter)
{
	kal_uint32 frame_length = 0,line_length=0;
        kal_uint32 extra_lines = 0;
	kal_uint32 max_exp_shutter = 0;
	unsigned long flags;	
	SENSORDB("[IMX134MIPI]enter IMX134MIPI_write_shutter function\n"); 
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
    SENSORDB("[IMX134MIPI]exit IMX134MIPI_write_shutter function\n");
}   /* write_IMX134MIPI_shutter */

static kal_uint16 IMX134MIPIReg2Gain(const kal_uint8 iReg)
{
	SENSORDB("[IMX134MIPI]enter IMX134MIPIReg2Gain function\n");
    kal_uint8 iI;
    // Range: 1x to 8x
    for (iI = 0; iI < IMX134MIPI_MaxGainIndex; iI++) 
	{
        if(iReg < IMX134MIPI_sensorGainMapping[iI][1])
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
    for (iI = 0; iI < (IMX134MIPI_MaxGainIndex-1); iI++) 
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
static void IMX134MIPI_SetGain(UINT16 iGain)
{   
    kal_uint8 iReg;
	SENSORDB("[IMX134MIPI]enter IMX134MIPI_SetGain function\n");
    iReg = IMX134MIPIGain2Reg(iGain);
	IMX134MIPI_write_cmos_sensor(0x0104, 1);
    IMX134MIPI_write_cmos_sensor(0x0205, (kal_uint8)iReg);
    IMX134MIPI_write_cmos_sensor(0x0104, 0);
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
static kal_uint16 read_IMX134MIPI_gain(void)
{  
	SENSORDB("[IMX134MIPI]enter read_IMX134MIPI_gain function\n");
    return (kal_uint16)((IMX134MIPI_read_cmos_sensor(0x0204)<<8) | IMX134MIPI_read_cmos_sensor(0x0205)) ;
}  /* read_IMX134MIPI_gain */

static void write_IMX134MIPI_gain(kal_uint16 gain)
{
    IMX134MIPI_SetGain(gain);
}
static void IMX134MIPI_camera_para_to_sensor(void)
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
static void IMX134MIPI_sensor_to_camera_para(void)
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
static kal_int32  IMX134MIPI_get_sensor_group_count(void)
{
    return GROUP_TOTAL_NUMS;
}

static void IMX134MIPI_get_sensor_group_info(kal_uint16 group_idx, kal_int8* group_name_ptr, kal_int32* item_count_ptr)
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

static void IMX134MIPI_get_sensor_item_info(kal_uint16 group_idx,kal_uint16 item_idx, MSDK_SENSOR_ITEM_INFO_STRUCT* info_ptr)
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
static kal_bool IMX134MIPI_set_sensor_item_info(kal_uint16 group_idx, kal_uint16 item_idx, kal_int32 ItemValue)
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
static void IMX134MIPI_Sensor_Init(void)
{
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
    IMX134MIPI_write_cmos_sensor(0x600A, 0x00); //0x00 //f00208919 20130930
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
    //Optimized reg setting remove redundant setting.
    //Defect Correction Recommended Setting
    //IMX134MIPI_write_cmos_sensor(0x380A,0x00);
    //IMX134MIPI_write_cmos_sensor(0x380B,0x00);
    //IMX134MIPI_write_cmos_sensor(0x4103,0x00);
    //Color Artifact Recommended Setting
    //IMX134MIPI_write_cmos_sensor(0x4243,0x9A);
    //IMX134MIPI_write_cmos_sensor(0x4330,0x01);
    //IMX134MIPI_write_cmos_sensor(0x4331,0x90);
    //IMX134MIPI_write_cmos_sensor(0x4332,0x02);
    //IMX134MIPI_write_cmos_sensor(0x4333,0x58);
    IMX134MIPI_write_cmos_sensor(0x4334,0x03);
    IMX134MIPI_write_cmos_sensor(0x4335,0x20);
    IMX134MIPI_write_cmos_sensor(0x4336,0x03);
    IMX134MIPI_write_cmos_sensor(0x4337,0x84);
    //IMX134MIPI_write_cmos_sensor(0x433C,0x01);
    //IMX134MIPI_write_cmos_sensor(0x4340,0x02);
    //IMX134MIPI_write_cmos_sensor(0x4341,0x58);
    //IMX134MIPI_write_cmos_sensor(0x4342,0x03);
    //IMX134MIPI_write_cmos_sensor(0x4343,0x52);
    //Moir reduction Parameter Setting
    IMX134MIPI_write_cmos_sensor(0x4364,0x0B);
    //IMX134MIPI_write_cmos_sensor(0x4368,0x00);
    //IMX134MIPI_write_cmos_sensor(0x4369,0x0F);
    //IMX134MIPI_write_cmos_sensor(0x436A,0x03);
    //IMX134MIPI_write_cmos_sensor(0x436B,0xA8);
    //IMX134MIPI_write_cmos_sensor(0x436C,0x00);
    IMX134MIPI_write_cmos_sensor(0x436D,0x00);
    //IMX134MIPI_write_cmos_sensor(0x436E,0x00);
    IMX134MIPI_write_cmos_sensor(0x436F,0x06);
    //CNR parameter setting
    IMX134MIPI_write_cmos_sensor(0x4281,0x21);
    //IMX134MIPI_write_cmos_sensor(0x4282,0x18);
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
    //ARNR Parameter Setting
    IMX134MIPI_write_cmos_sensor(0x4207,0x03);
    IMX134MIPI_write_cmos_sensor(0x4216,0x08);
    IMX134MIPI_write_cmos_sensor(0x4217,0x08);
    //DLC Parameter Setting
    //IMX134MIPI_write_cmos_sensor(0x4218,0x00);
    IMX134MIPI_write_cmos_sensor(0x421B,0x20);
    //IMX134MIPI_write_cmos_sensor(0x421F,0x04);
    //IMX134MIPI_write_cmos_sensor(0x4222,0x02);
    //IMX134MIPI_write_cmos_sensor(0x4223,0x22);
    IMX134MIPI_write_cmos_sensor(0x422E,0x54);
    //IMX134MIPI_write_cmos_sensor(0x422F,0xFB);
    IMX134MIPI_write_cmos_sensor(0x4230,0xFF);
    IMX134MIPI_write_cmos_sensor(0x4231,0xFE);
    IMX134MIPI_write_cmos_sensor(0x4232,0xFF);
    IMX134MIPI_write_cmos_sensor(0x4235,0x58);
    IMX134MIPI_write_cmos_sensor(0x4236,0xF7);
    IMX134MIPI_write_cmos_sensor(0x4237,0xFD);
    IMX134MIPI_write_cmos_sensor(0x4239,0x4E);
    IMX134MIPI_write_cmos_sensor(0x423A,0xFC);
    IMX134MIPI_write_cmos_sensor(0x423B,0xFD);
    //HDR Setting
    IMX134MIPI_write_cmos_sensor(0x4300,0x00);
    IMX134MIPI_write_cmos_sensor(0x4316,0x12);
    IMX134MIPI_write_cmos_sensor(0x4317,0x22);
    //IMX134MIPI_write_cmos_sensor(0x4318,0x00);
    //IMX134MIPI_write_cmos_sensor(0x4319,0x00);
    IMX134MIPI_write_cmos_sensor(0x431A,0x00);
    //IMX134MIPI_write_cmos_sensor(0x4324,0x03);
    //IMX134MIPI_write_cmos_sensor(0x4325,0x20);
    //IMX134MIPI_write_cmos_sensor(0x4326,0x03);
    //IMX134MIPI_write_cmos_sensor(0x4327,0x84);
    //IMX134MIPI_write_cmos_sensor(0x4328,0x03);
    //IMX134MIPI_write_cmos_sensor(0x4329,0x20);
    //IMX134MIPI_write_cmos_sensor(0x432A,0x03);
    IMX134MIPI_write_cmos_sensor(0x432B,0x20);
    //IMX134MIPI_write_cmos_sensor(0x432C,0x01);
    IMX134MIPI_write_cmos_sensor(0x432D,0x01);
    //IMX134MIPI_write_cmos_sensor(0x4338,0x02);
    //IMX134MIPI_write_cmos_sensor(0x4339,0x00);
    //IMX134MIPI_write_cmos_sensor(0x433A,0x00);
    //IMX134MIPI_write_cmos_sensor(0x433B,0x02);
    //IMX134MIPI_write_cmos_sensor(0x435A,0x03);
    //IMX134MIPI_write_cmos_sensor(0x435B,0x84);
    IMX134MIPI_write_cmos_sensor(0x435E,0x01);
    IMX134MIPI_write_cmos_sensor(0x435F,0xFF);
    //IMX134MIPI_write_cmos_sensor(0x4360,0x01);
    //IMX134MIPI_write_cmos_sensor(0x4361,0xF4);
    //IMX134MIPI_write_cmos_sensor(0x4362,0x03);
    //IMX134MIPI_write_cmos_sensor(0x4363,0x84);
    //IMX134MIPI_write_cmos_sensor(0x437B,0x01);
    IMX134MIPI_write_cmos_sensor(0x4401,0x3F);
    //IMX134MIPI_write_cmos_sensor(0x4402,0xFF);
    //IMX134MIPI_write_cmos_sensor(0x4404,0x13);
    //IMX134MIPI_write_cmos_sensor(0x4405,0x26);
    //IMX134MIPI_write_cmos_sensor(0x4406,0x07);
    //IMX134MIPI_write_cmos_sensor(0x4408,0x20);
    //IMX134MIPI_write_cmos_sensor(0x4409,0xE5);
    //IMX134MIPI_write_cmos_sensor(0x440A,0xFB);
    //IMX134MIPI_write_cmos_sensor(0x440C,0xF6);
    //IMX134MIPI_write_cmos_sensor(0x440D,0xEA);
    //IMX134MIPI_write_cmos_sensor(0x440E,0x20);
    //IMX134MIPI_write_cmos_sensor(0x4410,0x00);
    //IMX134MIPI_write_cmos_sensor(0x4411,0x00);
    IMX134MIPI_write_cmos_sensor(0x4412,0x3F);
    IMX134MIPI_write_cmos_sensor(0x4413,0xFF);
    //IMX134MIPI_write_cmos_sensor(0x4414,0x1F);
    //IMX134MIPI_write_cmos_sensor(0x4415,0xFF);
    //IMX134MIPI_write_cmos_sensor(0x4416,0x20);
    //IMX134MIPI_write_cmos_sensor(0x4417,0x00);
    //IMX134MIPI_write_cmos_sensor(0x4418,0x1F);
    //IMX134MIPI_write_cmos_sensor(0x4419,0xFF);
    //IMX134MIPI_write_cmos_sensor(0x441A,0x20);
    //IMX134MIPI_write_cmos_sensor(0x441B,0x00);
    IMX134MIPI_write_cmos_sensor(0x441D,0x40);
    IMX134MIPI_write_cmos_sensor(0x441E,0x1E);
    IMX134MIPI_write_cmos_sensor(0x441F,0x38);
    //IMX134MIPI_write_cmos_sensor(0x4420,0x01);
    //IMX134MIPI_write_cmos_sensor(0x4444,0x00);
    //IMX134MIPI_write_cmos_sensor(0x4445,0x00);
    IMX134MIPI_write_cmos_sensor(0x4446,0x1D);
    IMX134MIPI_write_cmos_sensor(0x4447,0xF9);
    IMX134MIPI_write_cmos_sensor(0x4452,0x00);
    IMX134MIPI_write_cmos_sensor(0x4453,0xA0);
    IMX134MIPI_write_cmos_sensor(0x4454,0x08);
    IMX134MIPI_write_cmos_sensor(0x4455,0x00);
    //IMX134MIPI_write_cmos_sensor(0x4456,0x0F);
    //IMX134MIPI_write_cmos_sensor(0x4457,0xFF);
    IMX134MIPI_write_cmos_sensor(0x4458,0x18);
    IMX134MIPI_write_cmos_sensor(0x4459,0x18);
    IMX134MIPI_write_cmos_sensor(0x445A,0x3F);
    IMX134MIPI_write_cmos_sensor(0x445B,0x3A);
    //IMX134MIPI_write_cmos_sensor(0x445C,0x00);
    IMX134MIPI_write_cmos_sensor(0x445D,0x28);
    //IMX134MIPI_write_cmos_sensor(0x445E,0x01);
    IMX134MIPI_write_cmos_sensor(0x445F,0x90);
    //IMX134MIPI_write_cmos_sensor(0x4460,0x00);
    //IMX134MIPI_write_cmos_sensor(0x4461,0x60);
    //IMX134MIPI_write_cmos_sensor(0x4462,0x00);
    IMX134MIPI_write_cmos_sensor(0x4463,0x00);
    //IMX134MIPI_write_cmos_sensor(0x4464,0x00);
    IMX134MIPI_write_cmos_sensor(0x4465,0x00);
    //IMX134MIPI_write_cmos_sensor(0x446C,0x00);
    //IMX134MIPI_write_cmos_sensor(0x446D,0x00);
    //IMX134MIPI_write_cmos_sensor(0x446E,0x00);
    //LSC Setting
    IMX134MIPI_write_cmos_sensor(0x452A,0x02);
    //White Balance Setting
    //IMX134MIPI_write_cmos_sensor(0x0712,0x01);
    //IMX134MIPI_write_cmos_sensor(0x0713,0x00);
    //IMX134MIPI_write_cmos_sensor(0x0714,0x01);
    //IMX134MIPI_write_cmos_sensor(0x0715,0x00);
    //IMX134MIPI_write_cmos_sensor(0x0716,0x01);
    //IMX134MIPI_write_cmos_sensor(0x0717,0x00);
    //IMX134MIPI_write_cmos_sensor(0x0718,0x01);
    //IMX134MIPI_write_cmos_sensor(0x0719,0x00);
    //Shading setting
    //IMX134MIPI_write_cmos_sensor(0x4500,0x1F);

	SENSORDB("IMX134MIPI_globle_setting  end \n");
}   /*  IMX134MIPI_Sensor_Init  */
static void IMX134VideoFullSizeSetting(void)//16:9   6M
{
    SENSORDB("[IMX134MIPI]enter VideoFullSizeSetting function\n");
    IMX134MIPI_write_cmos_sensor(0x41C0, 0x01);
    IMX134MIPI_write_cmos_sensor(0x0104, 0x01);//group
    IMX134MIPI_write_cmos_sensor(0x0100, 0x00);//STREAM OFF 	
    //IMX134MIPI_write_cmos_sensor(0x0103, 0x01);//SW reset
#if 1
#if 0 //size 3280 X 1664	
    //Clock Setting		                              
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
    //Mode setting		                              
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
    //OptionnalFunction setting		                  
    IMX134MIPI_write_cmos_sensor(	0x0700,0x00);     
    IMX134MIPI_write_cmos_sensor(	0x3A63,0x00);     
    IMX134MIPI_write_cmos_sensor(	0x4100,0xF8);     
    IMX134MIPI_write_cmos_sensor(	0x4203,0xFF);     
    IMX134MIPI_write_cmos_sensor(	0x4344,0x00);     
    IMX134MIPI_write_cmos_sensor(	0x441C,0x01);     
    //Size setting		                              
    IMX134MIPI_write_cmos_sensor(	0x0340,0x08);     
    IMX134MIPI_write_cmos_sensor(	0x0341,0x34);     
    IMX134MIPI_write_cmos_sensor(	0x0342,0x0E);     
    IMX134MIPI_write_cmos_sensor(	0x0343,0x10);     
    IMX134MIPI_write_cmos_sensor(	0x0344,0x00);     
    IMX134MIPI_write_cmos_sensor(	0x0345,0x00);     
    IMX134MIPI_write_cmos_sensor(	0x0346,0x01);     
    IMX134MIPI_write_cmos_sensor(	0x0347,0x90);     
    IMX134MIPI_write_cmos_sensor(	0x0348,0x0C);     
    IMX134MIPI_write_cmos_sensor(	0x0349,0xCF);     
    IMX134MIPI_write_cmos_sensor(	0x034A,0x08);     
    IMX134MIPI_write_cmos_sensor(	0x034B,0x0F);     
    IMX134MIPI_write_cmos_sensor(	0x034C,0x0C);     
    IMX134MIPI_write_cmos_sensor(	0x034D,0xD0);     
    IMX134MIPI_write_cmos_sensor(	0x034E,0x06);     
    IMX134MIPI_write_cmos_sensor(	0x034F,0x80);     
    IMX134MIPI_write_cmos_sensor(	0x0350,0x00);     
    IMX134MIPI_write_cmos_sensor(	0x0351,0x00);     
    IMX134MIPI_write_cmos_sensor(	0x0352,0x00);     
    IMX134MIPI_write_cmos_sensor(	0x0353,0x00);     
    IMX134MIPI_write_cmos_sensor(	0x0354,0x0C);     
    IMX134MIPI_write_cmos_sensor(	0x0355,0xD0);     
    IMX134MIPI_write_cmos_sensor(	0x0356,0x06);     
    IMX134MIPI_write_cmos_sensor(	0x0357,0x80);     
    IMX134MIPI_write_cmos_sensor(	0x301D,0x30);     
    IMX134MIPI_write_cmos_sensor(	0x3310,0x0C);     
    IMX134MIPI_write_cmos_sensor(	0x3311,0xD0);     
    IMX134MIPI_write_cmos_sensor(	0x3312,0x06);     
    IMX134MIPI_write_cmos_sensor(	0x3313,0x80);     
    IMX134MIPI_write_cmos_sensor(	0x331C,0x04);     
    IMX134MIPI_write_cmos_sensor(	0x331D,0x1E);     
    IMX134MIPI_write_cmos_sensor(	0x4084,0x00);     
    IMX134MIPI_write_cmos_sensor(	0x4085,0x00);     
    IMX134MIPI_write_cmos_sensor(	0x4086,0x00);     
    IMX134MIPI_write_cmos_sensor(	0x4087,0x00);     
    IMX134MIPI_write_cmos_sensor(	0x4400,0x00);     
    //Global Timing Setting		                      
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
    //Integration Time Setting		                  
    IMX134MIPI_write_cmos_sensor(	0x0202,0x08);     
    IMX134MIPI_write_cmos_sensor(	0x0203,0x30);     
    //Gain Setting		                              
    IMX134MIPI_write_cmos_sensor(	0x0205,0x00);     
    IMX134MIPI_write_cmos_sensor(	0x020E,0x01);     
    IMX134MIPI_write_cmos_sensor(	0x020F,0x00);     
    IMX134MIPI_write_cmos_sensor(	0x0210,0x01);     
    IMX134MIPI_write_cmos_sensor(	0x0211,0x00);     
    IMX134MIPI_write_cmos_sensor(	0x0212,0x01);     
    IMX134MIPI_write_cmos_sensor(	0x0213,0x00);     
    IMX134MIPI_write_cmos_sensor(	0x0214,0x01);     
    IMX134MIPI_write_cmos_sensor(	0x0215,0x00);     
    //HDR Setting		                                
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
#else   //size :3280 X 1832

    //Clock Setting		                         
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
    //Mode setting		                           
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
    //OptionnalFunction setting
#ifndef IMX134MIPI_USE_OTP
    IMX134MIPI_write_cmos_sensor(	0x0700,0x00);
    IMX134MIPI_write_cmos_sensor(	0x3A63,0x00);
#endif
    IMX134MIPI_write_cmos_sensor(	0x4100,0xF8);  
    IMX134MIPI_write_cmos_sensor(	0x4203,0xFF);  
    IMX134MIPI_write_cmos_sensor(	0x4344,0x00);  
    IMX134MIPI_write_cmos_sensor(	0x441C,0x01);  
    //Size setting		                           
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
    //Global Timing Setting		                   
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
    //Integration Time Setting		               
    IMX134MIPI_write_cmos_sensor(	0x0202,0x08);  
    IMX134MIPI_write_cmos_sensor(	0x0203,0x30);  
    //Gain Setting		                           
    IMX134MIPI_write_cmos_sensor(	0x0205,0x00);  
    IMX134MIPI_write_cmos_sensor(	0x020E,0x01);  
    IMX134MIPI_write_cmos_sensor(	0x020F,0x00);  
    IMX134MIPI_write_cmos_sensor(	0x0210,0x01);  
    IMX134MIPI_write_cmos_sensor(	0x0211,0x00);  
    IMX134MIPI_write_cmos_sensor(	0x0212,0x01);  
    IMX134MIPI_write_cmos_sensor(	0x0213,0x00);  
    IMX134MIPI_write_cmos_sensor(	0x0214,0x01);  
    IMX134MIPI_write_cmos_sensor(	0x0215,0x00);  
    //HDR Setting		                             
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
    //Clock Aetting
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
    //Mode setting		                           
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
    //OptionnalFunction setting		               
    IMX134MIPI_write_cmos_sensor(	0x0700,0x00);  
    IMX134MIPI_write_cmos_sensor(	0x3A63,0x00);  
    IMX134MIPI_write_cmos_sensor(	0x4100,0xF8);  
    IMX134MIPI_write_cmos_sensor(	0x4203,0xFF);  
    IMX134MIPI_write_cmos_sensor(	0x4344,0x00);  
    IMX134MIPI_write_cmos_sensor(	0x441C,0x01);  
    //Size setting		                           
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
    //Global Timing Setting		                   
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
    //Integration Time Setting		               
    IMX134MIPI_write_cmos_sensor(	0x0202,0x08);  
    IMX134MIPI_write_cmos_sensor(	0x0203,0x30);  
    //Gain Setting		                           
    IMX134MIPI_write_cmos_sensor(	0x0205,0x00);  
    IMX134MIPI_write_cmos_sensor(	0x020E,0x01);  
    IMX134MIPI_write_cmos_sensor(	0x020F,0x00);  
    IMX134MIPI_write_cmos_sensor(	0x0210,0x01);  
    IMX134MIPI_write_cmos_sensor(	0x0211,0x00);  
    IMX134MIPI_write_cmos_sensor(	0x0212,0x01);  
    IMX134MIPI_write_cmos_sensor(	0x0213,0x00);  
    IMX134MIPI_write_cmos_sensor(	0x0214,0x01);  
    IMX134MIPI_write_cmos_sensor(	0x0215,0x00);  
    //HDR Setting		                             
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
static void IMX134PreviewSetting(void)
{
    SENSORDB("[IMX134MIPI]enter PreviewSetting function\n");
    IMX134MIPI_write_cmos_sensor(0x0104, 0x01);//group
    IMX134MIPI_write_cmos_sensor(0x0100, 0x00);//STREAM OFF 	
    //IMX134MIPI_write_cmos_sensor(0x0103, 0x01);//SW reset


    //Clock Setting		                                                               //24fps               //29fps               //dummy pixel 360 binning        //232M pixel     //232               
    IMX134MIPI_write_cmos_sensor(0x011E,0x18 );          //0x011E	0x18              //0x011E	0x18         0x011E	0x18             0x011E	0x18                     0x011E	0x18      0x011E	0x18 
    IMX134MIPI_write_cmos_sensor(0x011F,0x00 );          //0x011F	0x00                0x011F	0x00         0x011F	0x00             0x011F	0x00                     0x011F	0x00      0x011F	0x00 
    IMX134MIPI_write_cmos_sensor(0x0301,0x05 );          //0x0301	0x05                0x0301	0x05         0x0301	0x05             0x0301	0x05                     0x0301	0x05      0x0301	0x05 
    IMX134MIPI_write_cmos_sensor(0x0303,0x01 );          //0x0303	0x01                0x0303	0x01         0x0303	0x01             0x0303	0x01                     0x0303	0x01      0x0303	0x01 
    IMX134MIPI_write_cmos_sensor(0x0305,0x0C );          //0x0305	0x0C                0x0305	0x0C         0x0305	0x0C             0x0305	0x0C                     0x0305	0x0C      0x0305	0x0C 
    IMX134MIPI_write_cmos_sensor(0x0309,0x05 );          //0x0309	0x05                0x0309	0x05         0x0309	0x05             0x0309	0x05                     0x0309	0x05      0x0309	0x05 
    IMX134MIPI_write_cmos_sensor(0x030B,0x01 );          //0x030B	0x01                0x030B	0x01         0x030B	0x01             0x030B	0x01                     0x030B	0x01      0x030B	0x01 
    IMX134MIPI_write_cmos_sensor(0x030C,0x01 );          //0x030C	0x01                0x030C	0x01         0x030C	0x01             0x030C	0x00                     0x030C	0x01      0x030C	0x01 
    IMX134MIPI_write_cmos_sensor(0x030D,0x22 );          //0x030D	0x22                0x030D	0x2C         0x030D	0x44             0x030D	0xFA                     0x030D	0x22      0x030D	0x22 
    IMX134MIPI_write_cmos_sensor(0x030E,0x01 );          //0x030E	0x01                0x030E	0x01         0x030E	0x01             0x030E	0x01                     0x030E	0x01      0x030E	0x01 
    IMX134MIPI_write_cmos_sensor(0x3A06,0x11 );          //0x3A06	0x11                0x3A06	0x11         0x3A06	0x11             0x3A06	0x11                     0x3A06	0x11      0x3A06	0x11 
    //Mode setting		                                   //                            	                   	                       	                               	                	           
    IMX134MIPI_write_cmos_sensor(0x0108,0x03 );          //0x0108	0x03                0x0108	0x03         0x0108	0x03             0x0108	0x03                     0x0108	0x03      0x0108	0x03 
    IMX134MIPI_write_cmos_sensor(0x0112,0x0A );          //0x0112	0x0A                0x0112	0x0A         0x0112	0x0A             0x0112	0x0A                     0x0112	0x0A      0x0112	0x0A 
    IMX134MIPI_write_cmos_sensor(0x0113,0x0A );          //0x0113	0x0A                0x0113	0x0A         0x0113	0x0A             0x0113	0x0A                     0x0113	0x0A      0x0113	0x0A 
    IMX134MIPI_write_cmos_sensor(0x0381,0x01 );          //0x0381	0x01                0x0381	0x01         0x0381	0x01             0x0381	0x01                     0x0381	0x01      0x0381	0x01 
    IMX134MIPI_write_cmos_sensor(0x0383,0x01 );          //0x0383	0x01                0x0383	0x01         0x0383	0x01             0x0383	0x01                     0x0383	0x01      0x0383	0x01 
    IMX134MIPI_write_cmos_sensor(0x0385,0x01 );          //0x0385	0x01                0x0385	0x01         0x0385	0x01             0x0385	0x01                     0x0385	0x01      0x0385	0x01 
    IMX134MIPI_write_cmos_sensor(0x0387,0x01 );          //0x0387	0x01                0x0387	0x01         0x0387	0x01             0x0387	0x01                     0x0387	0x01      0x0387	0x01 
    IMX134MIPI_write_cmos_sensor(0x0390,0x01 );          //0x0390	0x00                0x0390	0x01         0x0390	0x01             0x0390	0x01                     0x0390	0x01      0x0390	0x01 
    IMX134MIPI_write_cmos_sensor(0x0391,0x22 );          //0x0391	0x11                0x0391	0x22         0x0391	0x22             0x0391	0x22                     0x0391	0x22      0x0391	0x22 
    IMX134MIPI_write_cmos_sensor(0x0392,0x00 );          //0x0392	0x00                0x0392	0x00         0x0392	0x00             0x0392	0x00                     0x0392	0x00      0x0392	0x00 
    IMX134MIPI_write_cmos_sensor(0x0401,0x00 );          //0x0401	0x02                0x0401	0x00         0x0401	0x00             0x0401	0x00                     0x0401	0x00      0x0401	0x00 
    IMX134MIPI_write_cmos_sensor(0x0404,0x00 );          //0x0404	0x00                0x0404	0x00         0x0404	0x00             0x0404	0x00                     0x0404	0x00      0x0404	0x00 
    IMX134MIPI_write_cmos_sensor(0x0405,0x10 );          //0x0405	0x20                0x0405	0x10         0x0405	0x10             0x0405	0x10                     0x0405	0x10      0x0405	0x10 
    IMX134MIPI_write_cmos_sensor(0x4082,0x01 );          //0x4082	0x00                0x4082	0x01         0x4082	0x01             0x4082	0x01                     0x4082	0x01      0x4082	0x01 
    IMX134MIPI_write_cmos_sensor(0x4083,0x01 );          //0x4083	0x00                0x4083	0x01         0x4083	0x01             0x4083	0x01                     0x4083	0x01      0x4083	0x01 
    IMX134MIPI_write_cmos_sensor(0x7006,0x04 );          //0x7006	0x04                0x7006	0x04         0x7006	0x04             0x7006	0x04                     0x7006	0x04      0x7006	0x04 
    //OptionnalFunction setting		                       //                            	                   	                       	                               	                	           
#ifndef IMX134MIPI_USE_OTP
    IMX134MIPI_write_cmos_sensor(0x0700,0x00 );          //0x0700	0x00                0x0700	0x00         0x0700	0x00             0x0700	0x00                     0x0700	0x00      0x0700	0x00 
    IMX134MIPI_write_cmos_sensor(0x3A63,0x00 );          //0x3A63	0x00                0x3A63	0x00         0x3A63	0x00             0x3A63	0x00                     0x3A63	0x00      0x3A63	0x00 
#endif
    IMX134MIPI_write_cmos_sensor(0x4100,0xF8 );          //0x4100	0xF8                0x4100	0xF8         0x4100	0xF8             0x4100	0xF8                     0x4100	0xF8      0x4100	0xF8 
    IMX134MIPI_write_cmos_sensor(0x4203,0xFF );          //0x4203	0xFF                0x4203	0xFF         0x4203	0xFF             0x4203	0xFF                     0x4203	0xFF      0x4203	0xFF 
    IMX134MIPI_write_cmos_sensor(0x4344,0x00 );          //0x4344	0x00                0x4344	0x00         0x4344	0x00             0x4344	0x00                     0x4344	0x00      0x4344	0x00 
    IMX134MIPI_write_cmos_sensor(0x441C,0x01 );          //0x441C	0x01                0x441C	0x01         0x441C	0x01             0x441C	0x01                     0x441C	0x01      0x441C	0x01 
    //Size setting		                                   //                            	                   	                       	                               	                	           
    IMX134MIPI_write_cmos_sensor(0x0340,0x07 );          //0x0340	0x08                0x0340	0x0A         0x0340	0x09             0x0340	0x06                     0x0340	0x07      0x0340	0x07 
    IMX134MIPI_write_cmos_sensor(0x0341,0xA0 );          //0x0341	0x48                0x0341	0xD8         0x0341	0x7E             0x0341	0x70                     0x0341	0x80      0x0341	0xA0 
    IMX134MIPI_write_cmos_sensor(0x0342,0x0F );          //0x0342	0x0E                0x0342	0x0E         0x0342	0x0E             0x0342	0x0F                     0x0342	0x0F      0x0342	0x0F 
    IMX134MIPI_write_cmos_sensor(0x0343,0x50 );          //0x0343	0x10                0x0343	0x10         0x0343	0x10             0x0343	0xA0                     0x0343	0xA0      0x0343	0x50 
    IMX134MIPI_write_cmos_sensor(0x0344,0x00 );          //0x0344	0x00                0x0344	0x00         0x0344	0x00             0x0344	0x00                     0x0344	0x00      0x0344	0x00 
    IMX134MIPI_write_cmos_sensor(0x0345,0x00 );          //0x0345	0x00                0x0345	0x08         0x0345	0x08             0x0345	0x00                     0x0345	0x00      0x0345	0x00 
    IMX134MIPI_write_cmos_sensor(0x0346,0x00 );          //0x0346	0x00                0x0346	0x00         0x0346	0x00             0x0346	0x00                     0x0346	0x00      0x0346	0x00 
    IMX134MIPI_write_cmos_sensor(0x0347,0x00 );          //0x0347	0x00                0x0347	0x08         0x0347	0x14             0x0347	0x00                     0x0347	0x00      0x0347	0x00 
    IMX134MIPI_write_cmos_sensor(0x0348,0x0C );          //0x0348	0x0C                0x0348	0x0C         0x0348	0x0C             0x0348	0x0C                     0x0348	0x0C      0x0348	0x0C 
    IMX134MIPI_write_cmos_sensor(0x0349,0xCF );          //0x0349	0xCF                0x0349	0xC7         0x0349	0xC7             0x0349	0xCF                     0x0349	0xCF      0x0349	0xCF 
    IMX134MIPI_write_cmos_sensor(0x034A,0x09 );          //0x034A	0x09                0x034A	0x09         0x034A	0x09             0x034A	0x09                     0x034A	0x09      0x034A	0x09 
    IMX134MIPI_write_cmos_sensor(0x034B,0x9F );          //0x034B	0x9F                0x034B	0x97         0x034B	0x8B             0x034B	0x9F                     0x034B	0x9F      0x034B	0x9F 
    IMX134MIPI_write_cmos_sensor(0x034C,0x06 );          //0x034C	0x06                0x034C	0x06         0x034C	0x06             0x034C	0x06                     0x034C	0x06      0x034C	0x06 
    IMX134MIPI_write_cmos_sensor(0x034D,0x68 );          //0x034D	0x68                0x034D	0x60         0x034D	0x60             0x034D	0x68                     0x034D	0x68      0x034D	0x68 
    IMX134MIPI_write_cmos_sensor(0x034E,0x04 );          //0x034E	0x04                0x034E	0x04         0x034E	0x04             0x034E	0x04                     0x034E	0x04      0x034E	0x04 
    IMX134MIPI_write_cmos_sensor(0x034F,0xD0 );          //0x034F	0xD0                0x034F	0xC8         0x034F	0xBC             0x034F	0xD0                     0x034F	0xD0      0x034F	0xD0 
    IMX134MIPI_write_cmos_sensor(0x0350,0x00 );          //0x0350	0x00                0x0350	0x00         0x0350	0x00             0x0350	0x00                     0x0350	0x00      0x0350	0x00 
    IMX134MIPI_write_cmos_sensor(0x0351,0x00 );          //0x0351	0x00                0x0351	0x00         0x0351	0x00             0x0351	0x00                     0x0351	0x00      0x0351	0x00 
    IMX134MIPI_write_cmos_sensor(0x0352,0x00 );          //0x0352	0x00                0x0352	0x00         0x0352	0x00             0x0352	0x00                     0x0352	0x00      0x0352	0x00 
    IMX134MIPI_write_cmos_sensor(0x0353,0x00 );          //0x0353	0x00                0x0353	0x00         0x0353	0x00             0x0353	0x00                     0x0353	0x00      0x0353	0x00 
    IMX134MIPI_write_cmos_sensor(0x0354,0x06 );          //0x0354	0x0C                0x0354	0x06         0x0354	0x06             0x0354	0x06                     0x0354	0x06      0x0354	0x06 
    IMX134MIPI_write_cmos_sensor(0x0355,0x68 );          //0x0355	0xD0                0x0355	0x60         0x0355	0x60             0x0355	0x68                     0x0355	0x68      0x0355	0x68 
    IMX134MIPI_write_cmos_sensor(0x0356,0x04 );          //0x0356	0x09                0x0356	0x04         0x0356	0x04             0x0356	0x04                     0x0356	0x04      0x0356	0x04 
    IMX134MIPI_write_cmos_sensor(0x0357,0xD0 );          //0x0357	0xA0                0x0357	0xC8         0x0357	0xBC             0x0357	0xD0                     0x0357	0xD0      0x0357	0xD0 
    IMX134MIPI_write_cmos_sensor(0x301D,0x30 );          //0x301D	0x30                0x301D	0x30         0x301D	0x30             0x301D	0x30                     0x301D	0x30      0x301D	0x30 
    IMX134MIPI_write_cmos_sensor(0x3310,0x06 );          //0x3310	0x06                0x3310	0x06         0x3310	0x06             0x3310	0x06                     0x3310	0x06      0x3310	0x06 
    IMX134MIPI_write_cmos_sensor(0x3311,0x68 );          //0x3311	0x68                0x3311	0x60         0x3311	0x60             0x3311	0x68                     0x3311	0x68      0x3311	0x68 
    IMX134MIPI_write_cmos_sensor(0x3312,0x04 );          //0x3312	0x04                0x3312	0x04         0x3312	0x04             0x3312	0x04                     0x3312	0x04      0x3312	0x04 
    IMX134MIPI_write_cmos_sensor(0x3313,0xD0 );          //0x3313	0xD0                0x3313	0xC8         0x3313	0xBC             0x3313	0xD0                     0x3313	0xD0      0x3313	0xD0 
    IMX134MIPI_write_cmos_sensor(0x331C,0x04 );          //0x331C	0x04                0x331C	0x04         0x331C	0x04             0x331C	0x04                     0x331C	0x04      0x331C	0x04 
    IMX134MIPI_write_cmos_sensor(0x331D,0x06 );          //0x331D	0x1E                0x331D	0x06         0x331D	0x06             0x331D	0x06                     0x331D	0x06      0x331D	0x06 
    IMX134MIPI_write_cmos_sensor(0x4084,0x00 );          //0x4084	0x06                0x4084	0x00         0x4084	0x00             0x4084	0x00                     0x4084	0x00      0x4084	0x00 
    IMX134MIPI_write_cmos_sensor(0x4085,0x00 );          //0x4085	0x68                0x4085	0x00         0x4085	0x00             0x4085	0x00                     0x4085	0x00      0x4085	0x00 
    IMX134MIPI_write_cmos_sensor(0x4086,0x00 );          //0x4086	0x04                0x4086	0x00         0x4086	0x00             0x4086	0x00                     0x4086	0x00      0x4086	0x00 
    IMX134MIPI_write_cmos_sensor(0x4087,0x00 );          //0x4087	0xD0                0x4087	0x00         0x4087	0x00             0x4087	0x00                     0x4087	0x00      0x4087	0x00 
    IMX134MIPI_write_cmos_sensor(0x4400,0x00 );          //0x4400	0x00                0x4400	0x00         0x4400	0x00             0x4400	0x00                     0x4400	0x00      0x4400	0x00 
    //Global Timing Setting		                           //	                          	                   	                       	                               	                	           
    IMX134MIPI_write_cmos_sensor(0x0830,0x6F );          //0x0830	0x6F                0x0830	0x6F         0x0830	0x77             0x0830	0x67                     0x0830	0x6F      0x0830	0x6F 
    IMX134MIPI_write_cmos_sensor(0x0831,0x27 );          //0x0831	0x27                0x0831	0x27         0x0831	0x2F             0x0831	0x27                     0x0831	0x27      0x0831	0x27 
    IMX134MIPI_write_cmos_sensor(0x0832,0x4F );          //0x0832	0x4F                0x0832	0x4F         0x0832	0x4F             0x0832	0x47                     0x0832	0x4F      0x0832	0x4F 
    IMX134MIPI_write_cmos_sensor(0x0833,0x2F );          //0x0833	0x2F                0x0833	0x2F         0x0833	0x2F             0x0833	0x27                     0x0833	0x2F      0x0833	0x2F 
    IMX134MIPI_write_cmos_sensor(0x0834,0x2F );          //0x0834	0x2F                0x0834	0x2F         0x0834	0x2F             0x0834	0x27                     0x0834	0x2F      0x0834	0x2F 
    IMX134MIPI_write_cmos_sensor(0x0835,0x2F );          //0x0835	0x2F                0x0835	0x2F         0x0835	0x37             0x0835	0x1F                     0x0835	0x2F      0x0835	0x2F 
    IMX134MIPI_write_cmos_sensor(0x0836,0x9F );          //0x0836	0x9F                0x0836	0x9F         0x0836	0xA7             0x0836	0x87                     0x0836	0x9F      0x0836	0x9F 
    IMX134MIPI_write_cmos_sensor(0x0837,0x37 );          //0x0837	0x37                0x0837	0x37         0x0837	0x37             0x0837	0x2F                     0x0837	0x37      0x0837	0x37 
    IMX134MIPI_write_cmos_sensor(0x0839,0x1F );          //0x0839	0x1F                0x0839	0x1F         0x0839	0x1F             0x0839	0x1F                     0x0839	0x1F      0x0839	0x1F 
    IMX134MIPI_write_cmos_sensor(0x083A,0x17 );          //0x083A	0x17                0x083A	0x17         0x083A	0x17             0x083A	0x17                     0x083A	0x17      0x083A	0x17 
    IMX134MIPI_write_cmos_sensor(0x083B,0x02 );          //0x083B	0x02                0x083B	0x02         0x083B	0x02             0x083B	0x02                     0x083B	0x02      0x083B	0x02 
    //Integration Time Setting		                       //	                          	                   	                       	                               	                	           
    IMX134MIPI_write_cmos_sensor(0x0202,0x07 );          //0x0202	0x08                0x0202	0x0A         0x0202	0x09             0x0202	0x06                     0x0202	0x07      0x0202	0x07 
    IMX134MIPI_write_cmos_sensor(0x0203,0x9C );          //0x0203	0x44                0x0203	0xD4         0x0203	0x7A             0x0203	0x6C                     0x0203	0x7C      0x0203	0x9C 
    //Gain Setting		                                   //	                          	                   	                       	                               	                	           
    IMX134MIPI_write_cmos_sensor(0x0205,0x00 );          //0x0205	0x00                0x0205	0x00         0x0205	0x00             0x0205	0x00                     0x0205	0x00      0x0205	0x00 
    IMX134MIPI_write_cmos_sensor(0x020E,0x01 );          //0x020E	0x01                0x020E	0x01         0x020E	0x01             0x020E	0x01                     0x020E	0x01      0x020E	0x01 
    IMX134MIPI_write_cmos_sensor(0x020F,0x00 );          //0x020F	0x00                0x020F	0x00         0x020F	0x00             0x020F	0x00                     0x020F	0x00      0x020F	0x00 
    IMX134MIPI_write_cmos_sensor(0x0210,0x01 );          //0x0210	0x01                0x0210	0x01         0x0210	0x01             0x0210	0x01                     0x0210	0x01      0x0210	0x01 
    IMX134MIPI_write_cmos_sensor(0x0211,0x00 );          //0x0211	0x00                0x0211	0x00         0x0211	0x00             0x0211	0x00                     0x0211	0x00      0x0211	0x00 
    IMX134MIPI_write_cmos_sensor(0x0212,0x01 );          //0x0212	0x01                0x0212	0x01         0x0212	0x01             0x0212	0x01                     0x0212	0x01      0x0212	0x01 
    IMX134MIPI_write_cmos_sensor(0x0213,0x00 );          //0x0213	0x00                0x0213	0x00         0x0213	0x00             0x0213	0x00                     0x0213	0x00      0x0213	0x00 
    IMX134MIPI_write_cmos_sensor(0x0214,0x01 );          //0x0214	0x01                0x0214	0x01         0x0214	0x01             0x0214	0x01                     0x0214	0x01      0x0214	0x01 
    IMX134MIPI_write_cmos_sensor(0x0215,0x00 );          //0x0215	0x00                0x0215	0x00         0x0215	0x00             0x0215	0x00                     0x0215	0x00      0x0215	0x00 
    //HDR Setting		                                     //                            	                   	                       	                               	                	           
    IMX134MIPI_write_cmos_sensor(0x0230,0x00 );          //0x0230	0x00                0x0230	0x00         0x0230	0x00             0x0230	0x00                     0x0230	0x00      0x0230	0x00 
    IMX134MIPI_write_cmos_sensor(0x0231,0x00 );          //0x0231	0x00                0x0231	0x00         0x0231	0x00             0x0231	0x00                     0x0231	0x00      0x0231	0x00 
    IMX134MIPI_write_cmos_sensor(0x0233,0x00 );          //0x0233	0x00                0x0233	0x00         0x0233	0x00             0x0233	0x00                     0x0233	0x00      0x0233	0x00 
    IMX134MIPI_write_cmos_sensor(0x0234,0x00 );          //0x0234	0x00                0x0234	0x00         0x0234	0x00             0x0234	0x00                     0x0234	0x00      0x0234	0x00 
    IMX134MIPI_write_cmos_sensor(0x0235,0x40 );          //0x0235	0x40                0x0235	0x40         0x0235	0x40             0x0235	0x40                     0x0235	0x40      0x0235	0x40 
    IMX134MIPI_write_cmos_sensor(0x0238,0x00 );          //0x0238	0x00                0x0238	0x00         0x0238	0x00             0x0238	0x00                     0x0238	0x00      0x0238	0x00 
    IMX134MIPI_write_cmos_sensor(0x0239,0x04 );          //0x0239	0x04                0x0239	0x04         0x0239	0x04             0x0239	0x04                     0x0239	0x04      0x0239	0x04 
    IMX134MIPI_write_cmos_sensor(0x023B,0x00 );          //0x023B	0x00                0x023B	0x00         0x023B	0x00             0x023B	0x00                     0x023B	0x00      0x023B	0x00 
    IMX134MIPI_write_cmos_sensor(0x023C,0x01 );          //0x023C	0x01                0x023C	0x01         0x023C	0x01             0x023C	0x01                     0x023C	0x01      0x023C	0x01 
    IMX134MIPI_write_cmos_sensor(0x33B0,0x04 );          //0x33B0	0x04                0x33B0	0x04         0x33B0	0x04             0x33B0	0x04                     0x33B0	0x04      0x33B0	0x04 
    IMX134MIPI_write_cmos_sensor(0x33B1,0x00 );          //0x33B1	0x00                0x33B1	0x00         0x33B1	0x00             0x33B1	0x00                     0x33B1	0x00      0x33B1	0x00 
    IMX134MIPI_write_cmos_sensor(0x33B3,0x00 );          //0x33B3	0x00                0x33B3	0x00         0x33B3	0x00             0x33B3	0x00                     0x33B3	0x00      0x33B3	0x00 
    IMX134MIPI_write_cmos_sensor(0x33B4,0x01 );          //0x33B4	0x01                0x33B4	0x01         0x33B4	0x01             0x33B4	0x01                     0x33B4	0x01      0x33B4	0x01 
    IMX134MIPI_write_cmos_sensor(0x3800,0x00 );          //0x3800	0x00                0x3800	0x00         0x3800	0x00             0x3800	0x00                     0x3800	0x00      0x3800	0x00 
    IMX134MIPI_write_cmos_sensor(	0x3A43,0x01);   



    IMX134MIPI_write_cmos_sensor(0x0104, 0x00);//group
    IMX134MIPI_write_cmos_sensor(0x0100, 0x01);//STREAM ON
    // The register only need to enable 1 time.    
    spin_lock(&imx134_drv_lock);  
    IMX134MIPI_Auto_Flicker_mode = KAL_FALSE;	  // reset the flicker status	 
    spin_unlock(&imx134_drv_lock);
    SENSORDB("[IMX134MIPI]exit PreviewSetting function\n");
} 
  
static void IMX134MIPI_set_8M(void)
{	//77 capture setting
    	SENSORDB("[IMX134MIPI]enter IMX134MIPI_set_8M function\n");

    	IMX134MIPI_write_cmos_sensor(0x0104, 0x01);//group
    	IMX134MIPI_write_cmos_sensor(0x0100, 0x00);//STREAM OFF 	
    //IMX134MIPI_write_cmos_sensor(0x0103, 0x01);//SW reset

    IMX134MIPI_write_cmos_sensor(0x0101, 0x00); 
#if 1 //3280 X 2464		 
    //Clock Setting		                        
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
    //Mode setting		                          
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
    //OptionnalFunction setting
#ifndef IMX134MIPI_USE_OTP
    IMX134MIPI_write_cmos_sensor(0x0700,0x00);
    IMX134MIPI_write_cmos_sensor(0x3A63,0x00);
#endif
    IMX134MIPI_write_cmos_sensor(0x4100,0xF8);  
    IMX134MIPI_write_cmos_sensor(0x4203,0xFF);  
    IMX134MIPI_write_cmos_sensor(0x4344,0x00);  
    IMX134MIPI_write_cmos_sensor(0x441C,0x01);  
    //Size setting		                          
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
    //Global Timing Setting		                  
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
    //Integration Time Setting		              
    IMX134MIPI_write_cmos_sensor(0x0202,0x09);  
    IMX134MIPI_write_cmos_sensor(0x0203,0xE8);  
    //Gain Setting		                          
    IMX134MIPI_write_cmos_sensor(0x0205,0x00);  
    IMX134MIPI_write_cmos_sensor(0x020E,0x01);  
    IMX134MIPI_write_cmos_sensor(0x020F,0x00);  
    IMX134MIPI_write_cmos_sensor(0x0210,0x01);  
    IMX134MIPI_write_cmos_sensor(0x0211,0x00);  
    IMX134MIPI_write_cmos_sensor(0x0212,0x01);  
    IMX134MIPI_write_cmos_sensor(0x0213,0x00);  
    IMX134MIPI_write_cmos_sensor(0x0214,0x01);  
    IMX134MIPI_write_cmos_sensor(0x0215,0x00);  
    //HDR Setting		                            
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

    //Clock Setting		                                                 
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
    //Mode setting		                                  //               
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
    //OptionnalFunction setting		                      //               
    IMX134MIPI_write_cmos_sensor(0x0700,0x00);          //0x0700	0x00   
    IMX134MIPI_write_cmos_sensor(0x3A63,0x00);          //0x3A63	0x00   
    IMX134MIPI_write_cmos_sensor(0x4100,0xF8);          //0x4100	0xF8   
    IMX134MIPI_write_cmos_sensor(0x4203,0xFF);          //0x4203	0xFF   
    IMX134MIPI_write_cmos_sensor(0x4344,0x00);          //0x4344	0x00   
    IMX134MIPI_write_cmos_sensor(0x441C,0x01);          //0x441C	0x01   
    //Size setting		                                  //               
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
    //Global Timing Setting		                          //	             
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
    //Integration Time Setting		                      //	             
    IMX134MIPI_write_cmos_sensor(0x0202,0x08);          //0x0202	0x08   
    IMX134MIPI_write_cmos_sensor(0x0203,0x44);          //0x0203	0x44   
    //Gain Setting		                                  //	             
    IMX134MIPI_write_cmos_sensor(0x0205,0x00);          //0x0205	0x00   
    IMX134MIPI_write_cmos_sensor(0x020E,0x01);          //0x020E	0x01   
    IMX134MIPI_write_cmos_sensor(0x020F,0x00);          //0x020F	0x00   
    IMX134MIPI_write_cmos_sensor(0x0210,0x01);          //0x0210	0x01   
    IMX134MIPI_write_cmos_sensor(0x0211,0x00);          //0x0211	0x00   
    IMX134MIPI_write_cmos_sensor(0x0212,0x01);          //0x0212	0x01   
    IMX134MIPI_write_cmos_sensor(0x0213,0x00);          //0x0213	0x00   
    IMX134MIPI_write_cmos_sensor(0x0214,0x01);          //0x0214	0x01   
    IMX134MIPI_write_cmos_sensor(0x0215,0x00);          //0x0215	0x00   
    //HDR Setting		                                    //               
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


#ifdef IMX134MIPI_USE_OTP

/******************************************************************************
Function:        // IMX134MIPI_OTP_Read_ID
Description:    // read module info OTP.
Input:            //current_otp, pointer to otp struct to store otp data
Output:         //
Return:         //KAL_TRUE for read cucessful, KAL_FALSE for read otp fails.
Others:         //
******************************************************************************/
static bool IMX134MIPI_OTP_Read_ID(struct IMX134MIPI_Otp_Struct * otp_ptr)
{

    kal_uint8 vendor_id =0;

    otp_ptr->iProduct_Year = IMX134MIPI_read_eeprom_otp_data(OTP_ID_AWB_I2C_ADD,0x00);
    otp_ptr->iProduct_Month = IMX134MIPI_read_eeprom_otp_data(OTP_ID_AWB_I2C_ADD,0x01);
    otp_ptr->iProduct_Date = IMX134MIPI_read_eeprom_otp_data(OTP_ID_AWB_I2C_ADD,0x02);
    otp_ptr->iCamera_Id= IMX134MIPI_read_eeprom_otp_data(OTP_ID_AWB_I2C_ADD,0x03);
    otp_ptr->iSupplier_Version_Id= IMX134MIPI_read_eeprom_otp_data(OTP_ID_AWB_I2C_ADD,0x04);

    vendor_id = (otp_ptr->iSupplier_Version_Id >> 4);

    SENSORDB("%s, iProduct_Year:%d\n", __func__, otp_ptr->iProduct_Year);
    SENSORDB("%s, iSupplier_Version_Id:%d\n", __func__, otp_ptr->iSupplier_Version_Id);

    if(0x01 == vendor_id) //sunny
    {
        module_type = MODULE_SUNNY;
        SENSORDB("Read OTP module type:Sunny!\n");
    }
    else if(0x03 == vendor_id) //Liteon
    {
        module_type = MODULE_LITEON;
        SENSORDB("Read OTP module type:Liteon!\n");
    }
    else//unsuport
    {
        module_type = MODULE_UNSUPPORT;
        SENSORDB("Read OTP module type: unsuported!\n");
    }

    if(MODULE_LITEON == module_type)
    {
        IMX134MIPI_otp_read_flag |= IMX134MIPI_OTP_ID_READ;
        SENSORDB("Module info read sucessfully!\n");
    }
    else
    {
        SENSORDB("%s OTP data is worng!!!\n",__func__);
        return KAL_FALSE;
    }
    return KAL_TRUE;


}

/******************************************************************************
Function:        // IMX134MIPI_OTP_Read_AWB
Description:    // read awb  info OTP.
Input:            //current_otp, pointer to otp struct to store otp data
Output:         //
Return:         //KAL_TRUE for read cucessful, KAL_FALSE for read otp fails.
Others:         //
******************************************************************************/
static bool IMX134MIPI_OTP_Read_AWB(struct IMX134MIPI_Otp_Struct * otp_ptr)
{

    kal_uint16 rG_H = 0, rG_L = 0;// r/g high and low bits
    kal_uint16 bG_H = 0, bG_L = 0;// b/g high and low bits
    kal_uint16 gbGr_H = 0,  gbGr_L = 0;// gb/gr high and low bits

    rG_H = IMX134MIPI_read_eeprom_otp_data(OTP_ID_AWB_I2C_ADD,0x05);
    rG_L= IMX134MIPI_read_eeprom_otp_data(OTP_ID_AWB_I2C_ADD,0x06);
    bG_H= IMX134MIPI_read_eeprom_otp_data(OTP_ID_AWB_I2C_ADD,0x07);
    bG_L= IMX134MIPI_read_eeprom_otp_data(OTP_ID_AWB_I2C_ADD,0x08);
    gbGr_H= IMX134MIPI_read_eeprom_otp_data(OTP_ID_AWB_I2C_ADD,0x09);
    gbGr_L= IMX134MIPI_read_eeprom_otp_data(OTP_ID_AWB_I2C_ADD,0x0A);

    otp_ptr->iWB_RG = (rG_H << 8) + rG_L;
    otp_ptr->iWB_BG= (bG_H << 8) + bG_L;    
    otp_ptr->iWB_GbGr = (gbGr_H << 8) + gbGr_L;
    
    SENSORDB("%s, iWB_RG:0x%x\n",__func__,otp_ptr->iWB_RG);
    SENSORDB("%s, iWB_BG:0x%x\n",__func__,otp_ptr->iWB_BG);
    SENSORDB("%s, iWB_GbGr:0x%x\n",__func__,otp_ptr->iWB_GbGr);
    //if one of wb value is zero, the data is invalid
    if ((0 != otp_ptr->iWB_RG) && (0 != otp_ptr->iWB_BG) && (0 != otp_ptr->iWB_GbGr))
    {
        IMX134MIPI_otp_read_flag |= IMX134MIPI_OTP_AWB_READ;
        SENSORDB("AWB  info read sucessfully!\n");
        return KAL_TRUE;//otp read sucess
    }
    else
    {
        SENSORDB("%s,  Error !No AWB info found in OTP\n",__func__);
        return KAL_FALSE;//otp read fail
    }

}

/******************************************************************************
Function:        // IMX134MIPI_OTP_Read_LSC
Description:    // read lcs  info OTP.
Input:            //current_otp, pointer to otp struct to store otp data
Output:         //
Return:         //KAL_TRUE for read cucessful, KAL_FALSE for read otp fails.
Others:         //
******************************************************************************/
static bool IMX134MIPI_OTP_Read_LSC(struct IMX134MIPI_Otp_Struct * otp_ptr)
{
    int i = 0, j=0;

    memset(otp_ptr->iLsc_param, 0 , IMX134MIPI_OTP_LSC_SIZE * sizeof(kal_uint8));
    //Read LSC in A4 address
    for (i =0; i< (IMX134MIPI_OTP_LSC_A4_SIZE); i++ )
    {
        otp_ptr->iLsc_param[i] = IMX134MIPI_read_eeprom_otp_data(OTP_LSC_I2C_ADD_1, 0x0B+i);
    }
    //Read LSC in A6 address
    for (j = 0; j < (IMX134MIPI_OTP_LSC_A6_SIZE); j++)
    {
        otp_ptr->iLsc_param[j+i] = IMX134MIPI_read_eeprom_otp_data(OTP_LSC_I2C_ADD_2, 0x00+j);
    }
    SENSORDB("%s LCS[0]= 0x%x,LSC[247] = 0x%x  LSC[248]=0x%x,LSC[279]=0x%x\n",__func__, otp_ptr->iLsc_param[0], 
               otp_ptr->iLsc_param[247],otp_ptr->iLsc_param[248], otp_ptr->iLsc_param[279]);

    IMX134MIPI_otp_read_flag |= IMX134MIPI_OTP_LSC_READ;

    return KAL_TRUE;

}


/******************************************************************************
Function:        // IMX134MIPI_OTP_Read_VCM
Description:    // read vcm  info OTP.
Input:            //current_otp, pointer to otp struct to store otp data
Output:         //
Return:         //KAL_TRUE for read cucessful, KAL_FALSE for read otp fails.
Others:         //
******************************************************************************/
static bool IMX134MIPI_OTP_Read_VCM(struct IMX134MIPI_Otp_Struct * otp_ptr)
{

    kal_uint16 iVCM_StartH = 0,  iVCM_StartL = 0;// vcm start current MSB and LSB 
    kal_uint16 iVCM_EndH = 0,  iVCM_EndL = 0;// vcm end current MSB and LSB 

    //Read VCM start and end current values from A6 address
    iVCM_StartH = IMX134MIPI_read_eeprom_otp_data(OTP_VCM_I2C_ADD,0x40);
    iVCM_StartL= IMX134MIPI_read_eeprom_otp_data(OTP_VCM_I2C_ADD,0x41);
    iVCM_EndH= IMX134MIPI_read_eeprom_otp_data(OTP_VCM_I2C_ADD,0x42);
    iVCM_EndL= IMX134MIPI_read_eeprom_otp_data(OTP_VCM_I2C_ADD,0x43);

    otp_ptr->iVCM_Start= (iVCM_StartH << 8) + iVCM_StartL;
    otp_ptr->iVCM_End= (iVCM_EndH << 8) + iVCM_EndL;    

    SENSORDB("%s, iVCM_Start0x:%x\n",__func__,otp_ptr->iVCM_Start);
    SENSORDB("%s, iVCM_End:0x%x\n",__func__,otp_ptr->iVCM_End);

    //iVCM_Start and iVCM_End all not zero, so vcm info is correct
    if ((0 != otp_ptr->iVCM_Start) && (0 != otp_ptr->iVCM_End) )
    {
        IMX134MIPI_otp_read_flag |= IMX134MIPI_OTP_VCM_READ;
        SENSORDB("VCM  info read sucessfully!\n");
        return KAL_TRUE;//otp read sucess
    }
    else
    {
        SENSORDB("%s,  Error !No VCM info found in OTP\n",__func__);
        return KAL_FALSE;//otp read fail
    }

}

/******************************************************************************
Function:        // IMX134MIPI_OTP_Set_LSC
Description:    // set lcs data to sensor IC
Input:            //otp , otp struct to store otp data
Output:         //
Return:         //KAL_TRUE for set sucessful, KAL_FALSE for fails.
Others:         //
******************************************************************************/
static bool IMX134MIPI_OTP_Set_LSC(struct IMX134MIPI_Otp_Struct  otp)
{

    kal_uint8 *pval = NULL;
    int i = 0;

    //use lsc data only when all opt data read sucess
    if((IMX134MIPI_OTP_ALL_READ != (IMX134MIPI_otp_read_flag & IMX134MIPI_OTP_ALL_READ)))
    {
        //lsc data read fails, disable lsc function
        IMX134MIPI_write_cmos_sensor(0x0700, 0x00);

        SENSORDB("%s Error! LSC OTP data is worng!!!\n",__func__);
        return KAL_FALSE;
    }
    pval = otp.iLsc_param;
    /* Write lens shading parameters to sensor registers. */
    for (i=0; i<IMX134MIPI_OTP_LSC_SIZE; i++)
    {
        IMX134MIPI_write_cmos_sensor(0x4800+i, *(pval+i));
        //SENSORDB("LSC[%d] = %d  \n",i,*(pval+i));
    }

    //enable lsc function
    IMX134MIPI_write_cmos_sensor(0x4500, 0x1f);
    IMX134MIPI_write_cmos_sensor(0x0700, 0x01);
    IMX134MIPI_write_cmos_sensor(0x3a63, 0x01);
    SENSORDB("%s  LSC OTP data set sucess!!!\n",__func__);
    return KAL_TRUE;

}


/******************************************************************************
Function:       // IMX134MIPI_Otp_Debug
Description:   // Only for debug use
Input:           //
Output:        //
Return:         //
Others:         //
******************************************************************************/
static void IMX134MIPI_Otp_Debug(struct IMX134MIPI_Otp_Struct  otp_ptr)
{
    SENSORDB("%s,otp_ptr.iProduct_Year:%d\n",__func__,otp_ptr.iProduct_Year);
    SENSORDB("%s,otp_ptr.iProduct_Month:%d\n",__func__,otp_ptr.iProduct_Month);
    SENSORDB("%s,otp_ptr.iProduct_Date:%d\n",__func__,otp_ptr.iProduct_Date);
    SENSORDB("%s,otp_ptr.iCamera_Id:%d\n",__func__,otp_ptr.iCamera_Id);
    SENSORDB("%s,otp_ptr.iSupplier_Version_Id:0x%x\n",__func__,otp_ptr.iSupplier_Version_Id);
    SENSORDB("%s,otp_ptr.iWB_RG:0x%x\n",__func__, otp_ptr.iWB_RG);
    SENSORDB("%s,otp_ptr.iWB_BG:0x%x\n",__func__, otp_ptr.iWB_BG);
    SENSORDB("%s,otp_ptr.iWB_GbGr:0x%x\n",__func__, otp_ptr.iWB_GbGr);
    SENSORDB("%s,otp_ptr.iVCM_Start:0x%x\n",__func__,otp_ptr.iVCM_Start);
    SENSORDB("%s,otp_ptr.iVCM_End:0x%x\n",__func__,otp_ptr.iVCM_End);
    SENSORDB("%s,otp_ptr.iCheckSum:0x%x\n",__func__,otp_ptr.iCheckSum);

}


/******************************************************************************
Function:       // IMX134MIPI_Otp_Debug
Description:   // Only for debug use
Input:           //
Output:        //
Return:         //
Others:         //
******************************************************************************/
static void IMX134MIPI_Read_Otp(struct IMX134MIPI_Otp_Struct  *otp_ptr)
{
    kal_uint8 sum = 0;

    if ((IMX134MIPI_OTP_CHECKSUM_ERR ==(IMX134MIPI_otp_read_flag & IMX134MIPI_OTP_CHECKSUM_ERR) ) 
            || (IMX134MIPI_OTP_ALL_READ != (IMX134MIPI_otp_read_flag & IMX134MIPI_OTP_ALL_READ)) )
    {
         SENSORDB("Otp read error last time or read first time:IMX134MIPI_otp_read_flag:0x%x!\n", IMX134MIPI_otp_read_flag);
         OTPSUMVAL = 0;//calculate otp sum again
         IMX134MIPI_otp_read_flag = 0;//clear flags
         memset(otp_ptr, 0, sizeof(struct IMX134MIPI_Otp_Struct));
    }
    else
    {
        SENSORDB("OTP has benn readed sucessfully, escaping!\n");
        return;
    }
    
    IMX134MIPI_OTP_Read_ID(otp_ptr);
    IMX134MIPI_OTP_Read_AWB(otp_ptr);
    IMX134MIPI_OTP_Read_LSC(otp_ptr);
    IMX134MIPI_OTP_Read_VCM(otp_ptr);

    //read checksum value burned in eeprom
    otp_ptr->iCheckSum = IMX134MIPI_read_eeprom_otp_data(OTP_CHECKSUM_I2C_ADD, 0x44);

    sum = (OTPSUMVAL - otp_ptr->iCheckSum)%0xFF;

    SENSORDB("IMX134MIPI_Read_Otp iCheckSum is:0x%x , OTPSUMVAL:0x%x, sum:0x%x\n", otp_ptr->iCheckSum, OTPSUMVAL, sum);

    //check if checusum we calculated equal to we readed
    if (otp_ptr->iCheckSum == sum)
    {
        IMX134MIPI_otp_read_flag |= IMX134MIPI_OTP_CHECKSUM_READ;
        SENSORDB("OTP checksum read sucessfully, read flag:0x%x\n", IMX134MIPI_otp_read_flag);
    }
    else
    {
        //Checksum error, clear all bit and set checksum error bit
        IMX134MIPI_otp_read_flag = IMX134MIPI_OTP_CHECKSUM_ERR;
        SENSORDB("Error OTP checksum is worng!\n");
    }

}

/******************************************************************************
Function:        // IMX134MIPI_Check_OTP
Description:    // check if otp info has been read out succcessfully
Input:            //
Output:         //pFeatureReturnPara32 :0 OTP empty, 1 OTP exist.
Return:         //
Others:         //
******************************************************************************/
static UINT32 IMX134MIPI_Check_OTP(UINT32 *pFeatureReturnPara32)
{
    if (NULL == pFeatureReturnPara32)
    {
        SENSORDB("%s Error!Null poiner", __func__);
        return ERROR_INVALID_PARA;
    }
    //check if awb/module info/vcm/lsc OTP exist
    if (IMX134MIPI_OTP_ALL_READ== (IMX134MIPI_otp_read_flag & IMX134MIPI_OTP_ALL_READ))
    {    
        *pFeatureReturnPara32 = 1;//opt exist
    }
    else
    {
        *pFeatureReturnPara32 = 0;//no OTP
    }
    SENSORDB("IMX134MIPI_Check_OTP: IMX134MIPI_otp_read_flag:0x%x\n", IMX134MIPI_otp_read_flag);
    return ERROR_NONE;
}

#endif


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

UINT32 IMX134LITEONMIPIOpen(void)
{
    int  retry = 0; 
	kal_uint16 sensorid;
    // check if sensor ID correct
    retry = 3; 
	SENSORDB("[IMX134MIPI]enter IMX134LITEONMIPIOpen function\n");
    do {
	       sensorid=(kal_uint16)(((IMX134MIPI_read_cmos_sensor(0x0016) & 0x0f) << 8) | IMX134MIPI_read_cmos_sensor(0x0017));  
	       kal_uint16 imx134_module_id = mt_get_gpio_in(GPIO_CAMERA_ID_PIN);

	       spin_lock(&imx134_drv_lock);    
	       IMX134MIPI_sensor_id = (sensorid << 4) | (imx134_module_id & 0x01);
	       spin_unlock(&imx134_drv_lock);
		   if (IMX134MIPI_sensor_id == IMX134LITEON_SENSOR_ID)
			   break; 
		   retry--; 
	    }
	while (retry > 0);
    SENSORDB("Read Sensor ID = 0x%04x\n", IMX134MIPI_sensor_id);
    if (IMX134MIPI_sensor_id != IMX134LITEON_SENSOR_ID)
        return ERROR_SENSOR_CONNECT_FAIL;
    IMX134MIPI_Sensor_Init();
	sensorid=read_IMX134MIPI_gain();
	spin_lock(&imx134_drv_lock);	
    IMX134MIPI_sensor_gain_base = sensorid;
    IMX134MIPI_sensor.capture_mode = KAL_FALSE; //recover false or zsd caputure fail
	spin_unlock(&imx134_drv_lock);

#ifdef IMX134MIPI_USE_OTP
    //begin to read and update otp values
    IMX134MIPI_Read_Otp(&gcurrent_otp);
    IMX134MIPI_Otp_Debug(gcurrent_otp);
    IMX134MIPI_OTP_Set_LSC(gcurrent_otp);
#endif

	SENSORDB("[IMX134MIPI]exit IMX134LITEONMIPIOpen function\n");
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
static UINT32 IMX134MIPIGetSensorID(UINT32 *sensorID) 
{
    int  retry = 3; 
	SENSORDB("[IMX134MIPI]enter IMX134MIPIGetSensorID(liteon) function\n");
	
    //Optimize search process, avoid unneccesary ID reading.
    extern kal_bool searchMainSensor;

    if (KAL_FALSE == searchMainSensor)//not main sesor, escaping
    {
         SENSORDB("IMX134MIPIGetSensorID  searchMainSensor = FALSE!\n ");
        *sensorID = 0xFFFFFFFF;
        return ERROR_SENSOR_CONNECT_FAIL;
    }
	
    // check if sensor ID correct

    mt_set_gpio_mode(GPIO_CAMERA_ID_PIN, GPIO_MODE_00);
    mt_set_gpio_dir(GPIO_CAMERA_ID_PIN, GPIO_DIR_IN);
    mt_set_gpio_pull_enable(GPIO_CAMERA_ID_PIN, GPIO_PULL_ENABLE);
    mt_set_gpio_pull_select(GPIO_CAMERA_ID_PIN, GPIO_PULL_DOWN);
	
    do {		
	       UINT32 tmp_sensor_id = (kal_uint16)(((IMX134MIPI_read_cmos_sensor(0x0016) & 0x0f) << 8) | IMX134MIPI_read_cmos_sensor(0x0017)); 
	       UINT32 imx134_module_id = mt_get_gpio_in(GPIO_CAMERA_ID_PIN);
	       *sensorID = (tmp_sensor_id << 4) | (imx134_module_id & 0x01); 
           SENSORDB("[IMX134MIPIGetSensorID]: IMX134_module_id = 0x%4x;\n", *sensorID);

        if (*sensorID == IMX134LITEON_SENSOR_ID)
            break;
        retry--; 
    } while (retry > 0);

    if (*sensorID != IMX134LITEON_SENSOR_ID) {
        *sensorID = 0xFFFFFFFF; 
        return ERROR_SENSOR_CONNECT_FAIL;
    }
	SENSORDB("[IMX134MIPI]exit IMX134MIPIGetSensorID(liteon) function\n");
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
static void IMX134MIPI_SetShutter(kal_uint16 iShutter)
{

	SENSORDB("[IMX134MIPI]%s():shutter=%d\n",__FUNCTION__,iShutter);
    if (iShutter < 1)
        iShutter = 1; 
	else if(iShutter > 0xffff)
		iShutter = 0xffff;
	unsigned long flags;
	spin_lock_irqsave(&imx134_drv_lock,flags);
    IMX134MIPI_sensor.pv_shutter = iShutter;	
	spin_unlock_irqrestore(&imx134_drv_lock,flags);
    IMX134MIPI_write_shutter(iShutter);
	SENSORDB("[IMX134MIPI]exit IMX134MIPIGetSensorID function\n");
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
static UINT16 IMX134MIPI_read_shutter(void)
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
static void IMX134MIPI_NightMode(kal_bool bEnable)
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
UINT32 IMX134LITEONMIPIClose(void)
{
    //IMX134MIPI_write_cmos_sensor(0x0100,0x00);//Avoid i2c error
    return ERROR_NONE;
}	/* IMX134MIPIClose() */

static void IMX134MIPISetFlipMirror(kal_int32 imgMirror)
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
static UINT32 IMX134MIPIPreview(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
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
	IMX134MIPISetFlipMirror(IMAGE_NORMAL); //f00208919 20130930
	
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
static UINT32 IMX134MIPIVideo(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
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
	IMX134MIPISetFlipMirror(IMAGE_NORMAL); //f00208919 20130930
	
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

static UINT32 IMX134MIPICapture(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
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
	IMX134MIPISetFlipMirror(IMAGE_NORMAL); //f00208919 20130930
	
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

UINT32 IMX134LITEONMIPIGetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution)
{
    SENSORDB("[IMX134MIPI]enter IMX134LITEONMIPIGetResolution function\n");
    pSensorResolution->SensorPreviewWidth	= IMX134MIPI_REAL_PV_WIDTH;
    pSensorResolution->SensorPreviewHeight	= IMX134MIPI_REAL_PV_HEIGHT;
    pSensorResolution->SensorFullWidth		= IMX134MIPI_REAL_CAP_WIDTH;
    pSensorResolution->SensorFullHeight		= IMX134MIPI_REAL_CAP_HEIGHT;
    pSensorResolution->SensorVideoWidth		= IMX134MIPI_REAL_VIDEO_WIDTH;
    pSensorResolution->SensorVideoHeight    = IMX134MIPI_REAL_VIDEO_HEIGHT;
    SENSORDB("IMX134LITEONMIPIGetResolution :8-14");    

    return ERROR_NONE;
}   /* IMX134MIPIGetResolution() */

UINT32 IMX134LITEONMIPIGetInfo(MSDK_SCENARIO_ID_ENUM ScenarioId,
                                                MSDK_SENSOR_INFO_STRUCT *pSensorInfo,
                                                MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{ 
	SENSORDB("[IMX134MIPI]enter IMX134LITEONMIPIGetInfo function\n");
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
    //Add FOV values
    pSensorInfo->SensorHorFOV = 62;
    pSensorInfo->SensorVerFOV = 49;
    pSensorInfo->SensorVideoFrameRate=30;	
    pSensorInfo->SensorStillCaptureFrameRate=24;
    pSensorInfo->SensorWebCamCaptureFrameRate=24;
    pSensorInfo->SensorResetActiveHigh=FALSE;
    pSensorInfo->SensorResetDelayCount=5;
    pSensorInfo->SensorOutputDataFormat=SENSOR_OUTPUT_FORMAT_RAW_B; //f00208919 20130930 //SENSOR_OUTPUT_FORMAT_RAW_R; //
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
    SENSORDB("[IMX134MIPI]exit IMX134LITEONMIPIGetInfo function\n");
    return ERROR_NONE;
}   /* IMX134MIPIGetInfo() */


UINT32 IMX134LITEONMIPIControl(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow,
                                                MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{    
		spin_lock(&imx134_drv_lock);	
		IMX134CurrentScenarioId = ScenarioId;
		spin_unlock(&imx134_drv_lock);
		SENSORDB("[IMX134MIPI]enter IMX134LITEONMIPIControl function\n");
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
	SENSORDB("[IMX134MIPI]exit IMX134LITEONMIPIControl function\n");
    return ERROR_NONE;
} /* IMX134MIPIControl() */

static UINT32 IMX134MIPISetVideoMode(UINT16 u2FrameRate)
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

static UINT32 IMX134MIPISetAutoFlickerMode(kal_bool bEnable, UINT16 u2FrameRate)
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
static UINT32 IMX134MIPISetMaxFramerateByScenario(MSDK_SCENARIO_ID_ENUM scenarioId, MUINT32 frameRate) {
	kal_uint32 pclk;
	kal_int16 dummyLine;
	kal_uint16 lineLength,frameHeight;	
	SENSORDB("IMX134MIPISetMaxFramerateByScenario: scenarioId = %d, frame rate = %d\n",scenarioId,frameRate);
	switch (scenarioId) {
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
			pclk = 232000000;//180000000;//200000000;//240000000;
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
static UINT32 IMX134MIPIGetDefaultFramerateByScenario(MSDK_SCENARIO_ID_ENUM scenarioId, MUINT32 *pframeRate) 
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
static UINT32 IMX134MIPISetTestPatternMode(kal_bool bEnable)
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

UINT32 IMX134LITEONMIPIFeatureControl(MSDK_SENSOR_FEATURE_ENUM FeatureId,
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

#ifdef IMX134MIPI_USE_OTP
    stCam_SENSOR_AF_OTP *pSensorAFOtp = (stCam_SENSOR_AF_OTP *)pFeaturePara;
    stCam_SENSOR_AWB_OTP *pSensorAWBOtp = (stCam_SENSOR_AWB_OTP *)pFeaturePara;
#endif

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
                pSensorDefaultData->SensorId=IMX134LITEON_SENSOR_ID;
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
        case SENSOR_FEATURE_SET_MAX_FRAME_RATE_BY_SCENARIO:
            IMX134MIPISetMaxFramerateByScenario((MSDK_SCENARIO_ID_ENUM)*pFeatureData32, *(pFeatureData32+1));
            break;
        case SENSOR_FEATURE_GET_DEFAULT_FRAME_RATE_BY_SCENARIO:
            IMX134MIPIGetDefaultFramerateByScenario((MSDK_SCENARIO_ID_ENUM)*pFeatureData32, (MUINT32 *)(*(pFeatureData32+1)));
            break;
#ifdef IMX134MIPI_USE_OTP
        case SENSOR_FEATURE_GET_AF_OTP:
            if (*pFeatureParaLen >= sizeof(stCam_SENSOR_AF_OTP))
            {
                //pass VCM values to hal only when data valid
                if (IMX134MIPI_OTP_ALL_READ == (IMX134MIPI_otp_read_flag & IMX134MIPI_OTP_ALL_READ))
                {
                    pSensorAFOtp->vcmStart  = gcurrent_otp.iVCM_Start;
                    pSensorAFOtp->vcmEnd  = gcurrent_otp.iVCM_End;
                }
                else
                {
                    SENSORDB("SENSOR_FEATURE_GET_AF_OTP OTP invalid,IMX134MIPI_otp_read_flag:0x%x\n", IMX134MIPI_otp_read_flag);
                    return ERROR_INVALID_PARA;
                }
            }
            else//Error data length
            {
                SENSORDB("SENSOR_FEATURE_GET_AF_OTP error length: %d\n", *pFeatureParaLen);
                return ERROR_OUT_OF_BUFFER_NUMBER;
            }
            break;
        case SENSOR_FEATURE_GET_AWB_OTP:
            if (*pFeatureParaLen >= sizeof(stCam_SENSOR_AWB_OTP))
            {
                //pass WB values to hal only when data valid
                if (IMX134MIPI_OTP_ALL_READ == (IMX134MIPI_otp_read_flag & IMX134MIPI_OTP_ALL_READ))
                {
                    pSensorAWBOtp->wbRG = gcurrent_otp.iWB_RG;
                    pSensorAWBOtp->wbBG = gcurrent_otp.iWB_BG;
                    pSensorAWBOtp->wbGbGr  = gcurrent_otp.iWB_GbGr;
                    pSensorAWBOtp->wbGoldenRG = RG_GOLDEN;
                    pSensorAWBOtp->wbGoldenBG = BG_GOLDEN;
                    pSensorAWBOtp->wbGoldenGbGr = GG_GOLDEN;
                }
                else
                {
                    SENSORDB("SENSOR_FEATURE_GET_AWB_OTP OTP invalid,IMX134MIPI_otp_read_flag:0x%x\n", IMX134MIPI_otp_read_flag);
                    return ERROR_INVALID_PARA;
                }
            }
            else//Error data length
            {
                SENSORDB("SENSOR_FEATURE_GET_AWB_OTP error length: %d\n", *pFeatureParaLen);
                return ERROR_OUT_OF_BUFFER_NUMBER;
            }
            break;
        case SENSOR_FEATURE_GET_OTP_FLAG:
             if (*pFeatureParaLen >= sizeof(kal_uint32))
            {
                *pFeatureData32 = IMX134MIPI_otp_read_flag;
            }
             else//Error data length
            {
                return ERROR_OUT_OF_BUFFER_NUMBER;
            }
            break;
#endif
        default:
            break;
    }
    return ERROR_NONE;
}	/* IMX134MIPIFeatureControl() */


static SENSOR_FUNCTION_STRUCT	SensorFuncIMX134MIPI=
{
    IMX134LITEONMIPIOpen,
    IMX134LITEONMIPIGetInfo,
    IMX134LITEONMIPIGetResolution,
    IMX134LITEONMIPIFeatureControl,
    IMX134LITEONMIPIControl,
    IMX134LITEONMIPIClose
};

UINT32 IMX134LITEON_MIPI_RAW_SensorInit(PSENSOR_FUNCTION_STRUCT *pfFunc)
{
    /* To Do : Check Sensor status here */
    if (pfFunc!=NULL)
        *pfFunc=&SensorFuncIMX134MIPI;
    return ERROR_NONE;
}   /* SensorInit() */

