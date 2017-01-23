//add tianma otm9605a lcm driver for G6
/*****************************************************************************
	Copyright (C), 1988-2012, Huawei Tech. Co., Ltd.
	FileName: otm9605a
	Author: h84013687   Version: 0.1  Date: 2012/04/21
	Description: add driver for otm9605a
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
static unsigned int  dot_Inversion_value = 0x00;
static unsigned int  column_Inversion_value = 0x50;
static unsigned int cnt = 0; //running test num
static const max_error_num = 3; //judge error num
#define FRAME_WIDTH  		(540)
#define FRAME_HEIGHT 		(960)

//ON-chip's first 1-pulse more than 2us.pwm clk = 18KHz
static const int on_chip_min_duty = 12;
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


const static unsigned char LCD_MODULE_ID = 0x01;//ID0=1,ID1=0

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

static struct LCM_setting_table otm9605a_qhd_vdo_tm_init[] = {
	{0x00,1,{0x00}},
	{0xFF,3,{0x96,0x05,0x01}},// Enable cmd// OTM9605A
	{0x00,1,{0x80}},
	{0xFF,2,{0x96,0x05}},
	{0x00,1,{0x00}},
	{0xA0,1,{0x00}},//OTP select region
	{0x00,1,{0x92}},
	{0xFF,2,{0x10,0x02}},
	{0x00,1,{0xB3}},
	{0xC0,2,{0x00,0x50}},//Interval Scan Frame Setting
	{0x00,1,{0x80}},
	{0xC0,10,{0x00,0x48,0x00,0x10,0x10,0x00,0x48,0x10,0x10,0x01}},//TCON Setting Parameters
	{0x00,1,{0xA2}},
	{0xC0,3,{0x0C,0x05,0x02}},//Panel Timing Setting Parameter 		
	{0x00,1,{0x80}},
	{0xC1,2,{0x36,0x66}},//Oscillator Adjustment for Idle/Normal Mode	
	{0x00,1,{0xA0}},
	{0xC4,8,{0x33,0x09,0x94,0x30,0x33,0x09,0x94,0x30}},//DC2DC Setting	
	{0x00,1,{0x80}},
	{0xC5,4,{0x08,0x00,0xA0,0x11}},
	{0x00,1,{0x90}},
	{0xC5,7,{0x96,0x36,0x01,0x79,0x33,0x33,0x34}},//Power Control Setting2 for Normal Mode
	{0x00,1,{0xA0}},
	{0xC5,7,{0x96,0x16,0x00,0x79,0x33,0x33,0x34}},
	{0x00,1,{0xB0}},
	{0xC5,2,{0x04,0xA8}},
	{0x00,1,{0x80}},
	{0xC6,1,{0x64}},
	{0x00,1,{0xB0}},
	{0xC6,5,{0x03,0x10,0x00,0x1F,0x12}},
	{0x00,1,{0x00}},
	{0xD0,1,{0x40}},
	{0x00,1,{0x00}},
	{0xD1,2,{0x00,0x00}},
	{0x00,1,{0xB0}},
	{0xCB,16,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}},
	{0x00,1,{0xC0}},
	{0xCB,15,{0x14,0x14,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x14,0x00,0x14,0x14,0x00,0x00}},
	{0x00,1,{0xD0}},
	{0xCB,15,{0x00,0x00,0x00,0x00,0x00,0x14,0x14,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x14}},
	{0x00,1,{0xE0}},
	{0xCB,10,{0x00,0x14,0x14,0x00,0x00,0x00,0x00,0x00,0x00,0x00}},
	{0x00,1,{0xF0}},
	{0xCB,10,{0x0F,0x00,0xCC,0x00,0x00,0x0F,0x00,0xCC,0x03,0x00}},
	{0x00,1,{0x80}},
	{0xCC,10,{0x26,0x25,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0A}},
	{0x00,1,{0x90}},
	{0xCC,15,{0x00,0x0C,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x26,0x25,0x00,0x00,0x00}},
	{0x00,1,{0xA0}},
	{0xCC,15,{0x00,0x00,0x00,0x00,0x09,0x00,0x0B,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00}},
	{0x00,1,{0xB0}},
	{0xCC,10,{0x26,0x25,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0B}},
	{0x00,1,{0xC0}},
	{0xCC,15,{0x00,0x09,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x26,0x25,0x00,0x00,0x00}},
	{0x00,1,{0xD0}},
	{0xCC,15,{0x00,0x00,0x00,0x00,0x0C,0x00,0x0A,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00}},
	{0x00,1,{0x80}},
	{0xCE,6,{0x85,0x01,0x00,0x84,0x01,0x00}},
	{0x00,1,{0xA0}},
	{0xCE,14,{0x18,0x03,0x03,0xBF,0x00,0x1F,0x00,0x18,0x02,0x03,0xC0,0x00,0x1F,0x00}},
	{0x00,1,{0xB0}},
	{0xCE,14,{0x18,0x01,0x03,0xC1,0x00,0x1F,0x00,0x18,0x00,0x03,0xC2,0x00,0x1F,0x00}},
	{0x00,1,{0xC7}},
	{0xCF,1,{0x00}}, 
	{0x00,1,{0x80}},
	{0xCF,14,{0xF0,0x00,0x00,0x10,0x00,0x00,0x00,0xF0,0x00,0x00,0x10,0x00,0x00,0x00}},
	{0x00,1,{0x90}},
	{0xCF,14,{0xF0,0x00,0x00,0x10,0x00,0x00,0x00,0xF0,0x00,0x00,0x10,0x00,0x00,0x00}},
	{0x00,1,{0xA0}},
	{0xCF,14,{0xF0,0x00,0x00,0x10,0x00,0x00,0x00,0xF0,0x00,0x00,0x10,0x00,0x00,0x00}},
	{0x00,1,{0xB0}},
	{0xCF,14,{0xF0,0x00,0x00,0x10,0x00,0x00,0x00,0xF0,0x00,0x00,0x10,0x00,0x00,0x00}},
	{0x00,1,{0xC0}},
	{0xCF,10,{0x01,0x01,0x20,0x20,0x00,0x00,0x01,0x81,0x00,0x10}},	 
	{0x00,1,{0xA0}},
	{0xC1,1,{0x02}}, 				
	{0x00,1,{0x00}},
	{0xD8,2,{0x6F,0x6F}},
	//cancel vcom set
	//{0x00,1,{0x00}},
	//{0xD9,1,{0x31}},
	//gap3.1 and gap3.2 amend water ripple,optimize gamma index 20131212
	{0x00,1,{0x00}},
	{0xE1,16,{0x01,0x09,0x0E,0x0D,0x05,0x0F,0x0C,0x0B,0x02,0x06,0x0C,0x07,0x0e,0x14,0x0D,0x01}},
	{0x00,1,{0x00}},
	{0xE2,16,{0x01,0x09,0x0E,0x0D,0x05,0x0F,0x0C,0x0B,0x02,0x06,0x0C,0x07,0x0e,0x14,0x0D,0x01}},
	//deleted the TE signal
	//{0x35,1,{0x00}},
	
	//solve the problem tianma otm9605a LCD display
	{0x00,1,{0xC0}},
	{0xB0,1,{0x08}},
	{0x51,1,{0x00}},//
	{0x53,1,{0x24}},//
	//CABC on
	{0x55,1,{0x01}},
	
	{0x11, 1, {0x00}},
	{REGFLAG_DELAY, 120, {}},
	{0x29, 1, {0x00}},
	{REGFLAG_DELAY, 20, {}},
    {REGFLAG_END_OF_TABLE, 0x00, {}},
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
        params->dsi.vertical_sync_active                = 2;
        params->dsi.vertical_backporch                  = 32;
        params->dsi.vertical_frontporch                 = 32;
        params->dsi.vertical_active_line                = FRAME_HEIGHT;

        params->dsi.horizontal_sync_active              = 5;
        params->dsi.horizontal_backporch                = 29;//46;
        params->dsi.horizontal_frontporch               = 29;//46;
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
    mdelay(10);
    lcm_util.set_gpio_out(GPIO_DISP_LRSTB_PIN, GPIO_OUT_ZERO);
    mdelay(10);
    lcm_util.set_gpio_out(GPIO_DISP_LRSTB_PIN, GPIO_OUT_ONE);
    msleep(20);

    lcm_id_pin_handle();/*Handle GPIO_DISP_ID0_PIN and GPIO_DISP_ID1_PIN*/
    push_table(otm9605a_qhd_vdo_tm_init, sizeof(otm9605a_qhd_vdo_tm_init) / sizeof(struct LCM_setting_table), 1);
#ifdef BUILD_LK
	printf("LCD otm9605a_tm lcm_init\n");
#else
	printk("LCD otm9605a_tm lcm_init\n");
#endif
}
static void lcm_suspend(void)
{
#ifdef BUILD_LK
	printf("LCD otm9605a_tm lcm_suspend\n");
#else
	printk("LCD otm9605a_tm lcm_suspend\n");
#endif
    push_table(lcm_sleep_mode_in_setting, sizeof(lcm_sleep_mode_in_setting) / sizeof(struct LCM_setting_table), 1);
    lcm_util.set_gpio_out(GPIO_DISP_LRSTB_PIN, GPIO_OUT_ZERO);
    mdelay(10);
    lcm_util.set_gpio_out(GPIO_DISP_LRSTB_PIN, GPIO_OUT_ONE);
    msleep(20);
    //when phone resume , set backlight 1st pwm pulse > 2us to avoid ON backlight chip bug
    bl_flag = TRUE;
}

static void lcm_resume(void)
{
#ifdef BUILD_LK
	printf("LCD otm9605a_tm lcm_resume\n");
#else
	printk("LCD otm9605a_tm lcm_resume\n");
#endif
    lcm_util.set_gpio_out(GPIO_DISP_LRSTB_PIN, GPIO_OUT_ONE);
    msleep(20);
    lcm_util.set_gpio_out(GPIO_DISP_LRSTB_PIN, GPIO_OUT_ZERO);
    mdelay(10);
    lcm_util.set_gpio_out(GPIO_DISP_LRSTB_PIN, GPIO_OUT_ONE);
    msleep(20);
    push_table(otm9605a_qhd_vdo_tm_init, sizeof(otm9605a_qhd_vdo_tm_init) / sizeof(struct LCM_setting_table), 1);
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

static void lcm_update(unsigned int x, unsigned int y,
                       unsigned int width, unsigned int height)
{
	unsigned int x0 = x;
	unsigned int y0 = y;
	unsigned int x1 = x0 + width - 1;
	unsigned int y1 = y0 + height - 1;

	unsigned char x0_MSB = ((x0>>8)&0xFF);
	unsigned char x0_LSB = (x0&0xFF);
	unsigned char x1_MSB = ((x1>>8)&0xFF);
	unsigned char x1_LSB = (x1&0xFF);
	unsigned char y0_MSB = ((y0>>8)&0xFF);
	unsigned char y0_LSB = (y0&0xFF);
	unsigned char y1_MSB = ((y1>>8)&0xFF);
	unsigned char y1_LSB = (y1&0xFF);

	unsigned int data_array[16];

	data_array[0]= 0x00053902;
	data_array[1]= (x1_MSB<<24)|(x0_LSB<<16)|(x0_MSB<<8)|0x2a;
	data_array[2]= (x1_LSB);
	dsi_set_cmdq(data_array, 3, 1);
	
	data_array[0]= 0x00053902;
	data_array[1]= (y1_MSB<<24)|(y0_LSB<<16)|(y0_MSB<<8)|0x2b;
	data_array[2]= (y1_LSB);
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0]= 0x002c3909;
	dsi_set_cmdq(data_array, 1, 0);

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
                    printf("LK:otm9605a_tm ON backlight chip bug level=%d\n",level);
                #else
                    printk("kernel:otm9605a_tm ON backlight chip bug level=%d\n",level);
                #endif
            }
            bl_flag = FALSE;
        }
        #ifdef BUILD_LK
            printf("LK:otm9605a_tm_lcm_setbacklight level=%d\n",level);
        #else
            printk("kernel:otm9605a_tm_lcm_setbacklight level=%d\n",level);
        #endif
	lcm_backlight_level_setting[0].para_list[0] = level;

	push_table(lcm_backlight_level_setting, sizeof(lcm_backlight_level_setting) / sizeof(struct LCM_setting_table), 1);
}
#ifndef BUILD_LK
static struct LCM_setting_table lcm_Inversion_mode_setting[] = {
	{0x00,1,{0x00}},	
	{0xff,3,{0x96,0x05,0x01}},// Enable cmd
	{0x00,1,{0x80}},
	{0xff,2,{0x96,0x05}},
	{0x00,1,{0xb3}},
	{0xc0,2,{0x00,0x50}},
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};

static void lcm_setInversion_mode(unsigned int mode)
{
    printk("otm9605a_tm lcm_setInversion_mode mode = %d\n", mode);
    if(DOT_INVERSION == mode)//DOT_INVERSION
	{
        lcm_Inversion_mode_setting[5].para_list[1] = dot_Inversion_value;
	}
    else//column INVERSION
	{
	    lcm_Inversion_mode_setting[5].para_list[1] = column_Inversion_value;
    }
    push_table(lcm_Inversion_mode_setting, sizeof(lcm_Inversion_mode_setting) / sizeof(struct LCM_setting_table), 1);
}

static unsigned int lcm_check_inversion_set(unsigned int mode)
{
    unsigned char buffer[12] = {0};
    unsigned int array[16];
    array[0] = 0x00023902;
    array[1] = 0x00000000;
    dsi_set_cmdq(&array,2,1);
    array[0] = 0x00043902;
    array[1] = 0x010596FF;
    dsi_set_cmdq(&array,2,1);

    array[0] = 0x00023902;
    array[1] = 0x00008000;
    dsi_set_cmdq(&array,2,1);
    array[0] = 0x00033902;
    array[1] = 0x000596FF;
    dsi_set_cmdq(&array,2,1);

    array[0] = 0x00023902;
    array[1] = 0x0000B400;
    dsi_set_cmdq(&array, 2, 1); 
    array[0] = 0x00013700;
    dsi_set_cmdq(&array, 1,1);
    read_reg_v2(0xC0, buffer,7);
    printk("otm9605a_tm buffer[0] = %d\n",buffer[0]);
    if(COLUMN_INVERSION == mode)
    {
        if(buffer[0] == column_Inversion_value )
        {
            printk("otm9605a_tm column_Inversion_value set_inversion_mode success! buffer[0] = %d\n",buffer[0]);
    	    return TRUE;
    	}
        else
        {
            printk("otm9605a_tm column_Inversion_value set_inversion_mode fail! buffer[0] = %d\n",buffer[0]);
    	    return FALSE;
        }
    }
    else
    {
        if(buffer[0] == dot_Inversion_value )
    	{
    	    printk("otm9605a_tm dot_Inversion_value set_inversion_mode success! buffer[0] = %d\n",buffer[0]);
    	    return TRUE;
    	}
        else
		{
    	    printk("otm9605a_tm dot_Inversion_value set_inversion_mode fail! buffer[0] = %d\n",buffer[0]);
    	    return FALSE;
    	}
    }
}
extern UINT32 DSI_dcs_read_lcm_reg_v3(UINT8 cmd, UINT8 *buffer, UINT8 buffer_size);
static unsigned int lcm_check_state()
{
   	unsigned char buffer_1[12] = {0};
	unsigned int array_1[16] = {0};

   	//---------------------------------
   	// Read [9Ch, 00h, ECC] + Error Report(4 Bytes)
   	//---------------------------------
   	array_1[0] = 0x00013700;
   	dsi_set_cmdq(array_1, 1,1);
    DSI_dcs_read_lcm_reg_v3(0x0A, buffer_1,7);
    if(buffer_1[0] == 0x11)
    {
		if(buffer_1[1] != 0x9C)
		{
            cnt++;
            printk("otm9605a_tianma lcm_check_state fail! 0x0A = %x,cnt = %d\n",buffer_1[1],cnt);
		    //read three times fail , judge error
		    if( cnt >= max_error_num)
		    {
                cnt = 0;
                return FALSE;
		    }
		    return TRUE; 
		}
		else/*LCD work status ok*/
		{
		    return TRUE;
		}
    }
    else //error report 0x40 or ack ,No judge error
    {
         printk("otm9605a_tianma lcm_check_state ok! error report DT_0x0A = %x\n",buffer_1[0]);
         return TRUE;
    }

}
#endif
#ifndef BUILD_LK
/******************************************************************************
Function:       lcm_esd_check
Description:    check LCD work status for ESD
Input:          none
Output:         none
Return:         FALSE: NO ESD happened
                TRUE: ESD happened
Others:
******************************************************************************/
static unsigned int lcm_esd_check(void)
{
	unsigned char buffer[8] = {0};
    unsigned int array[4] = {0};

	array[0] = 0x00013700;
	dsi_set_cmdq(array,1,1);
    DSI_dcs_read_lcm_reg_v3(0x0A, buffer,7);
    if(buffer[0] == 0x11)
    {
        if(buffer[1] != 0x9c)
        {
            printk( "otm9605a_tianma lcm_esd_check fail!0x0A = %x\n",buffer[1]);
            return TRUE;
        }
		else
		{
			return FALSE;
		}		
    }
    else
    {
		return FALSE;
    }
}
static unsigned int lcm_esd_recover(void)
{
    /*LCD work status error ,so initialize*/
    printk( "otm9605a_tm lcm_esd_recover\n");
    lcm_util.set_gpio_out(GPIO_DISP_LRSTB_PIN, GPIO_OUT_ONE);
    msleep(20);//lcm power on , reset output high , delay 30ms ,then output low
    lcm_util.set_gpio_out(GPIO_DISP_LRSTB_PIN, GPIO_OUT_ZERO);
    msleep(20);
    lcm_util.set_gpio_out(GPIO_DISP_LRSTB_PIN, GPIO_OUT_ONE);
    msleep(120);//when reset applied during sleep out mode , after reset high-low-high , delay 120ms , then download initial code
    push_table(otm9605a_qhd_vdo_tm_init, sizeof(otm9605a_qhd_vdo_tm_init) / sizeof(struct LCM_setting_table), 1);
    lcm_setbacklight(100);
    return TRUE;
}
#endif
LCM_DRIVER otm9605a_qhd_vdo_tm_lcm_drv =
{
    .name           	= "otm9605a_qhd_vdo_tm",
    .set_util_funcs 	= lcm_set_util_funcs,
    .get_params     	= lcm_get_params,
    .init           	= lcm_init,
    .suspend        	= lcm_suspend,
    .resume         	= lcm_resume,
    .compare_id     	= lcm_compare_id,
    .set_backlight  	= lcm_setbacklight,
#ifndef BUILD_LK
    .set_inversion_mode =  lcm_setInversion_mode,
    .lcm_check_state  	=  lcm_check_state,
    .lcm_check_inversion_set = lcm_check_inversion_set,
	.esd_check      	= lcm_esd_check,
    .esd_recover    	= lcm_esd_recover,
#endif
};
