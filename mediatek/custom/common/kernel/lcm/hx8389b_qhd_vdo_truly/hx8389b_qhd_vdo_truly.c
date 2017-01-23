//add Truly HX8389B lcm driver for G6
/*****************************************************************************
	Copyright (C), 1988-2012, Huawei Tech. Co., Ltd.
	FileName: HX8389B
	Author: h84013687   Version: 0.1  Date: 2012/04/21
	Description: add HX8389B driver for G6
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
static unsigned int  dot_Inversion_value = 0x88;
static unsigned int  column_Inversion_value = 0x80;
#define FRAME_WIDTH  		(540)
#define FRAME_HEIGHT 		(960)
//ON-chip's first 1-pulse more than 2us. pwm clk = 20KHz
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



//ID0=0 ID1=0
const static unsigned char LCD_MODULE_ID = 0x00;

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

/****
39h:DCS Long Write/write_LUT Command Packet 
29h:Generic Long Write 
****/
static void lcm_init_setting() 
{ 
        unsigned int data_array[34]; 
                
        data_array[0]=0x00043902;
        data_array[1]=0x8983FFB9;//SET EXTENSION COMMAND
        dsi_set_cmdq(&data_array, 2, 1);

	 data_array[0]=0x00033902;
        data_array[1]=0x009341BA;//SET MIPI CONTROL //9201-9341
        dsi_set_cmdq(&data_array, 2, 1);

	  data_array[0]=0x00023902;
        data_array[1]=0x000008C6;
        dsi_set_cmdq(&data_array, 2, 1); 

	 data_array[0]=0x00033902;
        data_array[1]=0x005805DE;//SET POWER OPTION 
        dsi_set_cmdq(&data_array, 2, 1); 		

        data_array[0]=0x00143902;
        data_array[1]=0x040000B1;//SET POWER
	 data_array[2]=0x11109AE3;
	 data_array[3]=0x3028EF8F;
	 data_array[4]=0x00422323; 
	 data_array[5]=0x0020F53A;
        dsi_set_cmdq(&data_array, 6, 1);
        
        data_array[0]=0x00083902;
        data_array[1]=0x780000B2;//SET DISPLAY REALTED REGISTER
	 data_array[2]=0x803F0308;
        dsi_set_cmdq(&data_array, 3, 1); 
        
        data_array[0]=0x00183902; 
        data_array[1]=0x000880B4;//SET PANEL DRIVING TIMING-column inversion
        data_array[2]=0x00001032; 
        data_array[3]=0x00000000; 
        data_array[4]=0x400A3700; 
        data_array[5]=0x400A3704;
	 data_array[6]=0x0A555014; 
        dsi_set_cmdq(&data_array, 7, 1);

	 data_array[0]=0x00313902;
        data_array[1]=0x4C0000D5; 
        data_array[2]=0x00000100; 
        data_array[3]=0x99006000; 
        data_array[4]=0x88889988; 
        data_array[5]=0x88108832; 
        data_array[6]=0x10548876; 
        data_array[7]=0x88888832; 
        data_array[8]=0x99888888;
	 data_array[9]=0x45889988; 
        data_array[10]=0x01886788; 
        data_array[11]=0x01232388; 
        data_array[12]=0x88888888;
	 data_array[13]=0x00000088;
	 dsi_set_cmdq(&data_array,14 , 1);
	
	//update the gamma index close to gamma2.2
	data_array[0]=0x00233902;
	data_array[1]=0x151203E0;//SET GAMMA
	data_array[2]=0x1C3F3129; 
	data_array[3]=0x0E0D0639; 
	data_array[4]=0x12111210; 
	data_array[5]=0x12031A11; 
	data_array[6]=0x3F312915; 
	data_array[7]=0x0D06391C; 
	data_array[8]=0x1112100E;
	data_array[9]=0x001A1112; 
	dsi_set_cmdq(&data_array,10 , 1); 

	data_array[0]=0x00213902;
	data_array[1]=0x0A0101C1;
	data_array[2]=0x2D251D15;
	data_array[3]=0x4B433B33;
	data_array[4]=0x69625A52;
	data_array[5]=0x87807871;
	data_array[6]=0xA79F978F;
	data_array[7]=0xC8C0B7AF;
	data_array[8]=0xE7E0D6CE;
	data_array[9]=0x000000EF;
	dsi_set_cmdq(&data_array,10 , 1);
	data_array[0]=0x00212902;
	data_array[1]=0x50FFF8C1;
	data_array[2]=0x68778EED;
	data_array[3]=0xC08C131F;
	data_array[4]=0x1D150A01;
	data_array[5]=0x3B332D25;
	data_array[6]=0x5A524B43;
	data_array[7]=0x78716962;
	data_array[8]=0x978F8780;
	data_array[9]=0x0000009F;
	dsi_set_cmdq(&data_array,10 , 1);
	data_array[0]=0x00212902;
	data_array[1]=0xB7AFA7C1;
	data_array[2]=0xD6CEC8C0;
	data_array[3]=0xF8EFE7E0;
	data_array[4]=0x8EED50FF;
	data_array[5]=0x131F6877;
	data_array[6]=0x0A01C08C;
	data_array[7]=0x2D251D15;
	data_array[8]=0x4B433B33;
	data_array[9]=0x00000052;
	dsi_set_cmdq(&data_array,10 , 1);
	data_array[0]=0x00202902;
	data_array[1]=0x69625AC1;
	data_array[2]=0x87807871;
	data_array[3]=0xA79F978F;
	data_array[4]=0xC8C0B7AF;
	data_array[5]=0xE7E0D6CE;
	data_array[6]=0x50FFF8EF;
	data_array[7]=0x68778EED;
	data_array[8]=0xC08C131F;
	dsi_set_cmdq(&data_array, 9, 1); 
	
        data_array[0]=0x00023902;
        data_array[1]=0x000002CC;//SET PANEL
        dsi_set_cmdq(&data_array, 2, 1);

        data_array[0]=0x000A3902;
        data_array[1]=0x1E000FC9;//SET CABC CONTROL//
	  data_array[2]=0x0000001E;
	  data_array[3]=0x00003E01;
        dsi_set_cmdq(&data_array, 4, 1);
	  //cancel vcom set
	   //data_array[0]=0x00033902;
       //data_array[1]=0x009600B6;//VCOM
       //dsi_set_cmdq(&data_array, 2, 1); 
	 data_array[0]=0x00033902;
       data_array[1]=0x000707CB;//SET CLOCK
       dsi_set_cmdq(&data_array, 2, 1); 
	  
	 data_array[0]=0x00053902;
       data_array[1]=0xFF0000BB;//SET OTP
	 data_array[2]=0x00000080;
       dsi_set_cmdq(&data_array, 3, 1); 
         
       data_array[0]=0x00023902;
       data_array[1]=0x00000051;
       dsi_set_cmdq(&data_array, 2, 1); 

       data_array[0]=0x00023902;
       data_array[1]=0x00002453;
       dsi_set_cmdq(&data_array, 2, 1); 

       data_array[0]=0x00023902;
       data_array[1]=0x00000155;
       dsi_set_cmdq(&data_array, 2, 1); 

       data_array[0] = 0x00110500;
       dsi_set_cmdq(&data_array, 1, 1); 
       msleep(120);

      	data_array[0] = 0x00290500;
      	dsi_set_cmdq(&data_array, 1, 1);
      	msleep(20);
};

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
        params->dsi.vertical_sync_active                = 3;
        params->dsi.vertical_backporch                  = 9;
        params->dsi.vertical_frontporch                 = 7;
        params->dsi.vertical_active_line                = FRAME_HEIGHT;

        params->dsi.horizontal_sync_active              = 40;
        params->dsi.horizontal_backporch                = 24;//24;
        params->dsi.horizontal_frontporch               = 20;//63;
        params->dsi.horizontal_active_pixel             =FRAME_WIDTH;
        params->dsi.PLL_CLOCK = 228;//241;
		params->dsi.ssc_disable = 1;
}

/******************************************************************************
Function:       lcm_id_pin_handle
Description:    operate GPIO to prevent electric leakage
Input:          none
Output:         none
Return:         none
Others:         boe id0:0;id1:0,so pull down ID0 & ID1
******************************************************************************/
static void lcm_id_pin_handle(void)
{
    unsigned int ret = 0;
    ret = mt_set_gpio_pull_select(GPIO_DISP_ID0_PIN,GPIO_PULL_DOWN);
    if(0 != ret)
    {
        LCD_DEBUG("ID0 mt_set_gpio_pull_select->DOWN fail\n");
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
    mdelay(5);
    lcm_util.set_gpio_out(GPIO_DISP_LRSTB_PIN, GPIO_OUT_ZERO);
    mdelay(5);
    lcm_util.set_gpio_out(GPIO_DISP_LRSTB_PIN, GPIO_OUT_ONE);
    msleep(120);

    lcm_id_pin_handle();/*Handle GPIO_DISP_ID0_PIN and GPIO_DISP_ID1_PIN*/
    lcm_init_setting(); 
#ifdef BUILD_LK
	printf("LCD hx8389b_qhd_vdo_truly lcm_init\n");
#else
	printk("LCD hx8389b_qhd_vdo_truly lcm_init\n");
#endif
}
static void lcm_suspend(void)
{
    unsigned int data_array[16];
    // set 51 register to 00
    data_array[0]=0x00511500; 
    dsi_set_cmdq(&data_array, 1, 1);
    data_array[0]=0x00280500; // Display Off
    dsi_set_cmdq(&data_array, 1, 1);
    msleep(20);
    data_array[0] = 0x00100500; // Sleep In
    dsi_set_cmdq(&data_array, 1, 1);
    msleep(120);
    lcm_util.set_gpio_out(GPIO_DISP_LRSTB_PIN, GPIO_OUT_ONE);
    mdelay(5);
    lcm_util.set_gpio_out(GPIO_DISP_LRSTB_PIN, GPIO_OUT_ZERO);
    mdelay(5);
    lcm_util.set_gpio_out(GPIO_DISP_LRSTB_PIN, GPIO_OUT_ONE);
    mdelay(10);
#ifdef BUILD_LK
	printf("LCD hx8389b_qhd_vdo_truly lcm_suspend\n");
#else
	printk("LCD hx8389b_qhd_vdo_truly lcm_suspend\n");
#endif
    //when phone resume , set backlight 1st pwm pulse > 2us to avoid ON backlight chip bug
    bl_flag = TRUE;
}

static void lcm_resume(void)
{
    lcm_util.set_gpio_out(GPIO_DISP_LRSTB_PIN, GPIO_OUT_ONE);
    mdelay(5);
    lcm_util.set_gpio_out(GPIO_DISP_LRSTB_PIN, GPIO_OUT_ZERO);
    mdelay(5);
    lcm_util.set_gpio_out(GPIO_DISP_LRSTB_PIN, GPIO_OUT_ONE);
    mdelay(10);
    lcm_init_setting();
#ifdef BUILD_LK
	printf("LCD hx8389b_qhd_vdo_truly lcm_resume\n");
#else
	printk("LCD hx8389b_qhd_vdo_truly lcm_resume\n");
#endif
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
       unsigned int data_array[16];               
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
                    printf("LK:hx8389b_truly ON backlight chip bug level=%d\n",level);
                #else
                    printk("kernel:hx8389b_truly ON backlight chip bug level=%d\n",level);
                #endif
            }
            bl_flag = FALSE;
        }
        #ifdef BUILD_LK
            printf("LK:hx8389b_truly_lcm_setbacklight level=%d\n",level);
        #else
            printk("kernel:hx8389b_truly_lcm_setbacklight level=%d\n",level);
        #endif
		
        data_array[0] = 0x00023902;
        data_array[1] = (0x51|(level<<8));
        dsi_set_cmdq(&data_array, 2, 1);
}
#ifndef BUILD_LK
static void lcm_setInversion_mode(unsigned int mode)
{
    unsigned int array[16];
    printk("hx8389b-truly lcm_setInversion_mode mode = %d\n", mode);
    if(DOT_INVERSION == mode)//DOT_INVERSION
	{
        array[0]=0x00023902;
        array[1]=0x000088B4;//SET PANEL DRIVING TIMING-1-Dot inversion
        dsi_set_cmdq(&array,2,1);

	}
    else//column INVERSION
	{
	    array[0]=0x00023902;
        array[1]=0x000080B4;//SET PANEL DRIVING TIMING-column inversion
        dsi_set_cmdq(&array,2,1);
    }
}

static unsigned int lcm_check_inversion_set(unsigned int mode)
{
    unsigned char buffer[12] = {0};
    unsigned int array[16];

    array[0] = 0x00013700;
    dsi_set_cmdq(array,1,1);
    read_reg_v2(0xB4, buffer,1);
    printk("hx8389b-truly buffer[0] = %d\n",buffer[0]);
    if(COLUMN_INVERSION == mode)
    {
        if(buffer[0] == column_Inversion_value )
        {
            printk("hx8389b-truly column_Inversion_value set_inversion_mode success! buffer[0] = %d\n",buffer[0]);
    	    return TRUE;
    	}
        else
        {
            printk("hx8389b-truly column_Inversion_value set_inversion_mode fail! buffer[0] = %d\n",buffer[0]);
    	    return FALSE;
        }
    }
    else
    {
        if(buffer[0] == dot_Inversion_value )
    	{
    	   printk("hx8389b-truly dot_Inversion_value set_inversion_mode success! buffer[0] = %d\n",buffer[0]);
    	   return TRUE;
        	}
         else{
    	   printk("hx8389b-truly dot_Inversion_value set_inversion_mode fail! buffer[0] = %d\n",buffer[0]);
    	   return FALSE;
    	}
    }
}

static unsigned int lcm_check_state()
{
	unsigned char buffer_1[12] = {0};
	unsigned int array_1[16];

    //---------------------------------
    // Read [9Ch, 00h, ECC] + Error Report(4 Bytes)
    //---------------------------------
    array_1[0] = 0x00013700;
    dsi_set_cmdq(array_1, 1,1);
    read_reg_v2(0x0A, buffer_1,1);
	printk("hx8389b-truly buffer_1[0] = %d\n",buffer_1[0]);

    if(buffer_1[0] != 0x1C)/*LCD work status error,need re-initalize*/
    {
        printk("hx8389b-truly lcm_esd_check fail! buffer_1[0] = %d\n",buffer_1[0]);
        return FALSE;
    }
    else/*LCD work status ok*/
    {
        printk("hx8389b-truly lcm_esd_check ok! buffer_1[0] = %d\n",buffer_1[0]);
        return TRUE;
    }
}
#endif
LCM_DRIVER hx8389b_qhd_vdo_truly_lcm_drv =
{
    .name           = "hx8389b_qhd_vdo_truly",
    .set_util_funcs = lcm_set_util_funcs,
    .get_params     = lcm_get_params,
    .init           = lcm_init,
    .suspend        = lcm_suspend,
    .resume         = lcm_resume,
    .compare_id     = lcm_compare_id,
    .set_backlight	= lcm_setbacklight,
#ifndef BUILD_LK
	.set_inversion_mode =  lcm_setInversion_mode,
	.lcm_check_state	=  lcm_check_state,
	.lcm_check_inversion_set = lcm_check_inversion_set,
#endif
};
