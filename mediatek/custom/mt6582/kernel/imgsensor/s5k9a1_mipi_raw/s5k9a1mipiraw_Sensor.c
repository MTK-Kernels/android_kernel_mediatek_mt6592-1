/*******************************************************************************************/
   

/******************************************************************************************/

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

#include "kd_camera_hw.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"

#include "s5k9a1mipiraw_Sensor.h"
#include "s5k9a1mipiraw_Camera_Sensor_para.h"
#include "s5k9a1mipiraw_CameraCustomized.h"


static DEFINE_SPINLOCK(s5k9a1mipiraw_drv_lock);

#define S5K9A1_DEBUG
//#define S5K9A1_DEBUG_SOFIA

#ifdef S5K9A1_DEBUG
	#define S5K9A1DB(fmt, arg...) xlog_printk(ANDROID_LOG_DEBUG, "[S5K9A1Raw] ",  fmt, ##arg)
#else
	#define S5K9A1DB(fmt, arg...)
#endif

#ifdef S5K9A1_DEBUG_SOFIA
	#define S5K9A1DBSOFIA(fmt, arg...) xlog_printk(ANDROID_LOG_DEBUG, "[S5K9A1Raw] ",  fmt, ##arg)
#else
	#define S5K9A1DBSOFIA(fmt, arg...)
#endif

#define mDELAY(ms)  mdelay(ms)
#define BURST_WRITE_SUPPORT

kal_uint32 S5K9A1_FeatureControl_PERIOD_PixelNum=S5K9A1_PV_PERIOD_PIXEL_NUMS;
kal_uint32 S5K9A1_FeatureControl_PERIOD_LineNum=S5K9A1_PV_PERIOD_LINE_NUMS;

UINT16 S5K9A1_VIDEO_MODE_TARGET_FPS = 30;
static BOOL S5K9A1_ReEnteyCamera = KAL_FALSE;
kal_bool S5K9A1DuringTestPattern = KAL_FALSE;
#define S5K9A1_TEST_PATTERN_CHECKSUM (0x8da74951)


MSDK_SENSOR_CONFIG_STRUCT S5K9A1SensorConfigData;

kal_uint32 S5K9A1_FAC_SENSOR_REG;

MSDK_SCENARIO_ID_ENUM S5K9A1CurrentScenarioId = MSDK_SCENARIO_ID_CAMERA_PREVIEW;

/* FIXME: old factors and DIDNOT use now. s*/
SENSOR_REG_STRUCT S5K9A1SensorCCT[]=CAMERA_SENSOR_CCT_DEFAULT_VALUE;
SENSOR_REG_STRUCT S5K9A1SensorReg[ENGINEER_END]=CAMERA_SENSOR_REG_DEFAULT_VALUE;
/* FIXME: old factors and DIDNOT use now. e*/
struct S5K9A1_SENSOR_STRUCT S5K9A1_sensor=
{
    .i2c_write_id = 0x50,
    .i2c_read_id  = 0x51,

};

static S5K9A1_PARA_STRUCT S5K9A1;

#if 0
extern int iReadReg(u16 a_u2Addr , u8 * a_puBuff , u16 i2cId);
extern int iWriteReg(u16 a_u2Addr , u32 a_u4Data , u32 a_u4Bytes , u16 i2cId);
#define S5K9A1_write_cmos_sensor(addr, para) iWriteReg((u16) addr , (u32) para , 1, S5K9A1MIPI_WRITE_ID)

kal_uint16 S5K9A1_read_cmos_sensor(kal_uint32 addr)
{
	kal_uint16 get_byte=0;
    iReadReg((u16) addr ,(u8*)&get_byte,S5K9A1MIPI_WRITE_ID);
    return get_byte;
}
#endif

#define Sleep(ms) mdelay(ms)

extern int iWriteRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u16 i2cId);
extern int iReadRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u8 * a_pRecvData, u16 a_sizeRecvData, u16 i2cId);

inline void S5K9A1_write_cmos_sensor(u16 addr, u32 para)
{
	char puSendCmd[4] = {(char)(addr >> 8) , (char)(addr & 0xFF) ,  (char)(para >> 8),	(char)(para & 0xFF) };
	iWriteRegI2C(puSendCmd , 4, S5K9A1MIPI_WRITE_ID);
}



inline void S5K9A1_write_cmos_sensor1(u16 addr, u32 para)
{
	char puSendCmd[4] = {(char)(addr >> 8) , (char)(addr & 0xFF)  ,	(char)(para & 0xFF) };
	iWriteRegI2C(puSendCmd , 3, S5K9A1MIPI_WRITE_ID);
}



inline kal_uint16 S5K9A1_read_cmos_sensor(kal_uint32 addr)
{
	kal_uint16 get_byte=0;
	char puSendCmd[2] = {(char)(addr >> 8) , (char)(addr & 0xFF) };
	iReadRegI2C(puSendCmd , 2, (u8*)&get_byte,1,S5K9A1MIPI_WRITE_ID);
	return get_byte&0x00ff;
}
inline kal_uint16 S5K9A1_read_cmos_sensor_16(kal_uint32 addr)
{
	kal_uint16 get_byte=0;
	char puSendCmd[2] = {(char)(addr >> 8) , (char)(addr & 0xFF) };
	iReadRegI2C(puSendCmd , 2, (u8*)&get_byte,2,S5K9A1MIPI_WRITE_ID);
	return ((get_byte<<8)&0xff00)|((get_byte>>8)&0x00ff);
}

#ifdef BURST_WRITE_SUPPORT
extern int iBurstWriteReg(u8 *pData, u32 bytes, u16 i2cId);

#define I2C_BUFFER_LEN 254 


static kal_uint16 S5K9A1_burst_write_cmos_sensor(kal_uint16* para, kal_uint32 len)
{   
	char puSendCmd[I2C_BUFFER_LEN];
	kal_uint32 tosend = 0, IDX = 0;   
	kal_uint16 addr, addr_last, data;   

	while(IDX < len)   
	{       
		addr = para[IDX];       

		if(tosend == 0)    
		{           
			puSendCmd[tosend++] = (char)(addr >> 8);           
			puSendCmd[tosend++] = (char)(addr & 0xFF);           
			data = para[IDX+1];
			puSendCmd[tosend++] = (char)(data >>8 ); 
			puSendCmd[tosend++] = (char)(data & 0xFF);           
			IDX += 2;           
			addr_last = addr;       
		}       
		else if(addr == addr_last)     
		{           
			data = para[IDX+1];
			puSendCmd[tosend++] = (char)(data >>8 ); 
			puSendCmd[tosend++] = (char)(data & 0xFF);           
			IDX += 2;       
		}
		
		if (tosend == I2C_BUFFER_LEN || IDX == len || addr != addr_last)       
		{           
			iBurstWriteReg(puSendCmd , tosend, S5K9A1MIPI_WRITE_ID);           
			tosend = 0;       
		}   
	}   

	return 0;
}

#endif

/*******************************************************************************
*
********************************************************************************/
kal_uint16 read_S5K9A1MIPI_gain(void)
{

}
static kal_uint16 S5K9A1MIPIReg2Gain(const kal_uint8 iReg)
{
    kal_uint16 iGain;
    kal_uint16  Senosr_base=0x0100;
	S5K9A1_write_cmos_sensor(0x0028,0x7000);
    S5K9A1_write_cmos_sensor(0x002A,0x0104 );
	iGain=S5K9A1_read_cmos_sensor_16(0x0F12);
	iGain=(iGain*64)/Senosr_base;
	return iGain;
}
	
static kal_uint16 S5K9A1MIPIGain2Reg(kal_uint16 iGain)
{
	S5K9A1DB("[S5K9A1MIPIGain2Reg1] iGain is :%d \n", iGain);
    kal_uint16 Gain;
    kal_uint16  Senosr_base=0x0100;
    Gain=(iGain*Senosr_base)/64;
    S5K9A1_write_cmos_sensor(0x0028,0x7000);
    S5K9A1_write_cmos_sensor(0x002A,0x0104 );
    S5K9A1_write_cmos_sensor(0x0F12,Gain);

}

/*************************************************************************
* FUNCTION
*    S5K9A1MIPI_SetGain
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

void S5K9A1MIPI_SetGain(UINT16 iGain)
{
    //return;
	S5K9A1DB("[S5K9A1MIPI_SetGain] iGain is :%d \n ",iGain);
	S5K9A1MIPIGain2Reg(iGain);

}   /*  S5K9A1MIPI_SetGain  */

static void S5K9A1_SetDummy( const kal_uint32 iPixels, const kal_uint32 iLines )
{
    //return;
	kal_uint32 line_length = 0;
	kal_uint32 frame_length = 0;

	line_length = S5K9A1_PV_PERIOD_PIXEL_NUMS + iPixels;
	frame_length = S5K9A1_PV_PERIOD_LINE_NUMS + iLines;

	spin_lock(&s5k9a1mipiraw_drv_lock);
	S5K9A1_FeatureControl_PERIOD_PixelNum = line_length;
	S5K9A1_FeatureControl_PERIOD_LineNum = frame_length;
	spin_unlock(&s5k9a1mipiraw_drv_lock);

	//Set total line length
	S5K9A1_write_cmos_sensor(0x002A,0x010E);
	S5K9A1_write_cmos_sensor(0x0F12, frame_length); 

	S5K9A1DB("[S5K9A1MIPI_SetDummy] frame_length is :%d \n ",frame_length);
	S5K9A1DB("[S5K9A1MIPI_SetDummy] line_lengthis :%d \n ",line_length);

}   /*  S5K9A1_SetDummy */

static void S5K9A1_Sensor_Init(void)
{

S5K9A1DB("S5K9A1_Sensor_Init 1lane:\n ");	  

#ifdef BURST_WRITE_SUPPORT
			static const kal_uint16 addr_data_pair[] =
			{			   
			   0x0010  ,0x0001,    // Reset
			   0x1030  ,0x0000,    // Clear host interrupt so main will wait
			   0x0014  ,0x0001,    // ARM go
			   0x0028  ,0x7000,
			   0x002A  ,0x1074,
			   0x0F12  ,0xB510,
			   0x0F12  ,0x490C,
			   0x0F12  ,0x480C,
			   0x0F12  ,0xF000,
			   0x0F12  ,0xF9D1,
			   0x0F12  ,0x490C,
			   0x0F12  ,0x480C,
			   0x0F12  ,0xF000,
			   0x0F12  ,0xF9CD,
			   0x0F12  ,0x480C,
			   0x0F12  ,0x490C,
			   0x0F12  ,0x6688,
			   0x0F12  ,0x490C,
			   0x0F12  ,0x480D,
			   0x0F12  ,0xF000,
			   0x0F12  ,0xF9C6,
			   0x0F12  ,0x4809,
			   0x0F12  ,0x490C,
			   0x0F12  ,0x3080,
			   0x0F12  ,0x6001,
			   0x0F12  ,0x490B,
			   0x0F12  ,0x6041,
			   0x0F12  ,0xBC10,
			   0x0F12  ,0xBC08,
			   0x0F12  ,0x4718,
			   0x0F12  ,0x0000,
			   0x0F12  ,0x1374,
			   0x0F12  ,0x7000,
			   0x0F12  ,0x06EB,
			   0x0F12  ,0x0000,
			   0x0F12  ,0x12C0,
			   0x0F12  ,0x7000,
			   0x0F12  ,0x3DF7,
			   0x0F12  ,0x0000,
			   0x0F12  ,0x11C8,
			   0x0F12  ,0x7000,
			   0x0F12  ,0x0000,
			   0x0F12  ,0x7000,
			   0x0F12  ,0x1168,
			   0x0F12  ,0x7000,
			   0x0F12  ,0x0437,
			   0x0F12  ,0x0000,
			   0x0F12  ,0x10F4,
			   0x0F12  ,0x7000,
			   0x0F12  ,0x10D0,
			   0x0F12  ,0x7000,
			   0x0F12  ,0x4010,
			   0x0F12  ,0xE92D,
			   0x0F12  ,0x4320,
			   0x0F12  ,0xE59F,
			   0x0F12  ,0x0000,
			   0x0F12  ,0xE3A0,
			   0x0F12  ,0x02B4,
			   0x0F12  ,0xE1C4,
			   0x0F12  ,0x00D2,
			   0x0F12  ,0xEB00,
			   0x0F12  ,0x0001,
			   0x0F12  ,0xE3A0,
			   0x0F12  ,0x02B4,
			   0x0F12  ,0xE1C4,
			   0x0F12  ,0x4010,
			   0x0F12  ,0xE8BD,
			   0x0F12  ,0xFF1E,
			   0x0F12  ,0xE12F,
			   0x0F12  ,0x4010,
			   0x0F12  ,0xE92D,
			   0x0F12  ,0x00CF,
			   0x0F12  ,0xEB00,
			   0x0F12  ,0x12F8,
			   0x0F12  ,0xE59F,
			   0x0F12  ,0x0000,
			   0x0F12  ,0xE3A0,
			   0x0F12  ,0x02B4,
			   0x0F12  ,0xE1C1,
			   0x0F12  ,0x2000,
			   0x0F12  ,0xE3A0,
			   0x0F12  ,0x1B02,
			   0x0F12  ,0xE3A0,
			   0x0F12  ,0x0F85,
			   0x0F12  ,0xE3A0,
			   0x0F12  ,0x00CB,
			   0x0F12  ,0xEB00,
			   0x0F12  ,0x2000,
			   0x0F12  ,0xE3A0,
			   0x0F12  ,0x1080,
			   0x0F12  ,0xE3A0,
			   0x0F12  ,0x0F86,
			   0x0F12  ,0xE3A0,
			   0x0F12  ,0x00C7,
			   0x0F12  ,0xEB00,
			   0x0F12  ,0x2001,
			   0x0F12  ,0xE3A0,
			   0x0F12  ,0x1080,
			   0x0F12  ,0xE3A0,
			   0x0F12  ,0x0F85,
			   0x0F12  ,0xE3A0,
			   0x0F12  ,0x00C3,
			   0x0F12  ,0xEB00,
			   0x0F12  ,0x12C0,
			   0x0F12  ,0xE59F,
			   0x0F12  ,0x0001,
			   0x0F12  ,0xE3A0,
			   0x0F12  ,0x0AB2,
			   0x0F12  ,0xE1C1,
			   0x0F12  ,0x0000,
			   0x0F12  ,0xE3A0,
			   0x0F12  ,0x0001,
			   0x0F12  ,0xE280,
			   0x0F12  ,0x0064,
			   0x0F12  ,0xE350,
			   0x0F12  ,0xFFFC,
			   0x0F12  ,0x3AFF,
			   0x0F12  ,0x4010,
			   0x0F12  ,0xE8BD,
			   0x0F12  ,0x2001,
			   0x0F12  ,0xE3A0,
			   0x0F12  ,0x1080,
			   0x0F12  ,0xE3A0,
			   0x0F12  ,0x0F86,
			   0x0F12  ,0xE3A0,
			   0x0F12  ,0x00B7,
			   0x0F12  ,0xEA00,
			   0x0F12  ,0x4070,
			   0x0F12  ,0xE92D,
			   0x0F12  ,0x1000,
			   0x0F12  ,0xE590,
			   0x0F12  ,0x00FF,
			   0x0F12  ,0xE201,
			   0x0F12  ,0x10FF,
			   0x0F12  ,0xE201,
			   0x0F12  ,0x3284,
			   0x0F12  ,0xE59F,
			   0x0F12  ,0x2080,
			   0x0F12  ,0xE083,
			   0x0F12  ,0x49B2,
			   0x0F12  ,0xE1D2,
			   0x0F12  ,0x5DB2,
			   0x0F12  ,0xE1D2,
			   0x0F12  ,0x29B0,
			   0x0F12  ,0xE1D3,
			   0x0F12  ,0x6274,
			   0x0F12  ,0xE59F,
			   0x0F12  ,0x2001,
			   0x0F12  ,0xE202,
			   0x0F12  ,0x202C,
			   0x0F12  ,0xE5C6,
			   0x0F12  ,0x2000,
			   0x0F12  ,0xE3A0,
			   0x0F12  ,0x00AC,
			   0x0F12  ,0xEB00,
			   0x0F12  ,0x0902,
			   0x0F12  ,0xE314,
			   0x0F12  ,0x0001,
			   0x0F12  ,0x1A00,
			   0x0F12  ,0x0902,
			   0x0F12  ,0xE3C4,
			   0x0F12  ,0x02B8,
			   0x0F12  ,0xE1C6,
			   0x0F12  ,0x0902,
			   0x0F12  ,0xE315,
			   0x0F12  ,0x0001,
			   0x0F12  ,0x1A00,
			   0x0F12  ,0x0902,
			   0x0F12  ,0xE3C5,
			   0x0F12  ,0x02B2,
			   0x0F12  ,0xE1C6,
			   0x0F12  ,0x4070,
			   0x0F12  ,0xE8BD,
			   0x0F12  ,0xFF1E,
			   0x0F12  ,0xE12F,
			   0x0F12  ,0x5FFC,
			   0x0F12  ,0xE92D,
			   0x0F12  ,0xA000,
			   0x0F12  ,0xE1A0,
			   0x0F12  ,0x5234,
			   0x0F12  ,0xE59F,
			   0x0F12  ,0x01B0,
			   0x0F12  ,0xE1D5,
			   0x0F12  ,0xB000,
			   0x0F12  ,0xE1A0,
			   0x0F12  ,0x4220,
			   0x0F12  ,0xE59F,
			   0x0F12  ,0x3006,
			   0x0F12  ,0xE3A0,
			   0x0F12  ,0x2080,
			   0x0F12  ,0xE284,
			   0x0F12  ,0x1074,
			   0x0F12  ,0xE284,
			   0x0F12  ,0x009B,
			   0x0F12  ,0xEB00,
			   0x0F12  ,0x0004,
			   0x0F12  ,0xE58D,
			   0x0F12  ,0x606C,
			   0x0F12  ,0xE284,
			   0x0F12  ,0x7064,
			   0x0F12  ,0xE284,
			   0x0F12  ,0x009A,
			   0x0F12  ,0xEB00,
			   0x0F12  ,0x11B2,
			   0x0F12  ,0xE1D5,
			   0x0F12  ,0x1000,
			   0x0F12  ,0xE58D,
			   0x0F12  ,0x0000,
			   0x0F12  ,0xE260,
			   0x0F12  ,0x04B0,
			   0x0F12  ,0xE1CA,
			   0x0F12  ,0x5054,
			   0x0F12  ,0xE284,
			   0x0F12  ,0x805C,
			   0x0F12  ,0xE284,
			   0x0F12  ,0x4000,
			   0x0F12  ,0xE3A0,
			   0x0F12  ,0x11E4,
			   0x0F12  ,0xE59F,
			   0x0F12  ,0x002C,
			   0x0F12  ,0xE5D1,
			   0x0F12  ,0x0000,
			   0x0F12  ,0xE350,
			   0x0F12  ,0x0002,
			   0x0F12  ,0x0A00,
			   0x0F12  ,0x0084,
			   0x0F12  ,0xE081,
			   0x0F12  ,0x02B2,
			   0x0F12  ,0xE1D0,
			   0x0F12  ,0x0000,
			   0x0F12  ,0xEA00,
			   0x0F12  ,0x0A02,
			   0x0F12  ,0xE3A0,
			   0x0F12  ,0x20F2,
			   0x0F12  ,0xE0D6,
			   0x0F12  ,0x30F2,
			   0x0F12  ,0xE0D7,
			   0x0F12  ,0x10B0,
			   0x0F12  ,0xE1D1,
			   0x0F12  ,0x2193,
			   0x0F12  ,0xE021,
			   0x0F12  ,0x2004,
			   0x0F12  ,0xE59D,
			   0x0F12  ,0x1441,
			   0x0F12  ,0xE082,
			   0x0F12  ,0x20F2,
			   0x0F12  ,0xE0D5,
			   0x0F12  ,0x029B,
			   0x0F12  ,0xE002,
			   0x0F12  ,0x1422,
			   0x0F12  ,0xE081,
			   0x0F12  ,0x2084,
			   0x0F12  ,0xE08A,
			   0x0F12  ,0x12BC,
			   0x0F12  ,0xE1C2,
			   0x0F12  ,0x10B2,
			   0x0F12  ,0xE0D8,
			   0x0F12  ,0x0091,
			   0x0F12  ,0xE000,
			   0x0F12  ,0x16A0,
			   0x0F12  ,0xE1A0,
			   0x0F12  ,0x0000,
			   0x0F12  ,0xE59D,
			   0x0F12  ,0x0091,
			   0x0F12  ,0xE000,
			   0x0F12  ,0x9420,
			   0x0F12  ,0xE1A0,
			   0x0F12  ,0x0A02,
			   0x0F12  ,0xE359,
			   0x0F12  ,0x0001,
			   0x0F12  ,0x8A00,
			   0x0F12  ,0x0000,
			   0x0F12  ,0xE3A0,
			   0x0F12  ,0x0002,
			   0x0F12  ,0xEA00,
			   0x0F12  ,0x0009,
			   0x0F12  ,0xE1A0,
			   0x0F12  ,0x0077,
			   0x0F12  ,0xEB00,
			   0x0F12  ,0x000E,
			   0x0F12  ,0xE240,
			   0x0F12  ,0x1004,
			   0x0F12  ,0xE08A,
			   0x0F12  ,0x003C,
			   0x0F12  ,0xE5C1,
			   0x0F12  ,0x0164,
			   0x0F12  ,0xE59F,
			   0x0F12  ,0x9104,
			   0x0F12  ,0xE780,
			   0x0F12  ,0x4001,
			   0x0F12  ,0xE284,
			   0x0F12  ,0x0004,
			   0x0F12  ,0xE354,
			   0x0F12  ,0xFFD8,
			   0x0F12  ,0x3AFF,
			   0x0F12  ,0x5FFC,
			   0x0F12  ,0xE8BD,
			   0x0F12  ,0xFF1E,
			   0x0F12  ,0xE12F,
			   0x0F12  ,0x4070,
			   0x0F12  ,0xE92D,
			   0x0F12  ,0x5000,
			   0x0F12  ,0xE590,
			   0x0F12  ,0x4144,
			   0x0F12  ,0xE59F,
			   0x0F12  ,0x0005,
			   0x0F12  ,0xE1A0,
			   0x0F12  ,0x006B,
			   0x0F12  ,0xEB00,
			   0x0F12  ,0x0000,
			   0x0F12  ,0xE3A0,
			   0x0F12  ,0x2130,
			   0x0F12  ,0xE59F,
			   0x0F12  ,0x3000,
			   0x0F12  ,0xE085,
			   0x0F12  ,0x303C,
			   0x0F12  ,0xE5D3,
			   0x0F12  ,0x1100,
			   0x0F12  ,0xE792,
			   0x0F12  ,0x1331,
			   0x0F12  ,0xE1A0,
			   0x0F12  ,0x320D,
			   0x0F12  ,0xE284,
			   0x0F12  ,0x10B0,
			   0x0F12  ,0xE1C3,
			   0x0F12  ,0x0001,
			   0x0F12  ,0xE280,
			   0x0F12  ,0x4002,
			   0x0F12  ,0xE284,
			   0x0F12  ,0x0004,
			   0x0F12  ,0xE350,
			   0x0F12  ,0xFFF5,
			   0x0F12  ,0x3AFF,
			   0x0F12  ,0x4070,
			   0x0F12  ,0xE8BD,
			   0x0F12  ,0xFF1E,
			   0x0F12  ,0xE12F,
			   0x0F12  ,0x4010,
			   0x0F12  ,0xE92D,
			   0x0F12  ,0xE008,
			   0x0F12  ,0xE28D,
			   0x0F12  ,0x5000,
			   0x0F12  ,0xE89E,
			   0x0F12  ,0x4000,
			   0x0F12  ,0xE3A0,
			   0x0F12  ,0x40B0,
			   0x0F12  ,0xE1C0,
			   0x0F12  ,0x0000,
			   0x0F12  ,0xE352,
			   0x0F12  ,0x0000,
			   0x0F12  ,0x0A00,
			   0x0F12  ,0x1003,
			   0x0F12  ,0xE081,
			   0x0F12  ,0x0003,
			   0x0F12  ,0xE151,
			   0x0F12  ,0x000C,
			   0x0F12  ,0x9A00,
			   0x0F12  ,0x000E,
			   0x0F12  ,0xE15C,
			   0x0F12  ,0x000A,
			   0x0F12  ,0x2A00,
			   0x0F12  ,0x2003,
			   0x0F12  ,0xE041,
			   0x0F12  ,0x2802,
			   0x0F12  ,0xE1A0,
			   0x0F12  ,0x2822,
			   0x0F12  ,0xE1A0,
			   0x0F12  ,0x20B0,
			   0x0F12  ,0xE1C0,
			   0x0F12  ,0x200C,
			   0x0F12  ,0xE082,
			   0x0F12  ,0x000E,
			   0x0F12  ,0xE152,
			   0x0F12  ,0x0001,
			   0x0F12  ,0x9A00,
			   0x0F12  ,0x200C,
			   0x0F12  ,0xE04E,
			   0x0F12  ,0x20B0,
			   0x0F12  ,0xE1C0,
			   0x0F12  ,0x00B0,
			   0x0F12  ,0xE1D0,
			   0x0F12  ,0x1000,
			   0x0F12  ,0xE041,
			   0x0F12  ,0x4010,
			   0x0F12  ,0xE8BD,
			   0x0F12  ,0x00FF,
			   0x0F12  ,0xE201,
			   0x0F12  ,0xFF1E,
			   0x0F12  ,0xE12F,
			   0x0F12  ,0x41FC,
			   0x0F12  ,0xE92D,
			   0x0F12  ,0x5000,
			   0x0F12  ,0xE1A0,
			   0x0F12  ,0x0000,
			   0x0F12  ,0xE595,
			   0x0F12  ,0x6004,
			   0x0F12  ,0xE595,
			   0x0F12  ,0x4008,
			   0x0F12  ,0xE595,
			   0x0F12  ,0x300C,
			   0x0F12  ,0xE595,
			   0x0F12  ,0x2004,
			   0x0F12  ,0xE1A0,
			   0x0F12  ,0x1006,
			   0x0F12  ,0xE1A0,
			   0x0F12  ,0x003D,
			   0x0F12  ,0xEB00,
			   0x0F12  ,0x7000,
			   0x0F12  ,0xE1A0,
			   0x0F12  ,0x20B0,
			   0x0F12  ,0xE1D4,
			   0x0F12  ,0x3070,
			   0x0F12  ,0xE59F,
			   0x0F12  ,0x8070,
			   0x0F12  ,0xE59F,
			   0x0F12  ,0x000C,
			   0x0F12  ,0xE88D,
			   0x0F12  ,0x00B8,
			   0x0F12  ,0xE1D6,
			   0x0F12  ,0x33B8,
			   0x0F12  ,0xE1D8,
			   0x0F12  ,0x10B4,
			   0x0F12  ,0xE1D6,
			   0x0F12  ,0x20FF,
			   0x0F12  ,0xE200,
			   0x0F12  ,0x0004,
			   0x0F12  ,0xE284,
			   0x0F12  ,0xFFD1,
			   0x0F12  ,0xEBFF,
			   0x0F12  ,0x000C,
			   0x0F12  ,0xE5C4,
			   0x0F12  ,0x20B2,
			   0x0F12  ,0xE1D4,
			   0x0F12  ,0x3FB6,
			   0x0F12  ,0xE3A0,
			   0x0F12  ,0x000C,
			   0x0F12  ,0xE88D,
			   0x0F12  ,0x00B8,
			   0x0F12  ,0xE1D6,
			   0x0F12  ,0x33BA,
			   0x0F12  ,0xE1D8,
			   0x0F12  ,0x10B6,
			   0x0F12  ,0xE1D6,
			   0x0F12  ,0x20FF,
			   0x0F12  ,0xE200,
			   0x0F12  ,0x0006,
			   0x0F12  ,0xE284,
			   0x0F12  ,0xFFC7,
			   0x0F12  ,0xEBFF,
			   0x0F12  ,0x000D,
			   0x0F12  ,0xE5C4,
			   0x0F12  ,0x7000,
			   0x0F12  ,0xE585,
			   0x0F12  ,0x41FC,
			   0x0F12  ,0xE8BD,
			   0x0F12  ,0xFF1E,
			   0x0F12  ,0xE12F,
			   0x0F12  ,0x0200,
			   0x0F12  ,0xD000,
			   0x0F12  ,0xB000,
			   0x0F12  ,0xD000,
			   0x0F12  ,0x05AE,
			   0x0F12  ,0x7000,
			   0x0F12  ,0x0B08,
			   0x0F12  ,0x7000,
			   0x0F12  ,0x0B98,
			   0x0F12  ,0x7000,
			   0x0F12  ,0x14A0,
			   0x0F12  ,0x7000,
			   0x0F12  ,0x3408,
			   0x0F12  ,0x0000,
			   0x0F12  ,0x0508,
			   0x0F12  ,0x0000,
			   0x0F12  ,0x0188,
			   0x0F12  ,0x7000,
			   0x0F12  ,0x4778,
			   0x0F12  ,0x46C0,
			   0x0F12  ,0xC000,
			   0x0F12  ,0xE59F,
			   0x0F12  ,0xFF1C,
			   0x0F12  ,0xE12F,
			   0x0F12  ,0x461D,
			   0x0F12  ,0x0000,
			   0x0F12  ,0xC000,
			   0x0F12  ,0xE59F,
			   0x0F12  ,0xFF1C,
			   0x0F12  ,0xE12F,
			   0x0F12  ,0x2CD9,
			   0x0F12  ,0x0000,
			   0x0F12  ,0xC000,
			   0x0F12  ,0xE59F,
			   0x0F12  ,0xFF1C,
			   0x0F12  ,0xE12F,
			   0x0F12  ,0x2AA1,
			   0x0F12  ,0x0000,
			   0x0F12  ,0xC000,
			   0x0F12  ,0xE59F,
			   0x0F12  ,0xFF1C,
			   0x0F12  ,0xE12F,
			   0x0F12  ,0x1D07,
			   0x0F12  ,0x0000,
			   0x0F12  ,0xC000,
			   0x0F12  ,0xE59F,
			   0x0F12  ,0xFF1C,
			   0x0F12  ,0xE12F,
			   0x0F12  ,0x0437,
			   0x0F12  ,0x0000,
			   0x0F12  ,0xC000,
			   0x0F12  ,0xE59F,
			   0x0F12  ,0xFF1C,
			   0x0F12  ,0xE12F,
			   0x0F12  ,0x1C97,
			   0x0F12  ,0x0000,
			   0x0F12  ,0xC000,
			   0x0F12  ,0xE59F,
			   0x0F12  ,0xFF1C,
			   0x0F12  ,0xE12F,
			   0x0F12  ,0x3C9F,
			   0x0F12  ,0x0000,
			   0x0F12  ,0xC000,
			   0x0F12  ,0xE59F,
			   0x0F12  ,0xFF1C,
			   0x0F12  ,0xE12F,
			   0x0F12  ,0x1C21,
			   0x0F12  ,0x0000,
			   0x0F12  ,0xC000,
			   0x0F12  ,0xE59F,
			   0x0F12  ,0xFF1C,
			   0x0F12  ,0xE12F,
			   0x0F12  ,0x3DF7,
			   0x0F12  ,0x0000,
			   0x0F12  ,0xC000,
			   0x0F12  ,0xE59F,
			   0x0F12  ,0xFF1C,
			   0x0F12  ,0xE12F,
			   0x0F12  ,0x06EB,
			   0x0F12  ,0x0000,
			   0x0F12  ,0xA822,
			   0x0F12  ,0x0000, 	 
			   0x0028  ,0xD000,
			   0x061C  ,0x2ACC,    //trap index 6
			   0x061E  ,0x44C2,    //trap index 7
			   0x065C  ,0x0C6C,    //patch 6 (TrapAndPatchOpCodes array address save patch addresses)
			   0x065E  ,0x0C70,    //patch 7 (TrapAndPatchOpCodes array address save patch addresses)
			   0x002A  ,0x0C6C,
			   0x0F12  ,0x210C,    //set msb opcode
			   0x0F12  ,0x2A00,    //set lsb opcode
			   0x0F12  ,0x4832,    //set msb opcode
			   0x002A  ,0x0C6C,
			   0x0F12  ,0x210C,    //set lsb opcode
			   0x0600  ,0x00C0,    //tnp enable index 7 and 6
			   0x0028  ,0xD000,
			   0x002A  ,0x0218,
			   0x0F12  ,0x1B42,
			   0x002A  ,0x0214,
			   0x0F12  ,0x0030,
			   0x002A  ,0x3B0A,
			   0x0F12  ,0x2EFF,    
			   0x002A  ,0xF41C,
			   0x0F12  ,0x05FF,    
			   0x002A  ,0xF5DA,
			   0x0F12  ,0x0006,    
			   0x002A  ,0x370C,
			   0x0F12  ,0x0010,    
			   0x002A  ,0xF5C4,
			   0x0F12  ,0x0001,    
			   0x002A  ,0x0214,
			   0x0F12  ,0x0000,
			   0x002A  ,0x0218,
			   0x0F12  ,0x1B72,
			   0x0028  ,0x7000,
			   0x002A  ,0x0574,
			   0x0F12  ,0x00A0,    
			   0x002A  ,0x057A,
			   0x0F12  ,0x00A0,    
			   0x002A  ,0x0192,
			   0x0F12  ,0x0016,    
			   0x002A  ,0x014A,
			   0x0F12  ,0x0001,    
			   0x002A  ,0x0188,
			   0x0F12  ,0x0000,    
			   0x002A  ,0x0196,
			   0x0F12  ,0x1476,    
			   0x002A  ,0x0540,
			   0x0F12  ,0x0009,    
			   0x002A  ,0x01CC,
			   0x0F12  ,0x0005,    
			   0x0F12  ,0x0005,
			   0x0F12  ,0x05DD,    
			   0x0F12  ,0x07CF,
			   0x0F12  ,0x0001,    
			   0x0F12  ,0x0001,
			   0x0F12  ,0x05DD,    
			   0x0F12  ,0x03E5,
			   0x0F12  ,0x0001,    
			   0x0F12  ,0x03EB,
			   0x0F12  ,0x05DD,    
			   0x0F12  ,0x07CF,
			   0x0F12  ,0x0007,    
			   0x0F12  ,0x0007,
			   0x0F12  ,0x05DA,    
			   0x0F12  ,0x03E2,
			   0x0F12  ,0x0007,    
			   0x0F12  ,0x03F1,
			   0x0F12  ,0x05DA,    
			   0x0F12  ,0x07CC,
			   0x0F12  ,0x0188,    
			   0x0F12  ,0x0166,
			   0x0F12  ,0x01D3,    
			   0x0F12  ,0x01CF,
			   0x0F12  ,0x0188,    
			   0x0F12  ,0x0550,
			   0x0F12  ,0x01D3,    
			   0x0F12  ,0x05B9,
			   0x0F12  ,0x000B,    
			   0x0F12  ,0x000B,
			   0x0F12  ,0x05E2,    
			   0x0F12  ,0x03EA,
			   0x0F12  ,0x000B,    
			   0x0F12  ,0x03F5,
			   0x0F12  ,0x05E2,    
			   0x0F12  ,0x07D4,
			   0x0F12  ,0x0055,    
			   0x0F12  ,0x0446,
			   0x0F12  ,0x0003,    
			   0x0F12  ,0x03ED,
			   0x0F12  ,0x0055,    
			   0x0F12  ,0x005C,
			   0x0F12  ,0x0003,    
			   0x0F12  ,0x0003,
			   0x0F12  ,0x009B,    
			   0x0F12  ,0x0493,
			   0x0F12  ,0x0075,    
			   0x0F12  ,0x0461,
			   0x0F12  ,0x004F,    
			   0x0F12  ,0x0443,
			   0x0F12  ,0x0006,    
			   0x0F12  ,0x03F0,
			   0x0F12  ,0x009B,    
			   0x0F12  ,0x00A9,
			   0x0F12  ,0x0075,    
			   0x0F12  ,0x0077,
			   0x0F12  ,0x004F,    
			   0x0F12  ,0x0059,
			   0x0F12  ,0x0006,    
			   0x0F12  ,0x0006,
			   0x0F12  ,0x009E,    
			   0x0F12  ,0x0496,
			   0x0F12  ,0x0072,    
			   0x0F12  ,0x045E,
			   0x0F12  ,0x009E,    
			   0x0F12  ,0x00AC,
			   0x0F12  ,0x0072,    
			   0x0F12  ,0x0074,
			   0x0F12  ,0x0053,    
			   0x0F12  ,0x0447,
			   0x0F12  ,0x0002,    
			   0x0F12  ,0x03EC,
			   0x0F12  ,0x0053,    
			   0x0F12  ,0x005D,
			   0x0F12  ,0x0002,    
			   0x0F12  ,0x0002,
			   0x0F12  ,0x000C,    
			   0x0F12  ,0x000C,
			   0x0F12  ,0x0182,    
			   0x0F12  ,0x0160,
			   0x0F12  ,0x01DF,    
			   0x0F12  ,0x01DB,
			   0x0F12  ,0x05D8,    
			   0x0F12  ,0x03E2,
			   0x0F12  ,0x0000,    
			   0x0F12  ,0x03F6,
			   0x0F12  ,0x0000,    
			   0x0F12  ,0x054A,
			   0x0F12  ,0x0000,    
			   0x0F12  ,0x05C5,
			   0x0F12  ,0x0000,    
			   0x0F12  ,0x07CC,
			   0x0F12  ,0x000C,    
			   0x0F12  ,0x000C,
			   0x0F12  ,0x005E,    
			   0x0F12  ,0x005E,
			   0x0F12  ,0x0000,    
			   0x0F12  ,0x03F6,
			   0x0F12  ,0x0000,    
			   0x0F12  ,0x0448,
			   0x0F12  ,0x000C,    
			   0x0F12  ,0x000C,
			   0x0F12  ,0x0062,    
			   0x0F12  ,0x0062,
			   0x0F12  ,0x0000,    
			   0x0F12  ,0x03F6,
			   0x0F12  ,0x0000,    
			   0x0F12  ,0x044C,
			   0x0F12  ,0x0001,    
			   0x0F12  ,0x0001,
			   0x0F12  ,0x0185,    
			   0x0F12  ,0x0163,
			   0x0F12  ,0x0000,    
			   0x0F12  ,0x03EB,
			   0x0F12  ,0x0000,    
			   0x0F12  ,0x054D,
			   0x0F12  ,0x009D,    
			   0x0F12  ,0x00F7,
			   0x0F12  ,0x0182,    
			   0x0F12  ,0x0160,
			   0x0F12  ,0x0253,    
			   0x0F12  ,0x0227,
			   0x0F12  ,0x05D8,    
			   0x0F12  ,0x03E0,
			   0x0F12  ,0x0000,    
			   0x0F12  ,0x04E1,
			   0x0F12  ,0x0000,    
			   0x0F12  ,0x054A,
			   0x0F12  ,0x0000,    
			   0x0F12  ,0x0611,
			   0x0F12  ,0x0000,    
			   0x0F12  ,0x07CA,
			   0x0F12  ,0x010F,    
			   0x0F12  ,0x012B,
			   0x0F12  ,0x0185,    
			   0x0F12  ,0x0163,
			   0x0F12  ,0x0415,    
			   0x0F12  ,0x0303,
			   0x0F12  ,0x05DB,    
			   0x0F12  ,0x03E3,
			   0x0F12  ,0x0000,    
			   0x0F12  ,0x0515,
			   0x0F12  ,0x0000,    
			   0x0F12  ,0x054D,
			   0x0F12  ,0x0000,    
			   0x0F12  ,0x06ED,
			   0x0F12  ,0x0000,    
			   0x0F12  ,0x07CD,
			   0x0F12  ,0x018A,    
			   0x0F12  ,0x0168,
			   0x0F12  ,0x01D9,    
			   0x0F12  ,0x01D5,
			   0x0F12  ,0x0000,    
			   0x0F12  ,0x0552,
			   0x0F12  ,0x0000,    
			   0x0F12  ,0x05BF,
			   0x0F12  ,0x0000,    
			   0x0F12  ,0x0000,
			   0x0F12  ,0x0000,    
			   0x0F12  ,0x0000,
			   0x0F12  ,0x0000,    
			   0x0F12  ,0x0000,
			   0x0F12  ,0x0000,    
			   0x0F12  ,0x0000,
			   0x0F12  ,0x0073,    
			   0x0F12  ,0x0073,
			   0x0F12  ,0x05DB,    
			   0x0F12  ,0x03E3,
			   0x0F12  ,0x0000,    
			   0x0F12  ,0x045D,
			   0x0F12  ,0x0000,    
			   0x0F12  ,0x07CD,
			   0x0F12  ,0x0221,    
			   0x0F12  ,0x01C2,
			   0x0F12  ,0x05DB,    
			   0x0F12  ,0x03E3,
			   0x0F12  ,0x0000,    
			   0x0F12  ,0x05AC,
			   0x0F12  ,0x0000,    
			   0x0F12  ,0x07CD,
			   0x0F12  ,0x009F,    
			   0x0F12  ,0x00F9,
			   0x0F12  ,0x017F,    
			   0x0F12  ,0x015D,
			   0x0F12  ,0x0255,    
			   0x0F12  ,0x0229,
			   0x0F12  ,0x05D5,    
			   0x0F12  ,0x03DD,
			   0x0F12  ,0x0000,    
			   0x0F12  ,0x04E3,
			   0x0F12  ,0x0000,    
			   0x0F12  ,0x0547,
			   0x0F12  ,0x0000,    
			   0x0F12  ,0x0613,
			   0x0F12  ,0x0000,    
			   0x0F12  ,0x07C7,
			   0x0F12  ,0x009F,    
			   0x0F12  ,0x00F9,
			   0x0F12  ,0x0147,    
			   0x0F12  ,0x0144,
			   0x0F12  ,0x0255,    
			   0x0F12  ,0x0229,
			   0x0F12  ,0x059D,    
			   0x0F12  ,0x03C4,
			   0x0F12  ,0x0000,    
			   0x0F12  ,0x04E3,
			   0x0F12  ,0x0000,    
			   0x0F12  ,0x052E,
			   0x0F12  ,0x0000,    
			   0x0F12  ,0x0613,
			   0x0F12  ,0x0000,    
			   0x0F12  ,0x07AE,
			   0x0F12  ,0x009D,    
			   0x0F12  ,0x00F7,
			   0x0F12  ,0x0182,    
			   0x0F12  ,0x0160,
			   0x0F12  ,0x0000,    
			   0x0F12  ,0x0000,
			   0x0F12  ,0x0000,    
			   0x0F12  ,0x0000,
			   0x0F12  ,0x0253,    
			   0x0F12  ,0x0227,
			   0x0F12  ,0x05D8,    
			   0x0F12  ,0x03E0,
			   0x0F12  ,0x0000,    
			   0x0F12  ,0x04E1,
			   0x0F12  ,0x0000,    
			   0x0F12  ,0x054A,
			   0x0F12  ,0x0000,    
			   0x0F12  ,0x0611,
			   0x0F12  ,0x0000,    
			   0x0F12  ,0x07CA,
			   0x0F12  ,0x0182,    
			   0x0F12  ,0x0160,
			   0x0F12  ,0x0243,    
			   0x0F12  ,0x0207,
			   0x0F12  ,0x05D8,    
			   0x0F12  ,0x054A,
			   0x0F12  ,0x0000,    
			   0x0F12  ,0x05F1,
			   0x0F12  ,0x0000,    
			   0x0F12  ,0x07CA,
			   0x0F12  ,0x0185,    
			   0x0F12  ,0x0163,
			   0x0F12  ,0x018D,    
			   0x0F12  ,0x0173,
			   0x0F12  ,0x0000,    
			   0x0F12  ,0x03E3,
			   0x0F12  ,0x0000,    
			   0x0F12  ,0x03F3,
			   0x0F12  ,0x0000,    
			   0x0F12  ,0x054D,
			   0x0F12  ,0x0000,    
			   0x0F12  ,0x055D,
			   0x0F12  ,0x0189,    
			   0x0F12  ,0x016B,
			   0x0F12  ,0x0191,    
			   0x0F12  ,0x017B,
			   0x0F12  ,0x0000,    
			   0x0F12  ,0x03EB,
			   0x0F12  ,0x0000,    
			   0x0F12  ,0x03FB,
			   0x0F12  ,0x0000,    
			   0x0F12  ,0x0555,
			   0x0F12  ,0x0000,    
			   0x0F12  ,0x0565,
			   0x0F12  ,0x0002,    
			   0x0F12  ,0x0002,
			   0x0F12  ,0x0010,    
			   0x0F12  ,0x0010,
			   0x0F12  ,0x014A,    
			   0x0F12  ,0x0147,
			   0x0F12  ,0x0185,    
			   0x0F12  ,0x0163,
			   0x0F12  ,0x05A0,    
			   0x0F12  ,0x03C7,
			   0x0F12  ,0x05DB,    
			   0x0F12  ,0x03E3,
			   0x0F12  ,0x0000,    
			   0x0F12  ,0x0531,
			   0x0F12  ,0x0000,    
			   0x0F12  ,0x054D,
			   0x0F12  ,0x0000,    
			   0x0F12  ,0x07B1,
			   0x0F12  ,0x0000,    
			   0x0F12  ,0x07CD,
			   0x0F12  ,0x0008,    
			   0x0F12  ,0x000C,
			   0x0F12  ,0x0004,    
			   0x0F12  ,0x0008,
			   0x0F12  ,0x0000,    
			   0x0F12  ,0x0000,
			   0x0F12  ,0x0000,    
			   0x0F12  ,0x0000,
			   0x0F12  ,0x0000,    
			   0x0F12  ,0x0000,
			   0x0F12  ,0x0000,    
			   0x0F12  ,0x0000,
			   0x0F12  ,0x0000,    
			   0x0F12  ,0x0000,
			   0x0F12  ,0x0000,    
			   0x0F12  ,0x0000,
			   0x0F12  ,0x0000,    
			   0x0F12  ,0x0000,
			   0x0F12  ,0x0000,    
			   0x0F12  ,0x0000,
			   0x0F12  ,0x0000,    
			   0x0F12  ,0x0000,
			   0x0F12  ,0x0000,    
			   0x0F12  ,0x0000,
			   0x0F12  ,0x0000,    
			   0x0F12  ,0x0000,
			   0x0F12  ,0x0000,    
			   0x0F12  ,0x0000,
			   0x0F12  ,0x0000,    
			   0x0F12  ,0x0000,
			   0x0F12  ,0x0000,    
			   0x0F12  ,0x0000,
			   0x0F12  ,0x0000,    
			   0x0F12  ,0x0000,
			   0x0F12  ,0x0000,    
			   0x0F12  ,0x0000,
			   0x0F12  ,0x05E8,    
			   0x0F12  ,0x07DA,
			   0x0F12  ,0x0673,    
			   0x0F12  ,0x0873,
			   0x0F12  ,0x0000,    
			   0x0F12  ,0x0000,
			   0x0F12  ,0x0000,    
			   0x0F12  ,0x0000,
			   0x0F12  ,0x0000,    
			   0x0F12  ,0x0000,
			   0x0F12  ,0x021B,    
			   0x0F12  ,0x05F1,
			   0x0F12  ,0x0253,    
			   0x0F12  ,0x060D,
			   0x0F12  ,0x0673,    
			   0x0F12  ,0x0873,
			   0x0F12  ,0x0000,
			   0x0F12  ,0x0000, 									  
			   0x002A  ,0x070A,
			   0x0F12  ,0x0BB8,    // d3000
			   0x0F12  ,0x1770,    // d6000
			   0x002A  ,0x05AE,
			   0x0F12  ,0x0001,
			   0x002A  ,0x063A,
			   0x0F12  ,0x0080,    //128
			   0x002A  ,0x05D0,
			   0x0F12  ,0x0100,
			   0x002A  ,0x063E,
			   0x0F12  ,0x0001,
			   0x0F12  ,0xA000,    
			   0x0F12  ,0xA122,    
			   0x0F12  ,0xA122,    
			   0x0F12  ,0xA122,    
			   0x0F12  ,0xA122,    
			   0x0F12  ,0xA128,    
			   0x0F12  ,0xA08E,    
			   0x0F12  ,0xA08E,    
			   0x0F12  ,0xA08E,    
			   0x0F12  ,0xA08E,    
			   0x0F12  ,0xA092,    
			   0x0F12  ,0xA000,    
			   0x0F12  ,0xA000,    
			   0x0F12  ,0xA000,    
			   0x0F12  ,0xA000,    
			   0x0F12  ,0xA000,    
			   0x0F12  ,0x208E,    
			   0x0F12  ,0x208E,    
			   0x0F12  ,0x208E,    
			   0x0F12  ,0x208E,    
			   0x0F12  ,0x2091,    
			   0x0F12  ,0x2094,    
			   0x0F12  ,0x2121,    
			   0x0F12  ,0x2121,    
			   0x0F12  ,0x2121,    
			   0x0F12  ,0x2123,    
			   0x0F12  ,0x212A,    
			   0x0F12  ,0x21B8,    
			   0x0F12  ,0x21B8,    
			   0x0F12  ,0x21B8,    
			   0x0F12  ,0x21B8,    
			   0x0F12  ,0xA000,    
			   0x0F12  ,0xA1A6,    
			   0x0F12  ,0xA558,    
			   0x0F12  ,0xA36E,    
			   0x0F12  ,0xA1A6,    
			   0x0F12  ,0xA000,    
			   0x0F12  ,0x21A6,    
			   0x0F12  ,0xA558,    
			   0x0F12  ,0xA36E,    
			   0x0F12  ,0xA1A6,    
			   0x0F12  ,0xA000,    
			   0x0F12  ,0x21A6,    
			   0x0F12  ,0xA558,    
			   0x0F12  ,0xA36E,    
			   0x0F12  ,0xA000,    
			   0x0F12  ,0x21A6,    
			   0x0F12  ,0x236D,    
			   0x0F12  ,0xA564,    
			   0x0F12  ,0xA375,    
			   0x0F12  ,0xA1AA,    
			   0x0F12  ,0xA000,    
			   0x0F12  ,0x21A6,    
			   0x0F12  ,0x236D,    
			   0x0F12  ,0xA37D,    
			   0x0F12  ,0xA1AD,    
			   0x0F12  ,0xA000,    
			   0x0F12  ,0x21A6,    
			   0x0F12  ,0x236D,    
			   0x0F12  ,0xA384,    
			   0x0F12  ,0xA1B1,    
			   0x0F12  ,0xA000,    
			   0x0F12  ,0x21A7,    
			   0x0F12  ,0xA000,    
			   0x002A  ,0x0090,
			   0x0F12  ,0x5DC0,    //24MHz
			   0x0F12  ,0x0000,
			   0x002A  ,0x00A4,
			   0x0F12  ,0x0004,
			   0x0F12  ,0x0064,    //100 => VCO = 24/4*100 = 600MHz
			   0x0F12  ,0x0001,
			   0x0F12  ,0x000A,    //Sysclk = 60MHz
			   0x0F12  ,0x0001,    //MIPI OIF clk = 600MHz
			   0x0F12  ,0x0008,
			   0x002A  ,0x0102,
			   0x0F12  ,0x0001,
			   0x0F12  ,0x0100,
			   0x0F12  ,0x0001,
			   0x0F12  ,0x0100,
			   0x0F12  ,0x0001,
			   0x002A  ,0x00E8,
			   0x0F12  ,0x0001,    //REG_0TC_PCFG_Cfg_Input_bUseRelativeSizes
			   0x002A  ,0x00E0,
			   0x0F12  ,0x0508,    //REG_0TC_PCFG_Cfg_Input_Sizes_usWidth  1288d
			   0x0F12  ,0x02D8,    //REG_0TC_PCFG_Cfg_Input_Sizes_usHeight 0728d
			   0x002A  ,0x00F0,
			   0x0F12  ,0x014D,    //REG_0TC_PCFG_usMaxFrTimeMsecMult10
			   0x002A  ,0x00EA,
			   0x0F12  ,0x0003,    //REG_0TC_PCFG_usFrTimeType
			   0x002A  ,0x0102,
			   0x0F12  ,0x0002,    //REG_SF_USER_ExposureChanged
			   0x002A  ,0x00F2,
			   0x0F12  ,0x07D0,    //REG_0TC_PCFG_FrTimingCols	 <== Line_length   
			   0x0F12  ,0x03F0,    //REG_0TC_PCFG_FrTimingLines  <== Frame_length in ints (30fps)
			   0x002A  ,0x00DE,
			   0x0F12  ,0x0000,    //REG_0TC_PCFG_FrTimingLines
			   0x002A  ,0x010E,
			   0x0F12  ,0x03F0,    //REG_SF_USER_CIS_FrTimeLines  <== Frame_length for user
			   0x002A  ,0x00FE, 		  
			   0x0F12  ,0x03E8,    //REG_SF_USER_Exposure	   <==Fine_integration for user
			   0x0F12  ,0x0300,    //REG_SF_USER_ExposureHigh  <==Coarse_integration for user
			   0x002A  ,0x00F6,    //REG_0TC_PCFG_uPrevMirror
			   0x0F12  ,0x0000,    //Mirror funciton : 0 = Standard 	   GRBG    
			   0x002A  ,0x00A0,
			   0x0F12  ,0x0000,
			   0x0F12  ,0x0001,
			   0x002A  ,0x00CC,
			   0x0F12  ,0x0001,
			   0x0F12  ,0x0001,
			   0x002A  ,0x00C8,
			   0x0F12  ,0x0001,
			   0x0028  ,0xD000,
			   0x002A  ,0x1000,
			   0x0F12  ,0x0001,    // Host interrupt
			   0x0028  ,0x7000,
			   0x002A  ,0x0588,
			   0x0F12  ,0x0000,
            };
       S5K9A1_burst_write_cmos_sensor(addr_data_pair, sizeof(addr_data_pair)/sizeof(kal_uint16));

#else
	
	  //Global	setting
	  S5K9A1_write_cmos_sensor(0x0010, 0x0001);   // Reset
	  S5K9A1_write_cmos_sensor(0x1030, 0x0000);   // Clear host interrupt so main will wait
	  S5K9A1_write_cmos_sensor(0x0014, 0x0001);   // ARM go
	  
	  mDELAY(1);
	  S5K9A1_write_cmos_sensor(0x0028, 0x7000);
	  S5K9A1_write_cmos_sensor(0x002A, 0x1074);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xB510);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x490C);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x480C);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xF000);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xF9D1);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x490C);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x480C);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xF000);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xF9CD);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x480C);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x490C);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x6688);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x490C);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x480D);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xF000);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xF9C6);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x4809);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x490C);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x3080);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x6001);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x490B);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x6041);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xBC10);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xBC08);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x4718);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x0000);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x1374);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x7000);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x06EB);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x0000);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x12C0);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x7000);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x3DF7);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x0000);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x11C8);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x7000);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x0000);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x7000);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x1168);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x7000);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x0437);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x0000);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x10F4);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x7000);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x10D0);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x7000);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x4010);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE92D);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x4320);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE59F);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x0000);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE3A0);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x02B4);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE1C4);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x00D2);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xEB00);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x0001);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE3A0);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x02B4);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE1C4);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x4010);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE8BD);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xFF1E);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE12F);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x4010);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE92D);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x00CF);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xEB00);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x12F8);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE59F);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x0000);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE3A0);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x02B4);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE1C1);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x2000);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE3A0);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x1B02);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE3A0);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x0F85);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE3A0);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x00CB);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xEB00);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x2000);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE3A0);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x1080);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE3A0);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x0F86);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE3A0);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x00C7);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xEB00);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x2001);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE3A0);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x1080);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE3A0);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x0F85);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE3A0);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x00C3);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xEB00);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x12C0);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE59F);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x0001);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE3A0);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x0AB2);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE1C1);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x0000);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE3A0);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x0001);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE280);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x0064);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE350);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xFFFC);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x3AFF);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x4010);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE8BD);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x2001);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE3A0);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x1080);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE3A0);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x0F86);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE3A0);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x00B7);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xEA00);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x4070);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE92D);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x1000);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE590);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x00FF);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE201);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x10FF);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE201);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x3284);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE59F);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x2080);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE083);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x49B2);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE1D2);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x5DB2);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE1D2);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x29B0);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE1D3);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x6274);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE59F);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x2001);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE202);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x202C);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE5C6);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x2000);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE3A0);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x00AC);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xEB00);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x0902);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE314);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x0001);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x1A00);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x0902);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE3C4);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x02B8);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE1C6);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x0902);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE315);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x0001);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x1A00);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x0902);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE3C5);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x02B2);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE1C6);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x4070);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE8BD);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xFF1E);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE12F);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x5FFC);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE92D);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xA000);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE1A0);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x5234);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE59F);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x01B0);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE1D5);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xB000);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE1A0);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x4220);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE59F);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x3006);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE3A0);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x2080);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE284);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x1074);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE284);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x009B);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xEB00);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x0004);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE58D);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x606C);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE284);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x7064);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE284);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x009A);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xEB00);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x11B2);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE1D5);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x1000);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE58D);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x0000);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE260);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x04B0);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE1CA);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x5054);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE284);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x805C);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE284);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x4000);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE3A0);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x11E4);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE59F);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x002C);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE5D1);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x0000);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE350);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x0002);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x0A00);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x0084);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE081);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x02B2);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE1D0);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x0000);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xEA00);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x0A02);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE3A0);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x20F2);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE0D6);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x30F2);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE0D7);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x10B0);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE1D1);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x2193);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE021);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x2004);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE59D);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x1441);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE082);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x20F2);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE0D5);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x029B);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE002);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x1422);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE081);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x2084);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE08A);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x12BC);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE1C2);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x10B2);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE0D8);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x0091);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE000);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x16A0);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE1A0);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x0000);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE59D);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x0091);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE000);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x9420);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE1A0);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x0A02);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE359);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x0001);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x8A00);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x0000);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE3A0);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x0002);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xEA00);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x0009);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE1A0);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x0077);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xEB00);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x000E);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE240);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x1004);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE08A);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x003C);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE5C1);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x0164);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE59F);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x9104);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE780);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x4001);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE284);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x0004);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE354);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xFFD8);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x3AFF);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x5FFC);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE8BD);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xFF1E);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE12F);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x4070);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE92D);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x5000);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE590);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x4144);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE59F);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x0005);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE1A0);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x006B);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xEB00);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x0000);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE3A0);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x2130);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE59F);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x3000);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE085);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x303C);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE5D3);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x1100);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE792);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x1331);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE1A0);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x320D);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE284);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x10B0);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE1C3);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x0001);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE280);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x4002);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE284);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x0004);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE350);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xFFF5);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x3AFF);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x4070);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE8BD);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xFF1E);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE12F);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x4010);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE92D);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE008);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE28D);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x5000);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE89E);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x4000);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE3A0);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x40B0);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE1C0);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x0000);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE352);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x0000);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x0A00);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x1003);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE081);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x0003);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE151);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x000C);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x9A00);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x000E);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE15C);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x000A);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x2A00);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x2003);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE041);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x2802);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE1A0);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x2822);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE1A0);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x20B0);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE1C0);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x200C);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE082);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x000E);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE152);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x0001);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x9A00);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x200C);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE04E);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x20B0);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE1C0);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x00B0);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE1D0);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x1000);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE041);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x4010);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE8BD);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x00FF);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE201);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xFF1E);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE12F);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x41FC);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE92D);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x5000);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE1A0);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x0000);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE595);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x6004);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE595);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x4008);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE595);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x300C);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE595);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x2004);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE1A0);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x1006);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE1A0);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x003D);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xEB00);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x7000);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE1A0);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x20B0);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE1D4);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x3070);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE59F);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x8070);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE59F);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x000C);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE88D);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x00B8);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE1D6);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x33B8);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE1D8);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x10B4);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE1D6);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x20FF);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE200);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x0004);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE284);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xFFD1);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xEBFF);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x000C);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE5C4);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x20B2);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE1D4);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x3FB6);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE3A0);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x000C);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE88D);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x00B8);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE1D6);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x33BA);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE1D8);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x10B6);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE1D6);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x20FF);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE200);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x0006);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE284);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xFFC7);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xEBFF);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x000D);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE5C4);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x7000);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE585);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x41FC);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE8BD);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xFF1E);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE12F);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x0200);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xD000);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xB000);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xD000);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x05AE);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x7000);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x0B08);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x7000);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x0B98);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x7000);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x14A0);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x7000);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x3408);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x0000);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x0508);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x0000);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x0188);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x7000);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x4778);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x46C0);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xC000);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE59F);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xFF1C);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE12F);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x461D);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x0000);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xC000);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE59F);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xFF1C);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE12F);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x2CD9);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x0000);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xC000);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE59F);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xFF1C);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE12F);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x2AA1);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x0000);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xC000);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE59F);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xFF1C);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE12F);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x1D07);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x0000);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xC000);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE59F);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xFF1C);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE12F);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x0437);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x0000);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xC000);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE59F);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xFF1C);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE12F);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x1C97);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x0000);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xC000);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE59F);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xFF1C);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE12F);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x3C9F);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x0000);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xC000);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE59F);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xFF1C);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE12F);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x1C21);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x0000);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xC000);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE59F);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xFF1C);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE12F);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x3DF7);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x0000);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xC000);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE59F);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xFF1C);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xE12F);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x06EB);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x0000);
	  S5K9A1_write_cmos_sensor(0x0F12, 0xA822);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x0000);
	  
	  // End T&P part		 
	  S5K9A1_write_cmos_sensor(0x0028, 0xD000);
	  S5K9A1_write_cmos_sensor(0x061C, 0x2ACC);   //trap index 6
	  S5K9A1_write_cmos_sensor(0x061E, 0x44C2);   //trap index 7
	  S5K9A1_write_cmos_sensor(0x065C, 0x0C6C);   //patch 6 (TrapAndPatchOpCodes array address save patch addresses)
	  S5K9A1_write_cmos_sensor(0x065E, 0x0C70);   //patch 7 (TrapAndPatchOpCodes array address save patch addresses)
	  S5K9A1_write_cmos_sensor(0x002A, 0x0C6C);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x210C);   //set msb opcode
	  S5K9A1_write_cmos_sensor(0x0F12, 0x2A00);   //set lsb opcode
	  S5K9A1_write_cmos_sensor(0x0F12, 0x4832);   //set msb opcode
	  S5K9A1_write_cmos_sensor(0x002A, 0x0C6C);
	  S5K9A1_write_cmos_sensor(0x0F12, 0x210C);   //set lsb opcode
	  S5K9A1_write_cmos_sensor(0x0600, 0x00C0);   //tnp enable index 7 and 6
	  
	  //============================================================
	  // Analog settings
	  //============================================================
	  
	  //Hardware registers
	  S5K9A1_write_cmos_sensor(0x0028,0xD000);
	  S5K9A1_write_cmos_sensor(0x002A,0x0218);
	  S5K9A1_write_cmos_sensor(0x0F12,0x1B42);
	  S5K9A1_write_cmos_sensor(0x002A,0x0214);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0030);
	  S5K9A1_write_cmos_sensor(0x002A,0x3B0A);
	  S5K9A1_write_cmos_sensor(0x0F12,0x2EFF);	  
	  S5K9A1_write_cmos_sensor(0x002A,0xF41C);
	  S5K9A1_write_cmos_sensor(0x0F12,0x05FF);	  
	  S5K9A1_write_cmos_sensor(0x002A,0xF5DA);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0006);	  
	  S5K9A1_write_cmos_sensor(0x002A,0x370C);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0010);	  
	  S5K9A1_write_cmos_sensor(0x002A,0xF5C4);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0001);	  
	  
	  S5K9A1_write_cmos_sensor(0x002A,0x0214);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0000);
	  S5K9A1_write_cmos_sensor(0x002A,0x0218);
	  S5K9A1_write_cmos_sensor(0x0F12,0x1B72);
	  
	  //SW registers
	  S5K9A1_write_cmos_sensor(0x0028,0x7000);
	  S5K9A1_write_cmos_sensor(0x002A,0x0574);
	  S5K9A1_write_cmos_sensor(0x0F12,0x00A0);	  
	  S5K9A1_write_cmos_sensor(0x002A,0x057A);
	  S5K9A1_write_cmos_sensor(0x0F12,0x00A0);	  
	  S5K9A1_write_cmos_sensor(0x002A,0x0192);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0016);	  
	  S5K9A1_write_cmos_sensor(0x002A,0x014A);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0001);	  
	  S5K9A1_write_cmos_sensor(0x002A,0x0188);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0000);	  
	  S5K9A1_write_cmos_sensor(0x002A,0x0196);
	  S5K9A1_write_cmos_sensor(0x0F12,0x1476);	  
	  S5K9A1_write_cmos_sensor(0x002A,0x0540);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0009);	  
	  
	  //===========================================================
	  // CS timing
	  //===========================================================
	  S5K9A1_write_cmos_sensor(0x002A,0x01CC);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0005);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x0005);
	  S5K9A1_write_cmos_sensor(0x0F12,0x05DD);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x07CF);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0001);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x0001);
	  S5K9A1_write_cmos_sensor(0x0F12,0x05DD);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x03E5);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0001);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x03EB);
	  S5K9A1_write_cmos_sensor(0x0F12,0x05DD);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x07CF);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0007);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x0007);
	  S5K9A1_write_cmos_sensor(0x0F12,0x05DA);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x03E2);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0007);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x03F1);
	  S5K9A1_write_cmos_sensor(0x0F12,0x05DA);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x07CC);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0188);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x0166);
	  S5K9A1_write_cmos_sensor(0x0F12,0x01D3);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x01CF);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0188);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x0550);
	  S5K9A1_write_cmos_sensor(0x0F12,0x01D3);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x05B9);
	  S5K9A1_write_cmos_sensor(0x0F12,0x000B);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x000B);
	  S5K9A1_write_cmos_sensor(0x0F12,0x05E2);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x03EA);
	  S5K9A1_write_cmos_sensor(0x0F12,0x000B);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x03F5);
	  S5K9A1_write_cmos_sensor(0x0F12,0x05E2);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x07D4);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0055);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x0446);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0003);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x03ED);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0055);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x005C);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0003);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x0003);
	  S5K9A1_write_cmos_sensor(0x0F12,0x009B);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x0493);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0075);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x0461);
	  S5K9A1_write_cmos_sensor(0x0F12,0x004F);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x0443);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0006);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x03F0);
	  S5K9A1_write_cmos_sensor(0x0F12,0x009B);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x00A9);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0075);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x0077);
	  S5K9A1_write_cmos_sensor(0x0F12,0x004F);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x0059);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0006);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x0006);
	  S5K9A1_write_cmos_sensor(0x0F12,0x009E);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x0496);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0072);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x045E);
	  S5K9A1_write_cmos_sensor(0x0F12,0x009E);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x00AC);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0072);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x0074);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0053);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x0447);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0002);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x03EC);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0053);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x005D);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0002);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x0002);
	  S5K9A1_write_cmos_sensor(0x0F12,0x000C);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x000C);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0182);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x0160);
	  S5K9A1_write_cmos_sensor(0x0F12,0x01DF);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x01DB);
	  S5K9A1_write_cmos_sensor(0x0F12,0x05D8);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x03E2);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0000);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x03F6);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0000);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x054A);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0000);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x05C5);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0000);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x07CC);
	  S5K9A1_write_cmos_sensor(0x0F12,0x000C);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x000C);
	  S5K9A1_write_cmos_sensor(0x0F12,0x005E);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x005E);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0000);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x03F6);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0000);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x0448);
	  S5K9A1_write_cmos_sensor(0x0F12,0x000C);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x000C);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0062);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x0062);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0000);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x03F6);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0000);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x044C);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0001);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x0001);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0185);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x0163);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0000);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x03EB);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0000);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x054D);
	  S5K9A1_write_cmos_sensor(0x0F12,0x009D);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x00F7);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0182);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x0160);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0253);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x0227);
	  S5K9A1_write_cmos_sensor(0x0F12,0x05D8);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x03E0);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0000);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x04E1);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0000);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x054A);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0000);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x0611);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0000);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x07CA);
	  S5K9A1_write_cmos_sensor(0x0F12,0x010F);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x012B);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0185);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x0163);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0415);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x0303);
	  S5K9A1_write_cmos_sensor(0x0F12,0x05DB);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x03E3);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0000);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x0515);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0000);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x054D);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0000);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x06ED);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0000);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x07CD);
	  S5K9A1_write_cmos_sensor(0x0F12,0x018A);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x0168);
	  S5K9A1_write_cmos_sensor(0x0F12,0x01D9);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x01D5);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0000);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x0552);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0000);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x05BF);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0000);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x0000);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0000);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x0000);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0000);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x0000);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0000);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x0000);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0073);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x0073);
	  S5K9A1_write_cmos_sensor(0x0F12,0x05DB);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x03E3);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0000);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x045D);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0000);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x07CD);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0221);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x01C2);
	  S5K9A1_write_cmos_sensor(0x0F12,0x05DB);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x03E3);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0000);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x05AC);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0000);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x07CD);
	  S5K9A1_write_cmos_sensor(0x0F12,0x009F);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x00F9);
	  S5K9A1_write_cmos_sensor(0x0F12,0x017F);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x015D);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0255);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x0229);
	  S5K9A1_write_cmos_sensor(0x0F12,0x05D5);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x03DD);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0000);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x04E3);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0000);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x0547);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0000);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x0613);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0000);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x07C7);
	  S5K9A1_write_cmos_sensor(0x0F12,0x009F);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x00F9);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0147);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x0144);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0255);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x0229);
	  S5K9A1_write_cmos_sensor(0x0F12,0x059D);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x03C4);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0000);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x04E3);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0000);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x052E);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0000);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x0613);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0000);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x07AE);
	  S5K9A1_write_cmos_sensor(0x0F12,0x009D);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x00F7);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0182);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x0160);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0000);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x0000);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0000);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x0000);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0253);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x0227);
	  S5K9A1_write_cmos_sensor(0x0F12,0x05D8);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x03E0);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0000);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x04E1);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0000);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x054A);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0000);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x0611);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0000);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x07CA);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0182);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x0160);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0243);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x0207);
	  S5K9A1_write_cmos_sensor(0x0F12,0x05D8);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x054A);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0000);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x05F1);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0000);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x07CA);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0185);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x0163);
	  S5K9A1_write_cmos_sensor(0x0F12,0x018D);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x0173);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0000);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x03E3);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0000);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x03F3);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0000);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x054D);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0000);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x055D);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0189);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x016B);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0191);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x017B);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0000);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x03EB);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0000);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x03FB);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0000);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x0555);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0000);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x0565);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0002);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x0002);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0010);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x0010);
	  S5K9A1_write_cmos_sensor(0x0F12,0x014A);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x0147);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0185);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x0163);
	  S5K9A1_write_cmos_sensor(0x0F12,0x05A0);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x03C7);
	  S5K9A1_write_cmos_sensor(0x0F12,0x05DB);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x03E3);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0000);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x0531);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0000);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x054D);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0000);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x07B1);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0000);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x07CD);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0008);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x000C);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0004);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x0008);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0000);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x0000);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0000);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x0000);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0000);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x0000);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0000);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x0000);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0000);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x0000);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0000);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x0000);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0000);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x0000);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0000);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x0000);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0000);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x0000);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0000);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x0000);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0000);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x0000);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0000);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x0000);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0000);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x0000);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0000);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x0000);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0000);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x0000);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0000);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x0000);
	  S5K9A1_write_cmos_sensor(0x0F12,0x05E8);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x07DA);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0673);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x0873);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0000);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x0000);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0000);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x0000);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0000);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x0000);
	  S5K9A1_write_cmos_sensor(0x0F12,0x021B);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x05F1);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0253);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x060D);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0673);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x0873);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0000);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0000);										 
	  
	  //==============================
	  // System configuration
	  //==============================
	  S5K9A1_write_cmos_sensor(0x002A,0x070A);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0BB8);	  // d3000
	  S5K9A1_write_cmos_sensor(0x0F12,0x1770);	  // d6000
	  S5K9A1_write_cmos_sensor(0x002A,0x05AE);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0001);
	  
	  //===========================================================
	  // FE
	  //===========================================================
	  S5K9A1_write_cmos_sensor(0x002A,0x063A);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0080);	  //128
	  S5K9A1_write_cmos_sensor(0x002A,0x05D0);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0100);
	  S5K9A1_write_cmos_sensor(0x002A,0x063E);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0001);
	  
	  // RG ratio gains
	  S5K9A1_write_cmos_sensor(0x0F12,0xA000);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0xA122);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0xA122);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0xA122);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0xA122);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0xA128);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0xA08E);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0xA08E);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0xA08E);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0xA08E);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0xA092);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0xA000);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0xA000);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0xA000);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0xA000);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0xA000);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x208E);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x208E);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x208E);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x208E);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x2091);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x2094);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x2121);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x2121);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x2121);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x2123);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x212A);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x21B8);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x21B8);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x21B8);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x21B8);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0xA000);	  
	  
	  // BG ratio gains
	  S5K9A1_write_cmos_sensor(0x0F12,0xA1A6);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0xA558);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0xA36E);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0xA1A6);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0xA000);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x21A6);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0xA558);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0xA36E);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0xA1A6);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0xA000);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x21A6);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0xA558);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0xA36E);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0xA000);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x21A6);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x236D);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0xA564);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0xA375);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0xA1AA);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0xA000);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x21A6);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x236D);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0xA37D);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0xA1AD);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0xA000);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x21A6);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x236D);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0xA384);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0xA1B1);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0xA000);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0x21A7);	  
	  S5K9A1_write_cmos_sensor(0x0F12,0xA000);	  
	  
	  //===========================================================
	  // Clock settings
	  //===========================================================
	  
	  // Sppose the input frequency is 24MHz. Configuration makes VCO freq. 600MHz, System clock 70MHz, Output clock 70MHz
	  S5K9A1_write_cmos_sensor(0x002A,0x0090);
	  S5K9A1_write_cmos_sensor(0x0F12,0x5DC0);	  //24MHz
	  S5K9A1_write_cmos_sensor(0x0F12,0x0000);
	  S5K9A1_write_cmos_sensor(0x002A,0x00A4);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0004);
	  S5K9A1_write_cmos_sensor(0x0F12,0x0064);	  //100 => VCO = 24/4*100 = 600MHz
	  S5K9A1_write_cmos_sensor(0x0F12,0x0001);
	  S5K9A1_write_cmos_sensor(0x0F12,0x000A);	  //Sysclk = 60MHz
	  S5K9A1_write_cmos_sensor(0x0F12,0x0001);	  //MIPI OIF clk = 600MHz
	  S5K9A1_write_cmos_sensor(0x0F12,0x0008);

	  //preview setting
	S5K9A1_write_cmos_sensor(0x002A, 0x0102);
	S5K9A1_write_cmos_sensor(0x0F12, 0x0001);
	S5K9A1_write_cmos_sensor(0x0F12, 0x0100);
	S5K9A1_write_cmos_sensor(0x0F12, 0x0001);
	S5K9A1_write_cmos_sensor(0x0F12, 0x0100);
	S5K9A1_write_cmos_sensor(0x0F12, 0x0001);
	S5K9A1_write_cmos_sensor(0x002A, 0x00E8);
	S5K9A1_write_cmos_sensor(0x0F12, 0x0001);	//REG_0TC_PCFG_Cfg_Input_bUseRelativeSizes
	S5K9A1_write_cmos_sensor(0x002A, 0x00E0);
	S5K9A1_write_cmos_sensor(0x0F12, 0x0508);	//REG_0TC_PCFG_Cfg_Input_Sizes_usWidth	1288d
	S5K9A1_write_cmos_sensor(0x0F12, 0x02D8);	//REG_0TC_PCFG_Cfg_Input_Sizes_usHeight 0728d
	S5K9A1_write_cmos_sensor(0x002A, 0x00F0);
	S5K9A1_write_cmos_sensor(0x0F12, 0x014D);	//REG_0TC_PCFG_usMaxFrTimeMsecMult10
	S5K9A1_write_cmos_sensor(0x002A, 0x00EA);
	S5K9A1_write_cmos_sensor(0x0F12, 0x0003);	//REG_0TC_PCFG_usFrTimeType
	S5K9A1_write_cmos_sensor(0x002A, 0x0102);
	S5K9A1_write_cmos_sensor(0x0F12, 0x0002);	//REG_SF_USER_ExposureChanged
	S5K9A1_write_cmos_sensor(0x002A, 0x00F2);
	S5K9A1_write_cmos_sensor(0x0F12, 0x07D0);	//REG_0TC_PCFG_FrTimingCols   <== Line_length	
	S5K9A1_write_cmos_sensor(0x0F12, 0x03F0);	//REG_0TC_PCFG_FrTimingLines  <== FrameF_length in ints (30fps)
	S5K9A1_write_cmos_sensor(0x002A, 0x00DE);
	S5K9A1_write_cmos_sensor(0x0F12, 0x0000);	//REG_0TC_PCFG_FrTimingLines
	S5K9A1_write_cmos_sensor(0x002A, 0x010E);
	S5K9A1_write_cmos_sensor(0x0F12, 0x03F0);	//REG_SF_USER_CIS_FrTimeLines  <== Frame_length for user
	S5K9A1_write_cmos_sensor(0x002A, 0x00FE);			
	S5K9A1_write_cmos_sensor(0x0F12, 0x03E8);	//REG_SF_USER_Exposure		<==Fine_integration for user
	S5K9A1_write_cmos_sensor(0x0F12, 0x0300);	//REG_SF_USER_ExposureHigh	<==Coarse_integration for user
								   
	S5K9A1_write_cmos_sensor(0x002A, 0x00F6);	//REG_0TC_PCFG_uPrevMirror
	S5K9A1_write_cmos_sensor(0x0F12, 0x0000);	//Mirror funciton : 0 = Standard		GRBG	
			//			1 = Mirrored(H) 	RGGB
			//			1 = Filpped (V) 	BGGR
			//			1 = Mirrored + Filpped	GBRG
	
	//============================================================
	// Init Parameters
	//============================================================
	
	S5K9A1_write_cmos_sensor(0x002A, 0x00A0);
	S5K9A1_write_cmos_sensor(0x0F12, 0x0000);
	S5K9A1_write_cmos_sensor(0x0F12, 0x0001);
	S5K9A1_write_cmos_sensor(0x002A, 0x00CC);
	S5K9A1_write_cmos_sensor(0x0F12, 0x0001);
	S5K9A1_write_cmos_sensor(0x0F12, 0x0001);
	S5K9A1_write_cmos_sensor(0x002A, 0x00C8);
	S5K9A1_write_cmos_sensor(0x0F12, 0x0001);
	S5K9A1_write_cmos_sensor(0x0028, 0xD000);
	S5K9A1_write_cmos_sensor(0x002A, 0x1000);
	S5K9A1_write_cmos_sensor(0x0F12, 0x0001);	// Host interrupt
	mDELAY(10);
	S5K9A1_write_cmos_sensor(0x0028, 0x7000);
	S5K9A1_write_cmos_sensor(0x002A, 0x0588);
	S5K9A1_write_cmos_sensor(0x0F12, 0x0000);
	
#endif

    S5K9A1DB("S5K9A1_Sensor_Init exit :\n ");
	
}   /*  S5K9A1_Sensor_Init  */

void S5K9A1PreviewSetting(void)
{
    S5K9A1DB("S5K9A1Preview setting:");
    	
    
}


void S5K9A1VideoSetting(void)
{
	S5K9A1DB("S5K9A1VideoSetting:\n ");
	    
		
}


void S5K9A1CaptureSetting(void)
{
      S5K9A1DB("S5K9A1capture setting:");
	  //Full size capture
	  


}

/*************************************************************************
* FUNCTION
*   S5K9A1Open
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
/*======================LC898212AF=============================

extern	void StmvSet( stSmvPar ) ;
extern  void AfInit( unsigned char hall_bias, unsigned char hall_off );
extern  void ServoOn(void);


#define		REG_ADDR_START	  	0x80		// REG Start address

void LC898212_Init()
{
    stSmvPar StSmvPar;

    int HallOff = 0x75;	 	// Please Read Offset from EEPROM or OTP
    int HallBiase = 0x2E;   // Please Read Bias from EEPROM or OTP
    
	AfInit( HallOff, HallBiase );	// Initialize driver IC

    // Step move parameter set
    StSmvPar.UsSmvSiz	= STMV_SIZE ;
	StSmvPar.UcSmvItv	= STMV_INTERVAL ;
	StSmvPar.UcSmvEnb	= STMCHTG_SET | STMSV_SET | STMLFF_SET ;
	StmvSet( StSmvPar ) ;
	
	ServoOn();	// Close loop ON
	
}
=========================LC898212AF========================*/

UINT32 S5K9A1Open(void)
{

	volatile signed int i;
	kal_uint16 sensor_id = 0;

	S5K9A1DB("S5K9A1Open enter :\n ");

	//  Read sensor ID to adjust I2C is OK?
	for(i=0;i<3;i++)
	{	
	    S5K9A1_write_cmos_sensor(0x002c,0x0000);
		S5K9A1_write_cmos_sensor(0x002e,0x0040);
		sensor_id=S5K9A1_read_cmos_sensor_16(0x0F12);
		S5K9A1DB("OS5K9A1 READ ID :%x",sensor_id);
		if(sensor_id != S5K9A1_SENSOR_ID)
		{
			return ERROR_NONE;
		}else
			break;
	}

	spin_lock(&s5k9a1mipiraw_drv_lock);
	S5K9A1.sensorMode = SENSOR_MODE_INIT;
	S5K9A1.S5K9A1AutoFlickerMode = KAL_FALSE;
	S5K9A1.S5K9A1VideoMode = KAL_FALSE;
	spin_unlock(&s5k9a1mipiraw_drv_lock);
	S5K9A1_Sensor_Init();
	
	spin_lock(&s5k9a1mipiraw_drv_lock);
	S5K9A1.DummyLines= 0;
	S5K9A1.DummyPixels= 0;
	S5K9A1.pvPclk =  (600); 
	S5K9A1.videoPclk = (600);
	S5K9A1.capPclk = (600);
	
	S5K9A1.maxExposureLines =S5K9A1_PV_PERIOD_LINE_NUMS;
	
	spin_unlock(&s5k9a1mipiraw_drv_lock);
	S5K9A1DB("S5K9A1Open exit :\n ");

    return ERROR_NONE;
}

/*************************************************************************
* FUNCTION
*   S5K9A1GetSensorID
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
UINT32 S5K9A1GetSensorID(UINT32 *sensorID)
{
    int  retry = 1;

	S5K9A1DB("S5K9A1GetSensorID enter :\n ");

    // check if sensor ID correct
    do {
		S5K9A1_write_cmos_sensor(0x002C,0x0000);
		S5K9A1_write_cmos_sensor(0x002E,0x0040);
		*sensorID=S5K9A1_read_cmos_sensor_16(0x0F12);

		S5K9A1DB("REG0001 = 0x%04x\n",S5K9A1_read_cmos_sensor_16(0x0F12));
        if (*sensorID == S5K9A1_SENSOR_ID)
        	{
        		S5K9A1DB("Sensor ID = 0x%04x\n", *sensorID);
            	break;
        	}
        S5K9A1DB("Read Sensor ID Fail = 0x%04x\n", *sensorID);
        retry--;
    } while (retry > 0);

    if (*sensorID != S5K9A1_SENSOR_ID) {
        *sensorID = 0xFFFFFFFF;
        return ERROR_NONE;
    }
    return ERROR_NONE;
}


/*************************************************************************
* FUNCTION
*   S5K9A1_SetShutter
*
* DESCRIPTION
*   This function set e-shutter of S5K9A1 to change exposure time.
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
void S5K9A1_write_shutter(kal_uint16 shutter)
{
	S5K9A1DB("S5K9A1MIPI_Write_Shutter =%d \n ",shutter);

	S5K9A1_write_cmos_sensor(0x0104, 1);      	
   
    S5K9A1_write_cmos_sensor(0x0202, (shutter >> 8) & 0xFF);
    S5K9A1_write_cmos_sensor(0x0203, shutter  & 0xFF);
	
    S5K9A1_write_cmos_sensor(0x0104, 0);   

}	/* write_S5K9A1_shutter */

void S5K9A1_SetShutter(kal_uint16 iShutter)
{

   S5K9A1DB("S5K9A1MIPI_SetShutter =%d \n ",iShutter);
   kal_uint16 frame_length=0 ;
   if (iShutter < 1)
	   iShutter = 1; 

   S5K9A1_write_cmos_sensor(0x0028,0x7000); 	   
   S5K9A1_write_cmos_sensor(0x002A,0x0102);
   S5K9A1_write_cmos_sensor(0x0F12,0x0002);	   
   S5K9A1_write_cmos_sensor(0x002A,0x00EA);
   S5K9A1_write_cmos_sensor(0x0F12,0x0003);

   frame_length=S5K9A1_PV_PERIOD_LINE_NUMS+S5K9A1.DummyLines;
   	
   if(iShutter>frame_length-8)
   	{   
   	    frame_length=iShutter+8;
	    S5K9A1_write_cmos_sensor(0x002A,0x010E);
        S5K9A1_write_cmos_sensor(0x0F12, frame_length);	        
   	}
   else
   	{
	    S5K9A1_write_cmos_sensor(0x002A,0x010E);
        S5K9A1_write_cmos_sensor(0x0F12,frame_length);	        
  
   	}
   spin_lock(&s5k9a1mipiraw_drv_lock);
   S5K9A1.shutter= iShutter;
   S5K9A1_FeatureControl_PERIOD_LineNum=frame_length;
   spin_unlock(&s5k9a1mipiraw_drv_lock);

   S5K9A1_write_cmos_sensor(0x002A,0x0100);
   S5K9A1_write_cmos_sensor(0x0F12,iShutter);
  	  
   return;
}   /*  S5K9A1_SetShutter   */


/*************************************************************************
* FUNCTION
*   S5K9A1_read_shutter
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
UINT32 S5K9A1_read_shutter(void)
{

    kal_uint16 ishutter;

    ishutter = S5K9A1_read_cmos_sensor(0x0202); /* course_integration_time */

    S5K9A1DB("S5K9A1_read_shutter (0x%x)\n",ishutter);

    return ishutter;

}

/*************************************************************************
* FUNCTION
*   S5K9A1_night_mode
*
* DESCRIPTION
*   This function night mode of S5K9A1.
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
void S5K9A1_NightMode(kal_bool bEnable)
{
}/*	S5K9A1_NightMode */



/*************************************************************************
* FUNCTION
*   S5K9A1Close
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
UINT32 S5K9A1Close(void)
{
    S5K9A1_ReEnteyCamera = KAL_FALSE;
    return ERROR_NONE;
}	/* S5K9A1Close() */

void S5K9A1SetFlipMirror(kal_int32 imgMirror)
{
  	S5K9A1DB("imgMirror=%d\n",imgMirror);
	spin_lock(&s5k9a1mipiraw_drv_lock);
	S5K9A1.imgMirror = imgMirror;
	spin_unlock(&s5k9a1mipiraw_drv_lock);
	
	S5K9A1_write_cmos_sensor(0x0028,0x7000);
    S5K9A1_write_cmos_sensor(0x002A,0x00F6);
    switch (imgMirror)
    {

		case IMAGE_H_MIRROR://IMAGE_NORMAL:  bit0 mirror,   bit1 flip.
		     S5K9A1_write_cmos_sensor(0x0F12,0x0001);
            break;
        case IMAGE_NORMAL://IMAGE_V_MIRROR:
			 S5K9A1_write_cmos_sensor(0x0F12,0x0000); 
            break;
        case IMAGE_HV_MIRROR://IMAGE_H_MIRROR:
			 S5K9A1_write_cmos_sensor(0x0F12,0x0003);   //morror +flip
            break;
        case IMAGE_V_MIRROR://IMAGE_HV_MIRROR:
			 S5K9A1_write_cmos_sensor(0x0F12,0x0002); //flip
            break;
    }
	S5K9A1_write_cmos_sensor(0x002A,0x00D0);
    S5K9A1_write_cmos_sensor(0x0F12,0x0001);
	S5K9A1_write_cmos_sensor(0x002A,0x00D4);
    S5K9A1_write_cmos_sensor(0x0F12,0x0001);
	
}


/*************************************************************************
* FUNCTION
*   S5K9A1Preview
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
UINT32 S5K9A1Preview(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                                                MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{

	S5K9A1DB("S5K9A1Preview enter:");

	// preview size
	if(S5K9A1.sensorMode == SENSOR_MODE_PREVIEW)
	{
		// do nothing
		// FOR CCT PREVIEW
	}
	else
	{
		S5K9A1DB("S5K9A1Preview setting!!\n");
		//S5K9A1PreviewSetting();
	}
	
	spin_lock(&s5k9a1mipiraw_drv_lock);
	S5K9A1.sensorMode = SENSOR_MODE_PREVIEW; // Need set preview setting after capture mode
	S5K9A1.DummyPixels = 0;//define dummy pixels and lines
	S5K9A1.DummyLines = 0 ;
	S5K9A1_FeatureControl_PERIOD_PixelNum=S5K9A1_PV_PERIOD_PIXEL_NUMS+ S5K9A1.DummyPixels;
	S5K9A1_FeatureControl_PERIOD_LineNum=S5K9A1_PV_PERIOD_LINE_NUMS+S5K9A1.DummyLines;
	spin_unlock(&s5k9a1mipiraw_drv_lock);


	//set mirror & flip
	S5K9A1DB("[S5K9A1Preview] mirror&flip: %d \n",sensor_config_data->SensorImageMirror);
	spin_lock(&s5k9a1mipiraw_drv_lock);
	S5K9A1.imgMirror = sensor_config_data->SensorImageMirror;
	spin_unlock(&s5k9a1mipiraw_drv_lock);
	//S5K9A1SetFlipMirror(sensor_config_data->SensorImageMirror);

	S5K9A1DB("S5K9A1Preview exit:\n");
    return ERROR_NONE;
}	/* S5K9A1Preview() */



UINT32 S5K9A1Video(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                                                MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{

	S5K9A1DB("S5K9A1Video enter:");

	if(S5K9A1.sensorMode == SENSOR_MODE_VIDEO)
	{
		// do nothing
	}
	else
	{
		//S5K9A1VideoSetting();

	}
	spin_lock(&s5k9a1mipiraw_drv_lock);
	S5K9A1.sensorMode = SENSOR_MODE_VIDEO;
	S5K9A1.DummyPixels = 0;//define dummy pixels and lines
	S5K9A1.DummyLines = 0 ;
	S5K9A1_FeatureControl_PERIOD_PixelNum=S5K9A1_VIDEO_PERIOD_PIXEL_NUMS+ S5K9A1.DummyPixels;
	S5K9A1_FeatureControl_PERIOD_LineNum=S5K9A1_VIDEO_PERIOD_LINE_NUMS+S5K9A1.DummyLines;
	spin_unlock(&s5k9a1mipiraw_drv_lock);


	spin_lock(&s5k9a1mipiraw_drv_lock);
	S5K9A1.imgMirror = sensor_config_data->SensorImageMirror;
	spin_unlock(&s5k9a1mipiraw_drv_lock);
	//S5K9A1SetFlipMirror(sensor_config_data->SensorImageMirror);

	S5K9A1DB("S5K9A1Video exit:\n");
    return ERROR_NONE;
}


UINT32 S5K9A1Capture(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                                                MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{

 	kal_uint32 shutter = S5K9A1.shutter;
	kal_uint32 temp_data;
	//kal_uint32 pv_line_length , cap_line_length,

	if( SENSOR_MODE_CAPTURE== S5K9A1.sensorMode)
	{
		S5K9A1DB("S5K9A1Capture BusrtShot!!!\n");
	}
	else{
	S5K9A1DB("S5K9A1Capture enter:\n");

	// Full size setting
	//S5K9A1CaptureSetting();

	spin_lock(&s5k9a1mipiraw_drv_lock);
	S5K9A1.sensorMode = SENSOR_MODE_CAPTURE;
	S5K9A1.imgMirror = sensor_config_data->SensorImageMirror;
	S5K9A1.DummyPixels = 0;//define dummy pixels and lines
	S5K9A1.DummyLines = 0 ;
	S5K9A1_FeatureControl_PERIOD_PixelNum = S5K9A1_FULL_PERIOD_PIXEL_NUMS + S5K9A1.DummyPixels;
	S5K9A1_FeatureControl_PERIOD_LineNum = S5K9A1_FULL_PERIOD_LINE_NUMS + S5K9A1.DummyLines;
	spin_unlock(&s5k9a1mipiraw_drv_lock);

	S5K9A1DB("[S5K9A1Capture] mirror&flip: %d\n",sensor_config_data->SensorImageMirror);
	//S5K9A1SetFlipMirror(sensor_config_data->SensorImageMirror);

	S5K9A1DB("S5K9A1Capture exit:\n");
	}

    return ERROR_NONE;
}	/* S5K9A1Capture() */

UINT32 S5K9A1GetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution)
{

    S5K9A1DB("S5K9A1GetResolution!!\n");

	pSensorResolution->SensorPreviewWidth	= S5K9A1_IMAGE_SENSOR_PV_WIDTH;
    pSensorResolution->SensorPreviewHeight	= S5K9A1_IMAGE_SENSOR_PV_HEIGHT;
    pSensorResolution->SensorFullWidth		= S5K9A1_IMAGE_SENSOR_FULL_WIDTH;
    pSensorResolution->SensorFullHeight		= S5K9A1_IMAGE_SENSOR_FULL_HEIGHT;
    pSensorResolution->SensorVideoWidth		= S5K9A1_IMAGE_SENSOR_VIDEO_WIDTH;
    pSensorResolution->SensorVideoHeight    = S5K9A1_IMAGE_SENSOR_VIDEO_HEIGHT;
	
    return ERROR_NONE;
}   /* S5K9A1GetResolution() */

UINT32 S5K9A1GetInfo(MSDK_SCENARIO_ID_ENUM ScenarioId,
                                                MSDK_SENSOR_INFO_STRUCT *pSensorInfo,
                                                MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{      switch(ScenarioId)
	   {
		case MSDK_SCENARIO_ID_CAMERA_ZSD:
			pSensorInfo->SensorPreviewResolutionX=S5K9A1_IMAGE_SENSOR_FULL_WIDTH;
			pSensorInfo->SensorPreviewResolutionY=S5K9A1_IMAGE_SENSOR_FULL_HEIGHT;
            pSensorInfo->SensorFullResolutionX    =  S5K9A1_IMAGE_SENSOR_FULL_WIDTH;
            pSensorInfo->SensorFullResolutionY    =  S5K9A1_IMAGE_SENSOR_FULL_HEIGHT; 			
			pSensorInfo->SensorCameraPreviewFrameRate=30;
		break;

        case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
        case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
        	pSensorInfo->SensorPreviewResolutionX=S5K9A1_IMAGE_SENSOR_PV_WIDTH;
       		pSensorInfo->SensorPreviewResolutionY=S5K9A1_IMAGE_SENSOR_PV_HEIGHT;
            pSensorInfo->SensorFullResolutionX    =  S5K9A1_IMAGE_SENSOR_FULL_WIDTH;
            pSensorInfo->SensorFullResolutionY    =  S5K9A1_IMAGE_SENSOR_FULL_HEIGHT;       		
			pSensorInfo->SensorCameraPreviewFrameRate=30;            
            break;
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
        	pSensorInfo->SensorPreviewResolutionX=S5K9A1_IMAGE_SENSOR_VIDEO_WIDTH;
       		pSensorInfo->SensorPreviewResolutionY=S5K9A1_IMAGE_SENSOR_VIDEO_HEIGHT;
            pSensorInfo->SensorFullResolutionX    =  S5K9A1_IMAGE_SENSOR_FULL_WIDTH;
            pSensorInfo->SensorFullResolutionY    =  S5K9A1_IMAGE_SENSOR_FULL_HEIGHT;       		
			pSensorInfo->SensorCameraPreviewFrameRate=30;  
			break;
		default:
        	pSensorInfo->SensorPreviewResolutionX=S5K9A1_IMAGE_SENSOR_PV_WIDTH;
       		pSensorInfo->SensorPreviewResolutionY=S5K9A1_IMAGE_SENSOR_PV_HEIGHT;
            pSensorInfo->SensorFullResolutionX    =  S5K9A1_IMAGE_SENSOR_FULL_WIDTH;
            pSensorInfo->SensorFullResolutionY    =  S5K9A1_IMAGE_SENSOR_FULL_HEIGHT;       		
			pSensorInfo->SensorCameraPreviewFrameRate=30;
		break;
	}

	spin_lock(&s5k9a1mipiraw_drv_lock);
	S5K9A1.imgMirror = pSensorConfigData->SensorImageMirror ;
	spin_unlock(&s5k9a1mipiraw_drv_lock);

   	pSensorInfo->SensorOutputDataFormat= SENSOR_OUTPUT_FORMAT_RAW_Gr;
    pSensorInfo->SensorClockPolarity =SENSOR_CLOCK_POLARITY_LOW;
    pSensorInfo->SensorClockFallingPolarity=SENSOR_CLOCK_POLARITY_LOW;
    pSensorInfo->SensorHsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
    pSensorInfo->SensorVsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;

    pSensorInfo->SensroInterfaceType=SENSOR_INTERFACE_TYPE_MIPI;
	pSensorInfo->MIPIsensorType=MIPI_OPHY_CSI2;

    pSensorInfo->CaptureDelayFrame = 3;
    pSensorInfo->PreviewDelayFrame = 3;
    pSensorInfo->VideoDelayFrame = 3;

    pSensorInfo->SensorDrivingCurrent = ISP_DRIVING_8MA;
    pSensorInfo->AEShutDelayFrame = 0;//0;		    /* The frame of setting shutter default 0 for TG int */
    pSensorInfo->AESensorGainDelayFrame = 0 ;//0;     /* The frame of setting sensor gain */
    pSensorInfo->AEISPGainDelayFrame = 2;
	  

    switch (ScenarioId)
    {
        case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
            pSensorInfo->SensorClockFreq=24;
            pSensorInfo->SensorClockRisingCount= 0;

            pSensorInfo->SensorGrabStartX = S5K9A1_PV_X_START;
            pSensorInfo->SensorGrabStartY = S5K9A1_PV_Y_START;

            pSensorInfo->SensorMIPILaneNumber = SENSOR_MIPI_1_LANE;
            pSensorInfo->MIPIDataLowPwr2HighSpeedTermDelayCount = 0;
	     	pSensorInfo->MIPIDataLowPwr2HighSpeedSettleDelayCount = 26;
	    	pSensorInfo->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
			pSensorInfo->SensorWidthSampling = 0;	// 0 is default 1x
			pSensorInfo->SensorHightSampling = 0;	 // 0 is default 1x
			pSensorInfo->SensorPacketECCOrder = 1;
            break;
        case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
            pSensorInfo->SensorClockFreq=24;
            pSensorInfo->SensorClockRisingCount= 0;

            pSensorInfo->SensorGrabStartX = S5K9A1_VIDEO_X_START;
            pSensorInfo->SensorGrabStartY = S5K9A1_VIDEO_Y_START;

            pSensorInfo->SensorMIPILaneNumber = SENSOR_MIPI_1_LANE;
            pSensorInfo->MIPIDataLowPwr2HighSpeedTermDelayCount = 0;
	     	pSensorInfo->MIPIDataLowPwr2HighSpeedSettleDelayCount = 26;
	    	pSensorInfo->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
			pSensorInfo->SensorWidthSampling = 0;	// 0 is default 1x
			pSensorInfo->SensorHightSampling = 0;	 // 0 is default 1x
			pSensorInfo->SensorPacketECCOrder = 1;
            break;
        case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		case MSDK_SCENARIO_ID_CAMERA_ZSD:
            pSensorInfo->SensorClockFreq=24;
            pSensorInfo->SensorClockRisingCount= 0;

            pSensorInfo->SensorGrabStartX = S5K9A1_FULL_X_START;	//2*S5K9A1_IMAGE_SENSOR_PV_STARTX;
            pSensorInfo->SensorGrabStartY = S5K9A1_FULL_Y_START;	//2*S5K9A1_IMAGE_SENSOR_PV_STARTY;

            pSensorInfo->SensorMIPILaneNumber = SENSOR_MIPI_1_LANE;
            pSensorInfo->MIPIDataLowPwr2HighSpeedTermDelayCount = 0;
            pSensorInfo->MIPIDataLowPwr2HighSpeedSettleDelayCount = 26;
            pSensorInfo->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
			pSensorInfo->SensorWidthSampling = 0;	// 0 is default 1x
			pSensorInfo->SensorHightSampling = 0;	 // 0 is default 1x
			pSensorInfo->SensorPacketECCOrder = 1;

            break;
        default:
			pSensorInfo->SensorClockFreq=24;
            pSensorInfo->SensorClockRisingCount= 0;

            pSensorInfo->SensorGrabStartX = S5K9A1_PV_X_START;
            pSensorInfo->SensorGrabStartY = S5K9A1_PV_Y_START;

            pSensorInfo->SensorMIPILaneNumber = SENSOR_MIPI_1_LANE;
            pSensorInfo->MIPIDataLowPwr2HighSpeedTermDelayCount = 0;
	     	pSensorInfo->MIPIDataLowPwr2HighSpeedSettleDelayCount = 26;
	    	pSensorInfo->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
			pSensorInfo->SensorWidthSampling = 0;	// 0 is default 1x
			pSensorInfo->SensorHightSampling = 0;	 // 0 is default 1x
			pSensorInfo->SensorPacketECCOrder = 1;

            break;
    }

    memcpy(pSensorConfigData, &S5K9A1SensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));

    return ERROR_NONE;
}   /* S5K9A1GetInfo() */


UINT32 S5K9A1Control(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow,
                                                MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
		spin_lock(&s5k9a1mipiraw_drv_lock);
		S5K9A1CurrentScenarioId = ScenarioId;
		spin_unlock(&s5k9a1mipiraw_drv_lock);
		S5K9A1DB("S5K9A1CurrentScenarioId=%d\n",S5K9A1CurrentScenarioId);

	switch (ScenarioId)
    {
        case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
            S5K9A1Preview(pImageWindow, pSensorConfigData);
            break;
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		case MSDK_SCENARIO_ID_CAMERA_ZSD:
			S5K9A1Capture(pImageWindow, pSensorConfigData);
			break;
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			S5K9A1Video(pImageWindow, pSensorConfigData);
			break;
        default:
            return ERROR_INVALID_SCENARIO_ID;

    }
    return ERROR_NONE;
} /* S5K9A1Control() */


UINT32 S5K9A1SetVideoMode(UINT16 u2FrameRate)
{
    //return;
    kal_uint32 MIN_Frame_length =0,frameRate=0,extralines=0;
    S5K9A1DB("[S5K9A1SetVideoMode] frame rate = %d\n", u2FrameRate);

	spin_lock(&s5k9a1mipiraw_drv_lock);
	S5K9A1_VIDEO_MODE_TARGET_FPS=u2FrameRate;
	spin_unlock(&s5k9a1mipiraw_drv_lock);

	if(u2FrameRate==0)
	{
		S5K9A1DB("Disable Video Mode or dynimac fps\n");
		return KAL_TRUE;
	}
	if(u2FrameRate >30 || u2FrameRate <5)
	    S5K9A1DB("error frame rate seting\n");

    if(S5K9A1.sensorMode == SENSOR_MODE_VIDEO)//video ScenarioId recording
    {
    	if(S5K9A1.S5K9A1AutoFlickerMode == KAL_TRUE)
    	{
    		if (u2FrameRate==30)
				frameRate= 306;
			else if(u2FrameRate==15)
				frameRate= 148;//148;
			else
				frameRate=u2FrameRate*10;

			MIN_Frame_length = (S5K9A1.videoPclk*100000)/(S5K9A1_VIDEO_PERIOD_PIXEL_NUMS + S5K9A1.DummyPixels)/frameRate*10;
    	}
		else
			MIN_Frame_length = (S5K9A1.videoPclk*100000) /(S5K9A1_VIDEO_PERIOD_PIXEL_NUMS + S5K9A1.DummyPixels)/u2FrameRate;

		if((MIN_Frame_length <=S5K9A1_VIDEO_PERIOD_LINE_NUMS))
		{
			MIN_Frame_length = S5K9A1_VIDEO_PERIOD_LINE_NUMS;
			S5K9A1DB("[S5K9A1SetVideoMode]current fps = %d\n", (S5K9A1.pvPclk*100000)  /(S5K9A1_VIDEO_PERIOD_PIXEL_NUMS)/S5K9A1_VIDEO_PERIOD_LINE_NUMS);
		}
		S5K9A1DB("[S5K9A1SetVideoMode]current fps (10 base)= %d\n", (S5K9A1.pvPclk*100000)*10/(S5K9A1_VIDEO_PERIOD_PIXEL_NUMS + S5K9A1.DummyPixels)/MIN_Frame_length);
		extralines = MIN_Frame_length - S5K9A1_VIDEO_PERIOD_LINE_NUMS;
		S5K9A1DB("[S5K9A1SetVideoMode]extralines= %d\n", extralines);
		spin_lock(&s5k9a1mipiraw_drv_lock);
		S5K9A1.DummyPixels = 0;//define dummy pixels and lines
		S5K9A1.DummyLines = extralines ;
		spin_unlock(&s5k9a1mipiraw_drv_lock);
		
		//S5K9A1_SetDummy(S5K9A1.DummyPixels,extralines);
		S5K9A1_write_cmos_sensor(0x0028,0x7000);		
		S5K9A1_write_cmos_sensor(0x002A,0x0102);
		S5K9A1_write_cmos_sensor(0x0F12,0x0002);	
		S5K9A1_write_cmos_sensor(0x002A,0x00EA);
		S5K9A1_write_cmos_sensor(0x0F12,0x0003);

	    S5K9A1_write_cmos_sensor(0x002A,0x010E);
        S5K9A1_write_cmos_sensor(0x0F12, MIN_Frame_length);	 

		if(S5K9A1.shutter>MIN_Frame_length-8)
			{
			  S5K9A1.shutter=MIN_Frame_length-8;
			  S5K9A1_write_cmos_sensor(0x002A,0x0100);
			  S5K9A1_write_cmos_sensor(0x0F12,S5K9A1.shutter);
			}
    }
	S5K9A1DB("[S5K9A1SetVideoMode]MIN_Frame_length=%d,S5K9A1.DummyLines=%d\n",MIN_Frame_length,S5K9A1.DummyLines);

    return KAL_TRUE;
}

UINT32 S5K9A1SetAutoFlickerMode(kal_bool bEnable, UINT16 u2FrameRate)
{
    S5K9A1DB("[S5K9A1SetAutoFlickerMode] frame rate(10base) = %d %d\n", bEnable, u2FrameRate);
    if (KAL_TRUE == S5K9A1DuringTestPattern) return ERROR_NONE;
	if(bEnable) {   // enable auto flicker
		spin_lock(&s5k9a1mipiraw_drv_lock);
		S5K9A1.S5K9A1AutoFlickerMode = KAL_TRUE;
		spin_unlock(&s5k9a1mipiraw_drv_lock);
    } else {
    	spin_lock(&s5k9a1mipiraw_drv_lock);
        S5K9A1.S5K9A1AutoFlickerMode = KAL_FALSE;
		spin_unlock(&s5k9a1mipiraw_drv_lock);
        S5K9A1DB("Disable Auto flicker\n");
    }

    return ERROR_NONE;
}

UINT32 S5K9A1SetTestPatternMode(kal_bool bEnable)
{
    S5K9A1DB("[S5K9A1SetTestPatternMode] Test pattern enable:%d\n", bEnable);
	kal_uint16 temp;

	if(bEnable) 
	{
        spin_lock(&s5k9a1mipiraw_drv_lock);
	    S5K9A1DuringTestPattern = KAL_TRUE;
	    spin_unlock(&s5k9a1mipiraw_drv_lock);
		S5K9A1_write_cmos_sensor(0x0028,0x7000);
		S5K9A1_write_cmos_sensor(0x002A,0x05AE);
		S5K9A1_write_cmos_sensor(0x0F12,0x0000);
		S5K9A1_write_cmos_sensor(0x002A,0x05B0);
		S5K9A1_write_cmos_sensor(0x0F12,0x0003);
	}
   else		
	{	S5K9A1_write_cmos_sensor(0x0028,0x7000);
		S5K9A1_write_cmos_sensor(0x002A,0x05AE);
		S5K9A1_write_cmos_sensor(0x0F12,0x0000);
		S5K9A1_write_cmos_sensor(0x002A,0x05B0);
		S5K9A1_write_cmos_sensor(0x0F12,0x0000);	
	}

    return ERROR_NONE;
}

UINT32 S5K9A1MIPISetMaxFramerateByScenario(MSDK_SCENARIO_ID_ENUM scenarioId, MUINT32 frameRate) {
	kal_uint32 pclk;
	kal_int16 dummyLine;
	kal_uint16 lineLength,frameHeight;
	//return;	/////////////////////////////////////return////
	S5K9A1DB("S5K9A1MIPISetMaxFramerateByScenario: scenarioId = %d, frame rate = %d\n",scenarioId,frameRate);
	switch (scenarioId) {
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
			pclk = 60000000;
			lineLength = S5K9A1_PV_PERIOD_PIXEL_NUMS;
			frameHeight = (10 * pclk)/frameRate/lineLength;
			dummyLine = frameHeight - S5K9A1_PV_PERIOD_LINE_NUMS;
			if(dummyLine<0)
				dummyLine = 0;
			spin_lock(&s5k9a1mipiraw_drv_lock);
			S5K9A1.sensorMode = SENSOR_MODE_PREVIEW;
			spin_unlock(&s5k9a1mipiraw_drv_lock);
			S5K9A1_SetDummy(0, dummyLine);			
			break;			
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			pclk = 60000000;
			lineLength = S5K9A1_VIDEO_PERIOD_PIXEL_NUMS;
			frameHeight = (10 * pclk)/frameRate/lineLength;
			dummyLine = frameHeight - S5K9A1_VIDEO_PERIOD_LINE_NUMS;
			if(dummyLine<0)
				dummyLine = 0;
			spin_lock(&s5k9a1mipiraw_drv_lock);
			S5K9A1.sensorMode = SENSOR_MODE_VIDEO;
			spin_unlock(&s5k9a1mipiraw_drv_lock);
			S5K9A1_SetDummy(0, dummyLine);			
			break;			
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		case MSDK_SCENARIO_ID_CAMERA_ZSD:			
			pclk = 60000000;
			lineLength = S5K9A1_FULL_PERIOD_PIXEL_NUMS;
			frameHeight = (10 * pclk)/frameRate/lineLength;
			dummyLine = frameHeight - S5K9A1_FULL_PERIOD_LINE_NUMS;
			if(dummyLine<0)
				dummyLine = 0;
			spin_lock(&s5k9a1mipiraw_drv_lock);
			S5K9A1.sensorMode = SENSOR_MODE_CAPTURE;
			spin_unlock(&s5k9a1mipiraw_drv_lock);
			S5K9A1_SetDummy(0, dummyLine);			
			break;			
		default:
			break;
	}	
	return ERROR_NONE;
}


UINT32 S5K9A1MIPIGetDefaultFramerateByScenario(MSDK_SCENARIO_ID_ENUM scenarioId, MUINT32 *pframeRate) 
{

	switch (scenarioId) {
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			 *pframeRate = 300;
			 break;
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		case MSDK_SCENARIO_ID_CAMERA_ZSD:
			 *pframeRate = 300;
			break;			
		default:
			break;
	}

	return ERROR_NONE;
}



UINT32 S5K9A1FeatureControl(MSDK_SENSOR_FEATURE_ENUM FeatureId,
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
	   MSDK_SENSOR_ENG_INFO_STRUCT	  *pSensorEngInfo=(MSDK_SENSOR_ENG_INFO_STRUCT *) pFeaturePara;
	
	   S5K9A1DB("S5K9A1_FeatureControl FeatureId(%d)\n",FeatureId);
	
	   switch (FeatureId)
	   {
		   case SENSOR_FEATURE_GET_RESOLUTION:
			   *pFeatureReturnPara16++=S5K9A1_IMAGE_SENSOR_FULL_WIDTH;
			   *pFeatureReturnPara16=S5K9A1_IMAGE_SENSOR_FULL_HEIGHT;
			   *pFeatureParaLen=4;
			   break;
		   case SENSOR_FEATURE_GET_PERIOD:
			   *pFeatureReturnPara16++= S5K9A1_FeatureControl_PERIOD_PixelNum;
			   *pFeatureReturnPara16= S5K9A1_FeatureControl_PERIOD_LineNum;
			   *pFeatureParaLen=4;
			   break;
		   case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ:
			   switch(S5K9A1CurrentScenarioId)
			   {   
			       case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
					   *pFeatureReturnPara32 = S5K9A1.videoPclk * 100000;
					      *pFeatureParaLen=4;
					   break;
				   case MSDK_SCENARIO_ID_CAMERA_ZSD:
				   case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
					   *pFeatureReturnPara32 = S5K9A1.capPclk * 100000; //19500000;
						  *pFeatureParaLen=4;
					   break;
				   default:
					   *pFeatureReturnPara32 = S5K9A1.pvPclk * 100000; //19500000;
						  *pFeatureParaLen=4;
					   break;
			   }
			   break;
		   case SENSOR_FEATURE_SET_ESHUTTER:
			   S5K9A1_SetShutter(*pFeatureData16);
			   break;
		   case SENSOR_FEATURE_SET_NIGHTMODE:
			   S5K9A1_NightMode((BOOL) *pFeatureData16);
			   break;
		   case SENSOR_FEATURE_SET_GAIN:
			   S5K9A1MIPI_SetGain((UINT16) *pFeatureData16);
			   break;
		   case SENSOR_FEATURE_SET_FLASHLIGHT:
			   break;
		   case SENSOR_FEATURE_SET_ISP_MASTER_CLOCK_FREQ:
			  // S5K9A1_isp_master_clock=*pFeatureData32;
			   break;
		   case SENSOR_FEATURE_SET_REGISTER:
			   S5K9A1_write_cmos_sensor(pSensorRegData->RegAddr, pSensorRegData->RegData);
			   break;
		   case SENSOR_FEATURE_GET_REGISTER:
			   pSensorRegData->RegData = S5K9A1_read_cmos_sensor(pSensorRegData->RegAddr);
			   break;
		   case SENSOR_FEATURE_SET_CCT_REGISTER:
			   SensorRegNumber=FACTORY_END_ADDR;
			   for (i=0;i<SensorRegNumber;i++)
			   {
				   S5K9A1SensorCCT[i].Addr=*pFeatureData32++;
				   S5K9A1SensorCCT[i].Para=*pFeatureData32++;
			   }
			   break;
		   case SENSOR_FEATURE_GET_CCT_REGISTER:
			   SensorRegNumber=FACTORY_END_ADDR;
			   if (*pFeatureParaLen<(SensorRegNumber*sizeof(SENSOR_REG_STRUCT)+4))
				   return FALSE;
			   *pFeatureData32++=SensorRegNumber;
			   for (i=0;i<SensorRegNumber;i++)
			   {
				   *pFeatureData32++=S5K9A1SensorCCT[i].Addr;
				   *pFeatureData32++=S5K9A1SensorCCT[i].Para;
			   }
			   break;
		   case SENSOR_FEATURE_SET_ENG_REGISTER:
			   SensorRegNumber=ENGINEER_END;
			   for (i=0;i<SensorRegNumber;i++)
			   {
				   S5K9A1SensorReg[i].Addr=*pFeatureData32++;
				   S5K9A1SensorReg[i].Para=*pFeatureData32++;
			   }
			   break;
		   case SENSOR_FEATURE_GET_ENG_REGISTER:
			   SensorRegNumber=ENGINEER_END;
			   if (*pFeatureParaLen<(SensorRegNumber*sizeof(SENSOR_REG_STRUCT)+4))
				   return FALSE;
			   *pFeatureData32++=SensorRegNumber;
			   for (i=0;i<SensorRegNumber;i++)
			   {
				   *pFeatureData32++=S5K9A1SensorReg[i].Addr;
				   *pFeatureData32++=S5K9A1SensorReg[i].Para;
			   }
			   break;
		   case SENSOR_FEATURE_GET_REGISTER_DEFAULT:
			   if (*pFeatureParaLen>=sizeof(NVRAM_SENSOR_DATA_STRUCT))
			   {
				   pSensorDefaultData->Version=NVRAM_CAMERA_SENSOR_FILE_VERSION;
				   pSensorDefaultData->SensorId=S5K9A1_SENSOR_ID;
				   memcpy(pSensorDefaultData->SensorEngReg, S5K9A1SensorReg, sizeof(SENSOR_REG_STRUCT)*ENGINEER_END);
				   memcpy(pSensorDefaultData->SensorCCTReg, S5K9A1SensorCCT, sizeof(SENSOR_REG_STRUCT)*FACTORY_END_ADDR);
			   }
			   else
				   return FALSE;
			   *pFeatureParaLen=sizeof(NVRAM_SENSOR_DATA_STRUCT);
			   break;
		   case SENSOR_FEATURE_GET_CONFIG_PARA:
			   break;
		   case SENSOR_FEATURE_CAMERA_PARA_TO_SENSOR:
			   //S5K9A1_camera_para_to_sensor();
			   break;
	
		   case SENSOR_FEATURE_SENSOR_TO_CAMERA_PARA:
			   //S5K9A1_sensor_to_camera_para();
			   break;
		   case SENSOR_FEATURE_GET_GROUP_COUNT:

			   break;
		   case SENSOR_FEATURE_GET_GROUP_INFO:

			   break;
		   case SENSOR_FEATURE_GET_ITEM_INFO:

			   break;
	
		   case SENSOR_FEATURE_SET_ITEM_INFO:

			   break;
	
		   case SENSOR_FEATURE_GET_ENG_INFO:
			   pSensorEngInfo->SensorId = 221;
			   pSensorEngInfo->SensorType = CMOS_SENSOR;
	
			   pSensorEngInfo->SensorOutputDataFormat = SENSOR_OUTPUT_FORMAT_RAW_Gr;
	
			   *pFeatureParaLen=sizeof(MSDK_SENSOR_ENG_INFO_STRUCT);
			   break;
		   case SENSOR_FEATURE_GET_LENS_DRIVER_ID:
			   // get the lens driver ID from EEPROM or just return LENS_DRIVER_ID_DO_NOT_CARE
			   // if EEPROM does not exist in camera module.
			   *pFeatureReturnPara32=LENS_DRIVER_ID_DO_NOT_CARE;
			   *pFeatureParaLen=4;
			   break;
	
		   case SENSOR_FEATURE_SET_VIDEO_MODE:
			   S5K9A1SetVideoMode(*pFeatureData16);
			   break;
		   case SENSOR_FEATURE_CHECK_SENSOR_ID:
			   S5K9A1GetSensorID(pFeatureReturnPara32);
			   break;
	       case SENSOR_FEATURE_SET_AUTO_FLICKER_MODE:
               S5K9A1SetAutoFlickerMode((BOOL)*pFeatureData16, *(pFeatureData16+1));
	           break;
           case SENSOR_FEATURE_SET_TEST_PATTERN:
               S5K9A1SetTestPatternMode((BOOL)*pFeatureData16);
               break;
		   case SENSOR_FEATURE_GET_TEST_PATTERN_CHECKSUM_VALUE://for factory mode auto testing 			
               *pFeatureReturnPara32= S5K9A1_TEST_PATTERN_CHECKSUM;
			   *pFeatureParaLen=4; 							
			break;	
		   case SENSOR_FEATURE_SET_MAX_FRAME_RATE_BY_SCENARIO:
			   S5K9A1MIPISetMaxFramerateByScenario((MSDK_SCENARIO_ID_ENUM)*pFeatureData32, *(pFeatureData32+1));
			   break;
		   case SENSOR_FEATURE_GET_DEFAULT_FRAME_RATE_BY_SCENARIO:
			   S5K9A1MIPIGetDefaultFramerateByScenario((MSDK_SCENARIO_ID_ENUM)*pFeatureData32, (MUINT32 *)(*(pFeatureData32+1)));
			  break;

		   default:
			   break;

    }
    return ERROR_NONE;
}	/* S5K9A1FeatureControl() */


SENSOR_FUNCTION_STRUCT	SensorFuncS5K9A1=
{
    S5K9A1Open,
    S5K9A1GetInfo,
    S5K9A1GetResolution,
    S5K9A1FeatureControl,
    S5K9A1Control,
    S5K9A1Close
};

UINT32 S5K9A1_MIPI_RAW_SensorInit(PSENSOR_FUNCTION_STRUCT *pfFunc)
{
    /* To Do : Check Sensor status here */
    if (pfFunc!=NULL)
        *pfFunc=&SensorFuncS5K9A1;

    return ERROR_NONE;
}   /* SensorInit() */

