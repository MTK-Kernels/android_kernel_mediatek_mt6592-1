//add boe nt35517 lcm driver for G6
/*****************************************************************************
	Copyright (C), 1988-2012, Huawei Tech. Co., Ltd.
	FileName: nt35517
	Author: h84013687   Version: 0.1  Date: 2012/04/21
	Description: add nt35517 driver for G6
	Version: 0.1
	History: 
	<author>     <time>         <defeat ID>             <desc>
*****************************************************************************/
#ifndef BUILD_LK
#include <linux/string.h>
#endif
#include "lcm_drv.h"

#ifdef BUILD_LK
    #include <platform/disp_drv_platform.h>
#elif defined(BUILD_UBOOT)
    #include <asm/arch/mt_gpio.h>
#else
    #include <linux/delay.h>
    #include <mach/mt_gpio.h>
#endif
// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------
static unsigned int  dot_Inversion_value = 0x01;
static unsigned int  column_Inversion_value = 0x00;
static unsigned int cnt = 0; //running test num
static const max_error_num = 3; //judge error num
#define FRAME_WIDTH  		(540)
#define FRAME_HEIGHT 		(960)

//ON-chip's first 1-pulse more than 2us. pwm clk = 27KHz
static const int on_chip_min_duty = 15;
static int bl_flag=0;

#define REGFLAG_DELAY       		0xFE
#define REGFLAG_END_OF_TABLE    	0xFD   // END OF REGISTERS MARKER 
// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

#ifndef TRUE
    #define TRUE 1
#endif

#ifndef FALSE
    #define FALSE 0
#endif



//ID0=0 ID1=1
const static unsigned char LCD_MODULE_ID = 0x04;

static LCM_UTIL_FUNCS lcm_util = {0};

#define SET_RESET_PIN(v)    			(lcm_util.set_reset_pin((v)))

#define UDELAY(n) 				(lcm_util.udelay(n))
#define MDELAY(n) 				(lcm_util.mdelay(n))

// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------

#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)	lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)		lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)						lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)			lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg_v2(cmd, buffer, buffer_size)                   lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)

#ifdef BUILD_LK
#define LCD_DEBUG(fmt)  printf(fmt)
#else
#define LCD_DEBUG(fmt)  printk(fmt)
#endif

const static unsigned int BL_MIN_LEVEL = 20;
struct LCM_setting_table {
    unsigned char cmd;
    unsigned char count;
    unsigned char para_list[128];
};

static struct LCM_setting_table nt35517_qhd_vdo_boe_init[] = {
//Page 0              
{0xF0,5,{0x55,0xAA,0x52,0x08,0x00}},
{0xBC,1,{0x00}},      
//0x00-disable V-sync Detection; 0x80-enable V-sync Detection.
{0xB3,1,{0x00}},//Add by Novatek:guowenjun
{0xCC,3,{0x03,0x00,0x00}},
{0xB0,5,{0x00,0x0C,0x40,0x3C,0x3C }},
{0xB1,2,{0xFC,0x00}},
//delay hold time to making color transit smoothness.
{0xB6,1,{0x09}},
{0xB7,2,{0x03,0x02}},
{0xB8,4,{0x01,0x02,0x02,0x02}},
{0xBA,1,{0x01}},
{0xBB,3,{0x99,0x03,0x73}},
//misplacement
//{0xBD,5,{0x01,0x41,0x10,0x37,0x01}},
{0xBD,5,{0x01,0x54,0x06,0x10,0x01}},
//change CABC mode:UI mode
{0xE3,10,{0xF0,0xF0,0xEB,0xEB,0xEB,0xE5,0xE5,0xE5,0xDC,0xDC}},
         
//Page 1      
{0xF0,5,{0x55,0xAA,0x52,0x08,0x01}},
{0xB0,3,{0x0A,0x0A,0x0A}},                  //AVDD:5.5V 
{0xB6,3,{0x33,0x33,0x33}},                  //AVDD:3.0*
{0xB1,3,{0x0A,0x0A,0x0A}},                  //AVEE:-5.5V  
{0xB7,3,{0x24,0x24,0x24}},                  //AVEE:-2.0* 	        
{0xB2,3,{0x03,0x03,0x03}},                  //VCL:-4.0V 
{0xB8,3,{0x33,0x33,0x33}},                  //VCL:-2.0* 30->33
{0xB3,3,{0x0C,0x0C,0x0C}},                  //VGH:13V 
{0xB9,3,{0x25,0x25,0x25}},                  //VGH:AVDD-AVEE+VDDB
{0xB4,3,{0x0A,0x0A,0x0A}},                  //VGLX:-12V/0A    
{0xBA,3,{0x25,0x25,0x25}},                  //VGLX:AVEE+VCL-AVDD0     
{0xB5,3,{0x07,0x07,0x07}},                     
{0xBC,3,{0x00,0x90,0x06}},                  //VGMP:5.0V,VGSP:0.3V       00 78 00          
{0xBD,3,{0x00,0x90,0x06}},                  //VGMN:-5.0V,VGSN:-0.3V    
//cancel vcom set
//{0xBE,1,{0x56}},                       //VCOM DC OFFSET 4F->34 6.4
//{0xBF,1,{0x3F}},                       //ADD BTT VCOM DC OFFSET 3F 7.15

//Gamma
{0xD1,16,{0x00,0x43,0x00,0x51,0x00,0x6C,0x00,0x83,0x00,0x95,0x00,0xB5,0x00,0xD2,0x01,0x03}},
{0xD2,16,{0x01,0x2C,0x01,0x6E,0x01,0xA4,0x01,0xFB,0x02,0x44,0x02,0x46,0x02,0x87,0x02,0xCC}},
{0xD3,16,{0x02,0xF3,0x03,0x29,0x03,0x4D,0x03,0x78,0x03,0x96,0x03,0xBA,0x03,0xDE,0x03,0xF4}},
{0xD4,4, {0x03,0xFE,0x03,0xFF}},

{0xD5,16,{0x00,0x43,0x00,0x51,0x00,0x6C,0x00,0x83,0x00,0x95,0x00,0xB5,0x00,0xD2,0x01,0x03}},
{0xD6,16,{0x01,0x2C,0x01,0x6E,0x01,0xA4,0x01,0xFB,0x02,0x44,0x02,0x46,0x02,0x87,0x02,0xCC}},
{0xD7,16,{0x02,0xF3,0x03,0x29,0x03,0x4D,0x03,0x78,0x03,0x96,0x03,0xBA,0x03,0xDE,0x03,0xF4}},
{0xD8,4, {0x03,0xFE,0x03,0xFF}},

{0xD9,16,{0x00,0x43,0x00,0x51,0x00,0x6C,0x00,0x83,0x00,0x95,0x00,0xB5,0x00,0xD2,0x01,0x03}},
{0xDD,16,{0x01,0x2C,0x01,0x6E,0x01,0xA4,0x01,0xFB,0x02,0x44,0x02,0x46,0x02,0x87,0x02,0xCC}},
{0xDE,16,{0x02,0xF3,0x03,0x29,0x03,0x4D,0x03,0x78,0x03,0x96,0x03,0xBA,0x03,0xDE,0x03,0xF4}},
{0xDF,4, {0x03,0xFE,0x03,0xFF}},

{0xE0,16,{0x00,0x43,0x00,0x51,0x00,0x6C,0x00,0x83,0x00,0x95,0x00,0xB5,0x00,0xD2,0x01,0x03}},
{0xE1,16,{0x01,0x2C,0x01,0x6E,0x01,0xA4,0x01,0xFB,0x02,0x44,0x02,0x46,0x02,0x87,0x02,0xCC}},
{0xE2,16,{0x02,0xF3,0x03,0x29,0x03,0x4D,0x03,0x78,0x03,0x96,0x03,0xBA,0x03,0xDE,0x03,0xF4}},
{0xE3,4, {0x03,0xFE,0x03,0xFF}},                                    

{0xE4,16,{0x00,0x43,0x00,0x51,0x00,0x6C,0x00,0x83,0x00,0x95,0x00,0xB5,0x00,0xD2,0x01,0x03}},
{0xE5,16,{0x01,0x2C,0x01,0x6E,0x01,0xA4,0x01,0xFB,0x02,0x44,0x02,0x46,0x02,0x87,0x02,0xCC}},
{0xE6,16,{0x02,0xF3,0x03,0x29,0x03,0x4D,0x03,0x78,0x03,0x96,0x03,0xBA,0x03,0xDE,0x03,0xF4}},
{0xE7,4, {0x03,0xFE,0x03,0xFF}},                                    

{0xE8,16,{0x00,0x43,0x00,0x51,0x00,0x6C,0x00,0x83,0x00,0x95,0x00,0xB5,0x00,0xD2,0x01,0x03}},
{0xE9,16,{0x01,0x2C,0x01,0x6E,0x01,0xA4,0x01,0xFB,0x02,0x44,0x02,0x46,0x02,0x87,0x02,0xCC}},
{0xEA,16,{0x02,0xF3,0x03,0x29,0x03,0x4D,0x03,0x78,0x03,0x96,0x03,0xBA,0x03,0xDE,0x03,0xF4}},
{0xEB,4, {0x03,0xFE,0x03,0xFF}},                                    
//deleted the TE signal
//{0x35,1,{0x00}},
{0x51,1,{0x00}},
{0x53,1,{0x2C}},//boe suggest-0x2C
{0x55,1,{0x02}},//02-CABC on,00-CABC off
{0x11,1,{0x00}},
{REGFLAG_DELAY, 120, {}},
{0x29,1,{0x00}},
{REGFLAG_DELAY, 20, {}},

{REGFLAG_END_OF_TABLE, 0x00, {}}
};

//sleep in disbale v-sync detect, sleep out enbale v-sync detect
static struct LCM_setting_table lcm_sleep_out_setting[] = {
    // Sleep Out
    {0x11, 0, {0x00}},
    {REGFLAG_DELAY, 120, {}},

    // Display ON
    {0x29, 0, {0x00}},
    {REGFLAG_DELAY, 20, {}},
    {REGFLAG_END_OF_TABLE, 0x00, {}}
};
static struct LCM_setting_table lcm_sleep_mode_in_setting[] = {
    // set 51 register to 00
    {0x51, 1, {0x00}},
    // Display off sequence
    {0x28, 0, {0x00}},
    {REGFLAG_DELAY, 20, {}},

    // Sleep Mode On
    {0x10, 0, {0x00}},
    {REGFLAG_DELAY, 120, {}},
    {REGFLAG_END_OF_TABLE, 0x00, {}}
};
static struct LCM_setting_table lcm_backlight_level_setting[] = {
	{0x51, 1, {0xFF}},//
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};

/*Optimization LCD initialization time*/
static void push_table(struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
{
    unsigned int i;

    for(i = 0; i < count; i++) {

        unsigned cmd;
        cmd = table[i].cmd;

        switch (cmd) {

            case REGFLAG_DELAY :
                if(table[i].count < 20)
                    mdelay(table[i].count);
                else
                    msleep(table[i].count);
                break;
            case REGFLAG_END_OF_TABLE :
                break;

            default:
                dsi_set_cmdq_V2(cmd, table[i].count, table[i].para_list, force_update);
        }
    }

}

// ---------------------------------------------------------------------------
//  LCM Driver Implementations
// ---------------------------------------------------------------------------

static void lcm_set_util_funcs(const LCM_UTIL_FUNCS *util)
{
    memcpy(&lcm_util, util, sizeof(LCM_UTIL_FUNCS));
}


static void lcm_get_params(LCM_PARAMS *params)
{
        memset(params, 0, sizeof(LCM_PARAMS));
    
        params->type   = LCM_TYPE_DSI;

        params->width  = FRAME_WIDTH;
        params->height = FRAME_HEIGHT;


        params->dsi.mode   = SYNC_PULSE_VDO_MODE;
 
        // DSI
        /* Command mode setting */
        params->dsi.LANE_NUM                = LCM_TWO_LANE;
        params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;


        // Video mode setting       
        params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
		//steer clear of radio interfere and modified fps=58.5Hz
        params->dsi.vertical_sync_active                = 5;
        params->dsi.vertical_backporch                  = 15;
        params->dsi.vertical_frontporch                 = 7;
        params->dsi.vertical_active_line                = FRAME_HEIGHT;

        params->dsi.horizontal_sync_active              = 8;
        params->dsi.horizontal_backporch                = 39;//58;
        params->dsi.horizontal_frontporch               = 39;//58;
        params->dsi.horizontal_active_pixel             = FRAME_WIDTH;
        params->dsi.PLL_CLOCK = 228;//241;
	params->dsi.ssc_disable = 1;
}

/******************************************************************************
Function:       lcm_id_pin_handle
Description:    operate GPIO to prevent electric leakage
Input:          none
Output:         none
Return:         none
Others:         boe id0:1;id1:1,so pull up ID0 & ID1
******************************************************************************/
static void lcm_id_pin_handle(void)
{
    unsigned int ret = 0;
    ret = mt_set_gpio_pull_select(GPIO_DISP_ID0_PIN,GPIO_PULL_UP);
    if(0 != ret)
    {
        LCD_DEBUG("ID0 mt_set_gpio_pull_select->UP fail\n");
    }
    ret = mt_set_gpio_pull_select(GPIO_DISP_ID1_PIN,GPIO_PULL_DOWN);
    if(0 != ret)
    {
        LCD_DEBUG("ID1 mt_set_gpio_pull_select->DOWN fail\n");
    }
}
static void lcm_init(void)
{
    lcm_util.set_gpio_mode(GPIO_DISP_LRSTB_PIN, GPIO_MODE_00);
    lcm_util.set_gpio_dir(GPIO_DISP_LRSTB_PIN, GPIO_DIR_OUT);
    lcm_util.set_gpio_out(GPIO_DISP_LRSTB_PIN, GPIO_OUT_ONE);
    msleep(20);
    lcm_util.set_gpio_out(GPIO_DISP_LRSTB_PIN, GPIO_OUT_ZERO);
    msleep(20);
    lcm_util.set_gpio_out(GPIO_DISP_LRSTB_PIN, GPIO_OUT_ONE);
    msleep(120);

    lcm_id_pin_handle();/*Handle GPIO_DISP_ID0_PIN and GPIO_DISP_ID1_PIN*/
    push_table(nt35517_qhd_vdo_boe_init, sizeof(nt35517_qhd_vdo_boe_init) / sizeof(struct LCM_setting_table), 1);
#ifdef BUILD_LK
	printf("LCD nt35517_qhd_vdo_boe lcm_init\n");
#else
	printk("LCD nt35517_qhd_vdo_boe lcm_init\n");
#endif
}
static void lcm_suspend(void)
{
#ifdef BUILD_LK
	printf("LCD nt35517_qhd_vdo_boe lcm_suspend\n");
#else
	printk("LCD nt35517_qhd_vdo_boe lcm_suspend\n");
#endif
    push_table(lcm_sleep_mode_in_setting, sizeof(lcm_sleep_mode_in_setting) / sizeof(struct LCM_setting_table), 1);
    //when phone resume , set backlight 1st pwm pulse > 2us to avoid ON backlight chip bug
    bl_flag = TRUE;
}


static void lcm_resume(void)
{
#ifdef BUILD_LK
	printf("LCD nt35517_qhd_vdo_boe lcm_resume\n");
#else
	printk("LCD nt35517_qhd_vdo_boe lcm_resume\n");
#endif
    push_table(lcm_sleep_out_setting, sizeof(lcm_sleep_out_setting) / sizeof(struct LCM_setting_table), 1);
    //when phone resume , set backlight 1st pwm pulse > 2us to avoid ON backlight chip bug
    bl_flag = TRUE;
}

static unsigned int lcm_compare_id(void)
{
    unsigned char LCD_ID_value = 0;
    //make this adjust the triple LCD ID
    LCD_ID_value = which_lcd_module_triple();
    if(LCD_MODULE_ID == LCD_ID_value)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

static void lcm_setbacklight(unsigned int level)
{
       if(level <= 0)
            return;
        else if(level > 255)
        {
			level = 255;
        }
        //when phone resume , set backlight 1st pwm pulse > 2us to avoid ON backlight chip bug
        if(TRUE == bl_flag)
        {
            //when backlight_level=0, nothing should be changed.
            if((on_chip_min_duty > level) && (level != 0))
            {
                level = on_chip_min_duty;
                #ifdef BUILD_LK
                    printf("LK:nt35517_boe ON backlight chip bug level=%d\n",level);
                #else
                    printk("kernel:nt35517_boe ON backlight chip bug level=%d\n",level);
                #endif
            }
            bl_flag = FALSE;
        }
        #ifdef BUILD_LK
            printf("LK:nt35517_boe_lcm_setbacklight level=%d\n",level);
        #else
            printk("kernel:nt35517_boe_lcm_setbacklight level=%d\n",level);
        #endif
	lcm_backlight_level_setting[0].para_list[0] = level;

	push_table(lcm_backlight_level_setting, sizeof(lcm_backlight_level_setting) / sizeof(struct LCM_setting_table), 1);
}
#ifndef BUILD_LK
static struct LCM_setting_table lcm_Inversion_mode_setting[] = {
	{0xF0,5,{0x55,0xAA,0x52,0x08,0x00}},
	{0xBC,1,{0x00}},
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};

static void lcm_setInversion_mode(unsigned int mode)
{
    printk("nt35517_qhd_vdo_boe lcm_setInversion_mode mode = %d\n", mode);
    if(DOT_INVERSION == mode)//DOT_INVERSION
	{
        lcm_Inversion_mode_setting[1].para_list[0] = dot_Inversion_value;
	}
    else//column INVERSION
	{
	    lcm_Inversion_mode_setting[1].para_list[0] = column_Inversion_value;
    }
    push_table(lcm_Inversion_mode_setting, sizeof(lcm_Inversion_mode_setting) / sizeof(struct LCM_setting_table), 1);
}

static unsigned int lcm_check_inversion_set(unsigned int mode)
{
    unsigned char buffer[12] = {0};
    unsigned int array[16];

    array[0] = 0x00063902;
    array[1] = 0x52AA55F0;
    array[2] = 0x00000008;
    dsi_set_cmdq(array, 3,1);

    array[0] = 0x00013700;
    dsi_set_cmdq(array,1,1);
    read_reg_v2(0xBC, buffer,7);
    printk("nt35517_boe buffer[0] = %d\n",buffer[0]);
    if(COLUMN_INVERSION == mode)
    {
        if(buffer[0] == column_Inversion_value )
        {
            printk("nt35517_boe column_Inversion_value set_inversion_mode success! buffer[0] = %d\n",buffer[0]);
    	    return TRUE;
    	}
        else
        {
            printk("nt35517_boe column_Inversion_value set_inversion_mode fail! buffer[0] = %d\n",buffer[0]);
    	    return FALSE;
        }
    }
    else
    {
        if(buffer[0] == dot_Inversion_value )
    	{
    	   printk("nt35517_boe dot_Inversion_value set_inversion_mode success! buffer[0] = %d\n",buffer[0]);
    	   return TRUE;
        	}
         else{
    	   printk("nt35517_boe dot_Inversion_value set_inversion_mode fail! buffer[0] = %d\n",buffer[0]);
    	   return FALSE;
    	}
    }
}

static unsigned int lcm_check_state()
{
   	unsigned char buffer_1[12] = {0};
	unsigned char buffer_2[12] = {0};
	unsigned char buffer_3[12] = {0};
	unsigned char buffer_4[12] = {0};
	unsigned int array_1[16];
	unsigned int array_2[16];
	unsigned int array_3[16];
	unsigned int array_4[16];

    //---------------------------------
    // Read [9Ch, 00h, ECC] + Error Report(4 Bytes)
    //---------------------------------
    array_1[0] = 0x00013700;
    dsi_set_cmdq(array_1, 1,1);
    read_reg_v2(0x0A, buffer_1,7);

	array_2[0] = 0x00013700;
    dsi_set_cmdq(array_2, 1,1);	
	read_reg_v2(0x0B, buffer_2,7);

	array_3[0] = 0x00013700;
    dsi_set_cmdq(array_3, 1,1);	
	read_reg_v2(0x0C, buffer_3,7);

	array_4[0] = 0x00013700;
    dsi_set_cmdq(array_4, 1,1);	
	read_reg_v2(0x0D, buffer_4,7);	

    if((buffer_1[0] != 0x9C) || (buffer_2[0] != 0x00) || (buffer_3[0] != 0x77) || (buffer_4[0] != 0x00))/*LCD work status error,need re-initalize*/
    {
        cnt++;
        //read three times fail, judge error
        if( cnt >= max_error_num)
        {
            cnt = 0;
            printk("nt35517_boe lcm_check_state fail, 0x0A = %x, 0x0B = %x, 0x0C = %x, 0x0D = %x, cnt = %d\n",buffer_1[0],buffer_2[0],buffer_3[0],buffer_4[0],cnt);
            return FALSE;
        }
        return TRUE;
    }
    else/*LCD work status ok*/
    {
        return TRUE;
    }
}
#endif
LCM_DRIVER nt35517_qhd_vdo_boe_lcm_drv =
{
    .name           = "nt35517_qhd_vdo_boe",
    .set_util_funcs = lcm_set_util_funcs,
    .get_params     = lcm_get_params,
    .init           = lcm_init,
    .suspend        = lcm_suspend,
    .resume         = lcm_resume,
    .compare_id     = lcm_compare_id,
    .set_backlight  = lcm_setbacklight,
#ifndef BUILD_LK
    .set_inversion_mode =  lcm_setInversion_mode,
   	.lcm_check_state  	=  lcm_check_state,
	.lcm_check_inversion_set = lcm_check_inversion_set,
#endif
};
