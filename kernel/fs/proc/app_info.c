/******************************************************************************

  Copyright (C), 2011-2015, Huawei Tech. Co., Ltd.

 ******************************************************************************
  File Name     : app_info.c
  Description   : pirnt UE info to proc/app_info node
******************************************************************************/

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/hardware_self_adapt.h>
#include <linux/seq_file.h>
#include <linux/cdev.h>
#include <linux/poll.h>
#include <linux/sched.h>
#include <linux/version.h>
#include <asm/system.h>
#include <asm/mach-types.h>

#define MAX_ID_LEN       32
#define SPRINTF_SUCCESS  (0)
#define SPRINTF_ERROR    (1)


/*
* if you want to add a new id, please add id name here, and
* add a id type in id_type enum which defined in <linux/hardware_self_adapt.h>
*/
static const char *id_name[] = {
    "board_id",
    "emmc_id",
    "primary_camera_id",
    "slave_camera_id",
    "lcd_id",
    "gsensor_id",
    "compass_id",
    "tp_id",
    "hw_version",
    "battery_id",
#ifdef CONFIG_DL_CHECK_SUPPORT
    "dl_check_tag",
#endif
    "als_ps_id",

    "ddr_id",

};

struct BoardID_setting_table {
    hw_product_type Product_Version;
    char* Product_Name;
    char* Platform_Name;
    char* Hardward_Version;
};

#define DDR_ID_LENGTH    (4)  
struct ddr_id_info  
{  
    u32 ddr_id[DDR_ID_LENGTH];  
    char ddr_str[MAX_ID_LEN];  
};  

struct ddr_id_info ddr_infos[]=  
{  
    {  
        {0x00000001, 0x0, 0x0, 0x0},  
        "Samsung_K4PAG304EB_FGC2"  
    },  
    {  
        {0x00000003, 0x0, 0x0, 0x0},  
        "Elpida_EDBA232B1MA"  
    },  
    {  
        {0x00000006, 0x0, 0x0, 0x0},  
        "Hynix_H9TCNNNBLDMMPR"  
    },  
};

// you can add a new print infomation here
static struct BoardID_setting_table BoardID_string_init[] = {
    //{Product_Version, Product_Name, Platform_Name, Hardward_Version}

    // G610-T11
    {HW_G610T10_VER,     "G610-T11", "MTK6582", "HT2G610M"},
    {HW_G610T10_VER_B,   "G610-T11", "MTK6582", "HT3G610M"},
    {HW_G610T10_VER_C,   "G610-T11", "MTK6582", "HT3G610M"},
    {HW_G610T10_VER_D,   "G610-T11", "MTK6582", "HT3G610M"},
    // G750-T00
    {HW_G750_VER,     "G750-T00", "MTK6592", "HT2G750M"},
    {HW_G750_VER_B,   "G750-T00", "MTK6592", "HT2G750M"},
    {HW_G750_VER_C,   "G750-T00", "MTK6592", "HT2G750M"},
    {HW_G750_VER_D,   "G750-T01", "MTK6592", "HT3G750T01M"},
    {HW_G750_VER_E,   "G750-U10", "MTK6592", "HU3G750U10M"},
    {HW_G750_VER_F,   "G750-T20", "MTK6592", "HT4G750T20M"},
    // G750-U00
    {HW_G750U_VER,     "G750-U00", "MTK6592", "HD2G750M"},
    {HW_G750U_VER_B,   "G750-U00", "MTK6592", "HD2G750M"},
    {HW_G750U_VER_C,   "G750-U00", "MTK6592", "HD2G750M"},
    {HW_G750U_VER_D,   "G750-U00", "MTK6592", "HD2G750M"},
    // G730-T00
    {HW_G730_VER,     "G730-T00", "MTK6582", "HT1G730M"},
    {HW_G730_VER_B,   "G730-T00", "MTK6582", "HT1G730M"},
    {HW_G730_VER_C,   "G730-T00", "MTK6582", "HT1G730M"},
    {HW_G730_VER_D,   "G730-T20", "MTK6582", "HT1G730M"},

    // G730-U00
    {HW_G730U_VER,     "G730-U00", "MTK6582", "HT1G730M"},
    {HW_G730U_VER_B,   "G730-U00", "MTK6582", "HT1G730M"},
    {HW_G730U_VER_C,   "G730-U00", "MTK6582", "HT1G730M"},
    {HW_G730U_VER_D,   "G730-U251", "MTK6582", "HT1G730M"},
    {HW_G730U_VER_E,   "G730-U27", "MTK6582", "HT1G730M"},
    {HW_G730U_VER_F,   "G730-U10", "MTK6582", "HT1G730M"},


  // H30-T00
//	{HW_H30T_VER,     "H30-T00", "MTK6582", "HT1H30M"},
//	{HW_H30T_VER_B,   "H30-T00", "MTK6582", "HT1H30M"},
//	{HW_H30T_VER_C,   "H30-T00", "MTK6582", "HT1H30M"},
	{HW_H30T_VER,     "H30-T00", "MTK6582", "HT1R300MT"},
	{HW_H30T_VER_B,   "H30-T00", "MTK6582", "HT1R300MT"},
	{HW_H30T_VER_C,   "H30-T00", "MTK6582", "HT1R300MT"},
	{HW_H30T_VER_D,   "H30-T10", "MTK6582", "HD1H30TM"},	// H30-T10 verA
	{HW_H30T_VER_E,   "H30-T10", "MTK6582", "HD1H30TM"},	// H30-T10 verB
	{HW_H30T_VER_F,   "H30-T10", "MTK6582", "HD1H30TM"},	// H30-T10 verC

	// H30-U00
	{HW_H30U_VER,     "H30-U00", "MTK6582", "HD1H30M"},
	{HW_H30U_VER_B,   "H30-U00", "MTK6582", "HD1H30M"},
	{HW_H30U_VER_C,   "H30-U00", "MTK6582", "HD1H30M"},
	{HW_H30U_VER_D,   "H30-U10", "MTK6582", "HD1H30TM"},	// H30-U10 verA
	{HW_H30U_VER_E,   "H30-U10", "MTK6582", "HD1H30TM"},	// H30-U10 verB
	{HW_H30U_VER_F,   "H30-U10", "MTK6582", "HD1H30TM"},	// H30-U10 verC

    // G6-T00
    {HW_G6T_VER,     "G6-T00", "MTK6582", "HT1G6TM"},
    {HW_G6T_VER_B,   "G6-T00", "MTK6582", "HT1G6TM"},
    {HW_G6T_VER_C,   "G6-T00", "MTK6582", "HT1G6TM"},
    {HW_G6T_VER_D,   "G6-T00", "MTK6582", "HT1G6TM"},

    //G630-T00
    {HW_G630T_VER_A,   "G630-T00", "MTK6582", "HT1G630TM"},		// G630-T00 verA
    {HW_G630T_VER_B,   "G630-T00", "MTK6582", "HT1G630TM"},		// G630-T00 verB
    {HW_G630T_VER_C,   "G630-T00", "MTK6582", "HT1G630TM"},		// G630-T00 verC

    //G610-U10
    {HW_G610U10_VER_A,   "G610-U10", "MTK6582", "HU3G610TM"},		// G610-U10 verA
    {HW_G610U10_VER_B,   "G610-U10", "MTK6582", "HU3G610TM"},		// G610-U10 verB
    {HW_G610U10_VER_C,   "G610-U10", "MTK6582", "HU3G610TM"},		// G610-U10 verC
    // ULC02
    {HW_ULC02_VER_A,     "ULC02",    "MTK6582", "HT1GULC02"},
    {HW_ULC02_VER_B,     "ULC02",    "MTK6582", "HT1GULC02"},
    {HW_ULC02_VER_C,     "ULC02",    "MTK6582", "HT1GULC02"},
    {HW_ULC02_VER_D,     "ULC02",    "MTK6582", "HT1GULC02"}, 
    {HW_ULC02_VER_E,     "ULC02",    "MTK6582", "HT1GULC02"},
    {HW_ULC02_VER_N,     "ULC02",    "MTK6582", "HT1GULC02"},


    //reserved
    {HW_VER_NONE,    "unknown", "unknown", "unknown"}
};

static char *id_value[MAX_NUM_ID] = {NULL};

void set_id_value(id_type type, char *value)
{
    if ((type < MAX_NUM_ID) && (NULL != value))
    {
        snprintf(id_value[type], MAX_ID_LEN, "%s", value);
    }
}

/* initialize the the member of id_value to "unkonwn" */
static int id_value_mem_alloc(void)
{
    int i = 0;

    for (i = 0; i < MAX_NUM_ID; i++)
    {
        id_value[i] = kzalloc(MAX_ID_LEN, GFP_KERNEL);
        if (!id_value[i]) {
            printk("%s: memory allocation failed\n", __FUNCTION__);
            return -ENOMEM;
        }
        snprintf(id_value[i], MAX_ID_LEN, "unknown");
    }
    return 0;
}

static void id_value_mem_free(void)
{
    int i = 0;

    for (i = 0; i < MAX_NUM_ID; i++)
    {
        if (id_value[i] != NULL)
        {
            kfree(id_value[i]);
            id_value[i] = NULL;
        }
    }
}


static int set_Boardid_string(hw_product_type Product_Version)
{
    int  convert_version;
    int  main_version;
    int  ops_flag = SPRINTF_ERROR;
    char sub_version;
    unsigned int i;

    main_version = Product_Version & HW_VER_MAIN_MASK;
    if ((HW_G610T10_VER < Product_Version) && (HW_G610T10_VER == main_version))
    {
        convert_version = (Product_Version & HW_VER_SUB_MASK) - HW_VER_SUB_VB;
    }
    /*G730 sub product should begin with Ver.B*/
    else if( (Product_Version == HW_G730U_VER_D)
             ||(Product_Version == HW_G730U_VER_E)
             ||(Product_Version == HW_G730U_VER_F)
             ||(Product_Version == HW_G730_VER_D) )
    {
        convert_version = HW_VER_SUB_VB;
    }

	else if( (Product_Version >= HW_H30T_VER_D)
		   && (main_version == HW_H30T_VER) )
	{
		convert_version = (Product_Version & HW_VER_SUB_MASK) - HW_VER_SUB_VD;
	}
	else if( (Product_Version >= HW_H30U_VER_D)
		   && (main_version == HW_H30U_VER) )
	{
		convert_version = (Product_Version & HW_VER_SUB_MASK) - HW_VER_SUB_VD;
	}
    /* remain sub version as Ver.B */
    else if(HW_G750_VER == main_version && Product_Version > HW_G750_VER_C)
    {
        convert_version = HW_VER_SUB_VB;
    }
    else
    {
        convert_version = (Product_Version & HW_VER_SUB_MASK);
    }
    sub_version  = 'A' + convert_version;

    for (i=0; i< sizeof(BoardID_string_init)/sizeof(struct BoardID_setting_table); i++)
    {
        if (Product_Version == BoardID_string_init[i].Product_Version)
        {
            /* set board_id value */
            snprintf(id_value[BOARD_ID], MAX_ID_LEN, "%s_%s.Ver%c\0",
                    BoardID_string_init[i].Platform_Name,
                    BoardID_string_init[i].Product_Name,
                    sub_version);

            snprintf(id_value[HW_VERSION], MAX_ID_LEN, "%s Ver.%c\0",
                     BoardID_string_init[i].Hardward_Version,
                     sub_version);

            ops_flag = SPRINTF_SUCCESS;
            break;
        }
        else if (HW_VER_NONE == BoardID_string_init[i].Product_Version)
        {
            ops_flag = SPRINTF_ERROR;
            break;
        }
    }
    return ops_flag;
}


/* the function to set the value of board_id and hw_version */
static void set_app_info_and_hw_version(void)
{
    hw_product_type product_version;
    int main_version;
    int ops_flag = SPRINTF_ERROR;

    product_version  = get_hardware_product_version();
    ops_flag         = set_Boardid_string(product_version);

    if ( SPRINTF_ERROR == ops_flag )
    {
        printk("boot_id: %s: got a wrong product version number\n", __FUNCTION__);
    }

}

static void set_app_info_battery_type(void)
{
    const char* battery_id_string_default[BATT_ID_INVALID + 1] = {
        "BYD",
        "22K",
        "39K",
        "68K",
        "110K",
        "200K",
        "Scud",
        "2000K",
        "Invalid"
    };
    const char* battery_id_string_HB3742A0EBC[BATT_ID_INVALID + 1] = {
    //manufacture for G6
        "Sunwoda",
        "22K",
        "LiShen",
        "68K",
        "Scud",
        "200K",
        "470K",
        "2000K",
        "Invalid"
    };

    const char **battery_id_string = battery_id_string_default;
  if((get_hardware_product_version() & HW_VER_MAIN_MASK) == HW_G6T_VER)
    {
        battery_id_string = battery_id_string_HB3742A0EBC;
    }
    hw_battery_type battery_id = get_hardware_battery_type();

    int adc_value = get_battery_id_adc_value();
    snprintf(id_value[BATTERY_ID], MAX_ID_LEN, "%s(adc value-%d)", battery_id_string[battery_id], adc_value);

}

static int get_ddr_type(u32 id)  
{  
    int i = 0;  
    int ret = 1;  

    for(i = 0; i < sizeof(ddr_infos)/sizeof(struct ddr_id_info); i++)  
    {  
        if( (id&0x000000FF) == ddr_infos[i].ddr_id[0] )  
        {  
            return i;  
        }  
    }  
 
    return -ret;  
}
 
static void set_app_info_ddr_info(void)  
{  
    int ddr_type;  

    ddr_type = get_ddr_type( get_ddr_md5_value() );  
    if(ddr_type >= 0)  
        set_id_value(DDR_ID, ddr_infos[ddr_type].ddr_str);  
}


static int app_info_show(struct seq_file *m, void *v)
{
    int i;

    /* Show all app_info information */
    for(i=0;i<MAX_NUM_ID;i++)
    {

        seq_printf(m, "%-24s\t: %s\n",id_name[i], id_value[i]);
    }
    return 0;
}
static void *app_info_start(struct seq_file *m, loff_t *pos)
{
    return *pos < 1 ? (void *)1 : NULL;
}

static void *app_info_next(struct seq_file *m, void *v, loff_t *pos)
{
    ++*pos;
    return NULL;
}

static void app_info_stop(struct seq_file *m, void *v)
{
    return;
}

const struct seq_operations app_info_op = {
    .start   = app_info_start,
    .next    = app_info_next,
    .stop    = app_info_stop,
    .show    = app_info_show
};

static int app_info_open(struct inode *inode, struct file *file)
{
    return seq_open(file, &app_info_op);
}


static const struct file_operations proc_app_info_operations = {
    .open        = app_info_open,
    .read        = seq_read,
    .llseek      = seq_lseek,
    .release     = seq_release,
};


static int app_info_init(void)
{
    int ret;

    printk("boot_id: app_info_init\n");

    ret = id_value_mem_alloc();
    if (ret)
    {
        printk("boot_id: Init id_value failed\n");
        goto out_error;
    }

    /*
    * add all set id_value function here, the flag value_setted
    * used to make sure just set the id value once
    */
    set_app_info_and_hw_version();

    set_app_info_battery_type();

    set_app_info_ddr_info();

    proc_create("app_info", 0, NULL, &proc_app_info_operations);

    return 0;

out_error:
    printk("app_info: Failed in %s.\n", __FUNCTION__);
    id_value_mem_free();
    return ret;
}

module_init(app_info_init);
MODULE_LICENSE("GPL");
