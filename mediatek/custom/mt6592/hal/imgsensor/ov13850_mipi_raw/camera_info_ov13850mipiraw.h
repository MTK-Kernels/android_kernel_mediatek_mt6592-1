#ifndef _CAMERA_INFO_OV13850RAW_H
#define _CAMERA_INFO_OV13850RAW_H

/*******************************************************************************
*   Configuration
********************************************************************************/
#define SENSOR_ID                           OV13850_SENSOR_ID
#define SENSOR_DRVNAME                      SENSOR_DRVNAME_OV13850_MIPI_RAW
#define INCLUDE_FILENAME_ISP_REGS_PARAM     "camera_isp_regs_ov13850mipiraw.h"
#define INCLUDE_FILENAME_ISP_PCA_PARAM      "camera_isp_pca_ov13850mipiraw.h"
#define INCLUDE_FILENAME_ISP_LSC_PARAM      "camera_isp_lsc_ov13850mipiraw.h"
#define INCLUDE_FILENAME_TSF_PARA           "camera_tsf_para_ov13850mipiraw.h"
#define INCLUDE_FILENAME_TSF_DATA           "camera_tsf_data_ov13850mipiraw.h"
/*******************************************************************************
*
********************************************************************************/

#if defined(ISP_SUPPORT)

#define OV13850MIPIRAW_CAMERA_AUTO_DSC CAM_AUTO_DSC
#define OV13850MIPIRAW_CAMERA_PORTRAIT CAM_PORTRAIT
#define OV13850MIPIRAW_CAMERA_LANDSCAPE CAM_LANDSCAPE
#define OV13850MIPIRAW_CAMERA_SPORT CAM_SPORT
#define OV13850MIPIRAW_CAMERA_FLOWER CAM_FLOWER
#define OV13850MIPIRAW_CAMERA_NIGHTSCENE CAM_NIGHTSCENE
#define OV13850MIPIRAW_CAMERA_DOCUMENT CAM_DOCUMENT
#define OV13850MIPIRAW_CAMERA_ISO_ANTI_HAND_SHAKE CAM_ISO_ANTI_HAND_SHAKE
#define OV13850MIPIRAW_CAMERA_ISO100 CAM_ISO100
#define OV13850MIPIRAW_CAMERA_ISO200 CAM_ISO200
#define OV13850MIPIRAW_CAMERA_ISO400 CAM_ISO400
#define OV13850MIPIRAW_CAMERA_ISO800 CAM_ISO800
#define OV13850MIPIRAW_CAMERA_ISO1600 CAM_ISO1600
#define OV13850MIPIRAW_CAMERA_VIDEO_AUTO CAM_VIDEO_AUTO
#define OV13850MIPIRAW_CAMERA_VIDEO_NIGHT CAM_VIDEO_NIGHT
#define OV13850MIPIRAW_CAMERA_NO_OF_SCENE_MODE CAM_NO_OF_SCENE_MODE

#endif
#endif
