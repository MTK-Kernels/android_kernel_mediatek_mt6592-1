/*******************************************************************************************/
  
  
/*******************************************************************************************/

/* SENSOR FULL SIZE */
#ifndef __OV9724_SENSOR_H
#define __OV9724_SENSOR_H   

//#define CTS_VIDEO_FPS_REQ


typedef enum group_enum {
    PRE_GAIN=0,
    CMMCLK_CURRENT,
    FRAME_RATE_LIMITATION,
    REGISTER_EDITOR,
    GROUP_TOTAL_NUMS
} FACTORY_GROUP_ENUM;


#define ENGINEER_START_ADDR 10
#define FACTORY_START_ADDR 0

typedef enum engineer_index
{
    CMMCLK_CURRENT_INDEX=ENGINEER_START_ADDR,
    ENGINEER_END
} FACTORY_ENGINEER_INDEX;

typedef enum register_index
{
	SENSOR_BASEGAIN=FACTORY_START_ADDR,
	PRE_GAIN_R_INDEX,
	PRE_GAIN_Gr_INDEX,
	PRE_GAIN_Gb_INDEX,
	PRE_GAIN_B_INDEX,
	FACTORY_END_ADDR
} FACTORY_REGISTER_INDEX;

typedef struct
{
    SENSOR_REG_STRUCT	Reg[ENGINEER_END];
    SENSOR_REG_STRUCT	CCT[FACTORY_END_ADDR];
} SENSOR_DATA_STRUCT, *PSENSOR_DATA_STRUCT;

typedef enum {
    SENSOR_MODE_INIT = 0,
    SENSOR_MODE_PREVIEW,
    SENSOR_MODE_VIDEO,
    SENSOR_MODE_CAPTURE
} OV9724_SENSOR_MODE;


typedef struct
{
	kal_uint32 DummyPixels;
	kal_uint32 DummyLines;
	
	kal_uint32 pvShutter;
	kal_uint32 pvGain;
	
	kal_uint32 pvPclk;  
	kal_uint32 videoPclk;
	kal_uint32 capPclk;
	
	kal_uint32 shutter;

	kal_uint16 sensorGlobalGain;
	kal_uint16 ispBaseGain;
	kal_uint16 realGain;

	kal_int16 imgMirror;

	OV9724_SENSOR_MODE sensorMode;

	kal_bool OV9724AutoFlickerMode;
	kal_bool OV9724VideoMode;
	
}OV9724_PARA_STRUCT,*POV9724_PARA_STRUCT;


    #define OV9724_SHUTTER_MARGIN 			(5)
	//#define OV9724_GAIN_BASE				(xx) //no need for cal gain
	
#ifdef CTS_VIDEO_FPS_REQ
	#define OV9724_AUTOFLICKER_OFFSET_30 	(304)
#else
	#define OV9724_AUTOFLICKER_OFFSET_30	(298)
#endif

	#define OV9724_AUTOFLICKER_OFFSET_15 	(146)
	
	#define OV9724_PREVIEW_PCLK 			(36600000)
	#define OV9724_VIDEO_PCLK 				(36600000)
	#define OV9724_CAPTURE_PCLK 			(36600000)
	
	#define OV9724_MAX_FPS_PREVIEW			(300)
	#define OV9724_MAX_FPS_VIDEO			(300)
	#define OV9724_MAX_FPS_CAPTURE			(300)
	//#define OV9724_MAX_FPS_N3D				(300)

	//grab window
	#define OV9724_IMAGE_SENSOR_PV_WIDTH					(1280)
	#define OV9724_IMAGE_SENSOR_PV_HEIGHT					(720)
	#define OV9724_IMAGE_SENSOR_VIDEO_WIDTH 				(1280)
	#define OV9724_IMAGE_SENSOR_VIDEO_HEIGHT				(720)
	#define OV9724_IMAGE_SENSOR_FULL_WIDTH					(1280)	
	#define OV9724_IMAGE_SENSOR_FULL_HEIGHT 				(720)

	#define OV9724_PV_X_START									(0)
	#define OV9724_PV_Y_START									(0)
	#define OV9724_FULL_X_START						    		(0)
	#define OV9724_FULL_Y_START						    		(0)
	#define OV9724_VIDEO_X_START								(0)
	#define OV9724_VIDEO_Y_START								(0)

	#define OV9724_MAX_ANALOG_GAIN					(8)
	#define OV9724_MIN_ANALOG_GAIN					(1)


	/* SENSOR PIXEL/LINE NUMBERS IN ONE PERIOD */
	#define OV9724_PV_PERIOD_PIXEL_NUMS 				0x0628	//1576
	#define OV9724_PV_PERIOD_LINE_NUMS					0x02f8	//760
	
	#define OV9724_VIDEO_PERIOD_PIXEL_NUMS				0x0628	//1576
	#define OV9724_VIDEO_PERIOD_LINE_NUMS				0x02f8	//760
	
	#define OV9724_FULL_PERIOD_PIXEL_NUMS				0x0628	//1576
	#define OV9724_FULL_PERIOD_LINE_NUMS				0x02f8	//760
	
	
	#define OV9724MIPI_WRITE_ID 	(0x20)
	#define OV9724MIPI_READ_ID	(0x21)

	#define OV9724MIPI_SENSOR_ID            OV9724_SENSOR_ID


	UINT32 OV9724MIPIOpen(void);
	UINT32 OV9724MIPIGetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution);
	UINT32 OV9724MIPIGetInfo(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_INFO_STRUCT *pSensorInfo, MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData);
	UINT32 OV9724MIPIControl(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow, MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData);
	UINT32 OV9724MIPIFeatureControl(MSDK_SENSOR_FEATURE_ENUM FeatureId, UINT8 *pFeaturePara,UINT32 *pFeatureParaLen);
	UINT32 OV9724MIPIClose(void);

#endif 

