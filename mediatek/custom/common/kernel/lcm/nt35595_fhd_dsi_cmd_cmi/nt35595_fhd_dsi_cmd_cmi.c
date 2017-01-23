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
// ---------------------------------------------------------------------------
// Local Constants
// ---------------------------------------------------------------------------
#define LCM_DSI_CMD_MODE    1
#define FRAME_WIDTH  (1080)
#define FRAME_HEIGHT (1920)
#ifndef BUILD_LK
static unsigned int dot_Inversion_value = 0x00;
static unsigned int column_Inversion_value = 0x0F;
static unsigned int  inversion_cnt = 0;
static unsigned int  max_error_num = 3;
#endif
static const unsigned char LCM_ID_nt35595_cmi = 0x04;       //  CMI ID0 = 0  ID1 = 1 ; lcd_id =  lcd_id0 | (lcd_id1 << 2);

#define REGFLAG_DELAY 0xFE
#define REGFLAG_END_OF_TABLE 0xFC // END OF REGISTERS MARKER

#ifdef BUILD_LK
#define LCD_DEBUG(fmt)  dprintf(CRITICAL,fmt)
#else
#define LCD_DEBUG(fmt)  printk(fmt)
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

// ---------------------------------------------------------------------------
// Local Variables
// ---------------------------------------------------------------------------

static LCM_UTIL_FUNCS lcm_util = {0};

// ---------------------------------------------------------------------------
// Local Functions
// ---------------------------------------------------------------------------

#define dsi_set_cmdq_V2(cmd, count, ppara, force_update) lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update) lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd) lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums) lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg(cmd) lcm_util.dsi_dcs_read_lcm_reg(cmd)
#define read_reg_v2(cmd, buffer, buffer_size) lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size) 

struct LCM_setting_table {
unsigned char cmd;
unsigned char count;
unsigned char para_list[64];
};

static struct LCM_setting_table lcm_initialization_ret[] = {
//CMD2_P0 
{0xFF, 1, {0x20}}, 
{REGFLAG_DELAY, 1, {}},
//Set Normal Black   
{0x00, 1, {0x01}},  
{0x01, 1, {0x55}},
{0x02, 1, {0x45}},
{0x03, 1, {0x55}},
//VGH=2AVDD; VGL=2AVEE
{0x05, 1, {0x50}},
//Enable VGH clamp=12V
{0x06, 1, {0xBC}},
//Enable VGL clamp=-8V
{0x07, 1, {0x9E}},
{0x08, 1, {0x0C}},
//Set GVDDP=5.0V;GVDDN=-5.0V(For 5.5" Panel Module)
{0x0B, 1, {0xD7}},
{0x0C, 1, {0xD7}},
//VGHO=10V;VGLO=-6V
{0x0E, 1, {0xBF}},
{0x0F, 1, {0x9A}},
//VCOM=-0.2V(Forward&Backward)
//{0x11, 1, {0x13}},     //don't set vcom
{0x12, 1, {0x13}},
//Enable All LV Detect
//{0x13, 1, {0x83}},     //don't set vcom
//Disable Pre-regulating function
{0x14, 1, {0x0A}},
{0x15, 1, {0x1A}},
{0x16, 1, {0x1A}},
//DDB CTRL
{0x23, 1, {0X00}},
{0x24, 1, {0X00}},
{0x25, 1, {0X00}},
{0x26, 1, {0X00}},
{0x27, 1, {0X00}},
{0x28, 1, {0X00}},
//Set LV Detect Voltage(Default:VCI=2.17V,AVDD=4.3V,AVEE=-4.25V,VDDI=1.45V)
{0x30, 1, {0X60}},
{0x31, 1, {0X52}},
{0x32, 1, {0X3D}},
//Select Chopper Mode(Default)
{0x35, 1, {0X00}},
//SD CTRL(Default)
{0x69, 1, {0xAA}},
{0x6A, 1, {0x33}},
{0x6B, 1, {0x40}},
{0x6C, 1, {0x33}},
{0x6D, 1, {0x33}},
//LPX (Modify at 20130715)
{0x72, 1, {0x11}},
//Red Gamma Code (From INX_20130524)
{0x75, 1, {0x00}},  //R+
{0x76, 1, {0x9f}}, 
{0x77, 1, {0x00}},
{0x78, 1, {0xb3}},  
{0x79, 1, {0x00}},
{0x7A, 1, {0xd2}},  
{0x7B, 1, {0x00}},
{0x7C, 1, {0xec}}, 
{0x7D, 1, {0x01}},
{0x7E, 1, {0x00}},  
{0x7F, 1, {0x01}},
{0x80, 1, {0x16}}, 
{0x81, 1, {0x01}},
{0x82, 1, {0x27}}, 
{0x83, 1, {0x01}},
{0x84, 1, {0x34}}, 
{0x85, 1, {0x01}},
{0x86, 1, {0x41}}, 
{0x87, 1, {0x01}},
{0x88, 1, {0x6d}}, 
{0x89, 1, {0x01}},
{0x8A, 1, {0x92}}, 
{0x8B, 1, {0x01}},
{0x8C, 1, {0xca}}, 
{0x8D, 1, {0x01}},
{0x8E, 1, {0xf4}},
{0x8F, 1, {0x02}},
{0x90, 1, {0x38}},
{0x91, 1, {0x02}},
{0x92, 1, {0x6d}}, 
{0x93, 1, {0x02}},
{0x94, 1, {0x6f}}, 
{0x95, 1, {0x02}},
{0x96, 1, {0x9d}}, 
{0x97, 1, {0x02}},
{0x98, 1, {0xce}}, 
{0x99, 1, {0x02}},
{0x9A, 1, {0xec}}, 
{0x9B, 1, {0x03}},
{0x9C, 1, {0x12}}, 
{0x9D, 1, {0x03}},
{0x9E, 1, {0x2a}}, 
{0x9F, 1, {0x03}},
{0xA0, 1, {0x4a}}, 
{0xA2, 1, {0x03}},
{0xA3, 1, {0x56}}, 
{0xA4, 1, {0x03}},
{0xA5, 1, {0x5f}},  
{0xA6, 1, {0x03}},
{0xA7, 1, {0x6a}},  
{0xA9, 1, {0x03}},
{0xAA, 1, {0x76}}, 
{0xAB, 1, {0x03}},
{0xAC, 1, {0x87}},  
{0xAD, 1, {0x03}},
{0xAE, 1, {0x91}}, 
{0xAF, 1, {0x03}},
{0xB0, 1, {0xAf}}, 
{0xB1, 1, {0x03}},
{0xB2, 1, {0xc5}},  
{0xB3, 1, {0x00}},// R-
{0xB4, 1, {0x9f}},
{0xB5, 1, {0x00}},
{0xB6, 1, {0xb3}},
{0xB7, 1, {0x00}},
{0xB8, 1, {0xd2}},
{0xB9, 1, {0x00}},
{0xBA, 1, {0xec}},
{0xBB, 1, {0x01}},
{0xBC, 1, {0x00}},
{0xBD, 1, {0x01}},
{0xBE, 1, {0x16}},
{0xBF, 1, {0x01}},
{0xC0, 1, {0x27}},
{0xC1, 1, {0x01}},
{0xC2, 1, {0x34}},
{0xC3, 1, {0x01}},
{0xC4, 1, {0x41}},
{0xC5, 1, {0x01}},
{0xC6, 1, {0x6d}},
{0xC7, 1, {0x01}},
{0xC8, 1, {0x92}},
{0xC9, 1, {0x01}},
{0xCA, 1, {0xca}},
{0xCB, 1, {0x01}},
{0xCC, 1, {0xf4}},
{0xCD, 1, {0x02}},
{0xCE, 1, {0x38}},
{0xCF, 1, {0x02}},
{0xD0, 1, {0x6d}},
{0xD1, 1, {0x02}},
{0xD2, 1, {0x6f}},
{0xD3, 1, {0x02}},
{0xD4, 1, {0x9d}},
{0xD5, 1, {0x02}},
{0xD6, 1, {0xce}},
{0xD7, 1, {0x02}},
{0xD8, 1, {0xec}},
{0xD9, 1, {0x03}},
{0xDA, 1, {0x12}},
{0xDB, 1, {0x03}},
{0xDC, 1, {0x2a}},
{0xDD, 1, {0x03}},
{0xDE, 1, {0x4a}},
{0xDF, 1, {0x03}},
{0xE0, 1, {0x56}},
{0xE1, 1, {0x03}},
{0xE2, 1, {0x5f}},
{0xE3, 1, {0x03}},
{0xE4, 1, {0x6a}},
{0xE5, 1, {0x03}},
{0xE6, 1, {0x76}},
{0xE7, 1, {0x03}},
{0xE8, 1, {0x87}},
{0xE9, 1, {0x03}},
{0xEA, 1, {0x91}},
{0xEB, 1, {0x03}},
{0xEC, 1, {0xAf}},
{0xED, 1, {0x03}},
{0xEE, 1, {0xc5}},
//Green Gamma Code (From INX_20130524)
{0xEF, 1, {0x00}}, //G+
{0xF0, 1, {0x8a}},
{0xF1, 1, {0x00}},
{0xF2, 1, {0xa3}},
{0xF3, 1, {0x00}},
{0xF4, 1, {0xc3}},
{0xF5, 1, {0x00}},
{0xF6, 1, {0xe2}},
{0xF7, 1, {0x00}},
{0xF8, 1, {0xf8}},
{0xF9, 1, {0x01}},
{0xFA, 1, {0x0d}},
//Don't Reload MTP
{0xFB, 1, {0x01}},
//CMD2_P1
{0xFF, 1, {0x21}},
{REGFLAG_DELAY, 1, {}},
//Green Gamma Code_Continuous (From INX_20130524)
{0x00, 1, {0x01}},
{0x01, 1, {0x1e}},
{0x02, 1, {0x01}},
{0x03, 1, {0x2e}},
{0x04, 1, {0x01}},
{0x05, 1, {0x3c}},
{0x06, 1, {0x01}},
{0x07, 1, {0x69}},
{0x08, 1, {0x01}},
{0x09, 1, {0x8e}},
{0x0A, 1, {0x01}},
{0x0B, 1, {0xc7}},
{0x0C, 1, {0x01}},
{0x0D, 1, {0xf4}},
{0x0E, 1, {0x02}},
{0x0F, 1, {0x38}},
{0x10, 1, {0x02}},
{0x11, 1, {0x6d}},
{0x12, 1, {0x02}},
{0x13, 1, {0x6f}},
{0x14, 1, {0x02}},
{0x15, 1, {0x9e}},
{0x16, 1, {0x02}},
{0x17, 1, {0xcc}},
{0x18, 1, {0x02}},
{0x19, 1, {0xed}},
{0x1A, 1, {0x03}},
{0x1B, 1, {0x12}},
{0x1C, 1, {0x03}},
{0x1D, 1, {0x29}},
{0x1E, 1, {0x03}},
{0x1F, 1, {0x4c}},
{0x20, 1, {0x03}},
{0x21, 1, {0x57}},
{0x22, 1, {0x03}},
{0x23, 1, {0x60}},
{0x24, 1, {0x03}},
{0x25, 1, {0x6c}},
{0x26, 1, {0x03}},
{0x27, 1, {0x76}},
{0x28, 1, {0x03}},
{0x29, 1, {0x86}},
{0x2A, 1, {0x03}},
{0x2B, 1, {0x91}},
{0x2D, 1, {0x03}},
{0x2F, 1, {0xAF}},
{0x30, 1, {0x03}},
{0x31, 1, {0xc5}},
{0x32, 1, {0x00}}, //G-
{0x33, 1, {0x8a}},
{0x34, 1, {0x00}},
{0x35, 1, {0xa3}},
{0x36, 1, {0x00}},
{0x37, 1, {0xc3}},
{0x38, 1, {0x00}},
{0x39, 1, {0xe2}},
{0x3A, 1, {0x00}},
{0x3B, 1, {0xf8}},
{0x3D, 1, {0x01}},
{0x3F, 1, {0x0d}},
{0x40, 1, {0x01}},
{0x41, 1, {0x1e}},
{0x42, 1, {0x01}},
{0x43, 1, {0x2e}},
{0x44, 1, {0x01}},
{0x45, 1, {0x3c}},
{0x46, 1, {0x01}},
{0x47, 1, {0x69}},
{0x48, 1, {0x01}},
{0x49, 1, {0x8e}},
{0x4A, 1, {0x01}},
{0x4B, 1, {0xc7}},
{0x4C, 1, {0x01}},
{0x4D, 1, {0xf4}},
{0x4E, 1, {0x02}},
{0x4F, 1, {0x38}},
{0x50, 1, {0x02}},
{0x51, 1, {0x6d}},
{0x52, 1, {0x02}},
{0x53, 1, {0x6f}},
{0x54, 1, {0x02}},
{0x55, 1, {0x9e}},
{0x56, 1, {0x02}},
{0x58, 1, {0xcc}},
{0x59, 1, {0x02}},
{0x5A, 1, {0xed}},
{0x5B, 1, {0x03}},
{0x5C, 1, {0x12}},
{0x5D, 1, {0x03}},
{0x5E, 1, {0x29}},
{0x5F, 1, {0x03}},
{0x60, 1, {0x4c}},
{0x61, 1, {0x03}},
{0x62, 1, {0x57}},
{0x63, 1, {0x03}},
{0x64, 1, {0x60}},
{0x65, 1, {0x03}},
{0x66, 1, {0x6c}},
{0x67, 1, {0x03}},
{0x68, 1, {0x76}},
{0x69, 1, {0x03}},
{0x6A, 1, {0x86}},
{0x6B, 1, {0x03}},
{0x6C, 1, {0x91}},
{0x6D, 1, {0x03}},
{0x6E, 1, {0xAF}},
{0x6F, 1, {0x03}},
{0x70, 1, {0xc5}},
//Blue Gamma Code (From INX_20130524)
{0x71, 1, {0x00}}, //B+
{0x72, 1, {0x0A}},
{0x73, 1, {0x00}},
{0x74, 1, {0x5D}},
{0x75, 1, {0x00}},
{0x76, 1, {0x8f}},
{0x77, 1, {0x00}},
{0x78, 1, {0xb6}},
{0x79, 1, {0x00}},
{0x7A, 1, {0xd0}},
{0x7B, 1, {0x00}},
{0x7C, 1, {0xe8}},
{0x7D, 1, {0x00}},
{0x7E, 1, {0xfe}},
{0x7F, 1, {0x01}},
{0x80, 1, {0x0b}},
{0x81, 1, {0x01}},
{0x82, 1, {0x1e}},
{0x83, 1, {0x01}},
{0x84, 1, {0x4f}},
{0x85, 1, {0x01}},
{0x86, 1, {0x79}},
{0x87, 1, {0x01}},
{0x88, 1, {0xb7}},
{0x89, 1, {0x01}},
{0x8A, 1, {0xe6}},
{0x8B, 1, {0x02}},
{0x8C, 1, {0x2a}},
{0x8D, 1, {0x02}},
{0x8E, 1, {0x64}},
{0x8F, 1, {0x02}},
{0x90, 1, {0x66}},
{0x91, 1, {0x02}},
{0x92, 1, {0x95}},
{0x93, 1, {0x02}},
{0x94, 1, {0xC8}},
{0x95, 1, {0x02}},
{0x96, 1, {0xE8}},
{0x97, 1, {0x03}},
{0x98, 1, {0x0C}},
{0x99, 1, {0x03}},
{0x9A, 1, {0x23}},
{0x9B, 1, {0x03}},
{0x9C, 1, {0x4A}},
{0x9D, 1, {0x03}},
{0x9E, 1, {0x58}},
{0x9F, 1, {0x03}},
{0xA0, 1, {0x61}},
{0xA2, 1, {0x03}},
{0xA3, 1, {0x70}},
{0xA4, 1, {0x03}},
{0xA5, 1, {0x78}},
{0xA6, 1, {0x03}},
{0xA7, 1, {0x87}},
{0xA9, 1, {0x03}},
{0xAA, 1, {0x91}},
{0xAB, 1, {0x03}},
{0xAC, 1, {0xAF}},
{0xAD, 1, {0x03}},
{0xAE, 1, {0xc5}},

{0xAF, 1, {0x00}}, //B-
{0xB0, 1, {0x0A}},
{0xB1, 1, {0x00}},
{0xB2, 1, {0x5D}},
{0xB3, 1, {0x00}},
{0xB4, 1, {0x8f}},
{0xB5, 1, {0x00}},
{0xB6, 1, {0xb6}},
{0xB7, 1, {0x00}},
{0xB8, 1, {0xd0}},
{0xB9, 1, {0x00}},
{0xBA, 1, {0xe8}},
{0xBB, 1, {0x00}},
{0xBC, 1, {0xfe}},
{0xBD, 1, {0x01}},
{0xBE, 1, {0x0b}},
{0xBF, 1, {0x01}},
{0xC0, 1, {0x1e}},
{0xC1, 1, {0x01}},
{0xC2, 1, {0x4f}},
{0xC3, 1, {0x01}},
{0xC4, 1, {0x79}},
{0xC5, 1, {0x01}},
{0xC6, 1, {0xb7}},
{0xC7, 1, {0x01}},
{0xC8, 1, {0xe6}},
{0xC9, 1, {0x02}},
{0xCA, 1, {0x2a}},
{0xCB, 1, {0x02}},
{0xCC, 1, {0x64}},
{0xCD, 1, {0x02}},
{0xCE, 1, {0x66}},
{0xCF, 1, {0x02}},
{0xD0, 1, {0x95}},
{0xD1, 1, {0x02}},
{0xD2, 1, {0xC8}},
{0xD3, 1, {0x02}},
{0xD4, 1, {0xE8}},
{0xD5, 1, {0x03}},
{0xD6, 1, {0x0C}},
{0xD7, 1, {0x03}},
{0xD8, 1, {0x23}},
{0xD9, 1, {0x03}},
{0xDA, 1, {0x4A}},
{0xDB, 1, {0x03}},
{0xDC, 1, {0x58}},
{0xDD, 1, {0x03}},
{0xDE, 1, {0x61}},
{0xDF, 1, {0x03}},
{0xE0, 1, {0x70}},
{0xE1, 1, {0x03}},
{0xE2, 1, {0x78}},
{0xE3, 1, {0x03}},
{0xE4, 1, {0x87}},
{0xE5, 1, {0x03}},
{0xE6, 1, {0x91}},
{0xE7, 1, {0x03}},
{0xE8, 1, {0xAF}},
{0xE9, 1, {0x03}},
{0xEA, 1, {0xc5}}, 
//Don't Reload MTP
{0xFB, 1, {0x01}},
//CMD2_P4   
{0xFF, 1, {0x24}}, 
{REGFLAG_DELAY, 1, {}},
//Select CGOUT Output Signal  
//CGOUT1L=STV_L(RSTV)
{0x00, 1, {0x01}},
//CGOUT2L=GCK1_L(RCKV1)
{0x01, 1, {0x03}},
//CGOUT3L=GCK2_L(RCKV2)
{0x02, 1, {0x04}},
//CGOUT4L=GCK3_L(RCKV3)
{0x03, 1, {0x05}},
//CGOUT5L=GCK4_L(RCKV4)
{0x04, 1, {0x06}},
//CGOUT6L=VGL
{0x05, 1, {0x00}},
{0x06, 1, {0x00}},
{0x07, 1, {0x0F}},
{0x08, 1, {0x10}},
{0x09, 1, {0x0D}},
{0x0A, 1, {0x13}},
{0x0B, 1, {0x14}},
{0x0C, 1, {0x15}},
{0x0D, 1, {0x16}},
{0x0E, 1, {0x17}},
{0x0F, 1, {0x18}},
{0x10, 1, {0x01}},
{0x11, 1, {0x03}},
{0x12, 1, {0x04}},
{0x13, 1, {0x05}},
{0x14, 1, {0x06}},
{0x15, 1, {0x00}},
{0x16, 1, {0x00}},
{0x17, 1, {0x0F}},
{0x18, 1, {0x10}},
{0x19, 1, {0x0D}},
{0x1A, 1, {0x13}},
{0x1B, 1, {0x14}},
{0x1C, 1, {0x15}},
{0x1D, 1, {0x16}},
{0x1E, 1, {0x17}},
//CGOUT16R=BMUX3_R(CKHB[3])
{0x1F, 1, {0x18}},
//STV//     
{0x20, 1, {0x49}},
//Forward: R->L; Backward: L->R
{0x21, 1, {0x03}},
//STV_FALL_SEL=2nd line of last back porch
{0x22, 1, {0x01}},
{0x23, 1, {0x00}},
{0x24, 1, {0x68}},
{0x26, 1, {0x00}},
{0x27, 1, {0x68}},
{0x25, 1, {0x5D}},
{0x2F, 1, {0x02}},
{0x30, 1, {0x23}},
{0x31, 1, {0x49}},
{0x32, 1, {0x48}},
{0x33, 1, {0x41}},
{0x34, 1, {0x01}},
{0x35, 1, {0x75}},
{0x36, 1, {0x00}},
{0x37, 1, {0x1D}},
{0x38, 1, {0x08}},
{0x39, 1, {0x01}},
{0x3A, 1, {0x75}},
//GCKLK:R3Fh~R49h//
//CTRL:R5Bh~6Ah//
//RESET(CTRL1) has pulse(5~6 frame) in power on sequence
{0x5B, 1, {0x1A}},
//BGAS(CTRL2) has no pulse in power on sequence
{0x5C, 1, {0x00}},
{0x5D, 1, {0x00}},
{0x5E, 1, {0x00}},
//RESET(CTRL1) State
{0x5F, 1, {0x6D}},
//BGAS(CTRL2) State
{0x60, 1, {0x6D}},
{0x61, 1, {0x00}},
{0x62, 1, {0x00}},
{0x63, 1, {0x00}},
{0x64, 1, {0x00}},
{0x65, 1, {0x00}},
{0x66, 1, {0x00}},
{0x67, 1, {0x06}},
{0x68, 1, {0x06}},
{0x69, 1, {0x04}},
{0x6A, 1, {0x04}},
//PRECH:RF4h~RF9h//
{0xF4, 1, {0xD0}},
{0xF5, 1, {0x01}},
{0xF6, 1, {0x01}},
{0xF7, 1, {0x00}},
{0xF8, 1, {0x01}},
{0xF9, 1, {0x00}},
//MUX:R74h~R8Ch//
//Min. MUXS_V=2CLK: Display Ok
{0x74, 1, {0x02}},
{0x75, 1, {0x1C}},
{0x76, 1, {0x04}},
{0x77, 1, {0x03}},
{0x7A, 1, {0x00}},
{0x7B, 1, {0x91}},
{0x7C, 1, {0xD8}},
{0x7D, 1, {0x10}},
//Min. MUXS_V=2CLK: Display Ok
{0x7E, 1, {0x02}},
{0x7F, 1, {0x1C}},
{0x81, 1, {0x04}},
{0x82, 1, {0x03}},
{0x86, 1, {0x1B}},
{0x87, 1, {0x1B}},
{0x88, 1, {0x1B}},
{0x89, 1, {0x1B}},
//Pixel Inversion
//{0x8A, 1, {0x00}},
{0x8A, 1, {0x33}},
{0x8B, 1, {0x00}},
{0x8C, 1, {0x00}},
//FP, BP, RTN:R90h~R96h//
{0x90, 1, {0x79}},
{0x91, 1, {0x44}},
{0x92, 1, {0x79}},
{0x93, 1, {0x08}},
{0x94, 1, {0x08}},
{0x95, 1, {0x79}},
{0x96, 1, {0x79}},
//Add HSOUT & VSOUT
{0xC5, 1, {0x3A}},
{0xC6, 1, {0x09}},
//Column Inversion Type//
{0x9B, 1, {0x0F}},
//S1080 to S1
{0x9D, 1, {0x30}},
//Abnormal Power Off: RB3h~RB5h
{0xB3, 1, {0x28}},
{0xB4, 1, {0x0A}},
{0xB5, 1, {0x45}},
//Don't Reload MTP
{0xFB, 1, {0x01}},
//CMD3_PA (Remove it)          
//REGW 0xFF, 0xE0 
//CMD1
{0xFF, 1, {0x10}},
{REGFLAG_DELAY, 1, {}},
{0x3B, 3, {0x03,0x08, 0x08}},  // vedio porch
{0x35, 1, {0x00}},             // open TE
{0x44, 2, {0x05,0xDC}}, 
{0xBB, 1, {0x10}},             // command mode

{0x11, 1, {0x00}},
{REGFLAG_DELAY, 100, {}},
// Display ON
{0x29, 1, {0x00}},
{REGFLAG_DELAY, 20, {}},
{REGFLAG_END_OF_TABLE, 0x00, {}},
};

static struct LCM_setting_table lcm_deep_sleep_mode_in_setting[] = {
// Display off sequence
{0x28, 1, {0x00}},
{REGFLAG_DELAY, 20, {}},
// Sleep Mode On
{0x10, 1, {0x00}},
{REGFLAG_DELAY, 100, {}},
//enable DSTB, enter deep standby mode
{0x4F, 1, {0x01}},
{REGFLAG_END_OF_TABLE, 0x00, {}}
};

static void push_table(struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
{
	unsigned int i;

	for(i = 0; i < count; i++) {

		unsigned cmd;
		cmd = table[i].cmd;

		switch (cmd) {
			case REGFLAG_DELAY :
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
// LCM Driver Implementations
// ---------------------------------------------------------------------------

static void lcm_set_util_funcs(const LCM_UTIL_FUNCS *util)
{
	memcpy(&lcm_util, util, sizeof(LCM_UTIL_FUNCS));
}


static void lcm_get_params(LCM_PARAMS *params)
{

	memset(params, 0, sizeof(LCM_PARAMS));
	params->type = LCM_TYPE_DSI;
	params->width = FRAME_WIDTH;
	params->height = FRAME_HEIGHT;


	#if (LCM_DSI_CMD_MODE)
	params->dsi.mode = CMD_MODE;
	#else
	params->dsi.mode = SYNC_PULSE_VDO_MODE;
	#endif

	// DSI
	/* Command mode setting */
	params->dsi.LANE_NUM = LCM_FOUR_LANE;
	params->dsi.data_format.format = LCM_DSI_FORMAT_RGB888;
	params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;

	//bit rate
	params->dsi.PLL_CLOCK = 448;   //this value must be in MTK suggested table
	params->dsi.ssc_disable = 1;		

}

static void lcm_id_pin_handle(void)
{
     mt_set_gpio_pull_select(GPIO_DISP_ID0_PIN,GPIO_PULL_DOWN);
     mt_set_gpio_pull_select(GPIO_DISP_ID1_PIN,GPIO_PULL_UP);
}

static void lcm_init(void)
{
    //enable VSP & VSN
     lcm_util.set_gpio_out(GPIO_LCD_BIAS_ENP_PIN, GPIO_OUT_ONE);
     mdelay(5);
     lcm_util.set_gpio_out(GPIO_LCD_BIAS_ENN_PIN, GPIO_OUT_ONE);
     mdelay(10); 
	 
	 //reset low to high
	 lcm_util.set_gpio_out(GPIO_DISP_LRSTB_PIN, GPIO_OUT_ONE);
	 mdelay(5); 
	 lcm_util.set_gpio_out(GPIO_DISP_LRSTB_PIN, GPIO_OUT_ZERO);
	 mdelay(10); 
     lcm_util.set_gpio_out(GPIO_DISP_LRSTB_PIN, GPIO_OUT_ONE);
	 mdelay(30); 
     lcm_id_pin_handle();    // ID0 = 0; ID1 = 1

	 push_table(lcm_initialization_ret, sizeof(lcm_initialization_ret) / sizeof(struct LCM_setting_table), 1);
	 
     lcm_util.set_gpio_out(GPIO_LCD_DRV_EN_PIN, GPIO_OUT_ONE); 
	 LCD_DEBUG("uboot:nt35595_fhd_dsi_cmd_cmi_lcm_init\n");
}

static void lcm_suspend(void)
{
	// when phone sleep , config output low, disable backlight drv chip  
    lcm_util.set_gpio_out(GPIO_LCD_DRV_EN_PIN, GPIO_OUT_ZERO);
		
    push_table(lcm_deep_sleep_mode_in_setting, sizeof(lcm_deep_sleep_mode_in_setting) / sizeof(struct LCM_setting_table), 1);
	
    //disable VSP & VSN
    lcm_util.set_gpio_out(GPIO_LCD_BIAS_ENN_PIN, GPIO_OUT_ZERO);
	mdelay(5);
	lcm_util.set_gpio_out(GPIO_LCD_BIAS_ENP_PIN, GPIO_OUT_ZERO);
	mdelay(5);

	//enter deep standby mode
	
	LCD_DEBUG("uboot:nt35595_fhd_dsi_cmd_cmi_lcm_suspend\n");
}


static void lcm_resume(void)
{
	//exit deep standby mode, enter sleep in mode
	lcm_util.set_gpio_out(GPIO_DISP_LRSTB_PIN, GPIO_OUT_ZERO);
	mdelay(5);
	lcm_init();
	LCD_DEBUG("uboot:nt35595_fhd_dsi_cmd_cmi_lcm_resume\n");
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


static unsigned int lcm_compare_id_cmi(void)
{
    unsigned char LCD_ID_value = 0;
    LCD_ID_value = which_lcd_module_triple();
    if(LCM_ID_nt35595_cmi == LCD_ID_value)
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
     {0xFF, 1, {0x24}},          //enter to CMD2_P4 
	 {REGFLAG_DELAY, 1, {}},
     {0x9B, 1, {0x0F}},          //Column Inversion Type
	 {0xFF, 1, {0x10}},          //return to CMD1
	 {REGFLAG_DELAY, 1, {}},
     {REGFLAG_END_OF_TABLE, 0x00, {}}
};
/******************************************************************************
  Function:       // lcm_setInversion_mode
  Description:    // This function is used to set inversion mode.
  Input:          // unsigned int mode, the input parameter means inversion 
                     mode which be passed from application layer. 
  Output:         // no
  Return:         // Void
  Others:         // no
******************************************************************************/
static void lcm_setInversion_mode(unsigned int mode)   
{
    printk("nt35595_fhd_dsi_cmd_cmi lcm_setInversion_mode mode = %d\n", mode);
    if(DOT_INVERSION == mode)
    {
        lcm_Inversion_mode_setting[2].para_list[0] = dot_Inversion_value;
    }
    else
    {
        lcm_Inversion_mode_setting[2].para_list[0] = column_Inversion_value;
    }
    push_table(lcm_Inversion_mode_setting, sizeof(lcm_Inversion_mode_setting) / sizeof(struct LCM_setting_table), 1);
}
/******************************************************************************
  Function:       // lcm_check_inversion_set
  Description:    // This function is used to check if the inversion mode is setted successfully.
  Input:          // unsigned int mode, the input parameter means inversion 
                     mode which be passed from application layer. 
  Output:         // no
  Return:         // The result of the setting inversion mode.
  Others:         // no
******************************************************************************/
static unsigned int lcm_check_inversion_set(unsigned int mode)
{
    unsigned int buffer[] = {0};
    unsigned int array[] = {0};
	
	array[0] = 0x24FF1500;                      //enter  to  CMD2_P4
    dsi_set_cmdq(array, 1,1);
    mdelay(1);
	
    array[0] = 0x00013700;   
    dsi_set_cmdq(array, 1,1);       
    read_reg_v2(0x9B, buffer,1);                // read 9Bh

    array[0] = 0x10FF1500;                      //return  to  CMD1
    dsi_set_cmdq(array, 1,1);
    mdelay(1);
	
    printk("nt35595_fhd_dsi_cmd_cmi  0x9B = 0x%x \n",buffer[0]);
    if(COLUMN_INVERSION == mode)
    {
        if( buffer[0] == column_Inversion_value )
        {
            printk("nt35595_fhd_dsi_cmd_cmi column_Inversion_value set_inversion_mode success! 0x9B = 0x%x \n",buffer[0]);
            return TRUE;
        }
        else
        {
            printk("nt35595_fhd_dsi_cmd_cmi column_Inversion_value set_inversion_mode fail! 0x9B = 0x%x \n",buffer[0]);
            return FALSE;
        }
    }
    else
    {
        if( buffer[0] == dot_Inversion_value )
        {
           printk("nt35595_fhd_dsi_cmd_cmi dot_Inversion_value set_inversion_mode success! 0x9B = 0x%x \n",buffer[0]);
           return TRUE;
        }
        else
        {
           printk("nt35595_fhd_dsi_cmd_cmi dot_Inversion_value set_inversion_mode fail! 0x9B = 0x%x \n",buffer[0]);
           return FALSE;
        }
    }	
}
/******************************************************************************
  Function:       // lcm_check_state
  Description:    // This function is used to check if the status register 
                      are changed during the dot inversion mode.
  Input:          // no
  Output:         // no
  Return:         // The result of the running test2.
  Others:         // no
******************************************************************************/
static unsigned int lcm_check_state()
{
    unsigned char buffer_1[] = {0};
    unsigned char buffer_2[] = {0};
    unsigned char buffer_3[] = {0};
    unsigned char buffer_4[] = {0};
    unsigned int array[] = {0};
 
    array[0] = 0x00013700;
    dsi_set_cmdq(array, 1,1);
    read_reg_v2(0x0A, buffer_1,1);
    printk("nt35595_fhd_dsi_cmd_cmi 0x0A = 0x%x ",buffer_1[0]);
	
    array[0] = 0x00013700;
    dsi_set_cmdq(array, 1,1);	
    read_reg_v2(0x0B, buffer_2,1);
    printk("nt35595_fhd_dsi_cmd_cmi 0x0B = 0x%x ",buffer_2[0]);
	
    array[0] = 0x00013700;
    dsi_set_cmdq(array, 1,1);	
    read_reg_v2(0x0C, buffer_3,1);
    printk("nt35595_fhd_dsi_cmd_cmi 0x0C = 0x%x ",buffer_3[0]);
		
    array[0] = 0x00013700;
    dsi_set_cmdq(array, 1,1);	
    read_reg_v2(0x0D, buffer_4,1);	
    printk("nt35595_fhd_dsi_cmd_cmi 0x0D = 0x%x ",buffer_4[0]);
	
    if((buffer_1[0] != 0x9C) || (buffer_2[0] != 0x00) || (buffer_3[0] != 0x77) || (buffer_4[0] != 0x00))/*LCD work status error,need re-initalize*/
    {
        printk("nt35595_fhd_dsi_cmd_cmi Lcm_esd_check fail! 0x0A = 0x%x ; 0x0B = 0x%x ; 0x0C = 0x%x ; 0x0D = 0x%x \n",buffer_1[0],buffer_2[0],buffer_3[0],buffer_4[0]);
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
        printk("nt35595_fhd_dsi_cmd_cmi Lcm_esd_check success! 0x0A = 0x%x ; 0x0B = 0x%x ;0x0C = 0x%x ; 0x0D = 0x%x \n",buffer_1[0],buffer_2[0],buffer_3[0],buffer_4[0]);
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
    unsigned int array[4];

    array[0] = 0x00013700;
    dsi_set_cmdq(array, 1, 1);
    read_reg_v2(0x0A, buffer,1);

    if((buffer[0] != 0x9C))/*LCD work status error,need re-initalize*/
    {
        printk( "cmi_nt35595_fhd_cmd lcm_esd_check register:0Ah buffer[0] = %d\n",buffer[0]);
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

static unsigned int lcm_esd_recover(void)
{
    /*LCD work status error ,so initialize*/
    //reset low to high
    lcm_util.set_gpio_out(GPIO_DISP_LRSTB_PIN, GPIO_OUT_ONE);
    mdelay(5);
    lcm_util.set_gpio_out(GPIO_DISP_LRSTB_PIN, GPIO_OUT_ZERO);
    mdelay(10);
    lcm_util.set_gpio_out(GPIO_DISP_LRSTB_PIN, GPIO_OUT_ONE);
    msleep(120);

    push_table(lcm_initialization_ret, sizeof(lcm_initialization_ret) / sizeof(struct LCM_setting_table), 1);

    lcm_util.set_gpio_out(GPIO_LCD_DRV_EN_PIN, GPIO_OUT_ONE);
    LCD_DEBUG("cmi_nt35595_fhd_cmd ESD recover!\n");
    return TRUE;
}
#endif

// ---------------------------------------------------------------------------
// Get LCM Driver Hooks
// ---------------------------------------------------------------------------
LCM_DRIVER nt35595_fhd_dsi_cmd_cmi_lcm_drv = 
{
	.name = "cmi_nt35595_fhd_cmd",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params = lcm_get_params,
	.init = lcm_init,
	.suspend = lcm_suspend,
	.resume = lcm_resume,
    .compare_id = lcm_compare_id_cmi,
#ifndef BUILD_LK
    .set_inversion_mode =  lcm_setInversion_mode,
    .lcm_check_state  =  lcm_check_state,
    .lcm_check_inversion_set = lcm_check_inversion_set,
    .esd_check      = lcm_esd_check,
    .esd_recover    = lcm_esd_recover,
#endif
#if (LCM_DSI_CMD_MODE)
	.update = lcm_update,
#endif
};
