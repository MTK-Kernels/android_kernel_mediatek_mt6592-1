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
#ifndef BUILD_LK
#include <linux/string.h>
#endif
#include "lcm_drv.h"

#ifdef BUILD_LK
    #include <platform/disp_drv_platform.h>
	#include <string.h>
#elif defined(BUILD_UBOOT)
    #include <asm/arch/mt_gpio.h>
#else
    #include <linux/delay.h>
    #include <mach/mt_gpio.h>
#endif
static unsigned int dot_Inversion_value = 0x01;
static unsigned int column_Inversion_value = 0x00;
#ifdef BUILD_LK
#define LCD_DEBUG(fmt)  dprintf(CRITICAL,fmt)
#else
#define LCD_DEBUG(fmt)  printk(fmt)
#endif


static const unsigned char LCD_MODULE_ID = 0x02;
// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------
#define FRAME_WIDTH  										(720)
#define FRAME_HEIGHT 										(1280)


#define REGFLAG_DELAY             							0xFC
#define REGFLAG_END_OF_TABLE      							0xFD   // END OF REGISTERS MARKER

#ifndef TRUE
    #define TRUE 1
#endif

#ifndef FALSE
    #define FALSE 0
#endif

// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static LCM_UTIL_FUNCS lcm_util;
#define MDELAY(n) 											(lcm_util.mdelay(n))

// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------

#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)	lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)		lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)										lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)					lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg(cmd)										lcm_util.dsi_dcs_read_lcm_reg(cmd)
#define read_reg_v2(cmd, buffer, buffer_size)   			lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)    

struct LCM_setting_table {
    unsigned char cmd;
    unsigned char count;
    unsigned char para_list[128];
};
//update initial param for IC nt35521 0.01
static struct LCM_setting_table lcm_initialization_setting_tm[] = {
    {0xff,  4,  {0xaa,0x55,0xa5,0x80}},
	{0x6f,  2,  {0x11,0x00}},
	{0xf7,  2,  {0x20,0x00}},
    {0x6f,  1,  {0x11}},
    {0xf3,  1,  {0x01}},

	{0xf0,  5,  {0x55,0xaa,0x52,0x08,0x00}},
	{0xbd,  5,  {0x01,0xa0,0x0c,0x08,0x01}},
	{0x6f,  1,  {0x02}},
    {0xb8,1,{0x01}},
	{0xbb,  2,  {0x11,0x11}},
	{0xbc,  2,  {0x00,0x00}},
	{0xb6,  1,  {0x06}},
	{0xf0,  5,  {0x55,0xaa,0x52,0x08,0x01}},
	{0xb0,  2,  {0x09,0x09}},
	{0xb1,  2,  {0x09,0x09}},
	{0xbc,  2,  {0x78,0x00}},
	{0xbd,  2,  {0x78,0x00}},
	{0xca,  1,  {0x00}},
	{0xc0,  1,  {0x04}},
	{0xb5,  2,  {0x03,0x03}},
	{0xbe,  1,  {0x5b}},//vcom need reserve , will reload otp  
	{0xb3,  2,  {0x28,0x28}},
	{0xb4,  2,  {0x0f,0x0f}},
	{0xb9,  2,  {0x34,0x34}},
	{0xba,  2,  {0x15,0x15}},
	
	{0xf0,  5,  {0x55,0xaa,0x52,0x08,0x02}},
	{0xee,  1,  {0x01}},

	//update the gamma
	{0xB0,16,{0x00,0x00,0x00,0x95,0x00,0xBB,0x00,0xD6,0x00,0xEB,0x01,0x0D,0x01,0x28,0x01,0x54}},

    {0xB1,16,{0x01,0x76,0x01,0xAD,0x01,0xD8,0x02,0x1B,0x02,0x54,0x02,0x55,0x02,0x88,0x02,0xBE}},

    {0xB2,16,{0x02,0xE1,0x03,0x0E,0x03,0x2B,0x03,0x51,0x03,0x6A,0x03,0x89,0x03,0x9D,0x03,0xB5}},

    {0xB3,4,{0x03,0xE0,0x03,0xFF}},

	{0x6f,  1,  {0x02}},
	{0xf7,  1,  {0x47}},

	{0x6f,  1,  {0x0a}},
	{0xf7,  1,  {0x02}},

	{0x6f,  1,  {0x17}},
	{0xf4,  1,  {0x70}},

	{0x6f,  1,  {0x11}},
	{0xf3,  1,  {0x01}},

	{0xf0,  5,  {0x55,0xaa,0x52,0x08,0x06}},
	{0xb0,  2,  {0x12,0x10}},
	{0xb1,  2,  {0x18,0x16}},
	{0xb2,  2,  {0x00,0x02}},
	{0xb3,  2,  {0x31,0x31}},
	{0xb4,  2,  {0x31,0x31}},
	{0xb5,  2,  {0x31,0x31}},
	{0xb6,  2,  {0x31,0x31}},
	{0xb7,  2,  {0x31,0x31}},
	{0xb8,  2,  {0x31,0x08}},
	{0xb9,  2,  {0x2e,0x2d}},
	{0xba,  2,  {0x2d,0x2e}},
	{0xbb,  2,  {0x09,0x31}},
	{0xbc,  2,  {0x31,0x31}},
	{0xbd,  2,  {0x31,0x31}},
	{0xbe,  2,  {0x31,0x31}},
	{0xbf,  2,  {0x31,0x31}},
	{0xc0,  2,  {0x31,0x31}},
	{0xc1,  2,  {0x03,0x01}},
	{0xc2,  2,  {0x17,0x19}},
	{0xc3,  2,  {0x11,0x13}},
	{0xe5,  2,  {0x31,0x31}},

	{0xc4,  2,  {0x17,0x19}},
	{0xc5,  2,  {0x11,0x13}},
	{0xc6,  2,  {0x03,0x01}},
	{0xc7,  2,  {0x31,0x31}},
	{0xc8,  2,  {0x31,0x31}},
	{0xc9,  2,  {0x31,0x31}},
	{0xca,  2,  {0x31,0x31}},
	{0xcb,  2,  {0x31,0x31}},
	{0xcc,  2,  {0x31,0x09}},
	{0xcd,  2,  {0x2d,0x2e}},
	{0xce,  2,  {0x2e,0x2d}},
	{0xcf,  2,  {0x08,0x31}},
	{0xd0,  2,  {0x31,0x31}},
	{0xd1,  2,  {0x31,0x31}},
	{0xd2,  2,  {0x31,0x31}},
	{0xd3,  2,  {0x31,0x31}},
	{0xd4,  2,  {0x31,0x31}},
	{0xd5,  2,  {0x00,0x02}},
	{0xd6,  2,  {0x12,0x10}},
	{0xd7,  2,  {0x18,0x16}},

	{0xd8,  5,  {0x00,0x00,0x00,0x00,0x00}},
	{0xd9,  5,  {0x00,0x00,0x00,0x00,0x00}},
	{0xe7,  1,  {0x00}},

	{0xf0,  5,  {0x55,0xaa,0x52,0x08,0x05}},
	{0xed,  1,  {0x30}},

	{0xf0,  5,  {0x55,0xaa,0x52,0x08,0x03}},
	{0xb1,  2,  {0x20,0x00}},
	{0xb0,  2,  {0x20,0x00}},

	{0xf0,  5,  {0x55,0xaa,0x52,0x08,0x05}},
	{0xe5,  1,  {0x00}},

	{0xf0,  5,  {0x55,0xaa,0x52,0x08,0x05}},
	{0xb0,  2,  {0x17,0x06}},
	{0xb8,  1,  {0x00}},

        {0xbd,  5,  {0x03,0x03,0x01,0x00,0x03}},
	{0xb1,  2,  {0x17,0x06}},
	{0xb9,  2,  {0x00,0x03}},
	{0xb2,  2,  {0x17,0x06}},
	{0xba,  2,  {0x00,0x00}},
	{0xb3,  2,  {0x17,0x06}},
	{0xbb,  2,  {0x00,0x00}},
	{0xb4,  2,  {0x17,0x06}},
	{0xb5,  2,  {0x17,0x06}},
	{0xb6,  2,  {0x17,0x06}},
	{0xb7,  2,  {0x17,0x06}},
	{0xbc,  2,  {0x00,0x03}},
	{0xe5,  1,  {0x06}},
	{0xe6,  1,  {0x06}},
	{0xe7,  1,  {0x06}},
	{0xe8,  1,  {0x06}},
	{0xe9,  1,  {0x06}},
	{0xea,  1,  {0x06}},
	{0xeb,  1,  {0x06}},
	{0xec,  1,  {0x06}},
	{0xf0,  5,  {0x55,0xaa,0x52,0x08,0x05}},
	{0xc0,  1,  {0x0b}},
	{0xc1,  1,  {0x09}},
	{0xc2,  1,  {0x0b}},
	{0xc3,  1,  {0x09}},

	{0xf0,  5,  {0x55,0xaa,0x52,0x08,0x03}},
	{0xb2,  5,  {0x05,0x00,0x00,0x00,0x90}},
	{0xb3,  5,  {0x05,0x00,0x00,0x00,0x90}},
	{0xb4,  5,  {0x05,0x00,0x00,0x00,0x90}},
	{0xb5,  5,  {0x05,0x00,0x00,0x00,0x90}},

	{0xf0,  5,  {0x55,0xaa,0x52,0x08,0x05}},
	{0xc4,  1,  {0x10}},
	{0xc5,  1,  {0x10}},
	{0xc6,  1,  {0x10}},
	{0xc7,  1,  {0x10}},
	
	{0xf0,  5,  {0x55,0xaa,0x52,0x08,0x03}},
        {0xb6,  5,  {0x05,0x00,0x00,0x00,0x90}},
        {0xb7,  5,  {0x05,0x00,0x00,0x00,0x90}},
        {0xb8,  5,  {0x05,0x00,0x00,0x00,0x90}},
        {0xb9,  5,  {0x05,0x00,0x00,0x00,0x90}},

	{0xf0,  5,  {0x55,0xaa,0x52,0x08,0x05}},
	{0xc8,  2,  {0x08,0x20}},
	{0xc9,  2,  {0x04,0x20}},
	{0xca,  2,  {0x07,0x00}},
	{0xcb,  2,  {0x03,0x00}},

	{0xf0,  5,  {0x55,0xaa,0x52,0x08,0x03}},
    {0xBA,5,{0x53,0x01,0x00,0x01,0x00}},
    {0xBB,5,{0x53,0x01,0x00,0x01,0x00}},
    {0xBC,5,{0x53,0x01,0x00,0x01,0x00}},
    {0xBD,5,{0x53,0x01,0x00,0x01,0x00}},

	{0xf0,  5,  {0x55,0xaa,0x52,0x08,0x05}},
        {0xd1,  5,  {0x00,0x05,0x00,0x07,0x10}},
        {0xd2,  5,  {0x00,0x05,0x04,0x07,0x10}},
        {0xd3,  5,  {0x00,0x00,0x0a,0x07,0x10}},
        {0xd4,  5,  {0x00,0x00,0x0a,0x07,0x10}},

	{0xf0,  5,  {0x55,0xaa,0x52,0x08,0x05}},
        {0xd0,  7,  {0x00,0x00,0x00,0x00,0x00,0x00,0x00}},
        {0xd5,  11, {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}},
        {0xd6,  11, {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}},
        {0xd7,  11, {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}},
        {0xd8,  5,  {0x00,0x00,0x00,0x00,0x00}},

	{0xf0,  5,  {0x55,0xaa,0x52,0x08,0x03}},
	{0xc4,  1,  {0x60}},
	{0xc5,  1,  {0x40}},
	{0xc6,  1,  {0x60}},
	{0xc7,  1,  {0x40}},

	{0x6f,  1,  {0x01}},
	{0xf9,  1,  {0x46}},
	
	//SleepOut reload enable
	{0x6f,  1,  {0x12}},
	{0xf3,  1,  {0x80}},
	//MTP reload no-reload
	{0x6f,	1,	{0x0c}},
	{0xf3,	4,	{0xff,0x0f,0xff,0xff}},

	{0x11,1,{0x00}},
	{REGFLAG_DELAY, 150, {}},
	
    {0x6F,1,{0x12}},
    {0xF3,1,{0x00}},

    {0xF0,5,{0x55,0xAA,0x52,0x08,0x01}},

    {0xBC,2,{0x78,0x00}},

    {0xBD,2,{0x78,0x00}},

    {0xF0,5,{0x55,0xAA,0x52,0x08,0x02}},

    {0xEE,1,{0x01}},

	//update the gamma
	{0xB0,16,{0x00,0x00,0x00,0x95,0x00,0xBB,0x00,0xD6,0x00,0xEB,0x01,0x0D,0x01,0x28,0x01,0x54}},

    {0xB1,16,{0x01,0x76,0x01,0xAD,0x01,0xD8,0x02,0x1B,0x02,0x54,0x02,0x55,0x02,0x88,0x02,0xBE}},

    {0xB2,16,{0x02,0xE1,0x03,0x0E,0x03,0x2B,0x03,0x51,0x03,0x6A,0x03,0x89,0x03,0x9D,0x03,0xB5}},

    {0xB3,4,{0x03,0xE0,0x03,0xFF}},

    /* modify PWM frequency to 45.9 kHz */
    {0xF0,5,{0x55,0xAA,0x52,0x08,0x00}},

    {0xD9,2,{0x02,0x05}},

    //{0x55,1,{0x01}},

    //{0x51,1,{0x64}},

    //{0x53,1,{0x24}},
    
	{0x29,1,{0x00}},
	{REGFLAG_DELAY, 20, {}},
	{REGFLAG_END_OF_TABLE, 0x00, {}},
};
							
#if 0
static struct LCM_setting_table lcm_sleep_out_setting[] = {
    //Sleep Out
    {0x11, 1, {0x00}},
    {REGFLAG_DELAY, 120, {}},

    // Display ON
    {0x29, 1, {0x00}},
    {REGFLAG_DELAY, 20, {}},
    {REGFLAG_END_OF_TABLE, 0x00, {}}
};
#endif

static struct LCM_setting_table lcm_deep_sleep_mode_in_setting[] = {
    // Display off sequence
    {0x28, 1, {0x00}},
    {REGFLAG_DELAY, 20, {}},

    // Sleep Mode On
    {0x10, 1, {0x00}},
    {REGFLAG_DELAY, 120, {}},
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
                if(table[i].count <= 10)
                    mdelay(table[i].count);
                else
                    mdelay(table[i].count);
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
    params->dsi.LANE_NUM				= LCM_FOUR_LANE;
    //The following defined the fomat for data coming from LCD engine.
    params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
    params->dsi.data_format.trans_seq   = LCM_DSI_TRANS_SEQ_MSB_FIRST;
    params->dsi.data_format.padding     = LCM_DSI_PADDING_ON_LSB;
    params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;

   // Highly depends on LCD driver capability.
   //video mode timing

    params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;

    params->dsi.vertical_sync_active				= 2;
    params->dsi.vertical_backporch				= 20;
    params->dsi.vertical_frontporch				= 20;
    params->dsi.vertical_active_line				= FRAME_HEIGHT;

    params->dsi.horizontal_sync_active				= 10;
    params->dsi.horizontal_backporch				= 63;
    params->dsi.horizontal_frontporch				= 155;
    params->dsi.horizontal_active_pixel				= FRAME_WIDTH;
     
    //improve clk quality
    params->dsi.PLL_CLOCK = 241; //this value must be in MTK suggested table
    //params->dsi.compatibility_for_nvk = 1;

    params->dsi.ssc_disable = 1;//h84013687 add at 2013.09.16
}
/*to prevent electric leakage*/
static void lcm_id_pin_handle(void)
{
    mt_set_gpio_pull_select(GPIO_DISP_ID0_PIN,GPIO_PULL_DOWN);
    mt_set_gpio_pull_select(GPIO_DISP_ID1_PIN,GPIO_PULL_DOWN);
}

static void lcm_init_tm(void)
{
	//reset high to low to high
    lcm_util.set_gpio_out(GPIO_DISP_LRSTB_PIN, GPIO_OUT_ONE);
	msleep(20); 
    lcm_util.set_gpio_out(GPIO_DISP_LRSTB_PIN, GPIO_OUT_ZERO);
    msleep(20); 
    lcm_util.set_gpio_out(GPIO_DISP_LRSTB_PIN, GPIO_OUT_ONE);
    msleep(120); 
    lcm_id_pin_handle();
	// when phone initial , config output high, enable backlight drv chip  
    push_table(lcm_initialization_setting_tm, sizeof(lcm_initialization_setting_tm) / sizeof(struct LCM_setting_table), 1);  

    lcm_util.set_gpio_out(GPIO_LCD_DRV_EN_PIN, GPIO_OUT_ONE);

    LCD_DEBUG("uboot:tm_nt35521_lcm_init\n");
}

static void lcm_suspend(void)
{
    //Back to MP.P7 baseline , solve LCD display abnormal On the right
    // when phone sleep , config output low, disable backlight drv chip  
    lcm_util.set_gpio_out(GPIO_LCD_DRV_EN_PIN, GPIO_OUT_ZERO);
    push_table(lcm_deep_sleep_mode_in_setting, sizeof(lcm_deep_sleep_mode_in_setting) / sizeof(struct LCM_setting_table), 1);
    //reset low
    lcm_util.set_gpio_out(GPIO_DISP_LRSTB_PIN, GPIO_OUT_ZERO);
	mdelay(5);
    //disable VSP & VSN
    lcm_util.set_gpio_out(GPIO_LCD_BIAS_ENN_PIN, GPIO_OUT_ZERO);
	mdelay(10);
	lcm_util.set_gpio_out(GPIO_LCD_BIAS_ENP_PIN, GPIO_OUT_ZERO);
    LCD_DEBUG("uboot:tm_nt35521_lcm_suspend\n");

}

static void lcm_resume_tm(void)
{

    //enable VSP & VSN
	lcm_util.set_gpio_out(GPIO_LCD_BIAS_ENP_PIN, GPIO_OUT_ONE);
	mdelay(10);
	lcm_util.set_gpio_out(GPIO_LCD_BIAS_ENN_PIN, GPIO_OUT_ONE);
    msleep(50); 

    //reset low to high
    lcm_util.set_gpio_out(GPIO_DISP_LRSTB_PIN, GPIO_OUT_ONE);
    mdelay(5); 
    lcm_util.set_gpio_out(GPIO_DISP_LRSTB_PIN, GPIO_OUT_ZERO);
    mdelay(5); 
    lcm_util.set_gpio_out(GPIO_DISP_LRSTB_PIN, GPIO_OUT_ONE);
    msleep(20); 

    push_table(lcm_initialization_setting_tm, sizeof(lcm_initialization_setting_tm) / sizeof(struct LCM_setting_table), 1);
    //Back to MP.P7 baseline , solve LCD display abnormal On the right
    //when sleep out, config output high ,enable backlight drv chip  
    lcm_util.set_gpio_out(GPIO_LCD_DRV_EN_PIN, GPIO_OUT_ONE);

    LCD_DEBUG("uboot:tm_nt35521_lcm_resume\n");

}
static unsigned int lcm_compare_id_tm(void)
{
    unsigned char LCD_ID_value = 0;
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
#ifndef BUILD_LK
static struct LCM_setting_table lcm_Inversion_mode_setting[] = {
    {0xF0,5,{0x55,0xAA,0x52,0x08,0x00}}, 
    {0xBC,2,{0x00,0x00}},
    {REGFLAG_END_OF_TABLE, 0x00, {}}
};

static void lcm_setInversion_mode(unsigned int mode)
{
    printk("nt35521_hd720_dsi_vdo_tm lcm_setInversion_mode mode = %d\n", mode);
    if(DOT_INVERSION == mode)
    {
        lcm_Inversion_mode_setting[1].para_list[0] = dot_Inversion_value;
        lcm_Inversion_mode_setting[1].para_list[1] = dot_Inversion_value;
    }
    else
    {
        lcm_Inversion_mode_setting[1].para_list[0] = column_Inversion_value;
        lcm_Inversion_mode_setting[1].para_list[1] = column_Inversion_value;
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
	
    array[0] = 0x00023700;
    dsi_set_cmdq(array, 1,1);
    read_reg_v2(0xBC, buffer,7);

    printk("nt35521_hd720_dsi_vdo_tm buffer[0] = %d\n buffer[1] = %d",buffer[0],buffer[1]);
    if(COLUMN_INVERSION == mode)
    {
        if((buffer[0] == column_Inversion_value) && (buffer[1] == column_Inversion_value) )
        {
            printk("nt35521_hd720_dsi_vdo_tm column_Inversion_value set_inversion_mode success! buffer[0] = %d buffer[1] = %d\n",buffer[0],buffer[1]);
            return TRUE;
        }
        else
        {
            printk("nt35521_hd720_dsi_vdo_tm column_Inversion_value set_inversion_mode fail! buffer[0] = %d buffer[1] = %d\n",buffer[0],buffer[1]);
            return FALSE;
        }
    }
    else
    {
        if((buffer[0] == dot_Inversion_value) && (buffer[1] == dot_Inversion_value))
        {
           printk("nt35521_hd720_dsi_vdo_tm dot_Inversion_value set_inversion_mode success! buffer[0] = %d buffer[1] = %d\n",buffer[0],buffer[1]);
           return TRUE;
        }
        else
        {
           printk("nt35521_hd720_dsi_vdo_tm dot_Inversion_value set_inversion_mode fail! buffer[0] = %d buffer[1] = %d\n",buffer[0],buffer[1]);
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
 
     array_1[0] = 0x00013700;
     dsi_set_cmdq(array_1, 1,1);
    read_reg_v2(0x0A, buffer_1,7);
    printk("nt35521_hd720_dsi_vdo_tm buffer_1[0] = %d\n",buffer_1[0]);
	
    array_2[0] = 0x00013700;
    dsi_set_cmdq(array_2, 1,1);	
    read_reg_v2(0x0B, buffer_2,7);
    printk("nt35521_hd720_dsi_vdo_tm buffer_2[0] = %d\n",buffer_2[0]);
	
    array_3[0] = 0x00013700;
    dsi_set_cmdq(array_3, 1,1);	
    read_reg_v2(0x0C, buffer_3,7);
    printk("nt35521_hd720_dsi_vdo_tm buffer_3[0] = %d\n",buffer_3[0]);
		
    array_4[0] = 0x00013700;
    dsi_set_cmdq(array_4, 1,1);	
    read_reg_v2(0x0D, buffer_4,7);	
    printk("nt35521_hd720_dsi_vdo_tm buffer_4[0] = %d\n",buffer_4[0]);
	
    if((buffer_1[0] != 0x9C) || (buffer_2[0] != 0x00) || (buffer_3[0] != 0x70) || (buffer_4[0] != 0x00))/*LCD work status error,need re-initalize*/
    {
        printk("nt35521_hd720_dsi_vdo_tm lcm_esd_check fail! buffer_1[0] = %d buffer_2[0] = %d buffer_3[0] = %d buffer_4[0] = %d\n",buffer_1[0],buffer_2[0],buffer_3[0],buffer_4[0]);
        return FALSE;
    }
    else/*LCD work status ok*/
    {
        printk("nt35521_hd720_dsi_vdo_tm lcm_esd_check ok! buffer_1[0] = %d buffer_2[0] = %d buffer_3[0] = %d buffer_4[0] = %d\n",buffer_1[0],buffer_2[0],buffer_3[0],buffer_4[0]);
        return TRUE;
    }
}
#endif
LCM_DRIVER nt35521_hd720_dsi_vdo_tm_lcm_drv =
{
    .name           = "nt35521_hd720_dsi_vdo_tm",
    .set_util_funcs = lcm_set_util_funcs,
    .get_params     = lcm_get_params,
    .init           = lcm_init_tm,/*tianma init fun.*/
    .suspend        = lcm_suspend,
    .resume         = lcm_resume_tm,
    .compare_id     = lcm_compare_id_tm,
#ifndef BUILD_LK	
    .set_inversion_mode =  lcm_setInversion_mode,
    .lcm_check_state  =  lcm_check_state,
    .lcm_check_inversion_set = lcm_check_inversion_set,
#endif
};
