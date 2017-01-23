// Move code from Android 4.2 base
#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <asm/atomic.h>
#include <linux/xlog.h>
#include <linux/kernel.h>

#include "kd_camera_hw.h"

#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_camera_feature.h"

/******************************************************************************
 * Debug configuration
******************************************************************************/
#define PFX "[kd_camera_hw]"
#define PK_DBG_NONE(fmt, arg...)    do {} while (0)
#define PK_DBG_FUNC(fmt, arg...)    xlog_printk(ANDROID_LOG_INFO, PFX , fmt, ##arg)

#define DEBUG_CAMERA_HW_K
#ifdef DEBUG_CAMERA_HW_K
#define PK_DBG PK_DBG_FUNC
#define PK_ERR(fmt, arg...)         xlog_printk(ANDROID_LOG_ERR, PFX , fmt, ##arg)
#define PK_XLOG_INFO(fmt, args...) \
                do {    \
                   xlog_printk(ANDROID_LOG_INFO, PFX , fmt, ##arg); \
                } while(0)
#else
#define PK_DBG(a,...)
#define PK_ERR(a,...)
#define PK_XLOG_INFO(fmt, args...)
#endif

extern void ISP_MCLK1_EN(bool En);
//Optimize G750 Power on sequence
kal_bool searchMainSensor = KAL_TRUE; //if search main sensor

int kdCISModulePowerOn(CAMERA_DUAL_CAMERA_SENSOR_ENUM SensorIdx, char *currSensorName, BOOL On, char* mode_name)
{
u32 pinSetIdx = 0;//default main sensor


#define IDNONE 0

//main
#define IMX135  1

//sub
#define S5K4E1  11
#define OV5648  12

static int cameraType = IDNONE;



#define IDX_PS_CMRST 0
#define IDX_PS_CMPDN 4

#define IDX_PS_MODE 1
#define IDX_PS_ON   2
#define IDX_PS_OFF  3

u32 pinSet[2][8] = {
                    //for main sensor
                    {GPIO_CAMERA_CMRST_PIN,
                        GPIO_CAMERA_CMRST_PIN_M_GPIO,   /* mode */
                        GPIO_OUT_ONE,                   /* ON state */
                        GPIO_OUT_ZERO,                  /* OFF state */
                     GPIO_CAMERA_CMPDN_PIN,
                        GPIO_CAMERA_CMPDN_PIN_M_GPIO,
                        GPIO_OUT_ONE,
                        GPIO_OUT_ZERO,
                    },
                    //for sub sensor
                    {GPIO_CAMERA_CMRST1_PIN,
                     GPIO_CAMERA_CMRST1_PIN_M_GPIO,
                        GPIO_OUT_ONE,
                        GPIO_OUT_ZERO,
                     GPIO_CAMERA_CMPDN1_PIN,
                        GPIO_CAMERA_CMPDN1_PIN_M_GPIO,
                        GPIO_OUT_ONE,
                        GPIO_OUT_ZERO,
                    },
                   };

    if (DUAL_CAMERA_MAIN_SENSOR == SensorIdx){
        pinSetIdx = 0;
    }
    else if (DUAL_CAMERA_SUB_SENSOR == SensorIdx) {
        pinSetIdx = 1;
    }


   if (DUAL_CAMERA_MAIN_SENSOR == SensorIdx)//search main
    {
        searchMainSensor = KAL_TRUE;
        PK_DBG("[CAMERA SENSOR] Power On Main sensor, SensorIdx:%d!\n", SensorIdx);
    }
    else//not main
    {
        searchMainSensor = KAL_FALSE;
        PK_DBG("[CAMERA SENSOR] Power On Sub sensor, SensorIdx:%d!\n", SensorIdx);
    }

    //power ON
    if (On) {


        if(DUAL_CAMERA_MAIN_SENSOR  == SensorIdx)
        {
            printk("Power on for G750T main sensor: currSensorName:%s\n",currSensorName);

            if (currSensorName && (0 == strcmp(currSensorName,"imx135mipiraw")))
            {

                cameraType = IMX135;

                //First Power Pin low and Reset Pin Low
                if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMPDN]) {
                    //PDN pin
                    if(mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_CMPDN], pinSet[pinSetIdx][IDX_PS_CMPDN+IDX_PS_MODE])){PK_DBG("[CAMERA LENS] set gpio mode failed!! \n");}
                    if(mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMPDN], GPIO_DIR_OUT)){PK_DBG("[CAMERA LENS] set gpio dir failed!! \n");}
                    if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMPDN], GPIO_OUT_ZERO)){PK_DBG("[CAMERA LENS] set gpio failed!! \n");}
                }

                if(GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMRST]) {
                    //Reset pin
                    if(mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_CMRST], pinSet[pinSetIdx][IDX_PS_CMRST+IDX_PS_MODE])){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n");}
                    if(mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMRST], GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n");}
                    if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMRST],  GPIO_OUT_ZERO)){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}
                }

                //disable inactive sensor
                if (GPIO_CAMERA_INVALID != pinSet[1 - pinSetIdx][IDX_PS_CMPDN]) {
                    //PDN pin
                    if(mt_set_gpio_mode(pinSet[1 - pinSetIdx][IDX_PS_CMPDN], pinSet[1 - pinSetIdx][IDX_PS_CMPDN+IDX_PS_MODE])){PK_DBG("[CAMERA LENS] set gpio mode failed!! \n");}
                    if(mt_set_gpio_dir(pinSet[1 - pinSetIdx][IDX_PS_CMPDN], GPIO_DIR_OUT)){PK_DBG("[CAMERA LENS] set gpio dir failed!! \n");}
                    if(mt_set_gpio_out(pinSet[1 - pinSetIdx][IDX_PS_CMPDN], GPIO_OUT_ZERO)){PK_DBG("[CAMERA LENS] set gpio failed!! \n");}
                }
                if (GPIO_CAMERA_INVALID != pinSet[1 - pinSetIdx][IDX_PS_CMRST]) {
                    //Reset pin
                    if(mt_set_gpio_mode(pinSet[1 - pinSetIdx][IDX_PS_CMRST], pinSet[1 - pinSetIdx][IDX_PS_CMRST+IDX_PS_MODE])){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n");}
                    if(mt_set_gpio_dir(pinSet[1 - pinSetIdx][IDX_PS_CMRST], GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n");}
                    if(mt_set_gpio_out(pinSet[1 - pinSetIdx][IDX_PS_CMRST],  GPIO_OUT_ZERO)){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}
                }

                //Power on VCM power
                 if(TRUE != hwPowerOn(CAMERA_POWER_VCAM_A2, VOL_2800,mode_name))
                {
                    PK_DBG("[CAMERA SENSOR] Fail to enable analog power\n");
                    goto _kdCISModulePowerOn_exit_;
                }

                mdelay(1);

                 if(TRUE != hwPowerOn(CAMERA_POWER_VCAM_D2, VOL_1800,mode_name))//IOVDD=1.8 V
                {
                  PK_DBG("[CAMERA SENSOR] Fail to enable IO power\n");
                  goto _kdCISModulePowerOn_exit_;
                }
                mdelay(1);
                if(TRUE != hwPowerOn(CAMERA_POWER_VCAM_A, VOL_2800,mode_name))//AVDD=2.8v
                {
                  PK_DBG("[CAMERA SENSOR] Fail to enable analog power\n");
                  goto _kdCISModulePowerOn_exit_;
                }
                mdelay(1);

                //Power on DVDD
                if(mt_set_gpio_mode(GPIO_VCAM_DVDD_EN, GPIO_VCAM_DVDD_EN_M_GPIO)){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n");}
                if(mt_set_gpio_dir(GPIO_VCAM_DVDD_EN, GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n");}
                if(mt_set_gpio_out(GPIO_VCAM_DVDD_EN, GPIO_OUT_ONE)){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}

                if(TRUE != hwPowerOn(CAMERA_POWER_VCAM_D, VOL_1800,mode_name))//DVDD=1.8V
                {
                    PK_DBG("[CAMERA SENSOR] Fail to enable digital power\n");
                    goto _kdCISModulePowerOn_exit_;
                }


                mdelay(4);
                if(mt_set_gpio_mode(GPIO_VCAM_MCLOCK_PIN,GPIO_MODE_01)){PK_DBG("[CAMERA SENSOR] set MCLK mode failed!! \n");}
                ISP_MCLK1_EN(TRUE);
                mdelay(2);

                //enable active sensor
                if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMRST]) {
                    //PDN pin
                    //if(mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_CMPDN], pinSet[pinSetIdx][IDX_PS_CMPDN+IDX_PS_MODE])){PK_DBG("[CAMERA LENS] set gpio mode failed!! \n");}
                    //if(mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMPDN], GPIO_DIR_OUT)){PK_DBG("[CAMERA LENS] set gpio dir failed!! \n");}
                    //if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMPDN], GPIO_OUT_ONE)){PK_DBG("[CAMERA LENS] set gpio failed!! \n");}

                    //RST pin
                    mdelay(10);
                    if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMRST], GPIO_OUT_ONE)){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}
                }

                //Enable AF VCM PWD
                if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMRST]) {
                    if(mt_set_gpio_mode(GPIO_CAMERA_AF_EN_PIN, GPIO_CAMERA_AF_EN_PIN_M_GPIO)){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n");}
                    if(mt_set_gpio_dir(GPIO_CAMERA_AF_EN_PIN,GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n");}
                    if(mt_set_gpio_out(GPIO_CAMERA_AF_EN_PIN,GPIO_OUT_ONE)){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}
                }
                mdelay(10);
            }

            else
            {
                PK_DBG("[CAMERA SENSOR] Cannot identify main sensor name!\n");
                cameraType = IDNONE;
            }


        }

        else if ( DUAL_CAMERA_SUB_SENSOR == SensorIdx)
        {

            printk("Power on for G750T  sub sensor: currSensorName:%s\n",currSensorName);

            if (currSensorName && (0 == strcmp(currSensorName,"s5k4e1gamipiraw")))
            {

                cameraType = S5K4E1;
                //First Power Pin low and Reset Pin Low
                if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMPDN]) {
                    //PDN pin
                    if(mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_CMPDN], pinSet[pinSetIdx][IDX_PS_CMPDN+IDX_PS_MODE])){PK_DBG("[CAMERA LENS] set gpio mode failed!! \n");}
                    if(mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMPDN], GPIO_DIR_OUT)){PK_DBG("[CAMERA LENS] set gpio dir failed!! \n");}
                    if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMPDN], GPIO_OUT_ZERO)){PK_DBG("[CAMERA LENS] set gpio failed!! \n");}
                }
                if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMRST]) {
                    //Reset pin
                    if(mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_CMRST], pinSet[pinSetIdx][IDX_PS_CMRST+IDX_PS_MODE])){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n");}
                    if(mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMRST], GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n");}
                    if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMRST],  GPIO_OUT_ZERO)){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}
                }

                //disable inactive sensor
                if (GPIO_CAMERA_INVALID != pinSet[1 - pinSetIdx][IDX_PS_CMPDN]) {
                    //PDN pin
                    if(mt_set_gpio_mode(pinSet[1 - pinSetIdx][IDX_PS_CMPDN], pinSet[1 - pinSetIdx][IDX_PS_CMPDN+IDX_PS_MODE])){PK_DBG("[CAMERA LENS] set gpio mode failed!! \n");}
                    if(mt_set_gpio_dir(pinSet[1 - pinSetIdx][IDX_PS_CMPDN], GPIO_DIR_OUT)){PK_DBG("[CAMERA LENS] set gpio dir failed!! \n");}
                    if(mt_set_gpio_out(pinSet[1 - pinSetIdx][IDX_PS_CMPDN], GPIO_OUT_ZERO)){PK_DBG("[CAMERA LENS] set gpio failed!! \n");}
                }
                if (GPIO_CAMERA_INVALID != pinSet[1 - pinSetIdx][IDX_PS_CMRST]){
                    //Reset pin
                    if(mt_set_gpio_mode(pinSet[1 - pinSetIdx][IDX_PS_CMRST], pinSet[1 - pinSetIdx][IDX_PS_CMRST+IDX_PS_MODE])){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n");}
                    if(mt_set_gpio_dir(pinSet[1 - pinSetIdx][IDX_PS_CMRST], GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n");}
                    if(mt_set_gpio_out(pinSet[1 - pinSetIdx][IDX_PS_CMRST],  GPIO_OUT_ZERO)){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}
                }


                if(TRUE != hwPowerOn(CAMERA_POWER_VCAM_D2, VOL_1800,mode_name))//IOVDD=1.8 V
                {
                    PK_DBG("[CAMERA SENSOR] Fail to enable IO power\n");
                    goto _kdCISModulePowerOn_exit_;
                }
                mdelay(1);

                if(TRUE != hwPowerOn(CAMERA_POWER_VCAM_A, VOL_2800,mode_name))//AVDD=2.8v
                {
                    PK_DBG("[CAMERA SENSOR] Fail to enable analog power\n");
                    goto _kdCISModulePowerOn_exit_;
                }
                mdelay(1);

                if(TRUE != hwPowerOn(CAMERA_POWER_VCAM_D, VOL_1800,mode_name))//DVDD=1.8V
                {
                    PK_DBG("[CAMERA SENSOR] Fail to enable digital power\n");
                    goto _kdCISModulePowerOn_exit_;
                }
                mdelay(1);

                //Power on main DVDD
                if(mt_set_gpio_mode(GPIO_VCAM_DVDD_EN, GPIO_VCAM_DVDD_EN_M_GPIO)){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n");}
                if(mt_set_gpio_dir(GPIO_VCAM_DVDD_EN, GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n");}
                if(mt_set_gpio_out(GPIO_VCAM_DVDD_EN, GPIO_OUT_ONE)){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}


                mdelay(2);
                if(mt_set_gpio_mode(GPIO_VCAM_MCLOCK_PIN,GPIO_MODE_01)){PK_DBG("[CAMERA SENSOR] set MCLK mode failed!! \n");}
                ISP_MCLK1_EN(TRUE);
                mdelay(5);

                //enable active sensor
                if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMRST]) {
                    //PDN pin
                    //if(mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_CMPDN], pinSet[pinSetIdx][IDX_PS_CMPDN+IDX_PS_MODE])){PK_DBG("[CAMERA LENS] set gpio mode failed!! \n");}
                    //if(mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMPDN], GPIO_DIR_OUT)){PK_DBG("[CAMERA LENS] set gpio dir failed!! \n");}
                    //if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMPDN], GPIO_OUT_ONE)){PK_DBG("[CAMERA LENS] set gpio failed!! \n");}

                    //RST pin
                    if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMRST], GPIO_OUT_ZERO)){PK_DBG("[CAMERA LENS] set gpio failed!! \n");}                
                    mdelay(10);
                    if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMRST], GPIO_OUT_ONE)){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}
                }

                mdelay(10);
            }
            else if (currSensorName && (0 == strcmp(SENSOR_DRVNAME_OV5648_MIPI_RAW, currSensorName)))
            {
                cameraType = OV5648;
                //First Power Pin low and Reset Pin Low
                if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMPDN]) {
                    //PDN pin
                    if(mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_CMPDN], pinSet[pinSetIdx][IDX_PS_CMPDN+IDX_PS_MODE])){PK_DBG("[CAMERA LENS] set gpio mode failed!! \n");}
                    if(mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMPDN], GPIO_DIR_OUT)){PK_DBG("[CAMERA LENS] set gpio dir failed!! \n");}
                    if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMPDN], GPIO_OUT_ZERO)){PK_DBG("[CAMERA LENS] set gpio failed!! \n");}
                }
                if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMRST]) {
                    //Reset pin
                    if(mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_CMRST], pinSet[pinSetIdx][IDX_PS_CMRST+IDX_PS_MODE])){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n");}
                    if(mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMRST], GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n");}
                    if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMRST],  GPIO_OUT_ZERO)){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}
                }

                //disable inactive sensor
                if (GPIO_CAMERA_INVALID != pinSet[1 - pinSetIdx][IDX_PS_CMPDN]) {
                    //PDN pin
                    if(mt_set_gpio_mode(pinSet[1 - pinSetIdx][IDX_PS_CMPDN], pinSet[1 - pinSetIdx][IDX_PS_CMPDN+IDX_PS_MODE])){PK_DBG("[CAMERA LENS] set gpio mode failed!! \n");}
                    if(mt_set_gpio_dir(pinSet[1 - pinSetIdx][IDX_PS_CMPDN], GPIO_DIR_OUT)){PK_DBG("[CAMERA LENS] set gpio dir failed!! \n");}
                    if(mt_set_gpio_out(pinSet[1 - pinSetIdx][IDX_PS_CMPDN], GPIO_OUT_ZERO)){PK_DBG("[CAMERA LENS] set gpio failed!! \n");}
                }
                if (GPIO_CAMERA_INVALID != pinSet[1 - pinSetIdx][IDX_PS_CMRST]) {
                    //Reset pin
                    if(mt_set_gpio_mode(pinSet[1 - pinSetIdx][IDX_PS_CMRST], pinSet[1 - pinSetIdx][IDX_PS_CMRST+IDX_PS_MODE])){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n");}
                    if(mt_set_gpio_dir(pinSet[1 - pinSetIdx][IDX_PS_CMRST], GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n");}
                    if(mt_set_gpio_out(pinSet[1 - pinSetIdx][IDX_PS_CMRST],  GPIO_OUT_ZERO)){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}
                }

                //1.
                if(TRUE != hwPowerOn(CAMERA_POWER_VCAM_D2, VOL_1800, mode_name))
                {
                    PK_DBG("[CAMERA SENSOR] Fail to enable digital power\n");
                    //return -EIO;
                    goto _kdCISModulePowerOn_exit_;
                }

                mdelay(1);

                //2.
                if(TRUE != hwPowerOn(CAMERA_POWER_VCAM_A, VOL_2800, mode_name))
                {
                    PK_DBG("[CAMERA SENSOR] Fail to enable analog power\n");
                    //return -EIO;
                    goto _kdCISModulePowerOn_exit_;
                }

                mdelay(1);

                //3.
                if(TRUE != hwPowerOn(CAMERA_POWER_VCAM_D, VOL_1500,mode_name))
                {
                     PK_DBG("[CAMERA SENSOR] Fail to enable digital power\n");
                     //return -EIO;
                     goto _kdCISModulePowerOn_exit_;
                } 

                //Power on main DVDD
                if(mt_set_gpio_mode(GPIO_VCAM_DVDD_EN, GPIO_VCAM_DVDD_EN_M_GPIO)){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n");}
                if(mt_set_gpio_dir(GPIO_VCAM_DVDD_EN, GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n");}
                if(mt_set_gpio_out(GPIO_VCAM_DVDD_EN, GPIO_OUT_ONE)){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}
                // wait power to be stable 
                mdelay(5);

                if(mt_set_gpio_mode(GPIO_VCAM_MCLOCK_PIN, GPIO_MODE_01)){PK_DBG("[CAMERA SENSOR] set MCLK mode failed!! \n");}
                ISP_MCLK1_EN(TRUE);
                mdelay(2);


                //enable active sensor
                if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMPDN]) {
                    //PDN pin
                    if(mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_CMPDN], pinSet[pinSetIdx][IDX_PS_CMPDN+IDX_PS_MODE])){PK_DBG("[CAMERA LENS] set gpio mode failed!! \n");}
                    if(mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMPDN], GPIO_DIR_OUT)){PK_DBG("[CAMERA LENS] set gpio dir failed!! \n");}
                    if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMPDN], GPIO_OUT_ONE)){PK_DBG("[CAMERA LENS] set gpio failed!! \n");}
                }

                    mdelay(2);
                if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMRST]) {
                    //RST pin
                    if(mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_CMRST], pinSet[pinSetIdx][IDX_PS_CMRST+IDX_PS_MODE])){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n");}
                    if(mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMRST], GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n");}
                    if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMRST], GPIO_OUT_ONE)){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}
                }

                msleep(20);
            }

            else
            {
                PK_DBG("[CAMERA SENSOR] Cannot identify sub sensor name!\n");
                cameraType = IDNONE;
            }

        }

    }
    else {//power OFF

        if(DUAL_CAMERA_MAIN_SENSOR  == SensorIdx)
        {
            printk("Power off for G750 mainsensor: currSensorName:%s\n",currSensorName);

            if ( IMX135 == cameraType)
            {

                if(mt_set_gpio_mode(GPIO_VCAM_MCLOCK_PIN, GPIO_MODE_00)){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n");}
                if(mt_set_gpio_dir(GPIO_VCAM_MCLOCK_PIN, GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n");}
                if(mt_set_gpio_out(GPIO_VCAM_MCLOCK_PIN, GPIO_OUT_ZERO)){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}
                ISP_MCLK1_EN(FALSE);
                mdelay(1);

                //disable main--RESET pin to low
                if(mt_set_gpio_mode(GPIO_CAMERA_CMRST_PIN,GPIO_CAMERA_CMRST_PIN_M_GPIO)){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n");}
                if(mt_set_gpio_dir(GPIO_CAMERA_CMRST_PIN,GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n");}
                if(mt_set_gpio_out(GPIO_CAMERA_CMRST_PIN,GPIO_OUT_ZERO)){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}

                 //AF PWD Disable
                if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMRST]) {
                if(mt_set_gpio_mode(GPIO_CAMERA_AF_EN_PIN, GPIO_CAMERA_AF_EN_PIN_M_GPIO)){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n");}
                if(mt_set_gpio_dir(GPIO_CAMERA_AF_EN_PIN,GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n");}
                if(mt_set_gpio_out(GPIO_CAMERA_AF_EN_PIN,GPIO_OUT_ZERO)){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}
                }
                mdelay(1);

                if(TRUE != hwPowerDown(CAMERA_POWER_VCAM_A2,mode_name))
                {
                    PK_DBG("[CAMERA SENSOR] Fail to enable analog power\n");
                    goto _kdCISModulePowerOn_exit_;
                }

                if(TRUE != hwPowerDown(CAMERA_POWER_VCAM_D, mode_name)) {//DVDD=1.8V
                    PK_DBG("[CAMERA SENSOR] Fail to OFF digital power\n");
                    goto _kdCISModulePowerOn_exit_;
                }

                //Power down DVDD
                if(mt_set_gpio_mode(GPIO_VCAM_DVDD_EN,GPIO_VCAM_DVDD_EN_M_GPIO)){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n");}
                if(mt_set_gpio_dir(GPIO_VCAM_DVDD_EN,GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n");}
                if(mt_set_gpio_out(GPIO_VCAM_DVDD_EN,GPIO_OUT_ZERO)){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}

                mdelay(1);

                if(TRUE != hwPowerDown(CAMERA_POWER_VCAM_A,mode_name)) {//AVDD=2.8v
                    PK_DBG("[CAMERA SENSOR] Fail to OFF analog power\n");
                    goto _kdCISModulePowerOn_exit_;
                }
                mdelay(1);

                if(TRUE != hwPowerDown(CAMERA_POWER_VCAM_D2,mode_name)){//IOVDD=1.8V
                    PK_DBG("[CAMERA SENSOR] Fail to enable digital power\n");
                    goto _kdCISModulePowerOn_exit_;
                }

            }
        }
        else if ( DUAL_CAMERA_SUB_SENSOR == SensorIdx)
        {

            printk("Power off for G750 sub sensor: currSensorName:%s\n",currSensorName);
            if( S5K4E1 == cameraType)
            {
                if(mt_set_gpio_mode(GPIO_VCAM_MCLOCK_PIN,GPIO_MODE_00)){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n");}
                if(mt_set_gpio_dir(GPIO_VCAM_MCLOCK_PIN,GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n");}
                if(mt_set_gpio_out(GPIO_VCAM_MCLOCK_PIN,GPIO_OUT_ZERO)){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}
                ISP_MCLK1_EN(FALSE);
                mdelay(1);

                //disable sub sensor--RST pin
                if(mt_set_gpio_mode(GPIO_CAMERA_CMRST1_PIN,GPIO_CAMERA_CMRST1_PIN_M_GPIO)){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n");}
                if(mt_set_gpio_dir(GPIO_CAMERA_CMRST1_PIN,GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n");}
                if(mt_set_gpio_out(GPIO_CAMERA_CMRST1_PIN,GPIO_OUT_ZERO)){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}
                mdelay(1);


                 //Power off main DVDD
                 if(mt_set_gpio_mode(GPIO_VCAM_DVDD_EN,GPIO_VCAM_DVDD_EN_M_GPIO)){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n");}
                 if(mt_set_gpio_dir(GPIO_VCAM_DVDD_EN,GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n");}
                 if(mt_set_gpio_out(GPIO_VCAM_DVDD_EN,GPIO_OUT_ZERO)){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}

                if(TRUE != hwPowerDown(CAMERA_POWER_VCAM_D, mode_name)) {//DVDD=1.8V
                    PK_DBG("[CAMERA SENSOR] Fail to OFF digital power\n");
                    goto _kdCISModulePowerOn_exit_;
                }

                mdelay(1);

                if(TRUE != hwPowerDown(CAMERA_POWER_VCAM_A,mode_name)) {//AVDD=2.8v
                    PK_DBG("[CAMERA SENSOR] Fail to OFF analog power\n");
                    goto _kdCISModulePowerOn_exit_;
                 }
                 mdelay(1);

                if(TRUE != hwPowerDown(CAMERA_POWER_VCAM_D2,mode_name)){//IOVDD=1.8V
                    PK_DBG("[CAMERA SENSOR] Fail to enable digital power\n");
                    goto _kdCISModulePowerOn_exit_;
                }
                mdelay(1);

            }
            else if(OV5648 == cameraType)
            {
                if(mt_set_gpio_mode(GPIO_VCAM_MCLOCK_PIN,GPIO_MODE_00)){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n");}
                if(mt_set_gpio_dir(GPIO_VCAM_MCLOCK_PIN,GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n");}
                if(mt_set_gpio_out(GPIO_VCAM_MCLOCK_PIN,GPIO_OUT_ZERO)){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}
                ISP_MCLK1_EN(FALSE);
                mdelay(1);

                if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMPDN]) {
                    if(mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_CMPDN], pinSet[pinSetIdx][IDX_PS_CMPDN+IDX_PS_MODE])){PK_DBG("[CAMERA LENS] set gpio mode failed!! \n");}
                    if(mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMPDN], GPIO_DIR_OUT)){PK_DBG("[CAMERA LENS] set gpio dir failed!! \n");}
                    if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMPDN], GPIO_OUT_ZERO)){PK_DBG("[CAMERA LENS] set gpio failed!! \n");} //high == power down lens module

                }
                if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMRST]) {
                    if(mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_CMRST], pinSet[pinSetIdx][IDX_PS_CMRST+IDX_PS_MODE])){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n");}
                    if(mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMRST], GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n");}
                    if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMRST], GPIO_OUT_ZERO)){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");} //low == reset sensor
                 }

                //Power down DVDD
                if(mt_set_gpio_mode(GPIO_VCAM_DVDD_EN,GPIO_VCAM_DVDD_EN_M_GPIO)){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n");}
                if(mt_set_gpio_dir(GPIO_VCAM_DVDD_EN,GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n");}
                if(mt_set_gpio_out(GPIO_VCAM_DVDD_EN,GPIO_OUT_ZERO)){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}

                if(TRUE != hwPowerDown(CAMERA_POWER_VCAM_D, mode_name)) {
                    PK_DBG("[CAMERA SENSOR] Fail to OFF digital power\n");
                    goto _kdCISModulePowerOn_exit_;
                }
                mdelay(1);
                if(TRUE != hwPowerDown(CAMERA_POWER_VCAM_A,mode_name)) {
                    PK_DBG("[CAMERA SENSOR] Fail to OFF analog power\n");
                    goto _kdCISModulePowerOn_exit_;
                }
                mdelay(1);
                if(TRUE != hwPowerDown(CAMERA_POWER_VCAM_D2,mode_name))
                {
                    PK_DBG("[CAMERA SENSOR] Fail to enable digital power\n");
                    goto _kdCISModulePowerOn_exit_;
                }
            }
        }

        mdelay(10);
    }//

    return 0;

_kdCISModulePowerOn_exit_:
    return -EIO;
}
EXPORT_SYMBOL(kdCISModulePowerOn);


//!--
//



