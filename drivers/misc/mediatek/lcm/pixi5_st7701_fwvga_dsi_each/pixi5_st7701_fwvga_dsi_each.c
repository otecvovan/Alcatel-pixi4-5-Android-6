
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

#define FRAME_WIDTH  							(480)
#define FRAME_HEIGHT 							(854) //xiaopu.zhu

#define REGFLAG_DELAY             						0xFD
#define REGFLAG_END_OF_TABLE      				0xFE   // END OF REGISTERS MARKER

#define LCM_DSI_CMD_MODE						0


#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#define LCM_EACH			0

bool lcm_st7701_each_vendor=LCM_EACH;	//default to choose byd panel


//set LCM IC ID
#define LCM_ID_st7701 	(0x980604)
#define GPIO_LCM_ID1	(GPIO81 | 0x80000000)
#define GPIO_LCM_ID2	(GPIO82 | 0x80000000)

//#define GPIO_LCM_ID1	GPIO81_LCD_ID1
//#define GPIO_LCM_ID2	GPIO82_LCD_ID2

//#define LCM_ESD_DEBUG


/*--------------------------LCD module explaination begin---------------------------------------*/

//LCD module explaination				//Project		Custom		W&H		Glass	degree	data		HWversion


/*--------------------------LCD module explaination end----------------------------------------*/


// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static LCM_UTIL_FUNCS lcm_util = {0};

#define SET_RESET_PIN(v)    								(lcm_util.set_reset_pin((v)))

#define UDELAY(n) 											(lcm_util.udelay(n))
#define MDELAY(n) 											(lcm_util.mdelay(n))

#ifdef BUILD_LK
#define LCD_DEBUG(fmt, args...) printf(fmt, ##args)		
#else
#define LCD_DEBUG(fmt, args...) printk(fmt, ##args)
#endif
// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------

#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)	        	lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)			lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)										lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)						lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg(cmd)											lcm_util.dsi_dcs_read_lcm_reg(cmd)
#define read_reg_v2(cmd, buffer, buffer_size)   					lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)

struct LCM_setting_table {
	unsigned char cmd;
	unsigned char count;
	unsigned char para_list[64];
};

static struct LCM_setting_table lcm_each_initialization_setting[] = {

	{0x11,1 ,{0x00}},
	{REGFLAG_DELAY, 200, {0x00}},
	//------------------------------------------Bank0 Setting----------------------------------------------------//
	//------------------------------------Display Control setting----------------------------------------------//
    	{0xFF,5 ,{0x77,0x01,0x00,0x00,0x10}},
	{0xC0,2 ,{0xE9,0x03}},
	{0xC1,2 ,{0x0A,0x02}},
	{0xC2,2 ,{0x37,0x08}},
	//-------------------------------------Gamma Cluster Setting-------------------------------------------//
	{0xB0,16,{0x00,0x0D,0x5A,0x13,0x18,0x0A,0x0C,0x09,0x08,0x20,0x05,0x13,0x12,0x15,0x1B,0x15}},
	{0xB1,16,{0x00,0x0E,0x1A,0x11,0x16,0x0A,0x0E,0x08,0x09,0x24,0x09,0x17,0x12,0x15,0x1B,0x15}},
	//---------------------------------------End Gamma Setting----------------------------------------------//
	//------------------------------------End Display Control setting----------------------------------------//
	//-----------------------------------------Bank0 Setting End---------------------------------------------//
	//-------------------------------------------Bank1 Setting---------------------------------------------------//
	//-------------------------------- Power Control Registers Initial --------------------------------------//
	{0xFF,5 ,{0x77,0x01,0x00,0x00,0x11}},
	{0xB0,1 ,{0x4D}},
	//-------------------------------------------Vcom Setting---------------------------------------------------//
	{0xB1,1 ,{0x45}},
	//-----------------------------------------End Vcom Setting-----------------------------------------------//
	{0xB2,1 ,{0x07}},
	{0xB3,1 ,{0x80}},
	{0xB5,1 ,{0x47}},
	{0xB7,1 ,{0x8A}},
	{0xB8,1 ,{0x10}},
	{0xC1,1 ,{0x78}},
	{0xC2,1 ,{0x78}},
	{0xD0,1 ,{0x88}},
	//---------------------------------End Power Control Registers Initial -------------------------------//
	//---------------------------------------------GIP Setting----------------------------------------------------//
	{0xE0,3 ,{0x00,0x00,0x02}},
	{0xE1,11,{0x02,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x40,0x40}},
	{0xE2,13,{0x30,0x30,0x40,0x40,0x60,0x00,0x00,0x00,0x5F,0x00,0x00,0x00,0x00}},
	{0xE3,4 ,{0x00,0x00,0x33,0x33}},
	{0xE4,2 ,{0x44,0x44}},
	{0xE5,16,{0x07,0x6B,0xA0,0xA0,0x09,0x6B,0xA0,0xA0,0x0B,0x6B,0xA0,0xA0,0x0D,0x6B,0xA0,0xA0}},
	{0xE6,4 ,{0x00,0x00,0x33,0x33}},
	{0xE7,2 ,{0x44,0x44}},
	{0xE8,16,{0x06,0x6B,0xA0,0xA0,0x08,0x6B,0xA0,0xA0,0x0A,0x6B,0xA0,0xA0,0x0C,0x6B,0xA0,0xA0}},
	{0xEB,7 ,{0x02,0x00,0x93,0x93,0x88,0x00,0x00}},
	{0xED,16,{0xFA,0xB0,0x2F,0xF4,0x65,0x7F,0xFF,0xFF,0xFF,0xFF,0xF7,0x56,0x4F,0xF2,0x0B,0xAF}},
	//---------------------------------------------End GIP Setting-----------------------------------------------//
	//------------------------------ Power Control Registers Initial End-----------------------------------//
	//------------------------------------------Bank1 Setting----------------------------------------------------//
	{0xFF,5,{0x77,0x01,0x00,0x00,0x00}},
	{REGFLAG_DELAY, 120, {0x00}},
	{0x29,0,{0x00}},
	{REGFLAG_DELAY, 1, {0x00}},
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
	//LCD_DEBUG("\t\t 9806e [lcm_set_util_funcs]\n");

	memcpy(&lcm_util, util, sizeof(LCM_UTIL_FUNCS));
}


static void lcm_get_params(LCM_PARAMS *params)
{
	//LCD_DEBUG("\t\t 9806e [lcm_get_params]\n");

	memset(params, 0, sizeof(LCM_PARAMS));

	params->type   = LCM_TYPE_DSI;

	params->width  = FRAME_WIDTH;
	params->height = FRAME_HEIGHT;

	// enable tearing-free
	//params->dbi.te_mode 			= LCM_DBI_TE_MODE_VSYNC_ONLY;
	//params->dbi.te_edge_polarity		= LCM_POLARITY_RISING;
	params->dbi.te_mode 			= LCM_DBI_TE_MODE_DISABLED;

#if (LCM_DSI_CMD_MODE)
	params->dsi.mode   = CMD_MODE;
#else
	params->dsi.mode   = BURST_VDO_MODE;//SYNC_PULSE_VDO_MODE;
#endif

	// DSI
	/* Command mode setting */
	params->dsi.LANE_NUM				= LCM_TWO_LANE;
	//The following defined the fomat for data coming from LCD engine.
	params->dsi.data_format.color_order	= LCM_COLOR_ORDER_RGB;
	params->dsi.data_format.trans_seq   		= LCM_DSI_TRANS_SEQ_MSB_FIRST;
	params->dsi.data_format.padding     		= LCM_DSI_PADDING_ON_LSB;
	params->dsi.data_format.format      		= LCM_DSI_FORMAT_RGB888;

	// Highly depends on LCD driver capability.
	params->dsi.packet_size=256;

	// Video mode setting	

	// add by zhuqiang for FR437058 at 2013.4.25 begin
	params->dsi.intermediat_buffer_num = 2;	
	// add by zhuqiang for FR437058 at 2013.4.25 end
	params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;

	params->dsi.word_count=480*3;

	//here is for esd protect by legen
	//params->dsi.noncont_clock = true;
	//params->dsi.noncont_clock_period=2;
	params->dsi.lcm_ext_te_enable=false;
	//for esd protest end by legen

	//delete by zhuqiang 2013.3.4 
	//params->dsi.word_count=FRAME_WIDTH*3;	
	// add by zhuqiang for FR437058 at 2013.4.25 begin
	params->dsi.vertical_sync_active=6;  //6
	params->dsi.vertical_backporch=14;	//14
	params->dsi.vertical_frontporch=20;	//20
	// add by zhuqiang for FR437058 at 2013.4.25 end
	params->dsi.vertical_active_line=FRAME_HEIGHT;

	//delete by zhuqiang 2013.3.4 
	//	params->dsi.line_byte=2180;		
	// add by zhuqiang for FR437058 at 2013.4.25 begin
	params->dsi.horizontal_sync_active=10;  //10

	//zrl modify for improve the TP reort point begin,130916
	params->dsi.horizontal_backporch=80;   //80  60 
	params->dsi.horizontal_frontporch=80;  //80   200
	//zrl modify for improve the TP reort point end,130916

	// add by zhuqiang for FR437058 at 2013.4.25 end
	params->dsi.horizontal_active_pixel = FRAME_WIDTH;	//added by zhuqiang 2013.3.4 

	params->dsi.cont_clock=1;

	params->dsi.esd_check_enable = 1;
	params->dsi.customization_esd_check_enable = 1;
	params->dsi.lcm_esd_check_table[0].cmd          = 0x0A;
	params->dsi.lcm_esd_check_table[0].count        = 1;
	params->dsi.lcm_esd_check_table[0].para_list[0] = 0x9C;

	params->dsi.lcm_esd_check_table[1].cmd          = 0x53;
	params->dsi.lcm_esd_check_table[1].count        = 1;
	params->dsi.lcm_esd_check_table[1].para_list[0] = 0x9C;  

	// add by zhuqiang for FR437058 at 2013.4.25 begin
	//params->dsi.pll_div1=1;         //  div1=0,1,2,3;  div1_real=1,2,4,4
	//params->dsi.pll_div2=1;         // div2=0,1,2,3;div2_real=1,2,4,4

	//zrl modify for improve the TP reort point begin,130916
	//params->dsi.fbk_div =30;              // fref=26MHz,  fvco=fref*(fbk_div+1)*2/(div1_real*div2_real)   //32
	//zrl modify for improve the TP reort point end,130916

	// add by zhuqiang for FR437058 at 2013.4.25 end
 	params->dsi.PLL_CLOCK=200;//150;//205;//208;////add by longfang.liu PR527476
	//params->dsi.ssc_disable=1;
        //params->dsi.ssc_range = 7;//add by llf for 431570
}

//legen add for detect lcm vendor
#if 0
static bool lcm_select_panel(void)
{
	int value=0;

	//LCD_DEBUG("\t\t 9806e [lcm_select_panel]\n");

	mt_set_gpio_mode(GPIO_LCM_ID1,GPIO_MODE_00);
	mt_set_gpio_dir(GPIO_LCM_ID1, GPIO_DIR_IN);
	mt_set_gpio_pull_enable(GPIO_LCM_ID1, GPIO_PULL_DISABLE);
	mt_set_gpio_mode(GPIO_LCM_ID2,GPIO_MODE_00);
	mt_set_gpio_dir(GPIO_LCM_ID2, GPIO_DIR_IN);
	mt_set_gpio_pull_enable(GPIO_LCM_ID2, GPIO_PULL_DISABLE);

	MDELAY(10);
	value = mt_get_gpio_in(GPIO_LCM_ID1)<<1 | mt_get_gpio_in(GPIO_LCM_ID2);
	if(value)
		return value;

	return LCM_EACH;
}
#endif
//legen add end 

//static int first_init=0;
static void lcm_init(void)
{

	SET_RESET_PIN(1);
	MDELAY(10);
	SET_RESET_PIN(0);
	MDELAY(10);
	SET_RESET_PIN(1);
	MDELAY(120);

	push_table(lcm_each_initialization_setting, sizeof(lcm_each_initialization_setting) / sizeof(struct LCM_setting_table), 1);

}
static struct LCM_setting_table lcm_each_sleep_in_setting[] = {
	{0xFF,5,{0x77,0x01,0x00,0x00,0x00}},
	{REGFLAG_DELAY, 50, {0x00}},
	{0x28,0,{0x00}},
	{REGFLAG_DELAY, 20, {0x00}},
	{0x10,0,{0x00}},
	{REGFLAG_DELAY, 120, {0x00}},
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};


static void lcm_suspend(void)
{
	SET_RESET_PIN(0);
	MDELAY(10);
	push_table(lcm_each_sleep_in_setting, sizeof(lcm_each_sleep_in_setting) / sizeof(struct LCM_setting_table), 1);
}

static void lcm_resume(void)
{
	lcm_init();  
	//MDELAY(200);
	//push_table(lcm_sleep_out_setting, sizeof(lcm_sleep_out_setting) / sizeof(struct LCM_setting_table), 1);
}


#if 0
static void lcm_update(unsigned int x, unsigned int y, unsigned int width, unsigned int height)
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

	//LCD_DEBUG("\t\t 9806e [lcm_update]\n");
	data_array[0]= 0x00053902;
	data_array[1]= (x1_MSB<<24)|(x0_LSB<<16)|(x0_MSB<<8)|0x2a;
	data_array[2]= (x1_LSB);
	data_array[3]= 0x00053902;
	data_array[4]= (y1_MSB<<24)|(y0_LSB<<16)|(y0_MSB<<8)|0x2b;
	data_array[5]= (y1_LSB);
	data_array[6]= 0x002c3909;

	dsi_set_cmdq(data_array, 7, 0);

}
#endif

static unsigned int lcm_esd_check(void)
{
#ifndef BUILD_LK

	unsigned char buffer[4];
	unsigned int array[16];

#if defined(LCM_ESD_DEBUG)
	LCD_DEBUG("st7701: lcm_esd_check enter\n");
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
	LCD_DEBUG("st7701: lcm_esd_check  0x0A = %x\n",buffer[0]);
#endif

	if(buffer[0] != 0x9C)
	{
		return 1;
	}
	else
	{
		return 0 ;
	}

#endif
}


static unsigned int lcm_esd_recover(void)
{
#ifndef BUILD_LK

#if defined(LCM_ESD_DEBUG)
	LCD_DEBUG("st7701: lcm_esd_recover enter");
#endif

	lcm_init();
	return 1;

#endif 
}


// ---------------------------------------------------------------------------
//  Get LCM ID Information
// ---------------------------------------------------------------------------

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
	LCD_DEBUG("[LLF] st7701 [lcm_compare_id   id_type=%d ]\n" , id_type);

	if (id_type == 2 ) //st7701_EACH  source ,and ID_tpye is 0. 
	{
		return 1 ;
	}
	else
	{
		return  0 ;
	}

	//return 3;
} 

// ---------------------------------------------------------------------------
//  Get LCM Driver Hooks
// ---------------------------------------------------------------------------
LCM_DRIVER pixi5_st7701_fwvga_dsi_each_lcm_drv =
{
	.name		= "pixi5_st7701_fwvga_dsi_each",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params     = lcm_get_params,
	.init           = lcm_init,
	.suspend        = lcm_suspend,
	.resume         = lcm_resume,
#if 0
	.update         = lcm_update,
#endif

	.esd_check   = lcm_esd_check,
	.esd_recover   = lcm_esd_recover,
	.compare_id    = lcm_compare_id,
};
