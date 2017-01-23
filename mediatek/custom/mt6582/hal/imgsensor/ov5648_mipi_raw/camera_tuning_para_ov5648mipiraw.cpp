/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 */
/* MediaTek Inc. (C) 2010. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
 * AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek Software")
 * have been modified by MediaTek Inc. All revisions are subject to any receiver's
 * applicable license agreements with MediaTek Inc.
 */

/********************************************************************************************
 *     LEGAL DISCLAIMER
 *
 *     (Header of MediaTek Software/Firmware Release or Documentation)
 *
 *     BY OPENING OR USING THIS FILE, BUYER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 *     THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE") RECEIVED
 *     FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO BUYER ON AN "AS-IS" BASIS
 *     ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES, EXPRESS OR IMPLIED,
 *     INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR
 *     A PARTICULAR PURPOSE OR NONINFRINGEMENT. NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY
 *     WHATSOEVER WITH RESPECT TO THE SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY,
 *     INCORPORATED IN, OR SUPPLIED WITH THE MEDIATEK SOFTWARE, AND BUYER AGREES TO LOOK
 *     ONLY TO SUCH THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. MEDIATEK SHALL ALSO
 *     NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE RELEASES MADE TO BUYER'S SPECIFICATION
 *     OR TO CONFORM TO A PARTICULAR STANDARD OR OPEN FORUM.
 *
 *     BUYER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND CUMULATIVE LIABILITY WITH
 *     RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE, AT MEDIATEK'S OPTION,
TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE, OR REFUND ANY SOFTWARE LICENSE
 *     FEES OR SERVICE CHARGE PAID BY BUYER TO MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 *     THE TRANSACTION CONTEMPLATED HEREUNDER SHALL BE CONSTRUED IN ACCORDANCE WITH THE LAWS
 *     OF THE STATE OF CALIFORNIA, USA, EXCLUDING ITS CONFLICT OF LAWS PRINCIPLES.
 ************************************************************************************************/
#include <utils/Log.h>
#include <fcntl.h>
#include <math.h>

#include "camera_custom_nvram.h"
#include "camera_custom_sensor.h"
#include "image_sensor.h"
#include "kd_imgsensor_define.h"
#include "camera_AE_PLineTable_ov5648mipiraw.h"
#include "camera_info_ov5648mipiraw.h"
#include "camera_custom_AEPlinetable.h"
#include "camera_custom_tsf_tbl.h"
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
        72975,    // i4R_AVG
        14953,    // i4R_STD
        86925,    // i4B_AVG
        21469,    // i4B_STD
        {  // i4P00[9]
            4850000, -1642500, -645000, -810000, 3390000, -22500, 0, -2180000, 4742500
        },
        {  // i4P10[9]
            1067436, -904155, -165306, -98074, 200262, -101802, -35546, 672412, -642032
        },
        {  // i4P01[9]
            715976, -571066, -151014, -209676, 32554, 177478, -92220, -204578, 296149
        },
        {  // i4P20[9]
            0, 0, 0, 0, 0, 0, 0, 0, 0
        },
        {  // i4P11[9]
            0, 0, 0, 0, 0, 0, 0, 0, 0
        },
        {  // i4P02[9]
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
            10240,    // u4MaxGain, 16x
            51,    // u4MiniISOGain, ISOxx  
            64,    // u4GainStepUnit, 1x/8 
            35,    // u4PreExpUnit 
            30,     // u4PreMaxFrameRate
            35,    // u4VideoExpUnit  
            30,     // u4VideoMaxFrameRate
            1024,   // u4Video2PreRatio, 1024 base = 1x
            35,    // u4CapExpUnit 
            15,     // u4CapMaxFrameRate
            1024,   // u4Cap2PreRatio, 1024 base = 1x
            24,    // u4LensFno, Fno = 2.8
            350     // u4FocusLength_100x
         },
         // rHistConfig
        {
            2,   // u4HistHighThres
            40,  // u4HistLowThres
            2,   // u4MostBrightRatio
            1,   // u4MostDarkRatio
            160, // u4CentralHighBound
            20,  // u4CentralLowBound
            {240, 230, 220, 210, 200}, // u4OverExpThres[AE_CCT_STRENGTH_NUM]
            {86, 108, 128, 148, 170},  // u4HistStretchThres[AE_CCT_STRENGTH_NUM]
            {18, 22, 40, 30, 34}    // u4BlackLightThres[AE_CCT_STRENGTH_NUM] 
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
            35,    // u4AETarget
            0,    // u4StrobeAETarget

            50,                // u4InitIndex
            32,                 // u4BackLightWeight
            16,    // u4HistStretchWeight
            4,                 // u4AntiOverExpWeight
            2,                 // u4BlackLightStrengthIndex
            2,                 // u4HistStretchStrengthIndex
            2,                 // u4AntiOverExpStrengthIndex
            2,                 // u4TimeLPFStrengthIndex
            {1, 3, 5, 7, 8}, // u4LPFConvergeTable[AE_CCT_STRENGTH_NUM]
            90,                // u4InDoorEV = 9.0, 10 base
            -7,    // i4BVOffset delta BV = value/10 
            16,    // u4PreviewFlareOffset
            16,    // u4CaptureFlareOffset
            16,    // u4CaptureFlareThres
            16,    // u4VideoFlareOffset
            5,                 // u4VideoFlareThres
            8,    // u4StrobeFlareOffset
            2,                 // u4StrobeFlareThres
            8,                 // u4PrvMaxFlareThres
            0,                 // u4PrvMinFlareThres
            8,                 // u4VideoMaxFlareThres
            0,                 // u4VideoMinFlareThres            
            18,                // u4FlatnessThres              // 10 base for flatness condition.
            75                 // u4FlatnessStrength
         }
    },

    // AWB NVRAM
    {
    	// AWB calibration data
    	{
    		// rUnitGain (unit gain: 1.0 = 512)
    		{
    			0,	// i4R
    			0,	// i4G
    			0	// i4B
    		},
    		// rGoldenGain (golden sample gain: 1.0 = 512)
    		{
	            0,	// i4R
	            0,	// i4G
	            0	// i4B
            },
    		// rTuningUnitGain (Tuning sample unit gain: 1.0 = 512)
    		{
	            0,	// i4R
	            0,	// i4G
	            0	// i4B
            },
            // rD65Gain (D65 WB gain: 1.0 = 512)
            {
                863,    // i4R
                512,    // i4G
                576    // i4B
    		}
    	},
    	// Original XY coordinate of AWB light source
    	{
           // Strobe
            {
                37,    // i4X
                -406    // i4Y
            },
            // Horizon
            {
                -401,    // i4X
                -280    // i4Y
            },
            // A
            {
                -272,    // i4X
                -281    // i4Y
            },
            // TL84
            {
                -64,    // i4X
                -331    // i4Y
            },
            // CWF
            {
                -40,    // i4X
                -410    // i4Y
            },
            // DNP
            {
                1,    // i4X
                0    // i4Y
            },
            // D65
            {
                149,    // i4X
                -236    // i4Y
            },
            // DF
            {
                143,    // i4X
                -356    // i4Y
            }
        },
        // Rotated XY coordinate of AWB light source
        {
            // Strobe
            {
                -12,    // i4X
                -407    // i4Y
            },
            // Horizon
            {
                -432,    // i4X
                -229    // i4Y
            },
            // A
            {
                -304,    // i4X
                -246    // i4Y
            },
            // TL84
            {
                -104,    // i4X
                -321    // i4Y
            },
            // CWF
            {
                -89,    // i4X
                -402    // i4Y
            },
            // DNP
            {
                1,    // i4X
                0    // i4Y
            },
            // D65
            {
                119,    // i4X
                -252    // i4Y
            },
            // DF
            {
                99,    // i4X
                -371    // i4Y
    		}
    	},
	// AWB gain of AWB light source
	{
		// Strobe
            {
                933,    // i4R
                512,    // i4G
                845    // i4B
            },
            // Horizon 
            {
                512,    // i4R
                604,    // i4G
                1518    // i4B
            },
            // A 
            {
                519,    // i4R
                512,    // i4G
                1082    // i4B
            },
            // TL84 
            {
                736,    // i4R
                512,    // i4G
                874    // i4B
            },
            // CWF 
            {
                845,    // i4R
                512,    // i4G
                941    // i4B
            },
            // DNP 
            {
                513,    // i4R
                512,    // i4G
                512    // i4B
            },
            // D65 
            {
                863,    // i4R
                512,    // i4G
                576    // i4B
            },
            // DF 
            {
                1007,    // i4R
                512,    // i4G
                683    // i4B
		}
	},
    	// Rotation matrix parameter
    	{
            7,    // i4RotationAngle
            254,    // i4Cos
            31    // i4Sin
        },
        // Daylight locus parameter
        {
            -159,    // i4SlopeNumerator
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
            -154,    // i4RightBound
            -804,    // i4LeftBound
            -207,    // i4UpperBound
            -307    // i4LowerBound
            },
            // Warm fluorescent
            {
            -154,    // i4RightBound
            -804,    // i4LeftBound
            -307,    // i4UpperBound
            -427    // i4LowerBound
            },
            // Fluorescent
            {
            -49,    // i4RightBound
            -154,    // i4LeftBound
            -179,    // i4UpperBound
            -350    // i4LowerBound
            },
            // CWF
            {
            -49,    // i4RightBound
            -154,    // i4LeftBound
            -350,    // i4UpperBound
            -452    // i4LowerBound
            },
            // Daylight
            {
            144,    // i4RightBound
            -49,    // i4LeftBound
            -172,    // i4UpperBound
            -332    // i4LowerBound
            },
            // Shade
            {
            504,    // i4RightBound
            144,    // i4LeftBound
            -172,    // i4UpperBound
            -332    // i4LowerBound
            },
            // Daylight Fluorescent
            {
            144,    // i4RightBound
            -49,    // i4LeftBound
            -332,    // i4UpperBound
            -450    // i4LowerBound
            }
        },
        // PWB light area
        {
            // Reference area
            {
            504,    // i4RightBound
            -804,    // i4LeftBound
            0,    // i4UpperBound
            -452    // i4LowerBound
            },
            // Daylight
            {
            169,    // i4RightBound
            -49,    // i4LeftBound
            -172,    // i4UpperBound
            -332    // i4LowerBound
            },
            // Cloudy daylight
            {
            269,    // i4RightBound
            94,    // i4LeftBound
            -172,    // i4UpperBound
            -332    // i4LowerBound
            },
            // Shade
            {
            369,    // i4RightBound
            94,    // i4LeftBound
            -172,    // i4UpperBound
            -332    // i4LowerBound
            },
            // Twilight
            {
            -49,    // i4RightBound
            -209,    // i4LeftBound
            -172,    // i4UpperBound
            -332    // i4LowerBound
            },
            // Fluorescent
            {
            169,    // i4RightBound
            -204,    // i4LeftBound
            -202,    // i4UpperBound
            -452    // i4LowerBound
            },
            // Warm fluorescent
            {
            -204,    // i4RightBound
            -404,    // i4LeftBound
            -202,    // i4UpperBound
            -452    // i4LowerBound
            },
            // Incandescent
            {
            -204,    // i4RightBound
            -404,    // i4LeftBound
            -172,    // i4UpperBound
            -332    // i4LowerBound
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
            804,    // i4R
            512,    // i4G
            630    // i4B
            },
            // Cloudy daylight
            {
            928,    // i4R
            512,    // i4G
            524    // i4B
            },
            // Shade
            {
            984,    // i4R
            512,    // i4G
            486    // i4B
            },
            // Twilight
            {
            643,    // i4R
            512,    // i4G
            837    // i4B
            },
            // Fluorescent
            {
            821,    // i4R
            512,    // i4G
            773    // i4B
            },
            // Warm fluorescent
            {
            586,    // i4R
            512,    // i4G
            1191    // i4B
            },
            // Incandescent
            {
            523,    // i4R
            512,    // i4G
            1090    // i4B
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
            7177    // i4OffsetThr
            },
            // Warm fluorescent	
            {
            0,    // i4SliderValue
            4746    // i4OffsetThr
            },
            // Shade
            {
            0,    // i4SliderValue
            1343    // i4OffsetThr
            },
            // Daylight WB gain
            {
            750,    // i4R
            512,    // i4G
            688    // i4B
            },
            // Preference gain: strobe
            {
            512,    // i4R
            512,    // i4G
            512    // i4B
            },
            // Preference gain: tungsten
            {
            508,    // i4R
            512,    // i4G
            512    // i4B
            },
            // Preference gain: warm fluorescent
            {
            508,    // i4R
            512,    // i4G
            512    // i4B
            },
            // Preference gain: fluorescent
            {
            508,    // i4R
            512,    // i4G
            512    // i4B
            },
            // Preference gain: CWF
            {
            508,    // i4R
            512,    // i4G
            512    // i4B
            },
            // Preference gain: daylight
            {
            508,    // i4R
            512,    // i4G
            512    // i4B
            },
            // Preference gain: shade
            {
            508,    // i4R
            512,    // i4G
            512    // i4B
            },
            // Preference gain: daylight fluorescent
            {
            508,    // i4R
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
                -551,    // i4RotatedXCoordinate[0]
                -423,    // i4RotatedXCoordinate[1]
                -223,    // i4RotatedXCoordinate[2]
                -118,    // i4RotatedXCoordinate[3]
                0    // i4RotatedXCoordinate[4]
    		}
    	}
    },
	{0}
};
 
#include INCLUDE_FILENAME_ISP_LSC_PARAM
//};  //  namespace
const CAMERA_TSF_TBL_STRUCT CAMERA_TSF_DEFAULT_VALUE =
{
    #include INCLUDE_FILENAME_TSF_PARA
    #include INCLUDE_FILENAME_TSF_DATA
};


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
                                             sizeof(AE_PLINETABLE_T),
                                             0,
                                             sizeof(CAMERA_TSF_TBL_STRUCT)};

    if (CameraDataType > CAMERA_DATA_TSF_TABLE || NULL == pDataBuf || (size < dataSize[CameraDataType]))
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
        case CAMERA_DATA_TSF_TABLE:
            memcpy(pDataBuf,&CAMERA_TSF_DEFAULT_VALUE,sizeof(CAMERA_TSF_TBL_STRUCT));
            break;
        default:
            break;
    }
    return 0;
}};  //  NSFeature


