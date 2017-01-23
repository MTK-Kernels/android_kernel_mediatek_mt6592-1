#include <utils/Log.h>
#include <fcntl.h>
#include <math.h>

#include "camera_custom_nvram.h" 
#include "camera_custom_sensor.h"
#include "image_sensor.h"
#include "kd_imgsensor_define.h"
#include "camera_AE_PLineTable_imx134raw.h"
#include "camera_info_imx134raw.h"
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
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
        }
    },
    ISPPca:{
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
        70750,    // i4R_AVG
        16171,    // i4R_STD
        102600,    // i4B_AVG
        25061,    // i4B_STD
        { // i4P00[9]
            5287500, -2652500, -77500, -995000, 4225000, -667500, -120000, -2100000, 4780000
        },
        { // i4P10[9]
            1088893, -1248890, 158221, -23788, 26606, -14207, 10785, -240524, 229739
        },
        { // i4P01[9]
            824147, -917054, 87485, -213520, 23702, 170857, -93874, -929138, 1023012
        },
        { // i4P20[9]
            0, 0, 0, 0, 0, 0, 0, 0, 0
        },
        { // i4P11[9]
            0, 0, 0, 0, 0, 0, 0, 0, 0
        },
        { // i4P02[9]
            0, 0, 0, 0, 0, 0, 0, 0, 0
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
            10928,    // u4MaxGain, 16x
            100,    // u4MiniISOGain, ISOxx  
            128,    // u4GainStepUnit, 1x/8 
            21,    // u4PreExpUnit 
            30,    // u4PreMaxFrameRate
            17,    // u4VideoExpUnit  
            31,    // u4VideoMaxFrameRate 
            1024,    // u4Video2PreRatio, 1024 base = 1x 
            17,    // u4CapExpUnit 
            25,    // u4CapMaxFrameRate
            1024,    // u4Cap2PreRatio, 1024 base = 1x
            28,    // u4LensFno, Fno = 2.8
            350    // u4FocusLength_100x
        },
        // rHistConfig
        {
            2,    // u4HistHighThres
            40,    // u4HistLowThres
            2,    // u4MostBrightRatio
            1,    // u4MostDarkRatio
            160,    // u4CentralHighBound
            20,    // u4CentralLowBound
            {240, 230, 220, 210, 200},    // u4OverExpThres[AE_CCT_STRENGTH_NUM] 
            {86, 108, 128, 148, 170},    // u4HistStretchThres[AE_CCT_STRENGTH_NUM] 
            {18, 22, 26, 30, 34}    // u4BlackLightThres[AE_CCT_STRENGTH_NUM] 
        },
        // rCCTConfig
        {
            TRUE,    // bEnableBlackLight
            TRUE,    // bEnableHistStretch
            FALSE,    // bEnableAntiOverExposure
            TRUE,    // bEnableTimeLPF
            FALSE,    // bEnableCaptureThres
            FALSE,    // bEnableVideoThres
            FALSE,    // bEnableStrobeThres
            50,    // u4AETarget
            0,    // u4StrobeAETarget
            50,    // u4InitIndex
            4,    // u4BackLightWeight
            32,    // u4HistStretchWeight
            4,    // u4AntiOverExpWeight
            2,    // u4BlackLightStrengthIndex
            2,    // u4HistStretchStrengthIndex
            2,    // u4AntiOverExpStrengthIndex
            2,    // u4TimeLPFStrengthIndex
            {1, 3, 5, 7, 8},    // u4LPFConvergeTable[AE_CCT_STRENGTH_NUM] 
            90,    // u4InDoorEV = 9.0, 10 base 
            -12,    // i4BVOffset delta BV = value/10 
            64,    // u4PreviewFlareOffset
            64,    // u4CaptureFlareOffset
            5,    // u4CaptureFlareThres
            64,    // u4VideoFlareOffset
            5,    // u4VideoFlareThres
            2,    // u4StrobeFlareOffset
            2,    // u4StrobeFlareThres
            8,    // u4PrvMaxFlareThres
            0,    // u4PrvMinFlareThres
            8,    // u4VideoMaxFlareThres
            0,    // u4VideoMinFlareThres
            18,    // u4FlatnessThres    // 10 base for flatness condition.
            75    // u4FlatnessStrength
        }
    },
    // AWB NVRAM
    {
        // AWB calibration data
        {
            // rUnitGain (unit gain: 1.0 = 512)
            {
                0,    // i4R
                0,    // i4G
                0    // i4B
            },
            // rGoldenGain (golden sample gain: 1.0 = 512)
            {
                0,    // i4R
                0,    // i4G
                0    // i4B
            },
            // rTuningUnitGain (Tuning sample unit gain: 1.0 = 512)
            {
                0,    // i4R
                0,    // i4G
                0    // i4B
            },
            // rD65Gain (D65 WB gain: 1.0 = 512)
            {
                906,    // i4R
                512,    // i4G
                696    // i4B
            }
        },
        // Original XY coordinate of AWB light source
        {
           // Strobe
            {
                -316,    // i4X
                -378    // i4Y
            },
            // Horizon
            {
                -435,    // i4X
                -373    // i4Y
            },
            // A
            {
                -316,    // i4X
                -378    // i4Y
            },
            // TL84
            {
                -171,    // i4X
                -325    // i4Y
            },
            // CWF
            {
                -156,    // i4X
                -387    // i4Y
            },
            // DNP
            {
                -37,    // i4X
                -373    // i4Y
            },
            // D65
            {
                98,    // i4X
                -324    // i4Y
            },
            // DF
            {
                0,    // i4X
                0    // i4Y
            }
        },
        // Rotated XY coordinate of AWB light source
        {
            // Strobe
            {
                -347,    // i4X
                -349    // i4Y
            },
            // Horizon
            {
                -465,    // i4X
                -334    // i4Y
            },
            // A
            {
                -347,    // i4X
                -349    // i4Y
            },
            // TL84
            {
                -198,    // i4X
                -309    // i4Y
            },
            // CWF
            {
                -189,    // i4X
                -372    // i4Y
            },
            // DNP
            {
                -69,    // i4X
                -368    // i4Y
            },
            // D65
            {
                70,    // i4X
                -331    // i4Y
            },
            // DF
            {
                0,    // i4X
                0    // i4Y
            }
        },
        // AWB gain of AWB light source
        {
            // Strobe 
            {
                556,    // i4R
                512,    // i4G
                1310    // i4B
            },
            // Horizon 
            {
                512,    // i4R
                556,    // i4G
                1662    // i4B
            },
            // A 
            {
                556,    // i4R
                512,    // i4G
                1310    // i4B
            },
            // TL84 
            {
                631,    // i4R
                512,    // i4G
                1003    // i4B
            },
            // CWF 
            {
                700,    // i4R
                512,    // i4G
                1067    // i4B
            },
            // DNP 
            {
                807,    // i4R
                512,    // i4G
                892    // i4B
            },
            // D65 
            {
                906,    // i4R
                512,    // i4G
                696    // i4B
            },
            // DF 
            {
                512,    // i4R
                512,    // i4G
                512    // i4B
            }
        },
        // Rotation matrix parameter
        {
            5,    // i4RotationAngle
            255,    // i4Cos
            22    // i4Sin
        },
        // Daylight locus parameter
        {
            -154,    // i4SlopeNumerator
            128    // i4SlopeDenominator
        },
        // AWB light area
        {
            // Strobe:FIXME
            {
            0,    // i4RightBound
            0,    // i4LeftBound
            0,    // i4UpperBound
            0    // i4LowerBound
            },
            // Tungsten
            {
            -248,    // i4RightBound
            -898,    // i4LeftBound
            -291,    // i4UpperBound
            -391    // i4LowerBound
            },
            // Warm fluorescent
            {
            -248,    // i4RightBound
            -898,    // i4LeftBound
            -391,    // i4UpperBound
            -511    // i4LowerBound
            },
            // Fluorescent
            {
            -119,    // i4RightBound
            -248,    // i4LeftBound
            -244,    // i4UpperBound
            -340    // i4LowerBound
            },
            // CWF
            {
            -119,    // i4RightBound
            -248,    // i4LeftBound
            -340,    // i4UpperBound
            -422    // i4LowerBound
            },
            // Daylight
            {
            95,    // i4RightBound
            -119,    // i4LeftBound
            -251,    // i4UpperBound
            -411    // i4LowerBound
            },
            // Shade
            {
            455,    // i4RightBound
            95,    // i4LeftBound
            -251,    // i4UpperBound
            -411    // i4LowerBound
            },
            // Daylight Fluorescent
            {
            95,    // i4RightBound
            -119,    // i4LeftBound
            -411,    // i4UpperBound
            -502    // i4LowerBound
            }
        },
        // PWB light area
        {
            // Reference area
            {
            455,    // i4RightBound
            -898,    // i4LeftBound
            0,    // i4UpperBound
            -511    // i4LowerBound
            },
            // Daylight
            {
            120,    // i4RightBound
            -119,    // i4LeftBound
            -251,    // i4UpperBound
            -411    // i4LowerBound
            },
            // Cloudy daylight
            {
            220,    // i4RightBound
            45,    // i4LeftBound
            -251,    // i4UpperBound
            -411    // i4LowerBound
            },
            // Shade
            {
            320,    // i4RightBound
            45,    // i4LeftBound
            -251,    // i4UpperBound
            -411    // i4LowerBound
            },
            // Twilight
            {
            -119,    // i4RightBound
            -279,    // i4LeftBound
            -251,    // i4UpperBound
            -411    // i4LowerBound
            },
            // Fluorescent
            {
            120,    // i4RightBound
            -298,    // i4LeftBound
            -259,    // i4UpperBound
            -422    // i4LowerBound
            },
            // Warm fluorescent
            {
            -247,    // i4RightBound
            -447,    // i4LeftBound
            -259,    // i4UpperBound
            -422    // i4LowerBound
            },
            // Incandescent
            {
            -247,    // i4RightBound
            -447,    // i4LeftBound
            -251,    // i4UpperBound
            -411    // i4LowerBound
            },
            // Gray World
            {
            5000,    // i4RightBound
            -5000,    // i4LeftBound
            5000,    // i4UpperBound
            -5000    // i4LowerBound
            }
        },
        // PWB default gain	
        {
            // Daylight
            {
            832,    // i4R
            512,    // i4G
            769    // i4B
            },
            // Cloudy daylight
            {
            979,    // i4R
            512,    // i4G
            634    // i4B
            },
            // Shade
            {
            1041,    // i4R
            512,    // i4G
            589    // i4B
            },
            // Twilight
            {
            651,    // i4R
            512,    // i4G
            1031    // i4B
            },
            // Fluorescent
            {
            756,    // i4R
            512,    // i4G
            888    // i4B
            },
            // Warm fluorescent
            {
            550,    // i4R
            512,    // i4G
            1295    // i4B
            },
            // Incandescent
            {
            542,    // i4R
            512,    // i4G
            1280    // i4B
            },
            // Gray World
            {
            512,    // i4R
            512,    // i4G
            512    // i4B
            }
        },
        // AWB preference color	
        {
            // Tungsten
            {
            0,    // i4SliderValue
            6608    // i4OffsetThr
            },
            // Warm fluorescent	
            {
            0,    // i4SliderValue
            5351    // i4OffsetThr
            },
            // Shade
            {
            50,    // i4SliderValue
            342    // i4OffsetThr
            },
            // Daylight WB gain
            {
            764,    // i4R
            512,    // i4G
            852    // i4B
            },
            // Preference gain: strobe
            {
            512,    // i4R
            512,    // i4G
            512    // i4B
            },
            // Preference gain: tungsten
            {
            512,    // i4R
            512,    // i4G
            512    // i4B
            },
            // Preference gain: warm fluorescent
            {
            512,    // i4R
            512,    // i4G
            512    // i4B
            },
            // Preference gain: fluorescent
            {
            512,    // i4R
            512,    // i4G
            512    // i4B
            },
            // Preference gain: CWF
            {
            512,    // i4R
            512,    // i4G
            512    // i4B
            },
            // Preference gain: daylight
            {
            512,    // i4R
            512,    // i4G
            512    // i4B
            },
            // Preference gain: shade
            {
            512,    // i4R
            512,    // i4G
            512    // i4B
            },
            // Preference gain: daylight fluorescent
            {
            512,    // i4R
            512,    // i4G
            512    // i4B
            }
        },
        {// CCT estimation
            {// CCT
                2300,    // i4CCT[0]
                2850,    // i4CCT[1]
                4100,    // i4CCT[2]
                5100,    // i4CCT[3]
                6500    // i4CCT[4]
            },
            {// Rotated X coordinate
                -535,    // i4RotatedXCoordinate[0]
                -417,    // i4RotatedXCoordinate[1]
                -268,    // i4RotatedXCoordinate[2]
                -139,    // i4RotatedXCoordinate[3]
                0    // i4RotatedXCoordinate[4]
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
}}; // NSFeature


