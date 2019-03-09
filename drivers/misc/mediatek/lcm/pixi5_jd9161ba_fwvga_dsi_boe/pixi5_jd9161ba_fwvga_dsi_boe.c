#ifndef BUILD_LK
    #include <linux/string.h>
    #include <linux/kernel.h>
#endif
#include "lcm_drv.h"

#ifdef BUILD_LK
    #include <platform/mt_gpio.h>	
#elif defined(BUILD_UBOOT)
    #include <asm/arch/mt_gpio.h>
    #include <platform/mt_pmic.h>
#else
    #include <mt-plat/mt_gpio.h>
    #include <mach/gpio_const.h>
#endif


// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------

#define FRAME_WIDTH  (480)
#define FRAME_HEIGHT (854)

#define REGFLAG_DELAY             						0xFD
#define REGFLAG_END_OF_TABLE      				0xFE   // END OF REGISTERS MARKER

#define LCM_DSI_CMD_MODE						0
#ifndef TRUE
    #define TRUE 1
#endif

#ifndef FALSE
    #define FALSE 0
#endif

//set LCM IC ID
#define LCM_ID_jd9161ba 	(0x980604)
#define GPIO_LCM_ID1	(GPIO81 | 0x80000000)
#define GPIO_LCM_ID2	(GPIO82 | 0x80000000)

//#define GPIO_LCM_ID1	GPIO81_LCD_ID1
//#define GPIO_LCM_ID2	GPIO82_LCD_ID2

// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static LCM_UTIL_FUNCS lcm_util = {0};

#define SET_RESET_PIN(v)    (lcm_util.set_reset_pin((v)))

#define UDELAY(n) (lcm_util.udelay(n))
#define MDELAY(n) (lcm_util.mdelay(n))

#ifdef BUILD_LK
#define LCD_DEBUG(fmt, args...) printf(fmt, ##args)		
#else
#define LCD_DEBUG(fmt, args...) printk(fmt, ##args)
#endif

//#define LCM_ID       (0x7181)

// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------

#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)	lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)		lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)									lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)				lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg(cmd)											lcm_util.dsi_dcs_read_lcm_reg(cmd)
#define read_reg_v2(cmd, buffer, buffer_size)   				lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)    

struct LCM_setting_table {
    unsigned char cmd;
    unsigned char count;
    unsigned char para_list[64];
};


static struct LCM_setting_table lcm_initialization_setting[] = {
	{0xBF,3,{0x91,0x61,0xF2}},

	//VCOM
	{0xB3,2,{0x00,0x86}},

	//VCOM_R
	 {0xB4,2,{0x00,0x86}},

	//VGMP, VGSP, VGMN, VGSN
	{0xB8,6,{0x00,0xBF,0x00,0x00,0xBF,0x00}},  

	//GIP output voltage level
	{0xBA,3,{0x34,0x23,0x00}},  

	//line invension
	{0xC3,1,{0x04}},

	//TCON
	{0xC4,2,{0x30,0x6A}},  

	//POWER CTRL
	{0xC7,9,{0x00,0x01,0x31,0x05,0x65,0x2C,0x13,0xA5,0xA5}},  

	//Gamma
	//{0xC8,38,{0x70,0x78,0x6C,0x53,0x3B,0x20,0x14,0x00,0x1A,0x1E,0x24,0x4B,0x41,0x54,0x4E,0x56,
	//0x50,0x48,0x0F,0x70,0x78,0x6C,0x53,0x3B,0x20,0x14,0x00,0x1A,0x1E,0x24,0x4B,0x41,0x54,0x4E,0x56,0x50,0x48,0x0F}}, 
	{0xC8,38,{0x70,0x78,0x60,0x40,0x24,0x0F,0x13,0x00,0x1E,0x22,0x29,0x4F,0x46,0x59,0x53,0x5B,
	 0x55,0x4C,0x0F,0x70,0x78,0x60,0x40,0x24,0x0F,0x13,0x00,0x1E,0x22,0x29,0x4F,0x46,0x59,0x53,0x5B,0x55,0x4C,0x0F}},                           

	//SET GIP_L
	{0xD4,16,{0x1F,0x1E,0x05,0x07,0x01,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F}},

	//SET GIP_R
	{0xD5,16,{0x1F,0x1E,0x04,0x06,0x00,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F}}, 

	//SET GIP_GS_L
	{0xD6,15,{0x1F,0x1F,0x06,0x04,0x00,0x1E,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F}}, 

	//SET GIP_GS_R
	{0xD7,16,{0x1F,0x1F,0x07,0x05,0x01,0x1E,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F}}, 

	//SET GIP1
	{0xD8,20,{0x20,0x00,0x00,0x10,0x03,0x20,0x01,0x02,0x00,0x01,0x02,0x50,0x5F,0x00,0x00,0x32,0x03,0x50,0x5F,0x08}},  			

	//SET GIP2
	{0xD9,19,{0x00,0x0A,0x0A,0x88,0x00,0x00,0x06,0x7b,0x00,0x00,0x00,0x3B,0x2F,0x1F,0x00,0x00,0x00,0x03,0x7B}},  

	{0xBE,1,{0x01}}, 
	//esd 
	{0xCC,10,{0x34,0x20,0x38,0x60,0x11,0x91,0x00,0x40,0x00,0x00}}, 

	{0xBE,1,{0x00}}, 	

        {REGFLAG_DELAY, 50, {}},
	//SLP OUT
	{0x11, 0, {0x00}},
	{REGFLAG_DELAY, 120, {}},  
	//DISP ON
	{0x29, 0, {0x00}},

	{REGFLAG_END_OF_TABLE, 0x00, {}}
};
/*
static struct LCM_setting_table lcm_sleep_out_setting[] = {
    // Sleep Out
    {0x11, 1, {0x00}},
    {REGFLAG_DELAY, 120, {}},
    
    // Display ON
    {0x29, 1, {0x00}},
    {REGFLAG_DELAY, 50, {}},
    {REGFLAG_END_OF_TABLE, 0x00, {}}
};
*/

static struct LCM_setting_table lcm_deep_sleep_mode_in_setting[] = {
    // Display off sequence
    {0x28, 1, {0x00}},
    {REGFLAG_DELAY, 10, {}},
    
    // Sleep Mode On
    {0x10, 1, {0x00}},
    {REGFLAG_DELAY, 120, {}},
    
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
                MDELAY(table[i].count);
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

	// enable tearing-free
	//params->dbi.te_mode 			= LCM_DBI_TE_MODE_VSYNC_ONLY;
	//params->dbi.te_edge_polarity		= LCM_POLARITY_RISING;
	params->dbi.te_mode 				= LCM_DBI_TE_MODE_DISABLED;

#if (LCM_DSI_CMD_MODE)
	params->dsi.mode   = CMD_MODE;
#else
	params->dsi.mode   = SYNC_PULSE_VDO_MODE;
#endif

	// DSI
	/* Command mode setting */
	params->dsi.LANE_NUM				= LCM_TWO_LANE;
	//The following defined the fomat for data coming from LCD engine.
	params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
	params->dsi.data_format.trans_seq   = LCM_DSI_TRANS_SEQ_MSB_FIRST;
	params->dsi.data_format.padding     = LCM_DSI_PADDING_ON_LSB;
	params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;

	// Highly depends on LCD driver capability.
	// Not support in MT6573
	params->dsi.packet_size=256;

	// Video mode setting		
	params->dsi.intermediat_buffer_num = 2;

	params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
	params->dsi.word_count=480*3;

	params->dsi.lcm_ext_te_enable=false;

	params->dsi.vertical_sync_active				= 4;
	params->dsi.vertical_backporch					= 5;
	params->dsi.vertical_frontporch					= 7;
	params->dsi.vertical_active_line				= FRAME_HEIGHT; 

	params->dsi.horizontal_sync_active				= 20;//10
	params->dsi.horizontal_backporch				= 80;//50
	params->dsi.horizontal_frontporch				= 80;//50
	params->dsi.horizontal_active_pixel				= FRAME_WIDTH;


	params->dsi.HS_TRAIL= 7;  // 4.3406868
	params->dsi.HS_PRPR = 4;
	
	params->dsi.CLK_TRAIL= 50;
 
	params->dsi.esd_check_enable = 1;

	params->dsi.customization_esd_check_enable = 1;

	params->dsi.lcm_esd_check_table[0].cmd          = 0x0A;
	params->dsi.lcm_esd_check_table[0].count        = 1;
	params->dsi.lcm_esd_check_table[0].para_list[0] = 0x01;  //page0

	// Bit rate calculation
	//params->dsi.pll_div1=37;		// fref=26MHz, fvco=fref*(div1+1)	(div1=0~63, fvco=500MHZ~1GHz)
	//params->dsi.pll_div2=1; 		// div2=0~15: fout=fvo/(2*div2)

/*	// Bit rate calculation
	params->dsi.pll_div1=1;		// fref=26MHz, fvco=fref*(div1+1)	(div1=0~63, fvco=500MHZ~1GHz)
	params->dsi.pll_div2=1; 		// div2=0~15: fout=fvo/(2*div2)

	params->dsi.fbk_div =27; // fref=26MHz, fvco=fref*(fbk_div+1)*fbk_sel_real/(div1_real*div2_real) */

	params->dsi.PLL_CLOCK = 205;//240 180; //this value must be in MTK suggested table
	//params->dsi.ssc_disable	= 1;
	
}




static void lcm_init(void)
{

	SET_RESET_PIN(1);
	MDELAY(10);
	SET_RESET_PIN(0);
	MDELAY(10);
	SET_RESET_PIN(1);
	MDELAY(120);

	push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
}


static void lcm_suspend(void)
{
	push_table(lcm_deep_sleep_mode_in_setting, sizeof(lcm_deep_sleep_mode_in_setting) / sizeof(struct LCM_setting_table), 1);
	SET_RESET_PIN(0);
	MDELAY(20);
}

//static unsigned int lcm_compare_id(void);
//static int vcom = 0x50;
static void lcm_resume(void)
{
	lcm_init();
}    


static unsigned int lcm_esd_check(void)
{
#ifndef BUILD_LK

	unsigned char buffer[4];
	unsigned int array[16];

#if defined(LCM_ESD_DEBUG)
	LCD_DEBUG("jd9161ba: lcm_esd_check enter\n");
#endif

	array[0]=0x00063902;
	array[1]=0x0698ffff;
	array[2]=0x00000004;
	dsi_set_cmdq(array, 3, 1);
	MDELAY(10);

	array[0] = 0x00013700;
	dsi_set_cmdq(array, 1, 1);
	read_reg_v2(0x0A, buffer, 1);

#if defined(LCM_ESD_DEBUG)
	LCD_DEBUG("jd9161ba: lcm_esd_check  0x0A = %x\n",buffer[0]);
#endif

	if(buffer[0] != 0x9C)
	{
		return 1;
	}
	else
	{
		return 0 ;
	}

#if 0
	array[0] = 0x00013700;
	dsi_set_cmdq(array,1,1);
	read_reg_v2(0x0A, &buffer[0], 1);

	array[0] = 0x00013700;
	dsi_set_cmdq(array,1,1);
	read_reg_v2(0x0C, &buffer[1], 1);

	array[0] = 0x00013700;
	dsi_set_cmdq(array,1,1);
	read_reg_v2(0x0D, &buffer[2], 1);

	array[0] = 0x00013700;
	dsi_set_cmdq(array,1,1);
	read_reg_v2(0x0E, &buffer[3], 1);

#if defined(LCM_ESD_DEBUG)
	LCD_DEBUG("jd9161ba: lcm_esd_check  0x09(bit0~3) = %x \n",buffer[0]);
#endif

	//if ((buffer[0]==0x80)&&(buffer[1]==0x73)&&(buffer[2]==0x04)&&(buffer[3]==0x00))
	if ((buffer[0]==0x0)&&(buffer[1]==0x70)&&(buffer[2]==0x0)&&(buffer[3]==0x00))
	{
#if defined(LCM_ESD_DEBUG)
		//LCD_DEBUG("jd9161ba: lcm_esd_check exit\n");
#endif
		return 0;
	}
	else
	{
		return 0 ;
	}
#endif	

#endif
}


static unsigned int lcm_esd_recover(void)
{
#ifndef BUILD_LK

#if defined(LCM_ESD_DEBUG)
	LCD_DEBUG("jd9161ba: lcm_esd_recover enter");
#endif

	lcm_init();
	return 1;

#endif 
}
     

static unsigned int lcm_compare_id(void)
{
	int id_type=0;	

	mt_set_gpio_mode(GPIO_LCM_ID1,GPIO_MODE_00);
	mt_set_gpio_dir(GPIO_LCM_ID1, GPIO_DIR_IN);
	//mt_set_gpio_pull_select(GPIO_LCM_ID1,GPIO_PULL_DOWN);
	mt_set_gpio_pull_enable(GPIO_LCM_ID1, GPIO_PULL_DISABLE);// def 0
	
	mt_set_gpio_mode(GPIO_LCM_ID2,GPIO_MODE_00);
	mt_set_gpio_dir(GPIO_LCM_ID2, GPIO_DIR_IN);
	//mt_set_gpio_pull_select(GPIO_LCM_ID2,GPIO_PULL_DOWN);
	mt_set_gpio_pull_enable(GPIO_LCM_ID2, GPIO_PULL_DISABLE);//def 0

	MDELAY(10);
	id_type = mt_get_gpio_in(GPIO_LCM_ID1)<<1 | mt_get_gpio_in(GPIO_LCM_ID2) ;
	LCD_DEBUG("[LLF] jd9161ba [lcm_compare_id   id_type= %d ]\n" , id_type);

	if (id_type == 3 ) //jd9161ba_boe  source ,and ID_tpye is 3. 
	{
		return 1 ;
	}
	else
	{
		return  0 ;
	}
	
}


LCM_DRIVER pixi5_jd9161ba_fwvga_dsi_boe_lcm_drv = 
{
    	.name		= "pixi5_jd9161ba_fwvga_dsi_boe",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params     = lcm_get_params,
	.init           = lcm_init,
	.suspend        = lcm_suspend,
	.resume         = lcm_resume,
	.esd_check   = lcm_esd_check,
	.esd_recover   = lcm_esd_recover,
	.compare_id    = lcm_compare_id,
};

