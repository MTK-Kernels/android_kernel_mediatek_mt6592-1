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
//Modified for imx134 otp driver
//add liteon and sunny IMX134 module driver and para for H30
#define LOG_TAG "CamCalCamCal"

#include <cutils/xlog.h> //#include <utils/Log.h>
#include <cutils/properties.h>
#include <fcntl.h>
#include <math.h>


#include "camera_custom_nvram.h"
#include "cam_cal.h"
#include "cam_cal_define.h"
extern "C"{
//#include "cam_cal_layout.h"
#include "camera_custom_cam_cal.h"
}
#include "camera_calibration_cam_cal.h"

#include <stdio.h> //for rand?
#include <stdlib.h>  //sl121106 for atoi()//for rand?

//COMMON

#define CAM_CAL_SHOW_LOG 1
#define CAM_CAL_VER "ver8900~"   //83 : 6583, 00 : draft version 120920

#ifdef CAM_CAL_SHOW_LOG
//#define CAM_CAL_LOG(fmt, arg...)    LOGD(fmt, ##arg)
#define CAM_CAL_LOG(fmt, arg...)    XLOGD(CAM_CAL_VER " "fmt, ##arg)
#define CAM_CAL_ERR(fmt, arg...)    XLOGE(CAM_CAL_VER "Err: %5d: "fmt, __LINE__, ##arg)
#else
#define CAM_CAL_LOG(fmt, arg...)    void(0)
#define CAM_CAL_ERR(fmt, arg...)    void(0)
#endif
#define CAM_CAL_LOG_IF(cond, ...)      do { if ( (cond) ) { CAM_CAL_LOG(__VA_ARGS__); } }while(0)


static UINT32 DoCamCal2AGain(INT32 CamcamFID, UINT32 start_addr, UINT32 BlockSize, UINT32* pGetSensorCalData);


#define  MAX_CALIBRATION_LAYOUT_NUM 1

//check flagh must be same to dirver IMX134MIPI_OTP_ALL_READ
#define OTP_CHECK_FLAG 0x1F

typedef enum // : MUINT32
{
    CAM_CAL_LAYOUT_RTN_PASS = 0x0,
    CAM_CAL_LAYOUT_RTN_FAILED = 0x1,
    CAM_CAL_LAYOUT_RTN_QUEUE = 0x2
} CAM_CAL_LAYOUT_T;



typedef struct
{
    UINT16 Include; //calibration layout include this item?
    UINT32 StartAddr; // item Start Address
    UINT32 BlockSize;   //BlockSize
    UINT32 (*GetCalDataProcess)(INT32 CamcamFID, UINT32 start_addr, UINT32 BlockSize, UINT32* pGetSensorCalData);//(INT32 CamcamFID, UINT32 start_addr, UINT32 BlockSize, UINT32* pGetSensorCalData);
} CALIBRATION_ITEM_STRUCT;

typedef struct
{
    UINT32 HeaderAddr; //Header Address
    UINT32 HeaderId;   //Header ID
    UINT32 DataVer;   ////new for 658x CAM_CAL_SINGLE_EEPROM_DATA, CAM_CAL_SINGLE_OTP_DATA,CAM_CAL_N3D_DATA

    CALIBRATION_ITEM_STRUCT CalItemTbl[CAMERA_CAM_CAL_DATA_LIST];
} CALIBRATION_LAYOUT_STRUCT;

/*
//Const variable
*/

static const CALIBRATION_LAYOUT_STRUCT CalLayoutTbl[MAX_CALIBRATION_LAYOUT_NUM]=
{
    {//CALIBRATION_LAYOUT_SENSOR_OTP
        0x00000000, OTP_CHECK_FLAG, CAM_CAL_SINGLE_OTP_DATA,
        {
            {0x00000000, 0x00000000, 0x00000000, NULL}, //CAMERA_CAM_CAL_DATA_MODULE_VERSION
            {0x00000000, 0x00000000, 0x00000000, NULL}, //CAMERA_CAM_CAL_DATA_PART_NUMBER
            {0x00000000, 0x00000000, 0x00000000, NULL}, //CAMERA_CAM_CAL_DATA_SHADING_TABLE
            {0x00000001, 0x00000000, 0x00000000, DoCamCal2AGain}, //CAMERA_CAM_CAL_DATA_3A_GAIN
            {0x00000000, 0x00000000, 0x00000000, NULL}  //CAMERA_CAM_CAL_DATA_3D_GEO
        }
    }
};

/****************************************************************
//Global variable
****************************************************************/
static UINT16 LayoutType = (MAX_CALIBRATION_LAYOUT_NUM+1); //seanlin 121017 why static? Because cam_cal_drv will get data one block by one block instead of overall in one time.
static MINT32 dumpEnable=0;

static CAM_CAL_LAYOUT_T  gIsInitedCamCal = CAM_CAL_LAYOUT_RTN_QUEUE;//(CAM_CAL_LAYOUT_T)CAM_CAL_LAYOUT_RTN_QUEUE;//seanlin 121017 why static? Because cam_cal_drv will get data one block by one block instead of overall in one time.

static UINT32 ShowCmdErrorLog(CAMERA_CAM_CAL_TYPE_ENUM cmd)
{
       CAM_CAL_ERR("Return ERROR %s\n",CamCalErrString[cmd]);
       return 0;
}


/********************************************************/
//Please put your AWB+AF data funtion, here.
/********************************************************/
UINT32 DoCamCal2AGain(INT32 CamcamFID, UINT32 start_addr, UINT32 BlockSize, UINT32* pGetSensorCalData)
{
    stCAM_CAL_INFO_STRUCT  cam_calCfg = {0};
    PCAM_CAL_DATA_STRUCT pCamCalData = (PCAM_CAL_DATA_STRUCT)pGetSensorCalData;
  
    UINT32 ioctlerr = 0;
    UINT32 err =  CamCalReturnErr[pCamCalData->Command];

    stCam_SENSOR_AWB_OTP AwbOtp = {0};
    stCam_SENSOR_AF_OTP AfOtp = {0};
    UINT8 AWBAFConfig = 3;
    u16 rGain = 0, gGain = 0, bGain = 0;
    u16 rGainGolden = 0, gGainGolden = 0, bGainGolden = 0;
    UINT32 AFInf = 0, AFMacro = 0;
    UINT32 AFStatus = OTP_ERROR_NONE , WBStatus = OTP_ERROR_NONE;


    CAM_CAL_LOG_IF(dumpEnable,"DoCamCal2AGain:Command:%d,  DataVer:%d\n", pCamCalData->Command, pCamCalData->DataVer );
    if(pCamCalData->DataVer >= CAM_CAL_N3D_DATA)
    {
        err = CAM_CAL_ERR_NO_DEVICE;
        CAM_CAL_ERR("Error  DataVer \n");
        ShowCmdErrorLog(pCamCalData->Command);
    }
    else if(pCamCalData->DataVer < CAM_CAL_N3D_DATA)
    { 
        if(CAMERA_CAM_CAL_DATA_3A_GAIN !=  pCamCalData->Command)
        {
            err = CamCalReturnErr[pCamCalData->Command];
            CAM_CAL_ERR(" Unkown commnad:%d\n", pCamCalData->Command);
            ShowCmdErrorLog(pCamCalData->Command);            
        }
        else
        {

            pCamCalData->Single2A.S2aVer = 0x01;
            pCamCalData->Single2A.S2aBitEn = AWBAFConfig;
            pCamCalData->Single2A.S2aAfBitflagEn = 0x0C;

            // Read AWB OTP
            cam_calCfg.u4Offset = 0;
            cam_calCfg.u4Length = sizeof(stCam_SENSOR_AWB_OTP);
            cam_calCfg.pu1Params = (u8 *)&AwbOtp;            
            ioctlerr= ioctl(CamcamFID, CAM_CALIOC_G_READ_AWB, &cam_calCfg);
            if(!ioctlerr)
            {
                AFStatus = OTP_ERROR_NONE;
                //asign otp value readed
                rGain = AwbOtp.wbRG;
                bGain= AwbOtp.wbBG;
                gGain = AwbOtp.wbGbGr;
                rGainGolden = AwbOtp.wbGoldenRG;
                bGainGolden = AwbOtp.wbGoldenBG;
                gGainGolden = AwbOtp.wbGoldenGbGr;
                CAM_CAL_LOG_IF(dumpEnable,"WB vaule read,  rGain:0x%x, bGain:0x%x, gGain:0x%x\n", rGain, bGain, gGain);               
                CAM_CAL_LOG_IF(dumpEnable,"GoldenWB vaule read, rGain:0x%x,bGain:0x%x,gGain:0x%x\n", rGainGolden, bGainGolden, gGainGolden);
                CAM_CAL_LOG_IF(dumpEnable,"Start assign value\n");

                pCamCalData->Single2A.S2aAwb.rUnitGainu4R = (u32) ((1024.0/rGain)*512);
                pCamCalData->Single2A.S2aAwb.rUnitGainu4G = 512;
                pCamCalData->Single2A.S2aAwb.rUnitGainu4B = (u32) ((1024.0/ bGain)*512);
                pCamCalData->Single2A.S2aAwb.rGoldGainu4R = (u32) ((1024.0/rGainGolden)*512);
                pCamCalData->Single2A.S2aAwb.rGoldGainu4G = 512;
                pCamCalData->Single2A.S2aAwb.rGoldGainu4B = (u32) ((1024.0/bGainGolden)*512);
        
                CAM_CAL_LOG_IF(dumpEnable,"======================AWB CAM_CAL==================\n");
                CAM_CAL_LOG_IF(dumpEnable,"[rUnitGainu4R] = %d\n", pCamCalData->Single2A.S2aAwb.rUnitGainu4R);
                CAM_CAL_LOG_IF(dumpEnable,"[rUnitGainu4G] = %d\n", pCamCalData->Single2A.S2aAwb.rUnitGainu4G);
                CAM_CAL_LOG_IF(dumpEnable,"[rUnitGainu4B] = %d\n", pCamCalData->Single2A.S2aAwb.rUnitGainu4B);
                CAM_CAL_LOG_IF(dumpEnable,"[rGoldGainu4R] = %d\n", pCamCalData->Single2A.S2aAwb.rGoldGainu4R);
                CAM_CAL_LOG_IF(dumpEnable,"[rGoldGainu4G] = %d\n", pCamCalData->Single2A.S2aAwb.rGoldGainu4G);
                CAM_CAL_LOG_IF(dumpEnable,"[rGoldGainu4B] = %d\n", pCamCalData->Single2A.S2aAwb.rGoldGainu4B);
                CAM_CAL_LOG_IF(dumpEnable,"======================AWB CAM_CAL==================\n");
            }
            else
            {
                AFStatus = OTP_ERROR_PARAM_OR_CHECKSUM;
            } 

            // Read AF OTP
            cam_calCfg.u4Offset = 0;
            cam_calCfg.u4Length = sizeof(stCam_SENSOR_AF_OTP);
            cam_calCfg.pu1Params = (u8 *)&AfOtp;
            ioctlerr= ioctl(CamcamFID, CAM_CALIOC_G_READ_AF, &cam_calCfg);

            if(!ioctlerr)
            {               
                WBStatus = OTP_ERROR_NONE;
                AFInf = AfOtp.vcmStart;
                AFMacro =AfOtp.vcmEnd;

                CAM_CAL_LOG_IF(dumpEnable,"AFInf = 0x%x,AFMacro =0x%x \n",AFInf, AFMacro);

                pCamCalData->Single2A.S2aAf[0] = AFInf;
                pCamCalData->Single2A.S2aAf[1] = AFMacro;
            }
            else
            {
                WBStatus = OTP_ERROR_PARAM_OR_CHECKSUM;
            }
            //one of awb or af otp read fails
            if ((OTP_ERROR_NONE != WBStatus) || (OTP_ERROR_NONE != AFStatus))
            {
                pCamCalData->Single2A.S2aBitEn = CAM_CAL_NONE_BITEN;
                err = CamCalReturnErr[pCamCalData->Command];
                CAM_CAL_ERR("ioctl err, WBStatus:%d, AFStatus:%d\n", WBStatus, AFStatus);
                ShowCmdErrorLog(pCamCalData->Command);
            }
            else
            {
                //WB and AF otp all read correct
                err = CAM_CAL_ERR_NO_ERR;
            }

        }        
    }    
    return err;
}

/******************************************************************************
*seanlin 121017, MT658x
*In order to get data one block by one block instead of overall data in one time
*It must extract FileID and LayoutType from CAM_CALGetCalData()
*******************************************************************************/
static UINT32 DoCamCalLayoutCheck(void)
{
    MINT32 lCamcamFID = -1;  //seanlin 121017 01 local for layout check

    UCHAR cBuf[128] = "/dev/";
    UINT32 result = CAM_CAL_ERR_NO_DEVICE;
    //cam_cal
    stCAM_CAL_INFO_STRUCT  cam_calCfg;
    UINT32 CheckID = 0, i =0 ;
    INT32 err = 0;
    switch(gIsInitedCamCal)
    {
        case CAM_CAL_LAYOUT_RTN_PASS:
            result =  CAM_CAL_ERR_NO_ERR;
            break;
        case CAM_CAL_LAYOUT_RTN_QUEUE:
        case CAM_CAL_LAYOUT_RTN_FAILED:
        default:
            result =  CAM_CAL_ERR_NO_DEVICE;
            break;
    }
    if ((gIsInitedCamCal==CAM_CAL_LAYOUT_RTN_QUEUE) && (CAM_CALInit() != CAM_CAL_NONE) && (CAM_CALDeviceName(&cBuf[0]) == 0))
    {
        lCamcamFID = open(cBuf, O_RDWR);
        CAM_CAL_LOG_IF(dumpEnable,"lCamcamFID= 0x%x", lCamcamFID);
        if(lCamcamFID == -1)
        {            
            CAM_CAL_ERR("----error: can't open CAM_CAL %s----\n",cBuf);
            gIsInitedCamCal=CAM_CAL_LAYOUT_RTN_FAILED;
            result =  CAM_CAL_ERR_NO_DEVICE;      
            return result;//0;
        }
        //read ID
        cam_calCfg.u4Offset = 0xFFFFFFFF;
        for (i = 0; i< MAX_CALIBRATION_LAYOUT_NUM; i++)
        {
            if (cam_calCfg.u4Offset != CalLayoutTbl[i].HeaderAddr)
            {
                CheckID = 0x00000000;
                cam_calCfg.u4Offset = CalLayoutTbl[i].HeaderAddr;
                cam_calCfg.u4Length = sizeof(CheckID);
                cam_calCfg.pu1Params = (u8 *)&CheckID;
                err= ioctl(lCamcamFID, CAM_CALIOC_G_READ_FLAG , &cam_calCfg);
                if(err< 0)
                {
                    CAM_CAL_ERR("ioctl err\n");
                    CAM_CAL_ERR("Read header ID fail err = 0x%x \n",err);
                    gIsInitedCamCal=CAM_CAL_LAYOUT_RTN_FAILED;
                    result =  CAM_CAL_ERR_NO_DEVICE; 
                    break;
                }
            }
            CAM_CAL_LOG_IF(dumpEnable,"Table[%d] ID= 0x%x",i, CheckID);
            if(CheckID == CalLayoutTbl[i].HeaderId)
            {
                LayoutType = i;
                gIsInitedCamCal=CAM_CAL_LAYOUT_RTN_PASS;
                result =  CAM_CAL_ERR_NO_ERR;
                break;
            }
        }
        CAM_CAL_LOG_IF(dumpEnable,"LayoutType= 0x%x",LayoutType);
        CAM_CAL_LOG_IF(dumpEnable,"result= 0x%x",result);
        ////
        close(lCamcamFID);
    }	
    else //test
    {
        CAM_CAL_LOG_IF(dumpEnable,"----gIsInitedCamCal_0x%x!----\n",gIsInitedCamCal);
        CAM_CAL_LOG_IF(dumpEnable,"----NO CAM_CAL_%s!----\n",cBuf);
        CAM_CAL_LOG_IF(dumpEnable,"----NO CCAM_CALInit_%d!----\n",CAM_CALInit());
    }
    return  result;
}

/******************************************************************************
*
*******************************************************************************/
UINT32 IMX134LITEON_CAM_CALGetCalData(UINT32* pGetSensorCalData)
{
    UCHAR cBuf[128] = "/dev/";
    UINT32 result = CAM_CAL_ERR_NO_DEVICE;

    INT32 CamcamFID = 0;  //seanlin 121017 why static? Because cam_cal_drv will get data one block by one block instead of overall in one time.

    CAMERA_CAM_CAL_TYPE_ENUM lsCommand;
    
    PCAM_CAL_DATA_STRUCT pCamCalData = (PCAM_CAL_DATA_STRUCT)pGetSensorCalData;    
    //====== Get Property ======
    char value[PROPERTY_VALUE_MAX] = {'\0'};    
    property_get("imx134camcalcamcal.log", value, "0");
    dumpEnable = atoi(value);
    //====== Get Property ======


    lsCommand = pCamCalData->Command;
    CAM_CAL_LOG_IF(dumpEnable,"pCamCalData->Command = 0x%x \n",pCamCalData->Command);
    CAM_CAL_LOG_IF(dumpEnable,"lsCommand = 0x%x \n",lsCommand);    
    //Make sure Layout is confirmed, first
    if(DoCamCalLayoutCheck()==CAM_CAL_ERR_NO_ERR)
    {  
        pCamCalData->DataVer = (CAM_CAL_DATA_VER_ENUM)CalLayoutTbl[LayoutType].DataVer;   
        if ((CAM_CALInit() != CAM_CAL_NONE) && (CAM_CALDeviceName(&cBuf[0]) == 0))
        {
            CamcamFID = open(cBuf, O_RDWR);
            if(CamcamFID == -1)
            {
                CAM_CAL_LOG_IF(dumpEnable,"----error: can't open CAM_CAL %s----\n",cBuf);
                result =  CamCalReturnErr[lsCommand];       
                ShowCmdErrorLog(lsCommand);
                return result;
            } 
            /*********************************************/
            if ((CalLayoutTbl[LayoutType].CalItemTbl[lsCommand].Include != 0) 
                &&(CalLayoutTbl[LayoutType].CalItemTbl[lsCommand].GetCalDataProcess != NULL))
            {
                result =  CalLayoutTbl[LayoutType].CalItemTbl[lsCommand].GetCalDataProcess(
                            CamcamFID, 
                            CalLayoutTbl[LayoutType].CalItemTbl[lsCommand].StartAddr, 
                            CalLayoutTbl[LayoutType].CalItemTbl[lsCommand].BlockSize, 
                            pGetSensorCalData);
            }
            else
            {
                result =  CamCalReturnErr[lsCommand];       
                ShowCmdErrorLog(lsCommand);
            }
            /*********************************************/   
            close(CamcamFID);
        }
    }
    else
    {

       result =  CamCalReturnErr[lsCommand];       
       ShowCmdErrorLog(lsCommand);
        return result;
    }         
    CAM_CAL_LOG_IF(dumpEnable,"result = 0x%x\n",result);
    return  result;
}

