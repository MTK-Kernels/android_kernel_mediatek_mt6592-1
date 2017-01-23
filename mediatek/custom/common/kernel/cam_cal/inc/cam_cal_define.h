#ifndef _CAM_CAL_DATA_H
#define _CAM_CAL_DATA_H


typedef struct{
    u32 u4Offset;
    u32 u4Length;
    u8 *  pu1Params;
}stCAM_CAL_INFO_STRUCT, *stPCAM_CAL_INFO_STRUCT;


typedef struct {
    u16 wbRG;
    u16 wbBG;
    u16 wbGbGr;
    u16 wbGoldenRG;
    u16 wbGoldenBG;
    u16 wbGoldenGbGr;  
}stCam_SENSOR_AWB_OTP;

typedef struct {
    u16 vcmStart;
    u16 vcmEnd;
}stCam_SENSOR_AF_OTP;

typedef enum
{
    OTP_ERROR_NONE=0,
    OTP_ERROR_PARAM_OR_CHECKSUM,
}emCam_OTP_READ_STATUS;

#endif //_CAM_CAL_DATA_H
