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
#include <linux/hardware_self_adapt.h>

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

kal_bool searchMainSensor = KAL_TRUE;

int kdCISModulePowerOnULC02(CAMERA_DUAL_CAMERA_SENSOR_ENUM SensorIdx, char *currSensorName, BOOL On, char* mode_name);
//Add H30T power on sequence
int kdCISModulePowerOnH30T(CAMERA_DUAL_CAMERA_SENSOR_ENUM SensorIdx, char *currSensorName, BOOL On, char* mode_name);
int kdCISModulePowerOn(CAMERA_DUAL_CAMERA_SENSOR_ENUM SensorIdx, char *currSensorName, BOOL On, char* mode_name)
{
    hw_product_type boardType;
    boardType = get_hardware_product_main_version();

    //keep the main/sub search information
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


    printk("boardType=%x,MASK=%x\n",boardType,HW_VER_MAIN_MASK);
    if (HW_ULC02_VER == boardType)
    {
        PK_DBG("[CAMERA SENSOR] This machine is ULC02.\n");
        return kdCISModulePowerOnULC02(SensorIdx,currSensorName,On,mode_name);
    }
    else if (HW_H30T_VER == boardType)
    {
        PK_DBG("[CAMERA SENSOR] This machine is H30T.\n");
        return kdCISModulePowerOnH30T(SensorIdx,currSensorName,On,mode_name);
    }
    else if (HW_H30U_VER == boardType)
    {
        PK_DBG("[CAMERA SENSOR] This machine is H30U.\n");
        return kdCISModulePowerOnH30T(SensorIdx,currSensorName,On,mode_name);
    }
    else if (HW_G6T_VER == boardType)
    {
        PK_DBG("[CAMERA SENSOR] This machine is G6T.\n");
        return kdCISModulePowerOnH30T(SensorIdx,currSensorName,On,mode_name);
    }
    else
    {
        PK_DBG("[CAMERA SENSOR] ERROR!Cannot identify  machine type!!\n");
        return -ENXIO;
    }
}
int kdCISModulePowerOnULC02(CAMERA_DUAL_CAMERA_SENSOR_ENUM SensorIdx, char *currSensorName, BOOL On, char* mode_name)
{
#define IDNONE  0
//sub
//11,12,13...
#define GC2355 11
//main
//21,22,23...
#define IMX219 21
#define OV8858 22

    static int cameraType = IDNONE;

    PK_DBG("[kdCISModulePowerOnULC02 SensorIdx = %d, On =%d \n",SensorIdx,On);

    //power ON
    if (On) 
    {
        if(DUAL_CAMERA_SUB_SENSOR == SensorIdx)
        {
            PK_DBG("Power on for ULC02  subsensor: currSensorName:%s\n",currSensorName);
             
            if ( currSensorName && (0 == strcmp(currSensorName, SENSOR_DRVNAME_GC2355_MIPI_RAW)))
            {
                cameraType = GC2355;

                //pull down the sub cam reset pin
                if(mt_set_gpio_mode(GPIO_CAMERA_CMRST1_PIN,GPIO_CAMERA_CMRST1_PIN_M_GPIO)){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n");}
                if(mt_set_gpio_dir(GPIO_CAMERA_CMRST1_PIN,GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n");}
                if(mt_set_gpio_out(GPIO_CAMERA_CMRST1_PIN,GPIO_OUT_ZERO)){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}
                printk("Set sub reset pin to zero !\n");
                
                if(mt_set_gpio_mode(GPIO_CAMERA_CMPDN1_PIN, GPIO_CAMERA_CMPDN1_PIN_M_GPIO)){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n");}
                if(mt_set_gpio_dir(GPIO_CAMERA_CMPDN1_PIN,GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n");}
                if(mt_set_gpio_out(GPIO_CAMERA_CMPDN1_PIN,GPIO_OUT_ONE)){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}

                //Disable main sensor REST pin
                if(mt_set_gpio_mode(GPIO_CAMERA_CMRST_PIN,GPIO_CAMERA_CMRST_PIN_M_GPIO)){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n");}
                if(mt_set_gpio_dir(GPIO_CAMERA_CMRST_PIN,GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n");}
                if(mt_set_gpio_out(GPIO_CAMERA_CMRST_PIN,GPIO_OUT_ZERO)){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}

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
                if(TRUE != hwPowerOn(CAMERA_POWER_VCAM_D, VOL_1800,mode_name))//DVDD=1.8V
                {
                    PK_DBG("[CAMERA SENSOR] Fail to enable digital power\n");
                    goto _kdCISModulePowerOn_exit_;
                }
                mdelay(1);
                if(TRUE != hwPowerOn(CAMERA_POWER_VCAM_A, VOL_2800,mode_name))//AVDD=2.8v
                {
                    PK_DBG("[CAMERA SENSOR] Fail to enable analog power\n");
                    goto _kdCISModulePowerOn_exit_;
                }
                mdelay(4);
                if(mt_set_gpio_mode(GPIO_VCAM_MCLOCK_PIN,GPIO_MODE_01)){PK_DBG("[CAMERA SENSOR] set MCLK mode failed!! \n");}
                ISP_MCLK1_EN(TRUE);

                mdelay(10);
                if(mt_set_gpio_mode(GPIO_CAMERA_CMPDN1_PIN, GPIO_CAMERA_CMPDN1_PIN_M_GPIO)){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n");}
                if(mt_set_gpio_dir(GPIO_CAMERA_CMPDN1_PIN,GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n");}
                if(mt_set_gpio_out(GPIO_CAMERA_CMPDN1_PIN,GPIO_OUT_ZERO)){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}
                mdelay(1);
                if(mt_set_gpio_mode(GPIO_CAMERA_CMRST1_PIN,GPIO_CAMERA_CMRST1_PIN_M_GPIO)){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n");}
                if(mt_set_gpio_dir(GPIO_CAMERA_CMRST1_PIN,GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n");}
                if(mt_set_gpio_out(GPIO_CAMERA_CMRST1_PIN,GPIO_OUT_ONE)){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}
                mdelay(10);
            }
            else
            { 
                PK_DBG("[CAMERA SENSOR] Cannot identify sub sensor name!\n");
                cameraType = IDNONE;
            }
        }
        else if ( DUAL_CAMERA_MAIN_SENSOR == SensorIdx)
        {
            PK_DBG("Power on for ULC02  mainsensor: currSensorName:%s\n",currSensorName);

            if ( currSensorName && (0 == strcmp(currSensorName, SENSOR_DRVNAME_IMX219_MIPI_RAW)))
            {
                cameraType = IMX219;

                //pull down the main cam reset pin
                if(mt_set_gpio_mode(GPIO_CAMERA_CMRST_PIN,GPIO_CAMERA_CMRST_PIN_M_GPIO)){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n");}
                if(mt_set_gpio_dir(GPIO_CAMERA_CMRST_PIN,GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n");}
                if(mt_set_gpio_out(GPIO_CAMERA_CMRST_PIN,GPIO_OUT_ZERO)){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}
                printk("Set main reset pin to zero !\n");

                //Disable sub sensor REST pin
                if(mt_set_gpio_mode(GPIO_CAMERA_CMRST1_PIN,GPIO_CAMERA_CMRST1_PIN_M_GPIO)){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n");}
                if(mt_set_gpio_dir(GPIO_CAMERA_CMRST1_PIN,GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n");}
                if(mt_set_gpio_out(GPIO_CAMERA_CMRST1_PIN,GPIO_OUT_ZERO)){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}
                
                //Poweroff sub sensor PWDN pin
                if(mt_set_gpio_mode(GPIO_CAMERA_CMPDN1_PIN, GPIO_CAMERA_CMPDN1_PIN_M_GPIO)){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n");}
                if(mt_set_gpio_dir(GPIO_CAMERA_CMPDN1_PIN,GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n");}
                if(mt_set_gpio_out(GPIO_CAMERA_CMPDN1_PIN,GPIO_OUT_ONE)){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}

                //Power on VCM power
                if(TRUE != hwPowerOn(CAMERA_POWER_VCAM_A2, VOL_2800,mode_name))
                {
                    PK_DBG("[CAMERA SENSOR] Fail to enable analog power\n");
                    goto _kdCISModulePowerOn_exit_;
                }

                mdelay(1);
                if(TRUE != hwPowerOn(CAMERA_POWER_VCAM_A, VOL_2800,mode_name))//AVDD=2.8v
                {
                    PK_DBG("[CAMERA SENSOR] Fail to enable analog power\n");
                    goto _kdCISModulePowerOn_exit_;
                }
                mdelay(1);

                if(TRUE != hwPowerOn(CAMERA_POWER_VCAM_D, VOL_1200,mode_name))//DVDD=1.2V
                {
                    PK_DBG("[CAMERA SENSOR] Fail to enable digital power\n");
                    goto _kdCISModulePowerOn_exit_;
                }
                mdelay(1);

                if(TRUE != hwPowerOn(CAMERA_POWER_VCAM_D2, VOL_1800,mode_name))//IOVDD=1.8 V
                {
                    PK_DBG("[CAMERA SENSOR] Fail to enable IO power\n");
                    goto _kdCISModulePowerOn_exit_;
                }

                mdelay(4);
                if(mt_set_gpio_mode(GPIO_VCAM_MCLOCK_PIN,GPIO_MODE_01)){PK_DBG("[CAMERA SENSOR] set MCLK mode failed!! \n");}
                ISP_MCLK1_EN(TRUE);

                mdelay(10);
                if(mt_set_gpio_out(GPIO_CAMERA_CMRST_PIN,GPIO_OUT_ONE)){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}


                //Enable AF VCM PWD
                if(mt_set_gpio_mode(GPIO_CAMERA_AF_PWDN_PIN, GPIO_CAMERA_AF_PWDN_PIN_M_GPIO)){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n");}
                if(mt_set_gpio_dir(GPIO_CAMERA_AF_PWDN_PIN,GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n");}
                if(mt_set_gpio_out(GPIO_CAMERA_AF_PWDN_PIN,GPIO_OUT_ONE)){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}

                mdelay(10);
            }
            else if ( currSensorName && (0 == strcmp(currSensorName, SENSOR_DRVNAME_OV8858_MIPI_RAW)))
            {
                cameraType = OV8858;

                //pull down the main cam reset pin
                if(mt_set_gpio_mode(GPIO_CAMERA_CMRST_PIN,GPIO_CAMERA_CMRST_PIN_M_GPIO)){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n");}
                if(mt_set_gpio_dir(GPIO_CAMERA_CMRST_PIN,GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n");}
                if(mt_set_gpio_out(GPIO_CAMERA_CMRST_PIN,GPIO_OUT_ZERO)){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}
                printk("Set main reset pin to zero !\n");

                //Disable sub sensor REST pin
                if(mt_set_gpio_mode(GPIO_CAMERA_CMRST1_PIN,GPIO_CAMERA_CMRST1_PIN_M_GPIO)){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n");}
                if(mt_set_gpio_dir(GPIO_CAMERA_CMRST1_PIN,GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n");}
                if(mt_set_gpio_out(GPIO_CAMERA_CMRST1_PIN,GPIO_OUT_ZERO)){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}
                mdelay(1);
                
                //Poweroff sub sensor PWDN pin
                if(mt_set_gpio_mode(GPIO_CAMERA_CMPDN1_PIN, GPIO_CAMERA_CMPDN1_PIN_M_GPIO)){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n");}
                if(mt_set_gpio_dir(GPIO_CAMERA_CMPDN1_PIN,GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n");}
                if(mt_set_gpio_out(GPIO_CAMERA_CMPDN1_PIN,GPIO_OUT_ONE)){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}

                //Power on VCM power
                if(TRUE != hwPowerOn(CAMERA_POWER_VCAM_A2, VOL_2800,mode_name))
                {
                    PK_DBG("[CAMERA SENSOR] Fail to enable analog power\n");
                    goto _kdCISModulePowerOn_exit_;
                }

                mdelay(1);
                if(TRUE != hwPowerOn(CAMERA_POWER_VCAM_A, VOL_2800,mode_name))//AVDD=2.8v
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
                if(TRUE != hwPowerOn(CAMERA_POWER_VCAM_D, VOL_1200,mode_name))//DVDD=1.2V
                {
                    PK_DBG("[CAMERA SENSOR] Fail to enable digital power\n");
                    goto _kdCISModulePowerOn_exit_;
                }
                mdelay(4);

                if(mt_set_gpio_mode(GPIO_VCAM_MCLOCK_PIN,GPIO_MODE_01)){PK_DBG("[CAMERA SENSOR] set MCLK mode failed!! \n");}
                ISP_MCLK1_EN(TRUE);

                mdelay(10);
                if(mt_set_gpio_out(GPIO_CAMERA_CMRST_PIN,GPIO_OUT_ONE)){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}

                //Enable AF VCM PWD
                if(mt_set_gpio_mode(GPIO_CAMERA_AF_PWDN_PIN, GPIO_CAMERA_AF_PWDN_PIN_M_GPIO)){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n");}
                if(mt_set_gpio_dir(GPIO_CAMERA_AF_PWDN_PIN,GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n");}
                if(mt_set_gpio_out(GPIO_CAMERA_AF_PWDN_PIN,GPIO_OUT_ONE)){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}

                mdelay(10);
            }
            else
            {
                PK_DBG("[CAMERA SENSOR] Cannot identify main sensor name!\n");
                cameraType = IDNONE;
            }
        }

    }
    else
    {//power OFF
        if(DUAL_CAMERA_SUB_SENSOR == SensorIdx)
        {
            PK_DBG("Power off for ULC02 subsensor: currSensorName:%s\n",currSensorName);
            if (GC2355 == cameraType)
            {
                //pwd sub camera
                if(mt_set_gpio_mode(GPIO_CAMERA_CMPDN1_PIN, GPIO_CAMERA_CMPDN1_PIN_M_GPIO)){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n");}
                if(mt_set_gpio_dir(GPIO_CAMERA_CMPDN1_PIN,GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n");}
                if(mt_set_gpio_out(GPIO_CAMERA_CMPDN1_PIN,GPIO_OUT_ONE)){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}
                mdelay(1);
                
                //disable SUB--RESET pin to low
                if(mt_set_gpio_mode(GPIO_CAMERA_CMRST1_PIN,GPIO_CAMERA_CMRST_PIN_M_GPIO)){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n");}
                if(mt_set_gpio_dir(GPIO_CAMERA_CMRST1_PIN,GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n");}
                if(mt_set_gpio_out(GPIO_CAMERA_CMRST1_PIN,GPIO_OUT_ZERO)){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}
                mdelay(1);
                
                if(mt_set_gpio_mode(GPIO_VCAM_MCLOCK_PIN,GPIO_MODE_00)){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n");}
                if(mt_set_gpio_dir(GPIO_VCAM_MCLOCK_PIN,GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n");}
                if(mt_set_gpio_out(GPIO_VCAM_MCLOCK_PIN,GPIO_OUT_ZERO)){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}
                ISP_MCLK1_EN(FALSE);
                mdelay(1);

                if(TRUE != hwPowerDown(CAMERA_POWER_VCAM_A2,mode_name))//AF power
                {
                    PK_DBG("[CAMERA SENSOR] Fail to enable analog power\n");
                    goto _kdCISModulePowerOn_exit_;
                }
                mdelay(1);
                if(TRUE != hwPowerDown(CAMERA_POWER_VCAM_A,mode_name)) {//AVDD=2.8v
                    PK_DBG("[CAMERA SENSOR] Fail to OFF analog power\n");
                    goto _kdCISModulePowerOn_exit_;
                }
                mdelay(1);
                if(TRUE != hwPowerDown(CAMERA_POWER_VCAM_D, mode_name)) {//DVDD=1.8V
                    PK_DBG("[CAMERA SENSOR] Fail to OFF digital power\n");
                    goto _kdCISModulePowerOn_exit_;
                }
                mdelay(1);
                if(TRUE != hwPowerDown(CAMERA_POWER_VCAM_D2,mode_name)){//IOVDD=1.8V
                    PK_DBG("[CAMERA SENSOR] Fail to enable digital power\n");
                    goto _kdCISModulePowerOn_exit_;
                }
                mdelay(2);
                //set sub pwd zero
                if(mt_set_gpio_mode(GPIO_CAMERA_CMPDN1_PIN, GPIO_CAMERA_CMPDN1_PIN_M_GPIO)){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n");}
                if(mt_set_gpio_dir(GPIO_CAMERA_CMPDN1_PIN,GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n");}
                if(mt_set_gpio_out(GPIO_CAMERA_CMPDN1_PIN,GPIO_OUT_ZERO)){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}
            }
            else
            {
                PK_DBG("[CAMERA SENSOR] Cannot identify sub sensor name!\n");
            }
        }
        else if ( DUAL_CAMERA_MAIN_SENSOR == SensorIdx)
        {
            PK_DBG("Power off for ULC02 mainsensor: currSensorName:%s\n",currSensorName);

            if (IMX219 == cameraType)
            {

                if(mt_set_gpio_mode(GPIO_VCAM_MCLOCK_PIN,GPIO_MODE_00)){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n");}
                if(mt_set_gpio_dir(GPIO_VCAM_MCLOCK_PIN,GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n");}
                if(mt_set_gpio_out(GPIO_VCAM_MCLOCK_PIN,GPIO_OUT_ZERO)){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}
                ISP_MCLK1_EN(FALSE);
                mdelay(1);

                //disable main--RESET pin to low
                if(mt_set_gpio_mode(GPIO_CAMERA_CMRST_PIN,GPIO_CAMERA_CMRST_PIN_M_GPIO)){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n");}
                if(mt_set_gpio_dir(GPIO_CAMERA_CMRST_PIN,GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n");}
                if(mt_set_gpio_out(GPIO_CAMERA_CMRST_PIN,GPIO_OUT_ZERO)){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}

                //AF PWD Disable
                if(mt_set_gpio_mode(GPIO_CAMERA_AF_PWDN_PIN, GPIO_CAMERA_AF_PWDN_PIN_M_GPIO)){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n");}
                if(mt_set_gpio_dir(GPIO_CAMERA_AF_PWDN_PIN,GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n");}
                if(mt_set_gpio_out(GPIO_CAMERA_AF_PWDN_PIN,GPIO_OUT_ZERO)){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}

                mdelay(1);

                if(TRUE != hwPowerDown(CAMERA_POWER_VCAM_A2,mode_name))//AF power
                {
                    PK_DBG("[CAMERA SENSOR] Fail to enable analog power\n");
                    goto _kdCISModulePowerOn_exit_;
                }
                mdelay(1);
                if(TRUE != hwPowerDown(CAMERA_POWER_VCAM_D2,mode_name)){//IOVDD=1.8V
                    PK_DBG("[CAMERA SENSOR] Fail to enable digital power\n");
                    goto _kdCISModulePowerOn_exit_;
                }
                mdelay(1);
                if(TRUE != hwPowerDown(CAMERA_POWER_VCAM_D, mode_name)) {//DVDD=1.8V
                    PK_DBG("[CAMERA SENSOR] Fail to OFF digital power\n");
                    goto _kdCISModulePowerOn_exit_;
                }
                mdelay(1);
                if(TRUE != hwPowerDown(CAMERA_POWER_VCAM_A,mode_name)) {//AVDD=2.8v
                    PK_DBG("[CAMERA SENSOR] Fail to OFF analog power\n");
                    goto _kdCISModulePowerOn_exit_;
                }
                //set sub pwd zero
                if(mt_set_gpio_mode(GPIO_CAMERA_CMPDN1_PIN, GPIO_CAMERA_CMPDN1_PIN_M_GPIO)){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n");}
                if(mt_set_gpio_dir(GPIO_CAMERA_CMPDN1_PIN,GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n");}
                if(mt_set_gpio_out(GPIO_CAMERA_CMPDN1_PIN,GPIO_OUT_ZERO)){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}
            }
            else if (OV8858 == cameraType)
            {
                if(mt_set_gpio_mode(GPIO_VCAM_MCLOCK_PIN,GPIO_MODE_00)){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n");}
                if(mt_set_gpio_dir(GPIO_VCAM_MCLOCK_PIN,GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n");}
                if(mt_set_gpio_out(GPIO_VCAM_MCLOCK_PIN,GPIO_OUT_ZERO)){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}
                ISP_MCLK1_EN(FALSE);
                mdelay(1);

                //disable main--RESET pin to low
                if(mt_set_gpio_mode(GPIO_CAMERA_CMRST_PIN,GPIO_CAMERA_CMRST_PIN_M_GPIO)){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n");}
                if(mt_set_gpio_dir(GPIO_CAMERA_CMRST_PIN,GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n");}
                if(mt_set_gpio_out(GPIO_CAMERA_CMRST_PIN,GPIO_OUT_ZERO)){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}

                //AF PWD Disable
                if(mt_set_gpio_mode(GPIO_CAMERA_AF_PWDN_PIN, GPIO_CAMERA_AF_PWDN_PIN_M_GPIO)){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n");}
                if(mt_set_gpio_dir(GPIO_CAMERA_AF_PWDN_PIN,GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n");}
                if(mt_set_gpio_out(GPIO_CAMERA_AF_PWDN_PIN,GPIO_OUT_ZERO)){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}

                mdelay(1);

                if(TRUE != hwPowerDown(CAMERA_POWER_VCAM_A2,mode_name))//AF power
                {
                    PK_DBG("[CAMERA SENSOR] Fail to enable analog power\n");
                    goto _kdCISModulePowerOn_exit_;
                }

                mdelay(1);
                if(TRUE != hwPowerDown(CAMERA_POWER_VCAM_D, mode_name)) {//DVDD=1.2V
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
                //set sub pwd zero
                if(mt_set_gpio_mode(GPIO_CAMERA_CMPDN1_PIN, GPIO_CAMERA_CMPDN1_PIN_M_GPIO)){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n");}
                if(mt_set_gpio_dir(GPIO_CAMERA_CMPDN1_PIN,GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n");}
                if(mt_set_gpio_out(GPIO_CAMERA_CMPDN1_PIN,GPIO_OUT_ZERO)){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}
            }
            else
            {
                PK_DBG("[CAMERA SENSOR] Cannot identify main sensor name!\n");
            }

        }
        mdelay(10);
    }

    return 0;
_kdCISModulePowerOn_exit_:
    return -EIO;
}
int kdCISModulePowerOnH30T(CAMERA_DUAL_CAMERA_SENSOR_ENUM SensorIdx, char *currSensorName, BOOL On, char* mode_name)
{
#define IDNONE  0
//sub
#define S5K4E1  1
#define OV5648  2
//main
#define IMX134  3

    static int cameraType = IDNONE;

    PK_DBG("[kdCISModulePowerOnH30T SensorIdx = %d, On =%d \n",SensorIdx,On);

    //power ON
    if (On) {

        if(DUAL_CAMERA_SUB_SENSOR == SensorIdx)
        {
            PK_DBG("Power on for H30  subsensor: currSensorName:%s\n",currSensorName);
            if ( currSensorName && (0 == strcmp(currSensorName, SENSOR_DRVNAME_S5K4E1GA_MIPI_RAW)))
            {
                // Power on for sub sensor
                cameraType = S5K4E1;

                //Disable main camera--Reset pin to low
                if(mt_set_gpio_mode(GPIO_CAMERA_CMRST_PIN,GPIO_CAMERA_CMRST_PIN_M_GPIO)){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n");}
                if(mt_set_gpio_dir(GPIO_CAMERA_CMRST_PIN,GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n");}
                if(mt_set_gpio_out(GPIO_CAMERA_CMRST_PIN,GPIO_OUT_ZERO)){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}

                //disable sub sensor--RESET pin.
                if(mt_set_gpio_mode(GPIO_CAMERA_CMRST1_PIN,GPIO_CAMERA_CMRST1_PIN_M_GPIO)){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n");}
                if(mt_set_gpio_dir(GPIO_CAMERA_CMRST1_PIN,GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n");}
                if(mt_set_gpio_out(GPIO_CAMERA_CMRST1_PIN,GPIO_OUT_ZERO)){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}

                //Avdd power on first
                if(TRUE != hwPowerOn(CAMERA_POWER_VCAM_A, VOL_2800,mode_name))//AVDD=2.8v
                 {
                     PK_DBG("[CAMERA SENSOR] Fail to enable analog power\n");
                     goto _kdCISModulePowerOn_exit_;
                 }
                mdelay(1);

                 //Sub camera's IOVDD and DVDD connect together
                if(TRUE != hwPowerOn(CAMERA_POWER_VCAM_D, VOL_1800,mode_name))//DVDD=1.8V
                {
                    PK_DBG("[CAMERA SENSOR] Fail to enable digital power\n");
                    goto _kdCISModulePowerOn_exit_;
                }
 
                mdelay(1);

                //for main camera IOVDD
                if(TRUE != hwPowerOn(CAMERA_POWER_VCAM_D2, VOL_1800,mode_name))//IOVDD=1.8 V
                {
                    PK_DBG("[CAMERA SENSOR] Fail to enable IO power\n");
                    goto _kdCISModulePowerOn_exit_;
                }

                mdelay(3);
                if(mt_set_gpio_mode(GPIO_VCAM_MCLOCK_PIN,GPIO_MODE_01)){PK_DBG("[CAMERA SENSOR] set MCLK mode failed!! \n");}
                ISP_MCLK1_EN(TRUE);
                mdelay(5);

                //enable sub sensor---RESET pin
                if(mt_set_gpio_out(GPIO_CAMERA_CMRST1_PIN,GPIO_OUT_ZERO)){PK_DBG("[CAMERA LENS] set gpio failed!! \n");}
                mdelay(10);
                if(mt_set_gpio_out(GPIO_CAMERA_CMRST1_PIN,GPIO_OUT_ONE)){PK_DBG("[CAMERA LENS] set gpio failed!! \n");}

                mdelay(15);

            }
            else if (currSensorName && (0 == strcmp(currSensorName, SENSOR_DRVNAME_OV5648_MIPI_RAW)))
            {
                // Power on for sub sensor
                cameraType = OV5648;

                //Disable main camera--Reset pin to low
                if(mt_set_gpio_mode(GPIO_CAMERA_CMRST_PIN,GPIO_CAMERA_CMRST_PIN_M_GPIO)){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n");}
                if(mt_set_gpio_dir(GPIO_CAMERA_CMRST_PIN,GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n");}
                if(mt_set_gpio_out(GPIO_CAMERA_CMRST_PIN,GPIO_OUT_ZERO)){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}

                if(mt_set_gpio_mode(GPIO_CAMERA_CMPDN1_PIN,GPIO_CAMERA_CMPDN1_PIN_M_GPIO)){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n");}
                if(mt_set_gpio_dir(GPIO_CAMERA_CMPDN1_PIN,GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n");}
                if(mt_set_gpio_out(GPIO_CAMERA_CMPDN1_PIN,GPIO_OUT_ZERO)){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}

                if(mt_set_gpio_mode(GPIO_CAMERA_CMRST1_PIN,GPIO_CAMERA_CMRST1_PIN_M_GPIO)){PK_DBG("[CAMERA LENS] set gpio mode failed!! \n");}
                if(mt_set_gpio_dir(GPIO_CAMERA_CMRST1_PIN,GPIO_DIR_OUT)){PK_DBG("[CAMERA LENS] set gpio dir failed!! \n");}
                if(mt_set_gpio_out(GPIO_CAMERA_CMRST1_PIN,GPIO_OUT_ZERO)){PK_DBG("[CAMERA LENS] set gpio failed!! \n");}

                //for main camera IOVDD
                if(TRUE != hwPowerOn(CAMERA_POWER_VCAM_D2, VOL_1800,mode_name))//IOVDD=1.8 V
                {
                    PK_DBG("[CAMERA SENSOR] Fail to enable IO power\n");
                    goto _kdCISModulePowerOn_exit_;
                }
                mdelay(1);

                //Sub camera's IOVDD and DVDD connect together, Power on IOVDD first
                if(TRUE != hwPowerOn(CAMERA_POWER_VCAM_D, VOL_1800,mode_name))//DVDD=1.8V
                {
                    PK_DBG("[CAMERA SENSOR] Fail to enable digital power\n");
                    goto _kdCISModulePowerOn_exit_;
                }

                mdelay(1);

                if(TRUE != hwPowerOn(CAMERA_POWER_VCAM_A, VOL_2800,mode_name))//AVDD=2.8v
                {
                    PK_DBG("[CAMERA SENSOR] Fail to enable analog power\n");
                    goto _kdCISModulePowerOn_exit_;
                }
                mdelay(1);

                if(mt_set_gpio_mode(GPIO_VCAM_MCLOCK_PIN,GPIO_MODE_01)){PK_DBG("[CAMERA SENSOR] set MCLK mode failed!! \n");}
                ISP_MCLK1_EN(TRUE);

                mdelay(10);
                if(mt_set_gpio_out(GPIO_CAMERA_CMPDN1_PIN,GPIO_OUT_ONE)){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}
                mdelay(5);
                if(mt_set_gpio_out(GPIO_CAMERA_CMRST1_PIN,GPIO_OUT_ONE)){PK_DBG("[CAMERA LENS] set gpio failed!! \n");}

                mdelay(25);


            }
            else
            {
                PK_DBG("[CAMERA SENSOR] Cannot identify sub sensor name!\n");
                cameraType = IDNONE;
            }

        }
        else if ( DUAL_CAMERA_MAIN_SENSOR == SensorIdx)
        {
            PK_DBG("Power on for H30  mainsensor: currSensorName:%s\n",currSensorName);

            if ( currSensorName && ((0 == strcmp(currSensorName, SENSOR_DRVNAME_IMX134LITEON_MIPI_RAW)) || (0 == strcmp(currSensorName, SENSOR_DRVNAME_IMX134SUNNY_MIPI_RAW))))
            {
                cameraType = IMX134;

                //pull down the main cam reset pin
                if(mt_set_gpio_mode(GPIO_CAMERA_CMRST_PIN,GPIO_CAMERA_CMRST_PIN_M_GPIO)){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n");}
                if(mt_set_gpio_dir(GPIO_CAMERA_CMRST_PIN,GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n");}
                if(mt_set_gpio_out(GPIO_CAMERA_CMRST_PIN,GPIO_OUT_ZERO)){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}
                printk("Set main reset pin to zero !\n");

                //Disable sub sensor REST pin
                if(mt_set_gpio_mode(GPIO_CAMERA_CMRST1_PIN,GPIO_CAMERA_CMRST1_PIN_M_GPIO)){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n");}
                if(mt_set_gpio_dir(GPIO_CAMERA_CMRST1_PIN,GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n");}
                if(mt_set_gpio_out(GPIO_CAMERA_CMRST1_PIN,GPIO_OUT_ZERO)){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}

                //Disable sub sensor--PWD pin
                if(mt_set_gpio_mode(GPIO_CAMERA_CMPDN1_PIN,GPIO_CAMERA_CMPDN1_PIN_M_GPIO)){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n");}
                if(mt_set_gpio_dir(GPIO_CAMERA_CMPDN1_PIN,GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n");}
                if(mt_set_gpio_out(GPIO_CAMERA_CMPDN1_PIN,GPIO_OUT_ZERO)){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}

                //Power on VCM power
                if(TRUE != hwPowerOn(CAMERA_POWER_VCAM_A2, VOL_2800,mode_name))
                {
                    PK_DBG("[CAMERA SENSOR] Fail to enable analog power\n");
                    goto _kdCISModulePowerOn_exit_;
                }

                mdelay(1);
                //Modify pwd on seq to IOVDD-AVDD-DVDD to avoid half voltage waveform
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
                mdelay(4);
                if(mt_set_gpio_mode(GPIO_VCAM_MCLOCK_PIN,GPIO_MODE_01)){PK_DBG("[CAMERA SENSOR] set MCLK mode failed!! \n");}
                ISP_MCLK1_EN(TRUE);

                mdelay(10);
                if(mt_set_gpio_out(GPIO_CAMERA_CMRST_PIN,GPIO_OUT_ONE)){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}


                //Enable AF VCM PWD
                if(mt_set_gpio_mode(GPIO_CAMERA_AF_PWDN_PIN, GPIO_CAMERA_AF_PWDN_PIN_M_GPIO)){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n");}
                if(mt_set_gpio_dir(GPIO_CAMERA_AF_PWDN_PIN,GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n");}
                if(mt_set_gpio_out(GPIO_CAMERA_AF_PWDN_PIN,GPIO_OUT_ONE)){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}


                mdelay(10);
            }
            else
            {
                PK_DBG("[CAMERA SENSOR] Cannot identify main sensor name!\n");
                cameraType = IDNONE;
            }
        }

    }
    else {//power OFF

        if(DUAL_CAMERA_SUB_SENSOR == SensorIdx)
        {

            PK_DBG("Power off for H30 subsensor: currSensorName:%s\n",currSensorName);

            if (S5K4E1 == cameraType)
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
            else if (OV5648 == cameraType)
            {

                if(mt_set_gpio_mode(GPIO_CAMERA_CMPDN1_PIN,GPIO_CAMERA_CMPDN1_PIN_M_GPIO)){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n");}
                if(mt_set_gpio_dir(GPIO_CAMERA_CMPDN1_PIN,GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n");}
                if(mt_set_gpio_out(GPIO_CAMERA_CMPDN1_PIN,GPIO_OUT_ZERO)){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}

                if(mt_set_gpio_mode(GPIO_CAMERA_CMRST1_PIN,GPIO_CAMERA_CMRST1_PIN_M_GPIO)){PK_DBG("[CAMERA LENS] set gpio mode failed!! \n");}
                if(mt_set_gpio_dir(GPIO_CAMERA_CMRST1_PIN,GPIO_DIR_OUT)){PK_DBG("[CAMERA LENS] set gpio dir failed!! \n");}
                if(mt_set_gpio_out(GPIO_CAMERA_CMRST1_PIN,GPIO_OUT_ZERO)){PK_DBG("[CAMERA LENS] set gpio failed!! \n");}

                mdelay(1);

                if(TRUE != hwPowerDown(CAMERA_POWER_VCAM_A,mode_name)) {//AVDD=2.8v
                  PK_DBG("[CAMERA SENSOR] Fail to OFF CAMERA_POWER_VCAM_A power\n");
                  goto _kdCISModulePowerOn_exit_;
                }
                mdelay(1);

                if(TRUE != hwPowerDown(CAMERA_POWER_VCAM_D, mode_name)) {//DVDD=1.8V
                  PK_DBG("[CAMERA SENSOR] Fail to OFF CAMERA_POWER_VCAM_D power\n");
                  goto _kdCISModulePowerOn_exit_;
                }
                mdelay(1);

                if(mt_set_gpio_mode(GPIO_VCAM_MCLOCK_PIN,GPIO_MODE_00)){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n");}
                if(mt_set_gpio_dir(GPIO_VCAM_MCLOCK_PIN,GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n");}
                if(mt_set_gpio_out(GPIO_VCAM_MCLOCK_PIN,GPIO_OUT_ZERO)){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}
                ISP_MCLK1_EN(FALSE);

                mdelay(1);

                if(TRUE != hwPowerDown(CAMERA_POWER_VCAM_D2,mode_name)){//IOVDD=1.8V
                  PK_DBG("[CAMERA SENSOR] Fail to OFF CAMERA_POWER_VCAM_D2 power\n");
                  goto _kdCISModulePowerOn_exit_;
                }
                mdelay(1);

            }

        }
        else if ( DUAL_CAMERA_MAIN_SENSOR == SensorIdx)
        {
            PK_DBG("Power off for mainsensor \n");
            PK_DBG("Power off for H30 mainsensor: currSensorName:%s\n",currSensorName);

            if (IMX134 == cameraType)
            {

                if(mt_set_gpio_mode(GPIO_VCAM_MCLOCK_PIN,GPIO_MODE_00)){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n");}
                if(mt_set_gpio_dir(GPIO_VCAM_MCLOCK_PIN,GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n");}
                if(mt_set_gpio_out(GPIO_VCAM_MCLOCK_PIN,GPIO_OUT_ZERO)){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}
                ISP_MCLK1_EN(FALSE);
                mdelay(1);

                //disable main--RESET pin to low
                if(mt_set_gpio_mode(GPIO_CAMERA_CMRST_PIN,GPIO_CAMERA_CMRST_PIN_M_GPIO)){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n");}
                if(mt_set_gpio_dir(GPIO_CAMERA_CMRST_PIN,GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n");}
                if(mt_set_gpio_out(GPIO_CAMERA_CMRST_PIN,GPIO_OUT_ZERO)){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}

                //AF PWD Disable
                if(mt_set_gpio_mode(GPIO_CAMERA_AF_PWDN_PIN, GPIO_CAMERA_AF_PWDN_PIN_M_GPIO)){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n");}
                if(mt_set_gpio_dir(GPIO_CAMERA_AF_PWDN_PIN,GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n");}
                if(mt_set_gpio_out(GPIO_CAMERA_AF_PWDN_PIN,GPIO_OUT_ZERO)){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}

                mdelay(1);

                if(TRUE != hwPowerDown(CAMERA_POWER_VCAM_A2,mode_name))//AF power
                {
                    PK_DBG("[CAMERA SENSOR] Fail to enable analog power\n");
                    goto _kdCISModulePowerOn_exit_;
                }
                mdelay(1);
                //Modify pwd off seq to DVDD-AVDD-IOVDD to avoid half voltage waveform
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
                    PK_DBG("[CAMERA SENSOR] Fail to OFF digital power\n");
                    goto _kdCISModulePowerOn_exit_;
                }
            }
            else
            {
                PK_DBG("[CAMERA SENSOR] Cannot identify main sensor name!\n");
            }

        }
        mdelay(10);
    }

    return 0;
_kdCISModulePowerOn_exit_:
    return -EIO;

}
EXPORT_SYMBOL(kdCISModulePowerOn);

//!--
//





