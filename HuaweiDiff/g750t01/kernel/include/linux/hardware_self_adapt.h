/******************************************************************************

  Copyright (C), 2011-2015, Huawei Tech. Co., Ltd.

 ******************************************************************************
  File Name     : hardware_self_adapt.h
  Description   : In LK get the hardware version info from RAM
******************************************************************************/

#include <linux/types.h>
#include <asm/setup.h>


#define HW_VER_MAIN_MASK (0xFFF0)
#define HW_VER_SUB_MASK  (0x000F)

/*
* first byte: low 4 bits: sub version,  high 4 bits: , external version for example U8105
* high 3 bytes: base main board version
*/

typedef enum
{
/* 
   this define cannot be use in logic,it only keep head file the same,
   this file is only for MT6592
*/
    //G610-T10
    HW_G610T10_VER   = 0x300,
    HW_G610T10_VER_A = HW_G610T10_VER,
    HW_G610T10_VER_B ,
    HW_G610T10_VER_C ,
    HW_G610T10_VER_D ,
    HW_G610T10_VER_E ,
    HW_G610T10_VER_F ,

    //G610-T11
    HW_G610T11_VER   = HW_G610T10_VER,
    HW_G610T11_VER_A = HW_G610T10_VER_A,
    HW_G610T11_VER_B = HW_G610T10_VER_B,
    HW_G610T11_VER_C = HW_G610T10_VER_C,
    HW_G610T11_VER_D = HW_G610T10_VER_D,
    HW_G610T11_VER_E = HW_G610T10_VER_E,
    HW_G610T11_VER_F = HW_G610T10_VER_F,

    //G750-T00
    HW_G750_VER   = 0x100,
    HW_G750_VER_A = HW_G750_VER,
    HW_G750_VER_B ,
    HW_G750_VER_C ,
    HW_G750_VER_D ,      //as G750-T01 Ver.A
    HW_G750_VER_E ,       //as G750-U10 Ver.A
    HW_G750_VER_F ,         //as G750-T20 Ver.B

    //G750-U00
    HW_G750U_VER   = 0x200,
    HW_G750U_VER_A = HW_G750U_VER,
    HW_G750U_VER_B ,
    HW_G750U_VER_C ,
    HW_G750U_VER_D ,
    HW_G750U_VER_E ,
    HW_G750U_VER_F ,

/* 
   this define cannot be use in logic,it only keep head file the same,
   this file is only for MT6592
*/

    //G730-T00
    HW_G730_VER   = 0x400,
    HW_G730_VER_A = HW_G730_VER,
    HW_G730_VER_B ,
    HW_G730_VER_C ,
    HW_G730_VER_D ,		// G730-T20
    HW_G730_VER_E ,
    HW_G730_VER_F ,

    //G730-U00
    HW_G730U_VER   = 0x500,
    HW_G730U_VER_A = HW_G730U_VER,
    HW_G730U_VER_B ,
    HW_G730U_VER_C ,
    HW_G730U_VER_D ,	// G730-U251
    HW_G730U_VER_E ,	// G730-U27
    HW_G730U_VER_F ,

    //H30-T00
    HW_H30T_VER   = 0x600,
    HW_H30T_VER_A = HW_H30T_VER,
    HW_H30T_VER_B ,
    HW_H30T_VER_C ,
    HW_H30T_VER_D ,		// H30-T10 verA 
    HW_H30T_VER_E ,		// H30-T10 verB
    HW_H30T_VER_F ,		// H30-T10 verC

    //H30-U00
    HW_H30U_VER   = 0x700,
    HW_H30U_VER_A = HW_H30U_VER,
    HW_H30U_VER_B ,
    HW_H30U_VER_C ,
    HW_H30U_VER_D ,		// H30-U10 verA
    HW_H30U_VER_E ,		// H30-U10 verB
    HW_H30U_VER_F , 	// H30-U10 verC

    //G6-T00
    HW_G6T_VER   = 0x800,
    HW_G6T_VER_A = HW_G6T_VER,
    HW_G6T_VER_B ,
    HW_G6T_VER_C ,
    HW_G6T_VER_D ,
    HW_G6T_VER_E ,
    HW_G6T_VER_F ,

    //G630-T00
    HW_G630T_VER   = 0x900,
    HW_G630T_VER_A = HW_G630T_VER,		// G630-T00 verA
    HW_G630T_VER_B ,					// G630-T00 verB
    HW_G630T_VER_C ,					// G630-T00 verC
    HW_G630T_VER_D ,
    HW_G630T_VER_E ,
    HW_G630T_VER_F ,

    //G610-U10
    HW_G610U10_VER   = 0xA00,
    HW_G610U10_VER_A = HW_G610U10_VER,
    HW_G610U10_VER_B ,
    HW_G610U10_VER_C ,
    HW_G610U10_VER_D ,
    HW_G610U10_VER_E ,
    HW_G610U10_VER_F ,

    /* added other type at here, it should be started 0x200 */
    //ULC02
    HW_ULC02_VER   = 0x1100,
    HW_ULC02_VER_A = HW_ULC02_VER,
    HW_ULC02_VER_B ,
    HW_ULC02_VER_C ,
    HW_ULC02_VER_D ,
    HW_ULC02_VER_E ,
    HW_ULC02_VER_N ,
    HW_ULC02_VER_O ,
    HW_ULC02_VER_P ,
    HW_ULC02_VER_R ,
    HW_ULC02_VER_S ,


    HW_VER_NONE    = 0xFFFF,
}hw_product_type;




/*
* sub version type define
*/
typedef enum
{
    HW_VER_SUB_VA            = 0x0,
    HW_VER_SUB_VB            = 0x1,
    HW_VER_SUB_VC            = 0x2,
    HW_VER_SUB_VD            = 0x3,
    HW_VER_SUB_VE            = 0x4,
    HW_VER_SUB_VN            = 0x5,
    HW_VER_SUB_VO            = 0x6,
    HW_VER_SUB_VP            = 0x7,
    HW_VER_SUB_VR            = 0x8,
    HW_VER_SUB_VS            = 0x9,
    HW_VER_SUB_SURF          = 0xF,
    HW_VER_SUB_MAX           = 0xF
}hw_ver_sub_type;


/*
* app info type define
*/
typedef enum {
    BOARD_ID                 = 0x0,
    EMMC_ID                  = 0x1,
    PRIMARY_CAMERA_ID        = 0x2,
    SLAVE_CAMERA_ID          = 0x3,
    LCD_ID                   = 0x4,
    GSENSOR_ID               = 0x5,
    COMPASS_ID               = 0x6,
    TP_ID                    = 0x7,
    HW_VERSION               = 0x8,
    BATTERY_ID               = 0x9,
#ifdef CONFIG_DL_CHECK_SUPPORT
    DL_CHECK_TAG             = 0xA,
#endif
    ALS_PS_ID                = 0xB,

    DDR_ID                   = 0xC,

    MAX_NUM_ID
}id_type;


/*
 *because we can not set fixed ID resistor for the same manufactory,
 *a ID resistor does not match the only manufactory any more.
 *avoiding misunderstanding, just enum ID resistor instead of manufacture here.
*/
typedef enum
{
    BATT_ID_10K = 0,
    BATT_ID_22K,
    BATT_ID_39K,
    BATT_ID_68K,
    BATT_ID_110K,
    BATT_ID_200K,
    BATT_ID_470K,
    BATT_ID_2000K,
    BATT_ID_INVALID
}hw_battery_type;

#ifdef CONFIG_DL_CHECK_SUPPORT
typedef enum
{
    DL_PASS = 0,
    DL_FAIL,
    DL_INVALID
}hw_dl_check_tag;
#endif

#define hw_product_info_type  tag_hw_product_info
#define HW_PRODUCT_INFO_SIZE  sizeof(struct hw_product_info_type)
extern struct hw_product_info_type hw_product_info;

hw_product_type get_hardware_product_version (void);
hw_battery_type get_hardware_battery_type(void);
int get_battery_id_adc_value(void);

u32 get_ddr_md5_value(void);

#ifdef CONFIG_DL_CHECK_SUPPORT
hw_dl_check_tag get_hardware_dl_check_tag(void);
#endif

#define get_hardware_product_main_version() (get_hardware_product_version() & HW_VER_MAIN_MASK)
#define get_hardware_product_sub_version()  (get_hardware_product_version() & HW_VER_SUB_MASK)


void set_id_value(id_type type, char *value);
