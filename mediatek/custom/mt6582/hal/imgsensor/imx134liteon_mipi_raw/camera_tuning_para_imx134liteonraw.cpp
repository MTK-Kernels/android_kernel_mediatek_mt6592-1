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
//merge patch: ALPS01266310 and ALPS01415139; modify camera para for IMX134SUNNY IMX134LITEON
//add liteon and sunny IMX134 module driver and para for H30
#include <utils/Log.h>
#include <fcntl.h>
#include <math.h>

#include "camera_custom_nvram.h"
#include "camera_custom_sensor.h"
#include "image_sensor.h"
#include "kd_imgsensor_define.h"
//#include "camera_AE_PLineTable_imx134raw.h"
//#include "camera_info_imx134raw.h"
#include "camera_AE_PLineTable_imx134liteonraw.h"
#include "camera_info_imx134liteonraw.h"
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
        79075,    // i4R_AVG
        17329,    // i4R_STD
        99075,    // i4B_AVG
        22273,    // i4B_STD
        { // i4P00[9]
            4710000, -2250000, 100000, -727500, 3660000, -372500, -47500, -2165000, 4772500
        },
        { // i4P10[9]
            2057587, -2220256, 162669, 61322, -326491, 284487, 252227, 116889, -369115
        },
        { // i4P01[9]
            1719177, -1775800, 56622, -123582, -334427, 473608, 118169, -591163, 472994
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
            18,    // u4PreExpUnit 
            30,    // u4PreMaxFrameRate
            17,    // u4VideoExpUnit  
            31,    // u4VideoMaxFrameRate 
            1024,    // u4Video2PreRatio, 1024 base = 1x 
            17,    // u4CapExpUnit 
            25,    // u4CapMaxFrameRate
            1024,    // u4Cap2PreRatio, 1024 base = 1x
            20,    // u4LensFno, Fno = 2.8
            350     // u4FocusLength_100x
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
            {18, 22, 26, 32, 34}    // u4BlackLightThres[AE_CCT_STRENGTH_NUM] 
        },
        // rCCTConfig
        {
            TRUE,    // bEnableBlackLight
            TRUE,    // bEnableHistStretch
            FALSE,    // bEnableAntiOverExposure
            TRUE,    // bEnableTimeLPF
            TRUE,    // bEnableCaptureThres
            TRUE,    // bEnableVideoThres
            TRUE,    // bEnableStrobeThres
            47,    // u4AETarget
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
            -16,    // i4BVOffset delta BV = value/10 
            64,    // u4PreviewFlareOffset
            64,    // u4CaptureFlareOffset
            1,    // u4CaptureFlareThres
            64,    // u4VideoFlareOffset
            2,    // u4VideoFlareThres
            24,    // u4StrobeFlareOffset
            2,    // u4StrobeFlareThres
            160,    // u4PrvMaxFlareThres
            0,    // u4PrvMinFlareThres
            160,    // u4VideoMaxFlareThres
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
                1011,    // i4R
                512,    // i4G
                714    // i4B
            }
        },
        // Original XY coordinate of AWB light source
        {
           // Strobe
            {
                34,    // i4X
                -411    // i4Y
            },
            // Horizon
            {
                -419,    // i4X
                -359    // i4Y
            },
            // A
            {
                -291,    // i4X
                -379    // i4Y
            },
            // TL84
            {
                -99,    // i4X
                -408    // i4Y
            },
            // CWF
            {
                -84,    // i4X
                -451    // i4Y
            },
            // DNP
            {
                7,    // i4X
                -444    // i4Y
            },
            // D65
            {
                128,    // i4X
                -374    // i4Y
            },
            // DF
            {
                101,    // i4X
                -456    // i4Y
            }
        },
        // Rotated XY coordinate of AWB light source
        {
            // Strobe
            {
                34,    // i4X
                -411    // i4Y
            },
            // Horizon
            {
                -419,    // i4X
                -359    // i4Y
            },
            // A
            {
                -291,    // i4X
                -379    // i4Y
            },
            // TL84
            {
                -99,    // i4X
                -408    // i4Y
            },
            // CWF
            {
                -84,    // i4X
                -451    // i4Y
            },
            // DNP
            {
                7,    // i4X
                -444    // i4Y
            },
            // D65
            {
                128,    // i4X
                -374    // i4Y
            },
            // DF
            {
                101,    // i4X
                -456    // i4Y
            }
        },
        // AWB gain of AWB light source
        {
            // Strobe 
            {
                935,    // i4R
                512,    // i4G
                852    // i4B
            },
            // Horizon 
            {
                512,    // i4R
                555,    // i4G
                1590    // i4B
            },
            // A 
            {
                577,    // i4R
                512,    // i4G
                1268    // i4B
            },
            // TL84 
            {
                778,    // i4R
                512,    // i4G
                1017    // i4B
            },
            // CWF 
            {
                841,    // i4R
                512,    // i4G
                1057    // i4B
            },
            // DNP 
            {
                943,    // i4R
                512,    // i4G
                926    // i4B
            },
            // D65 
            {
                1011,    // i4R
                512,    // i4G
                714    // i4B
            },
            // DF 
            {
                1088,    // i4R
                512,    // i4G
                829    // i4B
            }
        },
        // Rotation matrix parameter
        {
            0,    // i4RotationAngle
            256,    // i4Cos
            0    // i4Sin
        },
        // Daylight locus parameter
        {
            -128,    // i4SlopeNumerator
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
            -149,    // i4RightBound
            -799,    // i4LeftBound
            -299,    // i4UpperBound
            -419    // i4LowerBound
            },
            // Warm fluorescent
            {
            -149,    // i4RightBound
            -799,    // i4LeftBound
            -419,    // i4UpperBound
            -539    // i4LowerBound
            },
            // Fluorescent
            {
            -43,    // i4RightBound
            -149,    // i4LeftBound
            -306,    // i4UpperBound
            -419    // i4LowerBound
            },
            // CWF
            {
            -43,    // i4RightBound
            -149,    // i4LeftBound
            -419,    // i4UpperBound
            -501    // i4LowerBound
            },
            // Daylight
            {
            143,    // i4RightBound
            -43,    // i4LeftBound
            -294,    // i4UpperBound
            -469    // i4LowerBound
            },
            // Shade
            {
            513,    // i4RightBound
            143,    // i4LeftBound
            -294,    // i4UpperBound
            -469    // i4LowerBound
            },
            // Daylight Fluorescent
            {
            128,    // i4RightBound
            -43,    // i4LeftBound
            -469,    // i4UpperBound
            -530    // i4LowerBound
            }
        },
        // PWB light area
        {
            // Reference area
            {
            513,    // i4RightBound
            -799,    // i4LeftBound
            0,    // i4UpperBound
            -539    // i4LowerBound
            },
            // Daylight
            {
            168,    // i4RightBound
            -43,    // i4LeftBound
            -294,    // i4UpperBound
            -469    // i4LowerBound
            },
            // Cloudy daylight
            {
            268,    // i4RightBound
            93,    // i4LeftBound
            -294,    // i4UpperBound
            -469    // i4LowerBound
            },
            // Shade
            {
            368,    // i4RightBound
            93,    // i4LeftBound
            -294,    // i4UpperBound
            -469    // i4LowerBound
            },
            // Twilight
            {
            -43,    // i4RightBound
            -203,    // i4LeftBound
            -294,    // i4UpperBound
            -469    // i4LowerBound
            },
            // Fluorescent
            {
            178,    // i4RightBound
            -199,    // i4LeftBound
            -324,    // i4UpperBound
            -501    // i4LowerBound
            },
            // Warm fluorescent
            {
            -191,    // i4RightBound
            -391,    // i4LeftBound
            -324,    // i4UpperBound
            -501    // i4LowerBound
            },
            // Incandescent
            {
            -191,    // i4RightBound
            -391,    // i4LeftBound
            -294,    // i4UpperBound
            -469    // i4LowerBound
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
            934,    // i4R
            512,    // i4G
            789    // i4B
            },
            // Cloudy daylight
            {
            1096,    // i4R
            512,    // i4G
            672    // i4B
            },
            // Shade
            {
            1172,    // i4R
            512,    // i4G
            628    // i4B
            },
            // Twilight
            {
            727,    // i4R
            512,    // i4G
            1014    // i4B
            },
            // Fluorescent
            {
            882,    // i4R
            512,    // i4G
            908    // i4B
            },
            // Warm fluorescent
            {
            604,    // i4R
            512,    // i4G
            1327    // i4B
            },
            // Incandescent
            {
            579,    // i4R
            512,    // i4G
            1273    // i4B
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
            6998    // i4OffsetThr
            },
            // Warm fluorescent	
            {
            0,    // i4SliderValue
            4774    // i4OffsetThr
            },
            // Shade
            {
            0,    // i4SliderValue
            1204    // i4OffsetThr
            },
            // Daylight WB gain
            {
            858,    // i4R
            512,    // i4G
            841    // i4B
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
            506,    // i4R
            512,    // i4G
            515    // i4B
            },
            // Preference gain: shade
            {
            506,    // i4R
            512,    // i4G
            515    // i4B
            },
            // Preference gain: daylight fluorescent
            {
            510,    // i4R
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
                -547,    // i4RotatedXCoordinate[0]
                -419,    // i4RotatedXCoordinate[1]
                -227,    // i4RotatedXCoordinate[2]
                -121,    // i4RotatedXCoordinate[3]
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

