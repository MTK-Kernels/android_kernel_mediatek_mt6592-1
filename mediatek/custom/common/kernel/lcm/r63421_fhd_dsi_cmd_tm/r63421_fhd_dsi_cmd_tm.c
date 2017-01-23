#ifndef BUILD_LK
#include <linux/string.h>
#endif
#include "lcm_drv.h"

#ifdef BUILD_LK
	#include <platform/mt_gpio.h>
     #include <platform/disp_drv_platform.h>
	 #include <string.h>
#elif defined(BUILD_UBOOT)
	#include <asm/arch/mt_gpio.h>
#else
    #include <linux/delay.h>
	#include <mach/mt_gpio.h>
#endif
#ifdef BUILD_LK
#define LCD_DEBUG(fmt)  dprintf(CRITICAL,fmt)
#else
#define LCD_DEBUG(fmt)  printk(fmt)
#endif
static const unsigned char LCM_ID_r63421 = 0x09; //  tianma ID0 = 1£»ID1 = NC(2) ; lcd_id =  lcd_id0 | (lcd_id1 << 2);
// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------
#define FRAME_WIDTH  (1080)
#define FRAME_HEIGHT (1920)

#ifndef TRUE
    #define TRUE 1
#endif

#ifndef FALSE
    #define FALSE 0
#endif
// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static LCM_UTIL_FUNCS lcm_util = {0};

//#define SET_RESET_PIN(v)    (lcm_util.set_reset_pin((v)))

#define UDELAY(n) (lcm_util.udelay(n))
#define MDELAY(n) (lcm_util.mdelay(n))


// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------

#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)	        lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)		lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)										lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)					lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg(cmd)											lcm_util.dsi_dcs_read_lcm_reg(cmd)
#define read_reg_v2(cmd, buffer, buffer_size)   				lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)   


#define   LCM_DSI_CMD_MODE						1
#ifndef BUILD_LK
static unsigned int dot_Inversion_value = 0x07;
static unsigned int column_Inversion_value = 0xF7;
#endif
static unsigned int  inversion_cnt = 0;
static unsigned int  max_error_num = 3;

static void init_lcm_registers(void)
{
	unsigned int data_array[16];
	
	     data_array[0] = 0x00022902;      // use 29h data type or 23h to write MCS(GCS) 
         data_array[1] = 0x000004B0;      // B0h  Manufacturer Command Access Protect
	     dsi_set_cmdq(data_array, 2, 1); 
		  
         data_array[0] = 0x00022902;  
	     data_array[1] = 0x000001D6;	  //D6h  T_SLPOUT=0(NVM load stop)
         dsi_set_cmdq(data_array, 2, 1);
	
	     data_array[0] = 0x00072902;                              
         data_array[1] = 0x000004B3;      //set command mode              
         data_array[2] = 0x00000000;                 
         dsi_set_cmdq(data_array, 3, 1); 
		 
         //set refresh rate 60Hz
		 data_array[0] = 0x00292902;
         data_array[1] = 0x000078C6;//LTPS timing setting
         data_array[2] = 0x00006608;
         data_array[3] = 0x00000000;
         data_array[4] = 0x00000000;
         data_array[5] = 0x16140000;
         data_array[6] = 0x00007807;
         data_array[7] = 0x00006608;
         data_array[8] = 0x00000000;
         data_array[9] = 0x00000000;
         data_array[10] = 0x16140000;
         data_array[11] = 0x00000007;
         dsi_set_cmdq(data_array, 12, 1);
         data_array[0] = 0x00172902;
         data_array[1] = 0x7FE084D7;//D7 setting
         data_array[2] = 0xFC38CEA8;
         data_array[3] = 0x0F2718C1;
         data_array[4] = 0xFA103C1E;
         data_array[5] = 0x41040FC3;
         data_array[6] = 0x00000000;
         dsi_set_cmdq(data_array, 7, 1);
         data_array[0] = 0x00351500;      // open TE                          
         dsi_set_cmdq(data_array, 1, 1);
	     data_array[0] = 0x00033902;
	     data_array[1] = 0x00DC0544;
	     dsi_set_cmdq(data_array, 2, 1);
         data_array[0] = 0x00290500;                          
         dsi_set_cmdq(data_array, 1, 1);
         mdelay(20); 
         data_array[0] = 0x00110500;                          
         dsi_set_cmdq(data_array, 1, 1);
         mdelay(120);
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
		// enable tearing-free
		params->dbi.te_mode 				= LCM_DBI_TE_MODE_VSYNC_ONLY;
		params->dbi.te_edge_polarity		= LCM_POLARITY_RISING;
		
        #if (LCM_DSI_CMD_MODE)
		params->dsi.mode   = CMD_MODE;
        #else
		params->dsi.mode   = BURST_VDO_MODE;
        #endif
		// DSI
		/* Command mode setting */
		//1 Three lane or Four lane
		params->dsi.LANE_NUM				= LCM_FOUR_LANE;
		//The following defined the fomat for data coming from LCD engine.
		params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;
		// Highly depends on LCD driver capability.
		// Not support in MT6573
		//params->dsi.packet_size=256;
		params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
		//params->dsi.word_count=1080*3;
	    params->dsi.PLL_CLOCK = 448;   //this value must be in MTK suggested table
	    params->dsi.ssc_disable = 1;
	
		//params->dsi.pll_div1=0;		// div1=0,1,2,3;div1_real=1,2,4,4 ----0: 546Mbps  1:273Mbps
		//params->dsi.pll_div2=0;		// div2=0,1,2,3;div1_real=1,2,4,4	
		//params->dsi.fbk_div =0x12;    // fref=26MHz, fvco=fref*(fbk_div+1)*2/(div1_real*div2_real)	
}

static void lcm_id_pin_handle(void)
{
     mt_set_gpio_pull_select(GPIO_DISP_ID0_PIN,GPIO_PULL_UP);
     mt_set_gpio_pull_select(GPIO_DISP_ID1_PIN,GPIO_PULL_DOWN);
}

static void lcm_init(void)
{
     //enable VSP & VSN
	 mdelay(10);
     lcm_util.set_gpio_out(GPIO_LCD_BIAS_ENP_PIN, GPIO_OUT_ONE);
     mdelay(5);
     lcm_util.set_gpio_out(GPIO_LCD_BIAS_ENN_PIN, GPIO_OUT_ONE);
     msleep(5); 
	 
	 //reset low to high
     lcm_util.set_gpio_out(GPIO_DISP_LRSTB_PIN, GPIO_OUT_ONE);
	 msleep(20); 
	 lcm_util.set_gpio_out(GPIO_DISP_LRSTB_PIN, GPIO_OUT_ZERO);
	 msleep(30); 
     lcm_util.set_gpio_out(GPIO_DISP_LRSTB_PIN, GPIO_OUT_ONE);
	 msleep(30); 

	 lcm_id_pin_handle();    // ID0 = 1; ID1 = NC (2)

     init_lcm_registers();
     lcm_util.set_gpio_out(GPIO_DISP_LRSTB_PIN, GPIO_OUT_ZERO);
     msleep(10);
     lcm_util.set_gpio_out(GPIO_DISP_LRSTB_PIN, GPIO_OUT_ONE);
     msleep(120);

     init_lcm_registers();
	 lcm_util.set_gpio_out(GPIO_LCD_DRV_EN_PIN, GPIO_OUT_ONE); 
	 LCD_DEBUG("uboot:tm_r63421_lcm_init\n");
}


static void lcm_suspend(void)
{
	unsigned int data_array[16];
	// when phone sleep , config output low, disable backlight drv chip  
    lcm_util.set_gpio_out(GPIO_LCD_DRV_EN_PIN, GPIO_OUT_ZERO);
		
	data_array[0] = 0x00280500; // Display Off
	dsi_set_cmdq(data_array, 1, 1);
    mdelay(20);
	
	data_array[0] = 0x00100500; // Sleep In
	dsi_set_cmdq(data_array, 1, 1);
	mdelay(120);

    //disable VSP & VSN
    lcm_util.set_gpio_out(GPIO_LCD_BIAS_ENN_PIN, GPIO_OUT_ZERO);
	mdelay(5);
	lcm_util.set_gpio_out(GPIO_LCD_BIAS_ENP_PIN, GPIO_OUT_ZERO);
	mdelay(5);
	//reset high to low
	lcm_util.set_gpio_out(GPIO_DISP_LRSTB_PIN, GPIO_OUT_ZERO);
	mdelay(5);
	
	LCD_DEBUG("kernel:tm_r63421_lcm_suspend\n");
}

static void lcm_resume(void)
{
	unsigned int data_array[16];
	
	 //enable VSP & VSN
	mdelay(10);
	lcm_util.set_gpio_out(GPIO_LCD_BIAS_ENP_PIN, GPIO_OUT_ONE);
	mdelay(10);
	lcm_util.set_gpio_out(GPIO_LCD_BIAS_ENN_PIN, GPIO_OUT_ONE);
    msleep(5); 
	
    //reset low to high
    lcm_util.set_gpio_out(GPIO_DISP_LRSTB_PIN, GPIO_OUT_ONE);
    mdelay(20); 
    lcm_util.set_gpio_out(GPIO_DISP_LRSTB_PIN, GPIO_OUT_ZERO);
    mdelay(30); 
    lcm_util.set_gpio_out(GPIO_DISP_LRSTB_PIN, GPIO_OUT_ONE);
    msleep(20); 
	
	init_lcm_registers();
    //when sleep out, config output high ,enable backlight drv chip  
    lcm_util.set_gpio_out(GPIO_LCD_DRV_EN_PIN, GPIO_OUT_ONE);
	LCD_DEBUG("kernel:tm_r63421_lcm_resume\n");
}
     
	 
#if (LCM_DSI_CMD_MODE)
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

	//data_array[0]= 0x00290508; //HW bug, so need send one HS packet
	//dsi_set_cmdq(data_array, 1, 1);
	
	data_array[0]= 0x002c3909;
	dsi_set_cmdq(data_array, 1, 0);

}
#endif

static unsigned int lcm_compare_id_tm(void)
{
    unsigned char LCD_ID_value = 0;
    LCD_ID_value = which_lcd_module_triple();
    if(LCM_ID_r63421 == LCD_ID_value)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}
#ifndef BUILD_LK
static void lcm_setInversion_mode(unsigned int mode)
{
    unsigned int array[4];
    printk("r63421_fhd_dsi_cmd_tm lcm_setInversion_mode mode = 0x%x \n", mode);
    if(DOT_INVERSION == mode)
    {
         array[0] = 0x00082902;            // C2h set to dot_inversion: 0x31,0x07,0x82,0x06,0x06,0x00,0x00
         array[1] = 0x820731C2;
         array[2] = 0x00000606;
         dsi_set_cmdq(array, 3, 1);
    }
    else
    {
         array[0] = 0x00082902;           // C2h set to column_inversion: 0x31,0xF7,0x82,0x06,0x06,0x00,0x00
         array[1] = 0x82F731C2;
         array[2] = 0x00000606;
         dsi_set_cmdq(array, 3, 1);
    }
}

static unsigned int lcm_check_inversion_set(unsigned int mode)
{
    unsigned char buffer[8] = {0};
    unsigned int array[2];

    array[0] = 0x00083700;
    dsi_set_cmdq(array, 1,1);
    read_reg_v2(0xC2, buffer,8);

    if(COLUMN_INVERSION == mode)
    {
        if(buffer[1] == column_Inversion_value)
        {
            printk("r63421_fhd_dsi_cmd_tm column_Inversion_value set_inversion_mode success! C2h_Sencond_Parameter = 0x%x\n",buffer[1]);
            return TRUE;
        }
        else
        {
            printk("r63421_fhd_dsi_cmd_tm column_Inversion_value set_inversion_mode fail! C2h_Sencond_Parameter = 0x%x\n",buffer[1]);
            return FALSE;
        }
    }
    else
    {
        if(buffer[1] == dot_Inversion_value)
        {
           printk("r63421_fhd_dsi_cmd_tm dot_Inversion_value set_inversion_mode success! C2h_Sencond_Parameter = 0x%x\n",buffer[1]);
           return TRUE;
        }
        else
        {
           printk("r63421_fhd_dsi_cmd_tm dot_Inversion_value set_inversion_mode fail! C2h_Sencond_Parameter = 0x%x\n",buffer[1]);
           return FALSE;
        }
    }
}

static unsigned int lcm_check_state()
{
    unsigned char buffer_1[1] = {0};
    unsigned char buffer_2[1] = {0};
    unsigned char buffer_3[1] = {0};
    unsigned int  array[2];

    array[0] = 0x00013700;                      // read 0x0A
    dsi_set_cmdq(array, 1,1);
    read_reg_v2(0x0A, buffer_1,1);

    array[0] = 0x00013700;                      // read 0x0B
    dsi_set_cmdq(array, 1,1);
    read_reg_v2(0x0B, buffer_2,1);

    array[0] = 0x00013700;                      // read 0x0C
    dsi_set_cmdq(array, 1,1);
    read_reg_v2(0x0C, buffer_3,1);

    if((buffer_1[0] != 0x1C) || (buffer_2[0] != 0x00) || (buffer_3[0] != 0x77)) /*LCD work status error,need re-initalize*/
    {
        printk("r63421_fhd_dsi_cmd_tm lcm_esd_check fail! 0Ah = 0x%x ; 0Bh = 0x%x ;  0Ch = 0x%x .",buffer_1[0],buffer_2[0],buffer_3[0]);
        inversion_cnt++;
        if(inversion_cnt >= max_error_num)
        {
            inversion_cnt = 0;
            printk("nt35595_fhd_dsi_cmd_cmi Lcm_esd_check fail! inversion_cnt = 3.\n");
            return FALSE;
        }
        else
        {
            return TRUE;
        }
    }
    else/*LCD work status ok*/
    {
        printk("r63421_fhd_dsi_cmd_tm lcm_esd_check ok!  0Ah = 0x%x ; 0Bh = 0x%x ;  0Ch = 0x%x .",buffer_1[0],buffer_2[0],buffer_3[0]);
        return TRUE;
    }
}
#endif
LCM_DRIVER r63421_fhd_dsi_cmd_tm_lcm_drv = 
{
    .name			= "tm_r63421_fhd_cmd",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params     = lcm_get_params,
	.init           = lcm_init,
	.suspend        = lcm_suspend,
	.resume         = lcm_resume,
	.compare_id     = lcm_compare_id_tm,
#ifndef BUILD_LK
    .set_inversion_mode =  lcm_setInversion_mode,
    .lcm_check_state  =  lcm_check_state,
    .lcm_check_inversion_set = lcm_check_inversion_set,
#endif
#if (LCM_DSI_CMD_MODE)
    .update         = lcm_update,
#endif
    };
