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

const static unsigned char lcd_id = 0x01; //ID0=1,ID1=0;lcd_id=lcd_id0|(lcd_id1 << 1)
// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------

#define FRAME_WIDTH  										(720)
#define FRAME_HEIGHT 										(1280)
#define REGFLAG_DELAY             							0xFE
#define REGFLAG_END_OF_TABLE      							0xFD// END OF REGISTERS MARKER

#ifndef TRUE
    #define TRUE 1
#endif

#ifndef FALSE
    #define FALSE 0
#endif
#define LCM_DSI_CMD_MODE									1

static unsigned int lcm_esd_test = FALSE;      ///only for ESD test
//avoid 12.5% duty brightness value for TI backlight driver chip bug
static unsigned int MIN_VALUE_DUTY_ONE_EIGHT = 29;
static unsigned int MAX_VALUE_DUTY_ONE_EIGHT = 34;
//avoid 25% duty brightness value for TI backlight driver chip bug
static unsigned int MIN_VALUE_DUTY_ONE_FOUR = 59;
static unsigned int MAX_VALUE_DUTY_ONE_FOUR = 69;
//avoid 50% duty brightness value for TI backlight driver chip bug
static unsigned int MIN_VALUE_DUTY_ONE_TWO = 123;
static unsigned int MAX_VALUE_DUTY_ONE_TWO = 133;

static const unsigned int min_value_duty = 8;
static const unsigned int max_value_duty = 245;

static unsigned int  dot_Inversion_value0 = 0x00;
static unsigned int  dot_Inversion_value1 = 0x08;

static unsigned int  column_Inversion_value0 = 0x55;
static unsigned int  column_Inversion_value1 = 0x0D;
// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static LCM_UTIL_FUNCS lcm_util = {0};

#define SET_RESET_PIN(v)    								(lcm_util.set_reset_pin((v)))

// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------

#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)	        lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)		lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)										lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)					lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg(cmd)											lcm_util.dsi_dcs_read_lcm_reg(cmd)
#define read_reg_v2(cmd, buffer, buffer_size)   				lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)    

struct LCM_setting_table {
    unsigned char cmd;
    unsigned char count;
    unsigned char para_list[64];
};

static struct LCM_setting_table nt35590_tm_init_table[] = {
	{0xFF,	1,	{0x00}},//enter cmd1
	{0xFB,	1,	{0x01}},
	{0x3B,	5,	{0x03,0x06,0x03,0x02,0x02,}},
	//Use OTP parameters
	{0xFF, 	1,	{0x04}},
	{0xFB, 	1,	{0x01}},
	{0x0A, 	1,	{0x02}},//pwm:42.9kHz
	{0xFF, 	1,	{0x00}},

	{0xFF, 	1,	{0x00}},
	{0xFB, 	1,	{0x01}},
	{0xC2, 	1,	{0x08}},//command mode
	{0xBA, 	1,	{0x03}},//mipi 4lane

	{0x53, 	1,	{0x24}},
	{0x55, 	1,	{0x00}},// CLOSE cabc
	{0x35, 	1,	{0x00}},

	{0x11, 	1,	{0x00}},
	{REGFLAG_DELAY, 120, {}},
	{0x29, 	1,	{0x00}},
	{REGFLAG_DELAY, 20, {}},
	{REGFLAG_END_OF_TABLE, 0x00, {}},
};

static struct LCM_setting_table lcm_sleep_out_setting[] = {
	// Sleep Out
	{0x11, 1, {0x00}},
	{REGFLAG_DELAY, 120, {}},

	// Display ON
	{0x29, 1, {0x00}},
	{REGFLAG_DELAY, 20, {}},
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};

static struct LCM_setting_table lcm_deep_sleep_mode_in_setting[] = {
	// Display off sequence
	{0x28, 1, {0x00}},
	{REGFLAG_DELAY, 20, {}},

	// Sleep Mode On
	{0x10, 1, {0x00}},
	{REGFLAG_DELAY, 120, {}},
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};

static struct LCM_setting_table lcm_backlight_level_setting[] = {
	{0x51, 1, {0xFF}},
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};

static void push_table(struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
{
    unsigned int i;

    for(i = 0; i < count; i++)
    {
        unsigned cmd;
        cmd = table[i].cmd;

        switch (cmd) {

            case REGFLAG_DELAY :
                if(table[i].count <= 20)
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

#if (LCM_DSI_CMD_MODE)
    params->dsi.mode   = CMD_MODE;
#else
    params->dsi.mode   = SYNC_PULSE_VDO_MODE;
#endif
    // DSI
    /* Command mode setting */
    params->dsi.LANE_NUM				= LCM_FOUR_LANE;
    //The following defined the fomat for data coming from LCD engine
    params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;
    params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;

    params->dsi.CLK_HS_POST = 29;
    params->dsi.CLK_HS_PRPR = 4;
    params->dsi.HS_PRPR     = 4;
    params->dsi.PLL_CLOCK = 224;
    params->dsi.ssc_disable = 1;//Close spread spectrum: 1; Open it: 0.
}

static void lcm_power_on(void)
{
    //reset high-low-high
    lcm_util.set_gpio_out(GPIO_DISP_LRSTB_PIN, GPIO_OUT_ONE);
    mdelay(5);
    lcm_util.set_gpio_out(GPIO_DISP_LRSTB_PIN, GPIO_OUT_ZERO);
    mdelay(5);
    lcm_util.set_gpio_out(GPIO_DISP_LRSTB_PIN, GPIO_OUT_ONE);
    mdelay(120);
}

static void lcm_resume_on(void)
{
    //enable VSP & VSN
    lcm_util.set_gpio_out(GPIO_LCD_BIAS_ENP_PIN, GPIO_OUT_ONE);
    mdelay(5);
    lcm_util.set_gpio_out(GPIO_LCD_BIAS_ENN_PIN, GPIO_OUT_ONE);
    mdelay(15);
    //reset high-low-high
    lcm_util.set_gpio_out(GPIO_DISP_LRSTB_PIN, GPIO_OUT_ONE);
    mdelay(5);
    lcm_util.set_gpio_out(GPIO_DISP_LRSTB_PIN, GPIO_OUT_ZERO);
    mdelay(5);
    lcm_util.set_gpio_out(GPIO_DISP_LRSTB_PIN, GPIO_OUT_ONE);
    mdelay(20);
}

static void lcm_id_pin(void)
{
    mt_set_gpio_pull_select(GPIO_DISP_ID0_PIN,GPIO_PULL_UP);
    mt_set_gpio_pull_select(GPIO_DISP_ID1_PIN,GPIO_PULL_DOWN);
}

static void lcm_init(void)
{
    lcm_power_on();
    lcm_id_pin();
    push_table(nt35590_tm_init_table, sizeof(nt35590_tm_init_table) / sizeof(struct LCM_setting_table), 1);
    lcm_util.set_gpio_out(GPIO_LCD_DRV_EN_PIN, GPIO_OUT_ONE); 
    #ifdef BUILD_LK
        printf("uboot:nt35590_tm_lcm_init\n");
    #else
        printk("kernel:nt35590_tm_lcm_init\n");
    #endif
}

static void lcm_suspend(void)
{
    lcm_util.set_gpio_out(GPIO_LCD_DRV_EN_PIN, GPIO_OUT_ZERO);
    push_table(lcm_deep_sleep_mode_in_setting, sizeof(lcm_deep_sleep_mode_in_setting) / sizeof(struct LCM_setting_table), 1);
    //reset low
    lcm_util.set_gpio_out(GPIO_DISP_LRSTB_PIN, GPIO_OUT_ZERO);
    mdelay(10);
    //disable VSP & VSN
    lcm_util.set_gpio_out(GPIO_LCD_BIAS_ENN_PIN, GPIO_OUT_ZERO);
    mdelay(5); //
    lcm_util.set_gpio_out(GPIO_LCD_BIAS_ENP_PIN, GPIO_OUT_ZERO);
    #ifdef BUILD_LK
        printf("uboot:nt35590_tm_lcm_suspend\n");
    #else
        printk("kernel:nt35590_tm_lcm_suspend\n");
    #endif
}

static void lcm_resume(void)
{
    lcm_resume_on();
    push_table(nt35590_tm_init_table, sizeof(nt35590_tm_init_table) / sizeof(struct LCM_setting_table), 1);
    lcm_util.set_gpio_out(GPIO_LCD_DRV_EN_PIN, GPIO_OUT_ONE);
    #ifdef BUILD_LK
        printf("uboot:nt35590_tm_lcm_resume\n");
    #else
        printk("kernel:nt35590_tm_lcm_resume\n");
    #endif
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
    if(level < 0)
        level = 0;
    if((0< level ) && (min_value_duty >= level) )
    {
        level = min_value_duty; //min duty > 500ns*45.5KHz
    }
    else if(max_value_duty <= level)
    {
        level = max_value_duty; //max duty < 1-500ns*45.5KHz
    }

    if((level >= MIN_VALUE_DUTY_ONE_EIGHT) && (level <= MAX_VALUE_DUTY_ONE_EIGHT )) //12.5% duty shanshuo
    {
        //avoid 12.5% duty brightness value for TI backlight driver chip bug
        level = MIN_VALUE_DUTY_ONE_EIGHT-1;
    }
    else if((level >= MIN_VALUE_DUTY_ONE_FOUR) && (level <= MAX_VALUE_DUTY_ONE_FOUR))
    {
        //avoid 25% duty brightness value for TI backlight driver chip bug
        level = MIN_VALUE_DUTY_ONE_FOUR-1;
    }
    else if((level >= MIN_VALUE_DUTY_ONE_TWO) && (level <= MAX_VALUE_DUTY_ONE_TWO))
    {
        //avoid 50% duty brightness value for TI backlight driver chip bug
        level = MIN_VALUE_DUTY_ONE_TWO-1;
    }

    #ifdef BUILD_LK
        printf("LK:lcm_setbacklight level=%d\n",level);
    #else
        printk("kernel:lcm_setbacklight level=%d\n",level);
    #endif
    lcm_backlight_level_setting[0].para_list[0] = level;

    push_table(lcm_backlight_level_setting, sizeof(lcm_backlight_level_setting) / sizeof(struct LCM_setting_table), 1);
}

static unsigned int lcm_compare_id(void)
{
    unsigned char module_id = which_lcd_module();
    return ((lcd_id == module_id )? 1 : 0);
}

#ifndef BUILD_LK
static struct LCM_setting_table lcm_Inversion_mode_setting[] = {
    {0xFF,1,{0x05}},
    {0xFB,1,{0x01}}, //
    {0x22,1,{0x55}},   //column inversion
    {0x23,1,{0x0D}},
    {REGFLAG_END_OF_TABLE, 0x00, {}}
};

static void lcm_setInversion_mode(unsigned int mode)
{
    printk("nt35590_hd720_cmd_tm50 lcm_setInversion_mode mode = %d\n", mode);
    if(DOT_INVERSION == mode)//DOT_INVERSION
    {
        lcm_Inversion_mode_setting[2].para_list[0] = dot_Inversion_value0;
        lcm_Inversion_mode_setting[3].para_list[0] = dot_Inversion_value1;
    }
    else  //column INVERSION
    {
        lcm_Inversion_mode_setting[2].para_list[0] = column_Inversion_value0;
        lcm_Inversion_mode_setting[3].para_list[0] = column_Inversion_value1;
    }
    push_table(lcm_Inversion_mode_setting, sizeof(lcm_Inversion_mode_setting) / sizeof(struct LCM_setting_table), 1);
}

static unsigned int lcm_check_inversion_set(unsigned int mode)
{
    unsigned char buffer_0[12] = {0};
    unsigned char buffer_1[12] = {0};
    unsigned int array[16];

    array[0] = 0x00023902;
    array[1] = 0x000005FF;   
    dsi_set_cmdq(array, 2,1);
	
    array[0] = 0x00023902;
    array[1] = 0x000001FB;   
    dsi_set_cmdq(array, 2,1);
	
    array[0] = 0x00013700;
    dsi_set_cmdq(array, 1,1);
    read_reg_v2(0x22, buffer_0,1);
    printk("nt35590_hd720_cmd_tm50 buffer_0[0] = %d\n",buffer_0[0]);
	
    array[0] = 0x00013700;
    dsi_set_cmdq(array, 1,1);
    read_reg_v2(0x23, buffer_1,1);
    printk("nt35590_hd720_cmd_tm50 buffer_1[0] = %d\n",buffer_1[0]);
    
    array[0] = 0x00023902;
    array[1] = 0x000000FF;   
    dsi_set_cmdq(array, 2,1);
	
    if(COLUMN_INVERSION == mode)
    {
        if(buffer_0[0] == column_Inversion_value0 && buffer_1[0] == column_Inversion_value1 )
        {
            printk("nt35590_hd720_cmd_tm50 column_Inversion_value set_inversion_mode success! buffer_0[0] = %d buffer_1[0] = %d\n",buffer_0[0],buffer_1[0]);
            return TRUE;
        }
        else
        {
            printk("nt35590_hd720_cmd_tm50 column_Inversion_value set_inversion_mode fail! buffer_0[0] = %d buffer_1[0] = %d\n",buffer_0[0],buffer_1[0]);
            return FALSE;
        }
    }
    else
    {
        if(buffer_0[0] == dot_Inversion_value0 && buffer_1[0] == dot_Inversion_value1 )
        {
            printk("nt35590_hd720_cmd_tm50 dot_Inversion_value set_inversion_mode success! buffer_0[0] = %d  buffer_1[0] = %d \n",buffer_0[0],buffer_1[0]);
            return TRUE;
        }
        else
        {
            printk("nt35590_hd720_cmd_tm50 dot_Inversion_value set_inversion_mode fail! buffer_0[0] = %d buffer_1[0] = %d\n",buffer_0[0],buffer_1[0]);
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
    unsigned int array[16] = {0};
    array[0] = 0x00023902;
    array[1] = 0x000000FF;
    dsi_set_cmdq(array, 2,1);
    array[0] = 0x00023902;
    array[1] = 0x000001FB;
    dsi_set_cmdq(array, 2,1);
    //---------------------------------
    // Read [9Ch, 00h, ECC] + Error Report(4 Bytes)
    //---------------------------------
    array_1[0] = 0x00013700;
    dsi_set_cmdq(array_1, 1,1);
    read_reg_v2(0x0A, buffer_1,7);
    printk("nt35590_hd720_cmd_tm50  buffer_1[0] = %d\n",buffer_1[0]);
	
    array_1[0] = 0x00013700;
    dsi_set_cmdq(array_1, 1,1);
    read_reg_v2(0x0B, buffer_2,7);
    printk("nt35590_hd720_cmd_tm50  buffer_2[0] = %d\n",buffer_2[0]);
	
    array_1[0] = 0x00013700;
    dsi_set_cmdq(array_1, 1,1);
    read_reg_v2(0x0C, buffer_3,7);
    printk("nt35590_hd720_cmd_tm50  buffer_3[0] = %d\n",buffer_3[0]);
	
    array_1[0] = 0x00013700;
    dsi_set_cmdq(array_1, 1,1);
    read_reg_v2(0x0D, buffer_4,7);
    printk("nt35590_hd720_cmd_tm50  buffer_4[0] = %d\n",buffer_4[0]);

    if((buffer_1[0] != 0x9C) || (buffer_2[0] != 0x00) || (buffer_3[0] != 0x77) || (buffer_4[0] != 0x00))/*LCD work status error,need re-initalize*/
    {
        printk("nt35590_hd720_cmd_tm50 lcm_esd_check fail! buffer_1[0] = %d buffer_2[0] = %d buffer_3[0] = %d buffer_4[0] = %d\n",buffer_1[0],buffer_2[0],buffer_3[0],buffer_4[0]);
        return FALSE;
    }
    else/*LCD work status ok*/
    {
        printk("nt35590_hd720_cmd_tm50 lcm_esd_check ok! buffer_1[0] = %d buffer_2[0] = %d buffer_3[0] = %d buffer_4[0] = %d\n",buffer_1[0],buffer_2[0],buffer_3[0],buffer_4[0]);
        return TRUE;
    }
}
#endif

// ---------------------------------------------------------------------------
//  Get LCM Driver Hooks
// ---------------------------------------------------------------------------
LCM_DRIVER nt35590_hd720_cmd_tm50_lcm_drv = 
{
    .name           = "nt35590_hd720_cmd_tm50",
    .set_util_funcs = lcm_set_util_funcs,
    .get_params     = lcm_get_params,
    .init           = lcm_init,
    .suspend        = lcm_suspend,
    .resume         = lcm_resume,
#if (LCM_DSI_CMD_MODE)
    .update         = lcm_update,
    .set_backlight = lcm_setbacklight,
    .compare_id     = lcm_compare_id,
#endif
#ifndef BUILD_LK
    .set_inversion_mode =  lcm_setInversion_mode,
    .lcm_check_state  	=  lcm_check_state,
    .lcm_check_inversion_set = lcm_check_inversion_set,
#endif
};
