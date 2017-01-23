#include <utils/Log.h>
#include <fcntl.h>
#include <math.h>

#include "camera_custom_nvram.h"
#include "camera_custom_sensor.h"
#include "image_sensor.h"
#include "kd_imgsensor_define.h"
#include "camera_AE_PLineTable_imx135raw.h"
#include "camera_info_imx135raw.h"
#include "camera_custom_AEPlinetable.h"

const NVRAM_CAMERA_ISP_PARAM_STRUCT CAMERA_ISP_DEFAULT_VALUE =
{{
    //Version
    Version: NVRAM_CAMERA_PARA_FILE_VERSION,

    //SensorId
    SensorId: SENSOR_ID,
    ISPComm:{
        {
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    	}
    },
    ISPPca: {
        #include INCLUDE_FILENAME_ISP_PCA_PARAM
    },
    ISPRegs:{
        #include INCLUDE_FILENAME_ISP_REGS_PARAM
    },
    ISPMfbMixer:{{
        {//00: MFB mixer for ISO 100
            0x00000000, 0x00000000
        },
        {//01: MFB mixer for ISO 200
            0x00000000, 0x00000000
        },
        {//02: MFB mixer for ISO 400
            0x00000000, 0x00000000
        },
        {//03: MFB mixer for ISO 800
            0x00000000, 0x00000000
        },
        {//04: MFB mixer for ISO 1600
            0x00000000, 0x00000000
        },
        {//05: MFB mixer for ISO 2400
            0x00000000, 0x00000000
        },
        {//06: MFB mixer for ISO 3200
            0x00000000, 0x00000000
    }
    }},
    ISPCcmPoly22:{
        75170, // i4R_AVG
        13190, // i4R_STD
        79140, // i4B_AVG
        26270, // i4B_STD
        { // i4P00[9]
            4448648, -1494813, -393843, -604477, 3414513, -250036,  85095, -1385454, 3860283 
        },
        { // i4P10[9]
            933698,  -628943, -304758, -247520,  -22220,  269740, -73861,   196166, -122555
        },
        { // i4P01[9]
            814367,  -494023, -320352, -358410, -180556,  538966, -57406,  -190454,  247689
        },
        { // i4P20[9]
            394007,  -491950,   98031,  -21525,   59812,  -38287, 140879,  -521951,  381045
        },
        { // i4P11[9]
            -35750,  -344806,  380738,  121574,   59500, -181074, 143388,  -309535,  166309
        },
        { // i4P02[9]
            -315751,    65233,  250618,  151463,   34149, -185612,  21808,    -8637,  -12997
        }        
    }
}};

const NVRAM_CAMERA_3A_STRUCT CAMERA_3A_NVRAM_DEFAULT_VALUE =
{
    NVRAM_CAMERA_3A_FILE_VERSION, // u4Version
    SENSOR_ID, // SensorId

    // AE NVRAM
    {
        // rDevicesInfo
        {
            1136,    // u4MinGain, 1024 base = 1x
            8192,    // u4MaxGain, 16x
            85,    // u4MiniISOGain, ISOxx  
            128,    // u4GainStepUnit, 1x/8 
            19770,    // u4PreExpUnit 
            31,    // u4PreMaxFrameRate
            19770,    // u4VideoExpUnit  
            31,    // u4VideoMaxFrameRate 
            1024,    // u4Video2PreRatio, 1024 base = 1x 
            13139,    // u4CapExpUnit 
            24,    // u4CapMaxFrameRate
            1024,    // u4Cap2PreRatio, 1024 base = 1x
            22,    // u4LensFno, Fno = 2.8
            350    // u4FocusLength_100x
         },
         // rHistConfig
        {
            4, // 2,   // u4HistHighThres
            40,  // u4HistLowThres
            2,   // u4MostBrightRatio
            1,   // u4MostDarkRatio
            160, // u4CentralHighBound
            20,  // u4CentralLowBound
            {240, 230, 220, 210, 200}, // u4OverExpThres[AE_CCT_STRENGTH_NUM]
            {82, 108, 128, 148, 170},  // u4HistStretchThres[AE_CCT_STRENGTH_NUM]
            {18, 22, 26, 30, 34}       // u4BlackLightThres[AE_CCT_STRENGTH_NUM]
        },
        // rCCTConfig
        {
            TRUE,            // bEnableBlackLight
            TRUE,            // bEnableHistStretch
            FALSE,           // bEnableAntiOverExposure
            TRUE,            // bEnableTimeLPF
            TRUE,            // bEnableCaptureThres
            TRUE,            // bEnableVideoThres
            TRUE,            // bEnableStrobeThres
            47,                // u4AETarget
            47,                // u4StrobeAETarget

            50,                // u4InitIndex
            4,                 // u4BackLightWeight
            32,                // u4HistStretchWeight
            4,                 // u4AntiOverExpWeight
            2,                 // u4BlackLightStrengthIndex
            0, // 2,                 // u4HistStretchStrengthIndex
            2,                 // u4AntiOverExpStrengthIndex
            2,                 // u4TimeLPFStrengthIndex
            {1, 3, 5, 7, 8}, // u4LPFConvergeTable[AE_CCT_STRENGTH_NUM]
            90,                // u4InDoorEV = 9.0, 10 base
            -12,               // i4BVOffset delta BV = -2.3
            64,                 // u4PreviewFlareOffset
            64,                 // u4CaptureFlareOffset
            1,                 // u4CaptureFlareThres
            64,                 // u4VideoFlareOffset
            1,                 // u4VideoFlareThres
            32,                 // u4StrobeFlareOffset
            1,                 // u4StrobeFlareThres
            160,                 // u4PrvMaxFlareThres
            0,                 // u4PrvMinFlareThres
            160,                 // u4VideoMaxFlareThres
            0,                 // u4VideoMinFlareThres            
            18,                // u4FlatnessThres              // 10 base for flatness condition.
            75                 // u4FlatnessStrength
         }
    },

    // AWB NVRAM
{								
	// AWB calibration data							
	{							
		// rCalGain (calibration gain: 1.0 = 512)						
		{						
			0,	// u4R				
			0,	// u4G				
			0	// u4B				
		},						
		// rDefGain (Default calibration gain: 1.0 = 512)						
		{						
			0,	// u4R				
			0,	// u4G				
			0	// u4B				
		},						
		// rDefGain (Default calibration gain: 1.0 = 512)						
		{						
			0,	// u4R				
			0,	// u4G				
			0	// u4B				
		},						
		// rD65Gain (D65 WB gain: 1.0 = 512)						
		{						
			1040,	// u4R				
			512,	// u4G				
			678	// u4B				
		}						
	},							
	// Original XY coordinate of AWB light source							
	{							
		// Strobe						
		{						
			99,	// i4X				
			-345	// i4Y				
		},						
		// Horizon						
		{						
			-368,	// i4X				
			-383	// i4Y				
		},						
		// A						
		{						
			-245,	// i4X				
			-397	// i4Y				
		},						
		// TL84						
		{						
			-97,	// i4X				
			-366	// i4Y				
		},						
		// CWF						
		{						
			-79,	// i4X				
			-435	// i4Y				
		},						
		// DNP						
		{						
			44,	// i4X				
			-417	// i4Y				
		},						
		// D65						
		{						
			158,	// i4X				
			-365	// i4Y				
		},						
		// DF						
		{						
			0, 	// i4X				
			0	// i4Y				
		}						
	},							
	// Rotated XY coordinate of AWB light source							
	{							
		// Strobe						
		{						
			81,	// i4X				
			-350	// i4Y				
		},						
		// Horizon						
		{						
			-387,	// i4X				
			-364	// i4Y				
		},						
		// A						
		{						
			-265,	// i4X				
			-385	// i4Y				
		},						
		// TL84						
		{						
			-116,	// i4X				
			-361	// i4Y				
		},						
		// CWF						
		{						
			-101,	// i4X				
			-431	// i4Y				
		},						
		// DNP						
		{						
			23,	// i4X				
			-419	// i4Y				
		},						
		// D65						
		{						
			139,	// i4X				
			-373	// i4Y				
		},						
		// DF						
		{						
			0,	// i4X				
			0	// i4Y				
		}						
	},							
	// AWB gain of AWB light source							
	{							
		// Strobe						
		{						
			934,	// u4R				
			512,	// u4G				
			714	// u4B				
		},						
		// Horizon						
		{						
			523,	// u4R				
			512,	// u4G				
			1415	// u4B				
		},						
		// A						
		{						
			629,	// u4R				
			512,	// u4G				
			1220	// u4B				
		},						
		// TL84						
		{						
			737,	// u4R				
			512,	// u4G				
			958	// u4B				
		},						
		// CWF						
		{						
			830,	// u4R				
			512,	// u4G				
			1026	// u4B				
		},						
		// DNP						
		{						
			956,	// u4R				
			512,	// u4G				
			848	// u4B				
		},						
		// D65						
		{						
			1040,	// u4R				
			512,	// u4G				
			678	// u4B				
		},						
		// DF						
		{						
			512,	// u4R				
			512,	// u4G				
			512 	// u4B				
		}						
	},							
	// Rotation matrix parameter							
	{							
		3,	// i4RotationAngle					
		256,	// i4Cos					
		13,	// i4Sin					
	},							
	// Daylight locus parameter							
	{							
		-140,	// i4SlopeNumerator					
		128	// i4SlopeDenominator					
	},							
	// AWB light area							
	{							
		// Strobe						
		{						
			184,	// i4RightBound				
			-7,	// i4LeftBound				
			-273,	// i4UpperBound				
			-433	// i4LowerBound				
		},						
		// Tungsten						
		{						
			-166,	// i4RightBound				
			-816,	// i4LeftBound				
			-325,	// i4UpperBound				
			-425	// i4LowerBound				
		},						
		// Warm fluorescent						
		{						
			-166,	// i4RightBound				
			-816,	// i4LeftBound				
			-425,	// i4UpperBound				
			-545	// i4LowerBound				
		},						
		// Fluorescent						
		{						
			-27,	// i4RightBound				
			-166,	// i4LeftBound				
			-311,	// i4UpperBound				
			-396	// i4LowerBound				
		},						
		// CWF						
		{						
			-27,	// i4RightBound				
			-166,	// i4LeftBound				
			-396,	// i4UpperBound				
			-481	// i4LowerBound				
		},						
		// Daylight						
		{						
			164,	// i4RightBound				
			-27,	// i4LeftBound				
			-308,	// i4UpperBound				
			-453	// i4LowerBound				
		},						
		// Shade						
		{						
			524,	// i4RightBound				
			164,	// i4LeftBound				
			-308,	// i4UpperBound				
			-453	// i4LowerBound				
		},						
		// Daylight Fluorescent						
		{						
			0,	// i4RightBound				
			0,	// i4LeftBound				
			0,	// i4UpperBound				
			0	// i4LowerBound				
		}						
	},							
	// PWB light area							
	{							
		// Reference area						
		{						
			524,	// i4RightBound				
			-816,	// i4LeftBound				
			-283,	// i4UpperBound				
			-545	// i4LowerBound				
		},						
		// Daylight						
		{						
			189,	// i4RightBound				
			-27,	// i4LeftBound				
			-308,	// i4UpperBound				
			-453	// i4LowerBound				
		},						
		// Cloudy daylight						
		{						
			289,	// i4RightBound				
			114,	// i4LeftBound				
			-308,	// i4UpperBound				
			-453	// i4LowerBound				
		},						
		// Shade						
		{						
			389,	// i4RightBound				
			114,	// i4LeftBound				
			-308,	// i4UpperBound				
			-453	// i4LowerBound				
		},						
		// Twilight						
		{						
			-27,	// i4RightBound				
			-187,	// i4LeftBound				
			-308,	// i4UpperBound				
			-453	// i4LowerBound				
		},						
		// Fluorescent						
		{						
			189,	// i4RightBound				
			-216,	// i4LeftBound				
			-311,	// i4UpperBound				
			-481	// i4LowerBound				
		},						
		// Warm fluorescent						
		{						
			-165,	// i4RightBound				
			-365,	// i4LeftBound				
			-311,	// i4UpperBound				
			-481	// i4LowerBound				
		},						
		// Incandescent						
		{						
			-165,	// i4RightBound				
			-365,	// i4LeftBound				
			-308,	// i4UpperBound				
			-453	// i4LowerBound				
		},						
		// Gray World						
		{						
			5000,	// i4RightBound				
			-5000,	// i4LeftBound				
			5000,	// i4UpperBound				
			-5000	// i4LowerBound				
		}						
	},							
	// PWB default gain							
	{							
		// Daylight						
		{						
			975,	// u4R				
			512,	// u4G				
			743	// u4B				
		},						
		// Cloudy daylight						
		{						
			1137,	// u4R				
			512,	// u4G				
			626	// u4B				
		},						
		// Shade						
		{						
			1213,	// u4R				
			512,	// u4G				
			584	// u4B				
		},						
		// Twilight						
		{						
			766,	// u4R				
			512,	// u4G				
			971	// u4B				
		},						
		// Fluorescent						
		{						
			883,	// u4R				
			512,	// u4G				
			867	// u4B				
		},						
		// Warm fluorescent						
		{						
			639,	// u4R				
			512,	// u4G				
			1239	// u4B				
		},						
		// Incandescent						
		{						
			625,	// u4R				
			512,	// u4G				
			1214	// u4B				
		},						
		// Gray World						
		{						
			512,	// u4R				
			512,	// u4G				
			512	// u4B				
		}						
	},							
	// AWB preference color							
	{							
		// Tungsten						
		{						
			50,	// i4SliderValue				
			4159	// i4OffsetThr				
		},						
		// Warm fluorescent						
		{						
			50,	// i4SliderValue				
			4159	// i4OffsetThr				
		},						
		// Shade						
		{						
			50,	// i4SliderValue				
			841	// i4OffsetThr				
		},						
		// Daylight WB gain						
		{						
			895,	// u4R				
			512,	// u4G				
			799	// u4B				
		},						
		// Preference gain: strobe						
		{						
			512,	// u4R				
			512,	// u4G				
			512	// u4B				
		},						
		// Preference gain: tungsten						
		{						
			512,	// u4R				
			512,	// u4G				
			512	// u4B				
		},						
		// Preference gain: warm fluorescent						
		{						
			512,	// u4R				
			512,	// u4G				
			512	// u4B				
		},						
		// Preference gain: fluorescent						
		{						
			512,	// u4R				
			512,	// u4G				
			512	// u4B				
		},						
		// Preference gain: CWF						
		{						
			512,	// u4R				
			512,	// u4G				
			512	// u4B				
		},						
		// Preference gain: daylight						
		{						
			512,	// u4R				
			512,	// u4G				
			512	// u4B				
		},						
		// Preference gain: shade						
		{						
			512,	// u4R				
			512,	// u4G				
			512	// u4B				
		},						
		// Preference gain: daylight fluorescent						
		{						
			512,	// u4R				
			512,	// u4G				
			512	// u4B				
		}						
	},							
	// CCT estimation							
	{							
		// CCT						
		{						
			2300,	// i4CCT[0]				
			2850,	// i4CCT[1]				
			4100,	// i4CCT[2]				
			5100,	// i4CCT[3]				
			6500 	// i4CCT[4]				
		},						
		// Rotated X coordinate						
		{						
			-526,	// i4RotatedXCoordinate[0]				
			-404,	// i4RotatedXCoordinate[1]				
			-255,	// i4RotatedXCoordinate[2]				
			-116,	// i4RotatedXCoordinate[3]				
			0 	// i4RotatedXCoordinate[4]				
		}						
	}							
    },
	{0}
};

#include INCLUDE_FILENAME_ISP_LSC_PARAM
//};  //  namespace


typedef NSFeature::RAWSensorInfo<SENSOR_ID> SensorInfoSingleton_T;


namespace NSFeature {
template <>
UINT32
SensorInfoSingleton_T::
impGetDefaultData(CAMERA_DATA_TYPE_ENUM const CameraDataType, VOID*const pDataBuf, UINT32 const size) const
{
    UINT32 dataSize[CAMERA_DATA_TYPE_NUM] = {sizeof(NVRAM_CAMERA_ISP_PARAM_STRUCT),
                                             sizeof(NVRAM_CAMERA_3A_STRUCT),
                                             sizeof(NVRAM_CAMERA_SHADING_STRUCT),
                                             sizeof(NVRAM_LENS_PARA_STRUCT),
                                             sizeof(AE_PLINETABLE_T)};

    if (CameraDataType > CAMERA_DATA_AE_PLINETABLE || NULL == pDataBuf || (size < dataSize[CameraDataType]))
    {
        return 1;
    }

    switch(CameraDataType)
    {
        case CAMERA_NVRAM_DATA_ISP:
            memcpy(pDataBuf,&CAMERA_ISP_DEFAULT_VALUE,sizeof(NVRAM_CAMERA_ISP_PARAM_STRUCT));
            break;
        case CAMERA_NVRAM_DATA_3A:
            memcpy(pDataBuf,&CAMERA_3A_NVRAM_DEFAULT_VALUE,sizeof(NVRAM_CAMERA_3A_STRUCT));
            break;
        case CAMERA_NVRAM_DATA_SHADING:
            memcpy(pDataBuf,&CAMERA_SHADING_DEFAULT_VALUE,sizeof(NVRAM_CAMERA_SHADING_STRUCT));
            break;
        case CAMERA_DATA_AE_PLINETABLE:
            memcpy(pDataBuf,&g_PlineTableMapping,sizeof(AE_PLINETABLE_T));
            break;
        default:
            break;
    }
    return 0;
}};  //  NSFeature


