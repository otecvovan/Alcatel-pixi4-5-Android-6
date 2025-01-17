/* *****************************************************************************
 *
 * Filename:
 * ---------
 *	 s5k5e8yxmipi_Sensor.c
 *
 * Project:
 * --------
 *	 ALPS
 *
 * Description:
 * ------------
 *	 Source code of Sensor driver
 *
 *
 *------------------------------------------------------------------------------
 * Upper this line, this part is controlled by CC/CQ. DO NOT MODIFY!!
 *============================================================================
 ****************************************************************************/

#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <asm/atomic.h>
#include <linux/types.h>

#include "kd_camera_typedef.h"
#include "kd_camera_hw.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"

#include "s5k5e8yxffmipiraw_Sensor.h"
#define S5k5e8_OTP
/****************************Modify following Strings for debug****************************/
#define PFX "s5k5e8yxff_camera_sensor"
#define LOG_1 LOG_INF("s5k5e8yxff,MIPI 2LANE\n")
#define LOG_2 LOG_INF("preview 1280*960@30fps,864Mbps/lane; video 1280*960@30fps,864Mbps/lane; capture 5M@30fps,864Mbps/lane\n")
/****************************   Modify end    *******************************************/
#define LOG_INF(format, args...)	pr_debug(PFX "[%s] " format, __FUNCTION__, ##args)
static int first_flag = 1;
static DEFINE_SPINLOCK(imgsensor_drv_lock);


static imgsensor_info_struct imgsensor_info = { 
	.sensor_id = S5K5E8YX_SENSOR_ID,
	
	.checksum_value = 0x2ae69154,
	
	.pre = {
		.pclk = 168000000,				//record different mode's pclk
		.linelength = 5200,				//record different mode's linelength
		.framelength = 1062,			//record different mode's framelength
		.startx = 0,					//record different mode's startx of grabwindow
		.starty = 0,					//record different mode's starty of grabwindow
		.grabwindow_width = 1296,		//record different mode's width of grabwindow
		.grabwindow_height = 972,		//record different mode's height of grabwindow
		/*	 following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario	*/
		.mipi_data_lp2hs_settle_dc = 85,
		/*	 following for GetDefaultFramerateByScenario()	*/
		.max_framerate = 300,	
	},
	.cap = {
		.pclk = 168000000,
		.linelength = 2856,
		.framelength = 1968,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 2592,
		.grabwindow_height = 1944,
		.mipi_data_lp2hs_settle_dc = 85,
		.max_framerate = 300,
	},
	.cap1 = {
		.pclk = 84000000,
		.linelength = 2856,
		.framelength = 1967,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 2592,
		.grabwindow_height = 1944,
		.mipi_data_lp2hs_settle_dc = 85,
		.max_framerate = 150,	
	},
	.cap2 = {
		.pclk = 137000000,
		.linelength = 2856,
		.framelength = 1967,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 2592,
		.grabwindow_height = 1944,
		.mipi_data_lp2hs_settle_dc = 85,
		.max_framerate = 240,	
	},
	.normal_video = {
		.pclk = 168000000,
		.linelength = 2856,
		.framelength = 1968,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 2592,
		.grabwindow_height = 1944,
		.mipi_data_lp2hs_settle_dc = 85,
		.max_framerate = 300,
	},
	.hs_video = {
		.pclk = 168000000,
		.linelength = 2776,
		.framelength = 508,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 640,
		.grabwindow_height = 480,
		.mipi_data_lp2hs_settle_dc = 85,
		.max_framerate = 1200,
	},
	.slim_video = {
		.pclk = 168000000,
		.linelength = 5248,
		.framelength = 1066,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 1280,
		.grabwindow_height = 720,
		.mipi_data_lp2hs_settle_dc = 85,
		.max_framerate = 300,
	},
	.margin = 6,
	.min_shutter = 2,
	.max_frame_length = 0xffff,
	.ae_shut_delay_frame = 0,
	.ae_sensor_gain_delay_frame = 1,
	.ae_ispGain_delay_frame = 2,
	.ihdr_support = 0,	  //1, support; 0,not support
	.ihdr_le_firstline = 0,  //1,le first ; 0, se first
	.sensor_mode_num = 7,	  //support sensor mode num
	
	.cap_delay_frame = 3, 
	.pre_delay_frame = 3, 
	.video_delay_frame = 3,
	.hs_video_delay_frame = 3,
	.slim_video_delay_frame = 3,
	
	.isp_driving_current = ISP_DRIVING_6MA,
	.sensor_interface_type = SENSOR_INTERFACE_TYPE_MIPI,
	.mipi_sensor_type = MIPI_OPHY_NCSI2, //0,MIPI_OPHY_NCSI2;  1,MIPI_OPHY_CSI2
	.mipi_settle_delay_mode = MIPI_SETTLEDELAY_MANUAL,//0,MIPI_SETTLEDELAY_AUTO; 1,MIPI_SETTLEDELAY_MANNUAL
	.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_Gr,
	.mclk = 24,
	.mipi_lane_num = SENSOR_MIPI_2_LANE,
	.i2c_addr_table = {0x20, 0x5a,0xff},
};


static imgsensor_struct imgsensor = {
	.mirror = IMAGE_NORMAL,				//mirrorflip information 
	.sensor_mode = IMGSENSOR_MODE_INIT, //IMGSENSOR_MODE enum value,record current sensor mode,such as: INIT, Preview, Capture, Video,High Speed Video, Slim Video
	.shutter = 0x3D0,					//current shutter
	.gain = 0x100,						//current gain
	.dummy_pixel = 0,					//current dummypixel
	.dummy_line = 0,					//current dummyline
	.current_fps = 0,  //full size current fps : 24fps for PIP, 30fps for Normal or ZSD
	.autoflicker_en = KAL_FALSE,  //auto flicker enable: KAL_FALSE for disable auto flicker, KAL_TRUE for enable auto flicker
	.test_pattern = KAL_FALSE,		//test pattern mode or not. KAL_FALSE for in test pattern mode, KAL_TRUE for normal output
	.current_scenario_id = MSDK_SCENARIO_ID_CAMERA_PREVIEW,//current scenario id
	.ihdr_en = 0, //sensor need support LE, SE with HDR feature
	.i2c_write_id = 0x20,
};


/* Sensor output window information */
static SENSOR_WINSIZE_INFO_STRUCT imgsensor_winsize_info[5] =	 
{{ 2592, 1944,	  0,	0, 2592, 1944, 1296,  972, 0000, 0000, 1296,  972,	  0,	0, 1296,  972}, // Preview 
 { 2592, 1944,	  0,	0, 2592, 1944, 2592, 1944, 0000, 0000, 2592, 1944,	  0,	0, 2592, 1944}, // capture 
 { 2592, 1944,	  0,    0, 2592, 1944, 2592, 1944, 0000, 0000, 2592, 1944,	  0,	0, 2592, 1944}, // video 
 { 2592, 1944,	  24, 260, 2560, 1440,  640,  480, 0000, 0000,  640,  480,	  0,	0,  640,  480}, //hight speed video 
 { 2592, 1944,	  24,  20, 2560, 1920, 1280,  720, 0000, 0000, 1280,  720,	  0,	0, 1280,  720}};// slim video 


static kal_uint16 read_cmos_sensor(kal_uint32 addr)
{
	kal_uint16 get_byte=0;

	char pu_send_cmd[2] = {(char)(addr >> 8), (char)(addr & 0xFF) };
	iReadRegI2C(pu_send_cmd, 2, (u8*)&get_byte, 1, imgsensor.i2c_write_id);

	return get_byte;
}

static void write_cmos_sensor(kal_uint32 addr, kal_uint32 para)
{
	char pu_send_cmd[3] = {(char)(addr >> 8), (char)(addr & 0xFF), (char)(para & 0xFF)};
	iWriteRegI2C(pu_send_cmd, 3, imgsensor.i2c_write_id);
}

static void write_cmos_sensor_8(kal_uint16 addr, kal_uint8 para)
{
    char pusendcmd[4] = {(char)(addr >> 8) , (char)(addr & 0xFF) ,(char)(para & 0xFF)};
    iWriteRegI2C(pusendcmd , 3, imgsensor.i2c_write_id);
}


static void set_dummy(void)
{
	LOG_INF("dummyline = %d, dummypixels = %d \n", imgsensor.dummy_line, imgsensor.dummy_pixel);

	write_cmos_sensor(0x0340, imgsensor.frame_length >> 8);
	write_cmos_sensor(0x0341, imgsensor.frame_length & 0xFF);	  
	write_cmos_sensor(0x0342, imgsensor.line_length >> 8);
	write_cmos_sensor(0x0343, imgsensor.line_length & 0xFF);
  
}	/*	set_dummy  */

static kal_uint32 return_sensor_id(void)
{
	return ((read_cmos_sensor(0x0000) << 8) | read_cmos_sensor(0x0001));
	
}


static void set_max_framerate(UINT16 framerate,kal_bool min_framelength_en)
{
	//kal_int16 dummy_line;
	kal_uint32 frame_length = imgsensor.frame_length;
	//unsigned long flags;

	//LOG_INF("framerate = %d, min framelength should enable? \n", framerate,min_framelength_en);
   
	frame_length = imgsensor.pclk / framerate * 10 / imgsensor.line_length;
	spin_lock(&imgsensor_drv_lock);
	imgsensor.frame_length = (frame_length > imgsensor.min_frame_length) ? frame_length : imgsensor.min_frame_length; 
	imgsensor.dummy_line = imgsensor.frame_length - imgsensor.min_frame_length;
	//dummy_line = frame_length - imgsensor.min_frame_length;
	//if (dummy_line < 0)
		//imgsensor.dummy_line = 0;
	//else
		//imgsensor.dummy_line = dummy_line;
	//imgsensor.frame_length = frame_length + imgsensor.dummy_line;
	if (imgsensor.frame_length > imgsensor_info.max_frame_length)
	{
		imgsensor.frame_length = imgsensor_info.max_frame_length;
		imgsensor.dummy_line = imgsensor.frame_length - imgsensor.min_frame_length;
	}
	if (min_framelength_en)
		imgsensor.min_frame_length = imgsensor.frame_length;
	spin_unlock(&imgsensor_drv_lock);
	set_dummy();
}	/*	set_max_framerate  */


static void write_shutter(kal_uint16 shutter)
{
	kal_uint16 realtime_fps = 0;
	//kal_uint32 frame_length = 0;	   
	
	
	// if shutter bigger than frame_length, should extend frame length first
	spin_lock(&imgsensor_drv_lock);
	if (shutter > imgsensor.min_frame_length - imgsensor_info.margin)		
		imgsensor.frame_length = shutter + imgsensor_info.margin;
	else
		imgsensor.frame_length = imgsensor.min_frame_length;
	if (imgsensor.frame_length > imgsensor_info.max_frame_length)
		imgsensor.frame_length = imgsensor_info.max_frame_length;
	spin_unlock(&imgsensor_drv_lock);
	shutter = (shutter < imgsensor_info.min_shutter) ? imgsensor_info.min_shutter : shutter;
	shutter = (shutter > (imgsensor_info.max_frame_length - imgsensor_info.margin)) ? (imgsensor_info.max_frame_length - imgsensor_info.margin) : shutter;
	
	if (imgsensor.autoflicker_en) { 
		realtime_fps = imgsensor.pclk / imgsensor.line_length * 10 / imgsensor.frame_length;
		if(realtime_fps >= 297 && realtime_fps <= 305)
			set_max_framerate(296,0);
		else if(realtime_fps >= 147 && realtime_fps <= 150)
			set_max_framerate(146,0);	
		else {
		// Extend frame length
		write_cmos_sensor(0x0340, imgsensor.frame_length >> 8);
		write_cmos_sensor(0x0341, imgsensor.frame_length & 0xFF);
		}
	} else {
		// Extend frame length
		write_cmos_sensor(0x0340, imgsensor.frame_length >> 8);
		write_cmos_sensor(0x0341, imgsensor.frame_length & 0xFF);
	}

	// Update Shutter
	//write_cmos_sensor(0x0104, 0x01);   //group hold
	write_cmos_sensor(0x0202, shutter >> 8);
	write_cmos_sensor(0x0203, shutter & 0xFF);	
	//write_cmos_sensor(0x0104, 0x00);   //group hold
	
	LOG_INF("shutter =%d, framelength =%d\n", shutter,imgsensor.frame_length);

	//LOG_INF("frame_length = %d ", frame_length);
	
}	/*	write_shutter  */



/*************************************************************************
* FUNCTION
*	set_shutter
*
* DESCRIPTION
*	This function set e-shutter of sensor to change exposure time.
*
* PARAMETERS
*	iShutter : exposured lines
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/
static void set_shutter(kal_uint16 shutter)
{
	unsigned long flags;
	spin_lock_irqsave(&imgsensor_drv_lock, flags);
	imgsensor.shutter = shutter;
	spin_unlock_irqrestore(&imgsensor_drv_lock, flags);
	
	write_shutter(shutter);
}	/*	set_shutter */


/*
static kal_uint16 gain2reg(const kal_uint16 gain)
{
	return gain>>1;
}
*/
/*************************************************************************
* FUNCTION
*	set_gain
*
* DESCRIPTION
*	This function is to set global gain to sensor.
*
* PARAMETERS
*	iGain : sensor global gain(base: 0x40)
*
* RETURNS
*	the actually gain set to sensor.
*
* GLOBALS AFFECTED
*
*************************************************************************/
static kal_uint16 set_gain(kal_uint16 gain)
{
	kal_uint16 reg_gain;

	/* 0x350A[0:1], 0x350B[0:7] AGC real gain */
	/* [0:3] = N meams N /16 X	*/
	/* [4:9] = M meams M X		 */
	/* Total gain = M + N /16 X   */

	//
	
 
	reg_gain = gain>>1;
	spin_lock(&imgsensor_drv_lock);
	imgsensor.gain = reg_gain; 
	spin_unlock(&imgsensor_drv_lock);
	LOG_INF("gain = %d , reg_gain = 0x%x\n ", gain, reg_gain);

	//write_cmos_sensor(0x0104, 0x01);   //group hold
	write_cmos_sensor(0x0204, reg_gain >> 8);
	write_cmos_sensor(0x0205, reg_gain & 0xFF);  
	//write_cmos_sensor(0x0104, 0x00);
	
	return gain;
}	/*	set_gain  */



//defined but not used
static void ihdr_write_shutter_gain(kal_uint16 le, kal_uint16 se, kal_uint16 gain)
{
	LOG_INF("le:0x%x, se:0x%x, gain:0x%x\n",le,se,gain);
	if (imgsensor.ihdr_en) {
		
		spin_lock(&imgsensor_drv_lock);
			if (le > imgsensor.min_frame_length - imgsensor_info.margin)		
				imgsensor.frame_length = le + imgsensor_info.margin;
			else
				imgsensor.frame_length = imgsensor.min_frame_length;
			if (imgsensor.frame_length > imgsensor_info.max_frame_length)
				imgsensor.frame_length = imgsensor_info.max_frame_length;
			spin_unlock(&imgsensor_drv_lock);
			if (le < imgsensor_info.min_shutter) le = imgsensor_info.min_shutter;
			if (se < imgsensor_info.min_shutter) se = imgsensor_info.min_shutter;
			
			
				// Extend frame length first
				write_cmos_sensor(0x380e, imgsensor.frame_length >> 8);
				write_cmos_sensor(0x380f, imgsensor.frame_length & 0xFF);

		write_cmos_sensor(0x3502, (le << 4) & 0xFF);
		write_cmos_sensor(0x3501, (le >> 4) & 0xFF);	 
		write_cmos_sensor(0x3500, (le >> 12) & 0x0F);
		
		write_cmos_sensor(0x3508, (se << 4) & 0xFF); 
		write_cmos_sensor(0x3507, (se >> 4) & 0xFF);
		write_cmos_sensor(0x3506, (se >> 12) & 0x0F); 

		set_gain(gain);
	}

}



static void set_mirror_flip(kal_uint8 image_mirror)
{
	LOG_INF("image_mirror = %d\n", image_mirror);

	/********************************************************
	   *
	   *   0x3820[2] ISP Vertical flip
	   *   0x3820[1] Sensor Vertical flip
	   *
	   *   0x3821[2] ISP Horizontal mirror
	   *   0x3821[1] Sensor Horizontal mirror
	   *
	   *   ISP and Sensor flip or mirror register bit should be the same!!
	   *
	   ********************************************************/
	
	switch (image_mirror) {
		case IMAGE_NORMAL:
			write_cmos_sensor(0x0101,0x00);  
			break;
		case IMAGE_H_MIRROR:
			write_cmos_sensor(0x0101,0x01);
			break;
		case IMAGE_V_MIRROR:
			write_cmos_sensor(0x0101,0x02);		
			break;
		case IMAGE_HV_MIRROR:
			write_cmos_sensor(0x0101,0x03);
			break;
		default:
			LOG_INF("Error image_mirror setting\n");
	}

}

/*************************************************************************
* FUNCTION
*	night_mode
*
* DESCRIPTION
*	This function night mode of sensor.
*
* PARAMETERS
*	bEnable: KAL_TRUE -> enable night mode, otherwise, disable night mode
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/
/*static void night_mode(kal_bool enable)
{*/
/*No Need to implement this function*/ 
/*}	*//*	night_mode	*/

static void sensor_init(void)
{
	LOG_INF("E\n");

   // +++++++++++++++++++++++++++//                                                               
	// Reset for operation     
	//Streaming off
	write_cmos_sensor(0x0100,0x00);
   
	write_cmos_sensor(0x3906,0x7E);
	write_cmos_sensor(0x3C01,0x0F);
	write_cmos_sensor(0x3C14,0x00);
	write_cmos_sensor(0x3235,0x08);
	write_cmos_sensor(0x3063,0x2E);
	write_cmos_sensor(0x307A,0x10);
	write_cmos_sensor(0x307B,0x0E);//20150713
	write_cmos_sensor(0x3079,0x20);
	write_cmos_sensor(0x3070,0x05);
	write_cmos_sensor(0x3067,0x06);
	write_cmos_sensor(0x3071,0x62);
	write_cmos_sensor(0x3203,0x43);
	write_cmos_sensor(0x3205,0x43);
	write_cmos_sensor(0x320b,0x42);//20150713
	write_cmos_sensor(0x3007,0x00);
	write_cmos_sensor(0x3008,0x14);
	write_cmos_sensor(0x3020,0x58);
	write_cmos_sensor(0x300D,0x34);
	write_cmos_sensor(0x300E,0x17);
	write_cmos_sensor(0x3021,0x02);
	write_cmos_sensor(0x3010,0x59);
	write_cmos_sensor(0x3002,0x01);
	write_cmos_sensor(0x3005,0x01);
	write_cmos_sensor(0x3008,0x04);
	write_cmos_sensor(0x300F,0x70);
	write_cmos_sensor(0x3010,0x69);
	write_cmos_sensor(0x3017,0x10);
	write_cmos_sensor(0x3019,0x19);
	write_cmos_sensor(0x300C,0x62);
	write_cmos_sensor(0x3064,0x10);
	write_cmos_sensor(0x3C08,0x0E);
	write_cmos_sensor(0x3C09,0x10);
	write_cmos_sensor(0x3C31,0x0D);
	write_cmos_sensor(0x3C32,0xAC);

	//sensor otp bpc
	write_cmos_sensor(0x3303,0x02);

		 
}	/*	sensor_init  */


static void preview_setting(void)
{
	// +++++++++++++++++++++++++++//                                                               
	// Reset for operation                                                                         
	
	//$MV1[MCLK:24,Width:1296,Height:792,Format:MIPI_RAW10,mipi_lane:2,mipi_datarate:836,pvi_pclk_inverwrite_cmos_sensor(0xe:0]);
	//Streaming off
	write_cmos_sensor(0x0100,0x00);
	//Delay 1 frame
	mDELAY(33);    
	//Extclk_frequency_mhz
	write_cmos_sensor(0x0136,0x18);
	write_cmos_sensor(0x0137,0x00);
	
	write_cmos_sensor(0x0305,0x06); //pre_pll_clk_div [5:0]
	write_cmos_sensor(0x0306,0x18); //pre_pll2_clk_div [15:8]
	write_cmos_sensor(0x0307,0xA8); //pll_multiplier
	write_cmos_sensor(0x0308,0x34); //pll2_multiplier
	write_cmos_sensor(0x0309,0x42); //op_pix_clk_div
	write_cmos_sensor(0x3C1F,0x00); //reg_pll_s
	write_cmos_sensor(0x3C17,0x00); //reg_pll_s2
	write_cmos_sensor(0x3C0B,0x04); //reg_sys_clk_byp
	write_cmos_sensor(0x3C1C,0x47); //reg_div_sys [7:4], reg_div_dbr[3:0] (only can change div_sys)
	write_cmos_sensor(0x3C1D,0x15); //reg_div_gclk[7:4], reg_pll_i[3:0]
	write_cmos_sensor(0x3C14,0x04); //reg_outclk_pll_clk_sel[2], outclk_pll_extclk_sel[1], main_pll_extclk_sel[0]
	write_cmos_sensor(0x3C16,0x00); //reg_pll2_pd[0] (0:pll2 on, 1:pll2 off)
	//requested_link_bit_rate_mbps  836Mbps/lane  ox0344
	write_cmos_sensor(0x0820,0x03);
	write_cmos_sensor(0x0821,0x44);
	
	write_cmos_sensor(0x0114,0x01); //CSI_lane_mode 2lane
	//x_addr_start
	write_cmos_sensor(0x0344,0x00);
	write_cmos_sensor(0x0345,0x08);
	//y_addr_start
	write_cmos_sensor(0x0346,0x00);
	write_cmos_sensor(0x0347,0x08);
	//x_addr_end
	write_cmos_sensor(0x0348,0x0A);
	write_cmos_sensor(0x0349,0x27);
	//y_addr_end
	write_cmos_sensor(0x034A,0x07);
	write_cmos_sensor(0x034B,0x9F);
	//x_output_size
	write_cmos_sensor(0x034C,0x05);
	write_cmos_sensor(0x034D,0x10);
	//y_output_size
	write_cmos_sensor(0x034E,0x03);
	write_cmos_sensor(0x034F,0xCC);
	
	write_cmos_sensor(0x0900,0x01); //binning_mode (0: disable, 1: enable)
	write_cmos_sensor(0x0901,0x22); //binning_type(Column, Row)
	write_cmos_sensor(0x0381,0x01); //x_even_inc
	write_cmos_sensor(0x0383,0x01); //x_odd_inc
	write_cmos_sensor(0x0385,0x01); //y_even_inc
	write_cmos_sensor(0x0387,0x03); //y_odd_inc
	//frame_length_lines
	write_cmos_sensor(0x0340,0x04);
	write_cmos_sensor(0x0341,0x26);
	//line_length_pck
	write_cmos_sensor(0x0342,0x14);
	write_cmos_sensor(0x0343,0x50);
	//fine_integration_time
	write_cmos_sensor(0x0200,0x00);
	write_cmos_sensor(0x0201,0x00);
	//coarse_integration_time
	write_cmos_sensor(0x0202,0x03);
	write_cmos_sensor(0x0203,0xDE);
	
	write_cmos_sensor(0x3303,0x02); //bpmarker_allcure_en[1], bpmarker_flat2bit_bypass[0]	
	write_cmos_sensor(0x3400,0x00); //shade_bypass LSC off
	//Streaming on
	write_cmos_sensor(0x3C0D,0x04);
	write_cmos_sensor(0x0100,0x01);
	write_cmos_sensor(0x3C22,0x00);
	write_cmos_sensor(0x3C22,0x00); //interval
	write_cmos_sensor(0x3C0D,0x00); //pll select
	
}	/*	preview_setting  */

static void capture_setting(kal_uint16 currefps)
{
	LOG_INF("E! currefps:%d\n",currefps);
	if (currefps == 150){
		 
		write_cmos_sensor(0x0100,0X00);
         //Delay 1 frame	  
         mDELAY(67); 
		write_cmos_sensor(0x0136,0X18);
		write_cmos_sensor(0x0137,0X00);
		write_cmos_sensor(0x0305,0X06);
		write_cmos_sensor(0x0306,0X18);
		write_cmos_sensor(0x0307,0XA8); //20150713
		write_cmos_sensor(0x0308,0X19); //20150713
		write_cmos_sensor(0x0309,0X02); //20150713
		write_cmos_sensor(0x3C1F,0X00);
		write_cmos_sensor(0x3C17,0X00);
		write_cmos_sensor(0x3C0B,0X04);
		write_cmos_sensor(0x3C1C,0X87); //20150713
		write_cmos_sensor(0x3C1D,0X25); //20150713
		write_cmos_sensor(0x3C14,0X04);
		write_cmos_sensor(0x3C16,0X00);
		write_cmos_sensor(0x0820,0X01); //20150713
		write_cmos_sensor(0x0821,0X90); //20150713
		write_cmos_sensor(0x0114,0X01);
		write_cmos_sensor(0x0344,0X00);
		write_cmos_sensor(0x0345,0X08);
		write_cmos_sensor(0x0346,0X00);
		write_cmos_sensor(0x0347,0X08);
		write_cmos_sensor(0x0348,0X0A);
		write_cmos_sensor(0x0349,0X27);
		write_cmos_sensor(0x034A,0X07);
		write_cmos_sensor(0x034B,0X9F);
		write_cmos_sensor(0x034C,0X0A);
		write_cmos_sensor(0x034D,0X20);
		write_cmos_sensor(0x034E,0X07);
		write_cmos_sensor(0x034F,0X98);
		write_cmos_sensor(0x0900,0X00);
		write_cmos_sensor(0x0901,0X00);
		write_cmos_sensor(0x0381,0X01);
		write_cmos_sensor(0x0383,0X01);
		write_cmos_sensor(0x0385,0X01);
		write_cmos_sensor(0x0387,0X01);
		write_cmos_sensor(0x0340,0X07); //20150713
		write_cmos_sensor(0x0341,0XAF); //20150713
		write_cmos_sensor(0x0342,0X0B);
		write_cmos_sensor(0x0343,0X28);
		write_cmos_sensor(0x0200,0X00);
		write_cmos_sensor(0x0201,0X00);
		write_cmos_sensor(0x0202,0X03);
		write_cmos_sensor(0x0203,0XDE);
		write_cmos_sensor(0x3303,0X02);
		write_cmos_sensor(0x3400,0X00);
		write_cmos_sensor(0x3C0D,0X04);
		write_cmos_sensor(0x0100,0X01);
		write_cmos_sensor(0x3C22,0X00);
		write_cmos_sensor(0x3C22,0X00);
		write_cmos_sensor(0x3C0D,0X00);
		}
	else if (currefps == 240) { //24fps for PIP
	// +++++++++++++++++++++++++++//                                                               
	 
	//$MV1[MCLK:24,Width:2592,Height:1944,Format:MIPI_RAW10,mipi_lane:2,mipi_datarate:836,pvi_pclk_inverwrite_cmos_sensor 
	  write_cmos_sensor(0x0100,0x00);
	//Delay 1 frame 
	  mDELAY(42); 
		write_cmos_sensor(0x0136,0X18);
		write_cmos_sensor(0x0137,0X00);
		write_cmos_sensor(0x0305,0X06);
		write_cmos_sensor(0x0306,0X18);
		write_cmos_sensor(0x0307,0X89);  //20150713
		write_cmos_sensor(0x0308,0X32);  //20150713
		write_cmos_sensor(0x0309,0X02);  //20150713
		write_cmos_sensor(0x3C1F,0X00);
		write_cmos_sensor(0x3C17,0X00);
		write_cmos_sensor(0x3C0B,0X04);
		write_cmos_sensor(0x3C1C,0X47);
		write_cmos_sensor(0x3C1D,0X15);
		write_cmos_sensor(0x3C14,0X04);
		write_cmos_sensor(0x3C16,0X00);
		write_cmos_sensor(0x0820,0X03);
		write_cmos_sensor(0x0821,0X44);
		write_cmos_sensor(0x0114,0X01);
		write_cmos_sensor(0x0344,0X00);
		write_cmos_sensor(0x0345,0X08);
		write_cmos_sensor(0x0346,0X00);
		write_cmos_sensor(0x0347,0X08);
		write_cmos_sensor(0x0348,0X0A);
		write_cmos_sensor(0x0349,0X27);
		write_cmos_sensor(0x034A,0X07);
		write_cmos_sensor(0x034B,0X9F);
		write_cmos_sensor(0x034C,0X0A);
		write_cmos_sensor(0x034D,0X20);
		write_cmos_sensor(0x034E,0X07);
		write_cmos_sensor(0x034F,0X98);
		write_cmos_sensor(0x0900,0X00);
		write_cmos_sensor(0x0901,0X00);
		write_cmos_sensor(0x0381,0X01);
		write_cmos_sensor(0x0383,0X01);
		write_cmos_sensor(0x0385,0X01);
		write_cmos_sensor(0x0387,0X01);
		write_cmos_sensor(0x0340,0X07);         //20150713
		write_cmos_sensor(0x0341,0XAF);         //20150713
		write_cmos_sensor(0x0342,0X0B);
		write_cmos_sensor(0x0343,0X28);
		write_cmos_sensor(0x0200,0X00);
		write_cmos_sensor(0x0201,0X00);
		write_cmos_sensor(0x0202,0X03);
		write_cmos_sensor(0x0203,0XDE);
		write_cmos_sensor(0x3303,0X02);
		write_cmos_sensor(0x3400,0X00);
		
		write_cmos_sensor(0x3C0D,0X04);
		write_cmos_sensor(0x0100,0X01);
		write_cmos_sensor(0x3C22,0X00);
		write_cmos_sensor(0x3C22,0X00);
		write_cmos_sensor(0x3C0D,0X00);
	
	 
	}
	else{
		// Reset for operation     30fps for normal capture    
	// streaming OFF      
	write_cmos_sensor(0x0100,0x00);
	//Delay 1 frame  
	mDELAY(33);
	write_cmos_sensor(0x0136,0x18);
	write_cmos_sensor(0x0137,0x00);
	write_cmos_sensor(0x0305,0x06);
	write_cmos_sensor(0x0306,0x18);
	write_cmos_sensor(0x0307,0xA8);
	write_cmos_sensor(0x0308,0x34);
	write_cmos_sensor(0x0309,0x42);
	write_cmos_sensor(0x3C1F,0x00);
	write_cmos_sensor(0x3C17,0x00);
	write_cmos_sensor(0x3C0B,0x04);
	write_cmos_sensor(0x3C1C,0x47);
	write_cmos_sensor(0x3C1D,0x15);
	write_cmos_sensor(0x3C14,0x04);
	write_cmos_sensor(0x3C16,0x00);
	write_cmos_sensor(0x0820,0x03);
	write_cmos_sensor(0x0821,0x44);
	write_cmos_sensor(0x0114,0x01);
	write_cmos_sensor(0x0344,0x00);
	write_cmos_sensor(0x0345,0x08);
	write_cmos_sensor(0x0346,0x00);
	write_cmos_sensor(0x0347,0x08);
	write_cmos_sensor(0x0348,0x0A);
	write_cmos_sensor(0x0349,0x27);
	write_cmos_sensor(0x034A,0x07);
	write_cmos_sensor(0x034B,0x9F);
	write_cmos_sensor(0x034C,0x0A);
	write_cmos_sensor(0x034D,0x20);
	write_cmos_sensor(0x034E,0x07);
	write_cmos_sensor(0x034F,0x98);
	write_cmos_sensor(0x0900,0x00);
	write_cmos_sensor(0x0901,0x00);
	write_cmos_sensor(0x0381,0x01);
	write_cmos_sensor(0x0383,0x01);
	write_cmos_sensor(0x0385,0x01);
	write_cmos_sensor(0x0387,0x01);
	write_cmos_sensor(0x0340,0x07);
	write_cmos_sensor(0x0341,0xB0);
	write_cmos_sensor(0x0342,0x0B);
	write_cmos_sensor(0x0343,0x28);
	write_cmos_sensor(0x0200,0x00);
	write_cmos_sensor(0x0201,0x00);
	write_cmos_sensor(0x0202,0x03);
	write_cmos_sensor(0x0203,0xDE);
	write_cmos_sensor(0x3303,0x02);
	write_cmos_sensor(0x3400,0x00);
		// streaming ON
	write_cmos_sensor(0x3C0D,0x04);
	write_cmos_sensor(0x0100,0x01);
	write_cmos_sensor(0x3C22,0x00);
	write_cmos_sensor(0x3C22,0x00);
	write_cmos_sensor(0x3C0D,0x00);
 
	}
}

static void normal_video_setting(kal_uint16 currefps)
{
	LOG_INF("E! currefps:%d\n",currefps);
	// +++++++++++++++++++++++++++//                                                               
	// Reset for operation                                                                         
		// streaming OFF
	write_cmos_sensor(0x0100,0x00);
	//Delay 1 frame  
	mDELAY(33);
	write_cmos_sensor(0x0136,0x18);
	write_cmos_sensor(0x0137,0x00);
	write_cmos_sensor(0x0305,0x06);
	write_cmos_sensor(0x0306,0x18);
	write_cmos_sensor(0x0307,0xA8);
	write_cmos_sensor(0x0308,0x34);
	write_cmos_sensor(0x0309,0x42);
	write_cmos_sensor(0x3C1F,0x00);
	write_cmos_sensor(0x3C17,0x00);
	write_cmos_sensor(0x3C0B,0x04);
	write_cmos_sensor(0x3C1C,0x47);
	write_cmos_sensor(0x3C1D,0x15);
	write_cmos_sensor(0x3C14,0x04);
	write_cmos_sensor(0x3C16,0x00);
	write_cmos_sensor(0x0820,0x03);
	write_cmos_sensor(0x0821,0x44);
	write_cmos_sensor(0x0114,0x01);
	write_cmos_sensor(0x0344,0x00);
	write_cmos_sensor(0x0345,0x08);
	write_cmos_sensor(0x0346,0x00);
	write_cmos_sensor(0x0347,0x08);
	write_cmos_sensor(0x0348,0x0A);
	write_cmos_sensor(0x0349,0x27);
	write_cmos_sensor(0x034A,0x07);
	write_cmos_sensor(0x034B,0x9F);
	write_cmos_sensor(0x034C,0x0A);
	write_cmos_sensor(0x034D,0x20);
	write_cmos_sensor(0x034E,0x07);
	write_cmos_sensor(0x034F,0x98);
	write_cmos_sensor(0x0900,0x00);
	write_cmos_sensor(0x0901,0x00);
	write_cmos_sensor(0x0381,0x01);
	write_cmos_sensor(0x0383,0x01);
	write_cmos_sensor(0x0385,0x01);
	write_cmos_sensor(0x0387,0x01);
	write_cmos_sensor(0x0340,0x07);
	write_cmos_sensor(0x0341,0xB0);
	write_cmos_sensor(0x0342,0x0B);
	write_cmos_sensor(0x0343,0x28);
	write_cmos_sensor(0x0200,0x00);
	write_cmos_sensor(0x0201,0x00);
	write_cmos_sensor(0x0202,0x03);
	write_cmos_sensor(0x0203,0xDE);
	write_cmos_sensor(0x3303,0x02);
	write_cmos_sensor(0x3400,0x00);
		// streaming ON
	write_cmos_sensor(0x3C0D,0x04);
	write_cmos_sensor(0x0100,0x01);
	write_cmos_sensor(0x3C22,0x00);
	write_cmos_sensor(0x3C22,0x00);
	write_cmos_sensor(0x3C0D,0x00);
	
		
}
static void hs_video_setting(void)
{
	LOG_INF("E! VGA 120fps\n");    
  	//VGA 120fps  	
	//$MV1[MCLK:24,Width:640,Height:480,Format:MIPI_RAW10,mipi_lane:2,mipi_datarate:836,pvi_pclk_inverwrite_cmos_sensor(0xe:0] 
	// streaming OFF
	write_cmos_sensor(0x0100,0x00);
	//Delay 1 frame  
	mDELAY(83);
	write_cmos_sensor(0x0136,0x18);
	write_cmos_sensor(0x0137,0x00);
	write_cmos_sensor(0x0305,0x06);
	write_cmos_sensor(0x0306,0x18);
	write_cmos_sensor(0x0307,0xA8);
	write_cmos_sensor(0x0308,0x34);
	write_cmos_sensor(0x0309,0x42);
	write_cmos_sensor(0x3C1F,0x00);
	write_cmos_sensor(0x3C17,0x00);
	write_cmos_sensor(0x3C0B,0x04);
	write_cmos_sensor(0x3C1C,0x47);
	write_cmos_sensor(0x3C1D,0x15);
	write_cmos_sensor(0x3C14,0x04);
	write_cmos_sensor(0x3C16,0x00);
	write_cmos_sensor(0x0820,0x03);
	write_cmos_sensor(0x0821,0x44);
	write_cmos_sensor(0x0114,0x01);
	write_cmos_sensor(0x0344,0x00);
	write_cmos_sensor(0x0345,0x18);
	write_cmos_sensor(0x0346,0x00);
	write_cmos_sensor(0x0347,0x14);
	write_cmos_sensor(0x0348,0x0A);
	write_cmos_sensor(0x0349,0x17);
	write_cmos_sensor(0x034A,0x07);
	write_cmos_sensor(0x034B,0x93);
	write_cmos_sensor(0x034C,0x02);
	write_cmos_sensor(0x034D,0x80);
	write_cmos_sensor(0x034E,0x01);
	write_cmos_sensor(0x034F,0xE0);
	
	write_cmos_sensor(0x0900,0x01);//2015.07.30
	write_cmos_sensor(0x0901,0x44);
	write_cmos_sensor(0x0381,0x00);
	write_cmos_sensor(0x0383,0x03);
	
	write_cmos_sensor(0x0385,0x01);
	write_cmos_sensor(0x0387,0x07);
	write_cmos_sensor(0x0340,0x01);
	write_cmos_sensor(0x0341,0xFC);
	write_cmos_sensor(0x0342,0x0A);
	write_cmos_sensor(0x0343,0xD8);
	write_cmos_sensor(0x0200,0x00);
	write_cmos_sensor(0x0201,0x00);
	write_cmos_sensor(0x0202,0x01);
	write_cmos_sensor(0x0203,0xEC);
	write_cmos_sensor(0x3303,0x02);
	write_cmos_sensor(0x3400,0x00);
	// streaming ON
	write_cmos_sensor(0x3C0D,0x04);
	write_cmos_sensor(0x0100,0x01);
	write_cmos_sensor(0x3C22,0x00);
	write_cmos_sensor(0x3C22,0x00);
	write_cmos_sensor(0x3C0D,0x00);
}

static void slim_video_setting(void)
{
	LOG_INF("E! HD 30fps\n");
	// +++++++++++++++++++++++++++//                                                               
// Reset for operation                                                                         
	
	//$MV1[MCLK:24,Width:1280,Height:720,Format:MIPI_RAW10,mipi_lane:2,mipi_datarate:836,pvi_pclk_inverwrite_cmos_sensor(0xe:0]);
	// streaming OFF
	write_cmos_sensor(0x0100,0x00);
	//Delay 1 frame	 
	mDELAY(33);
	write_cmos_sensor(0x0136,0x18);
	write_cmos_sensor(0x0137,0x00);
	write_cmos_sensor(0x0305,0x06);
	write_cmos_sensor(0x0306,0x18);
	write_cmos_sensor(0x0307,0xA8);
	write_cmos_sensor(0x0308,0x34);
	write_cmos_sensor(0x0309,0x42);
	write_cmos_sensor(0x3C1F,0x00);
	write_cmos_sensor(0x3C17,0x00);
	write_cmos_sensor(0x3C0B,0x04);
	write_cmos_sensor(0x3C1C,0x47);
	write_cmos_sensor(0x3C1D,0x15);
	write_cmos_sensor(0x3C14,0x04);
	write_cmos_sensor(0x3C16,0x00);
	write_cmos_sensor(0x0820,0x03);
	write_cmos_sensor(0x0821,0x44);
	write_cmos_sensor(0x0114,0x01);
	write_cmos_sensor(0x0344,0x00);
	write_cmos_sensor(0x0345,0x18);
	write_cmos_sensor(0x0346,0x01);
	write_cmos_sensor(0x0347,0x04);
	write_cmos_sensor(0x0348,0x0A);
	write_cmos_sensor(0x0349,0x17);
	write_cmos_sensor(0x034A,0x06);
	write_cmos_sensor(0x034B,0xA3);
	write_cmos_sensor(0x034C,0x05);
	write_cmos_sensor(0x034D,0x00);
	write_cmos_sensor(0x034E,0x02);
	write_cmos_sensor(0x034F,0xD0);
	write_cmos_sensor(0x0900,0x01);
	write_cmos_sensor(0x0901,0x22);
	write_cmos_sensor(0x0381,0x01);
	write_cmos_sensor(0x0383,0x01);
	write_cmos_sensor(0x0385,0x01);
	write_cmos_sensor(0x0387,0x03);
	write_cmos_sensor(0x0340,0x04);
	write_cmos_sensor(0x0341,0x2A);
	write_cmos_sensor(0x0342,0x14);
	write_cmos_sensor(0x0343,0x80);
	write_cmos_sensor(0x0200,0x00);
	write_cmos_sensor(0x0201,0x00);
	write_cmos_sensor(0x0202,0x03);
	write_cmos_sensor(0x0203,0xDE);
	write_cmos_sensor(0x3303,0x02);
	write_cmos_sensor(0x3400,0x00);
	// streaming ON
	write_cmos_sensor(0x3C0D,0x04);
	write_cmos_sensor(0x0100,0x01);
	write_cmos_sensor(0x3C22,0x00);
	write_cmos_sensor(0x3C22,0x00);
	write_cmos_sensor(0x3C0D,0x00);
 	
}
#ifdef S5k5e8_OTP
#define GAIN_DEFAULT       0x0100
static int OTP_vailiable=0;
static int OTP_vailiable_mini=0;
static int vcm_id=0;
static kal_uint32 r_ratio=0;
static kal_uint32 b_ratio=0;
static void read_OTP(void)
{
	int page_flag =0;
	int page1_flag =0;
	int module_id=0,lens_id=0,driver_ic_id=0,year=0,month=0,date=0;
	int rg_ratio=0;
	int bg_ratio=0;
	int g_rg_ratio=0;
	int g_bg_ratio=0;
	write_cmos_sensor(0x0A00,0x04);
	write_cmos_sensor(0x0A02,0x0e);//select page 14
	write_cmos_sensor(0x0A00,0x01);
	page_flag = read_cmos_sensor(0x0A43);
	if(page_flag==0x01){
		printk("TCL s5k5e8 OTP select page 14\n");
		module_id=read_cmos_sensor(0x0A04);
		year=read_cmos_sensor(0x0A05);
		month=read_cmos_sensor(0x0A06);
		date=read_cmos_sensor(0x0A07);
		lens_id=read_cmos_sensor(0x0A08);
		vcm_id=read_cmos_sensor(0x0A09);
		driver_ic_id=read_cmos_sensor(0x0A0A);
		printk("TCL s5k5e8 OTP module_id is 0x%x,lens_id is 0x%x,vcm_id is 0x%x,driver_ic_id is 0x%x\n",module_id,lens_id,vcm_id,driver_ic_id);
		printk("TCL s5k5e8 OTP date is %d-%d-%d\n",year,month,date);
		rg_ratio=(read_cmos_sensor(0x0A0F)<<8) |(read_cmos_sensor(0x0A10)&0xff);
		bg_ratio=(read_cmos_sensor(0x0A11)<<8) |(read_cmos_sensor(0x0A12)&0xff);
		g_rg_ratio=(read_cmos_sensor(0x0A16)<<8) |(read_cmos_sensor(0x0A17)&0xff);
		g_bg_ratio=(read_cmos_sensor(0x0A18)<<8) |(read_cmos_sensor(0x0A19)&0xff);
		
		printk("TCL s5k5e8 OTP rg_ratio is 0x%x,bg_ratio is 0x%x\n",rg_ratio,bg_ratio);
		printk("TCL s5k5e8 OTP g_rg_ratio is 0x%x,g_bg_ratio is 0x%x\n",g_rg_ratio,g_bg_ratio);
		r_ratio = 512 * g_rg_ratio /rg_ratio;
    	      b_ratio = 512 * g_bg_ratio /bg_ratio;
	     printk("TCL s5k5e8 OTP : r_ratio=%d  b_ratio=%d\n",r_ratio,b_ratio);
	if((module_id==0x45)&&(lens_id==0x71)&&(vcm_id==0x0)&&(driver_ic_id==0x0))
		{
			OTP_vailiable=1;
		}
		else{
			OTP_vailiable=0;
		}
	if((module_id==0x45)&&(lens_id==0x71))
		{
			OTP_vailiable_mini=1;
		}
		else{
			OTP_vailiable_mini=0;
		}
		
	}
	else{
	
	write_cmos_sensor(0x0A00,0x04);
	write_cmos_sensor(0x0A02,0x0f);//select page 15
	write_cmos_sensor(0x0A00,0x01);
	page1_flag=read_cmos_sensor(0x0A43);
	if(page1_flag==0x01){
	printk("TCL s5k5e8 OTP select page 15\n");
	module_id=read_cmos_sensor(0x0A04);
	year=read_cmos_sensor(0x0A05);
	month=read_cmos_sensor(0x0A06);
	date=read_cmos_sensor(0x0A07);
	lens_id=read_cmos_sensor(0x0A08);
	vcm_id=read_cmos_sensor(0x0A09);
	driver_ic_id=read_cmos_sensor(0x0A0A);
	printk("TCL s5k5e8 OTP module_id is 0x%x,lens_id is 0x%x,vcm_id is 0x%x,driver_ic_id is 0x%x\n",module_id,lens_id,vcm_id,driver_ic_id);
	printk("TCL s5k5e8 OTP date is %d-%d-%d\n",year,month,date);
	rg_ratio=(read_cmos_sensor(0x0A0F)<<8) |(read_cmos_sensor(0x0A10)&0xff);
	bg_ratio=(read_cmos_sensor(0x0A11)<<8) |(read_cmos_sensor(0x0A12)&0xff);
	g_rg_ratio=(read_cmos_sensor(0x0A16)<<8) |(read_cmos_sensor(0x0A17)&0xff);
	g_bg_ratio=(read_cmos_sensor(0x0A18)<<8) |(read_cmos_sensor(0x0A19)&0xff);
	printk("TCL s5k5e8 OTP rg_ratio is 0x%x,bg_ratio is 0x%x\n",rg_ratio,bg_ratio);
	printk("TCL s5k5e8 OTP g_rg_ratio is 0x%x,g_bg_ratio is 0x%x\n",g_rg_ratio,g_bg_ratio);
	r_ratio = 512 * g_rg_ratio /rg_ratio;
    	b_ratio = 512 * g_bg_ratio /bg_ratio;
	printk("TCL s5k5e8 OTP : r_ratio=%d  b_ratio=%d\n",r_ratio,b_ratio);	
	if((module_id==0x45)&&(lens_id==0x71)&&(vcm_id==0x0)&&(driver_ic_id==0x0))
		{
			OTP_vailiable=1;
		}
	else{
			OTP_vailiable=0;
		}
	if((module_id==0x45)&&(lens_id==0x71))
		{
			OTP_vailiable_mini=1;
		}
	else{
			OTP_vailiable_mini=0;
		}
	}
	else{
	OTP_vailiable=0;
	OTP_vailiable_mini=0;
	printk("TCL s5k5e8 no OTP");
	}
	
	}
	
	write_cmos_sensor(0x0A00,0x04);
	write_cmos_sensor(0x0A00,0x00);
}
static void set_otp_awb(kal_uint16 R_Gain ,kal_uint16 B_Gain,kal_uint16 G_Gain);
static void awb_gain_cal_set(void)
{
    USHORT R_GAIN;
    USHORT B_GAIN;
    USHORT Gr_GAIN;
    USHORT Gb_GAIN;
    USHORT G_GAIN;

    printk("S5K5E8 zcw++: OTP WB Gain Set Start: \n"); 
    if(!r_ratio || !b_ratio)
    {
        printk("S5K5E8 zcw++: OTP WB ratio Data Err!\n");
 
    }
//S5K5E2YA_wordwrite_cmos_sensor(GAIN_GREEN1_ADDR, GAIN_DEFAULT); //Green 1 default gain 1x
//S5K5E2YA_wordwrite_cmos_sensor(GAIN_GREEN2_ADDR, GAIN_DEFAULT); //Green 2 default gain 1x
    if(r_ratio >= 512 )
    {
        if(b_ratio>=512) 
        {
            R_GAIN = (USHORT)(GAIN_DEFAULT * r_ratio / 512);						
            G_GAIN = GAIN_DEFAULT;
            B_GAIN = (USHORT)(GAIN_DEFAULT * b_ratio / 512);
        }
        else
        {
            R_GAIN =  (USHORT)(GAIN_DEFAULT*r_ratio / b_ratio );
            G_GAIN = (USHORT)(GAIN_DEFAULT*512 / b_ratio );
            B_GAIN = GAIN_DEFAULT;    
        }
    }
    else                      
    {		
        if(b_ratio >= 512)
        {
            R_GAIN = GAIN_DEFAULT;
            G_GAIN = (USHORT)(GAIN_DEFAULT*512 /r_ratio);		
            B_GAIN =  (USHORT)(GAIN_DEFAULT*b_ratio / r_ratio );
        } 
        else 
        {
            Gr_GAIN = (USHORT)(GAIN_DEFAULT*512/ r_ratio );						
            Gb_GAIN = (USHORT)(GAIN_DEFAULT*512/b_ratio );						
            if(Gr_GAIN >= Gb_GAIN)						
            {						
                R_GAIN = GAIN_DEFAULT;						
                G_GAIN = (USHORT)(GAIN_DEFAULT *512/ r_ratio );						
                B_GAIN =  (USHORT)(GAIN_DEFAULT*b_ratio / r_ratio );						
            } 
            else
            {						
                R_GAIN =  (USHORT)(GAIN_DEFAULT*r_ratio  / b_ratio);						
                G_GAIN = (USHORT)(GAIN_DEFAULT*512 / b_ratio );						
                B_GAIN = GAIN_DEFAULT;
            }
        }        
    }

    printk("TCL: WB Gain Set [R_GAIN=%d],[G_GAIN=%d],[B_GAIN=%d] \n",R_GAIN, G_GAIN, B_GAIN);
    set_otp_awb(R_GAIN,B_GAIN,G_GAIN);
}


static void set_otp_awb(kal_uint16 R_Gain ,kal_uint16 B_Gain,kal_uint16 G_Gain)
{
	kal_uint16 R_GainH, B_GainH, G_GainH;
	kal_uint16 R_GainL, B_GainL, G_GainL;
	kal_uint32 temp;

	temp = R_Gain;
	R_GainH = (temp & 0xff00)>>8;
	temp = R_Gain;
	R_GainL = (temp & 0x00ff);

	temp = B_Gain;
	B_GainH = (temp & 0xff00)>>8;
	temp = B_Gain;
	B_GainL = (temp & 0x00ff);

	temp = G_Gain;
	G_GainH = (temp & 0xff00)>>8;
	temp = G_Gain;
	G_GainL = (temp & 0x00ff);

	printk("TCL:[S5K5E8Y] [S5K5E2Y_MIPI_read_otp_wb] R_GainH=0x%x\n", R_GainH);	
	printk("TCL:[S5K5E8Y] [S5K5E2Y_MIPI_read_otp_wb] R_GainL=0x%x\n", R_GainL);	
	printk("TCL:[S5K5E8Y] [S5K5E2Y_MIPI_read_otp_wb] B_GainH=0x%x\n", B_GainH);
	printk("TCL:[S5K5E8Y] [S5K5E2Y_MIPI_read_otp_wb] B_GainL=0x%x\n", B_GainL);	
	printk("TCL:[S5K5E8Y] [S5K5E2Y_MIPI_read_otp_wb] G_GainH=0x%x\n", G_GainH);	
	printk("TCL:[S5K5E8Y] [S5K5E2Y_MIPI_read_otp_wb] G_GainL=0x%x\n", G_GainL);
#if 1
	write_cmos_sensor(0x020e, G_GainH);
	write_cmos_sensor(0x020f, G_GainL);//GR
	write_cmos_sensor(0x0210, R_GainH);
	write_cmos_sensor(0x0211, R_GainL);
	write_cmos_sensor(0x0212, B_GainH);
	write_cmos_sensor(0x0213, B_GainL);
	write_cmos_sensor(0x0214, G_GainH);
	write_cmos_sensor(0x0215, G_GainL);//GB
#endif	
}


static void apply_lsc_otp(void)
{
        printk("TCL:S5K5E8 OTP : LSC Auto Correct \n");
        write_cmos_sensor(0x0B00,0x01);
        write_cmos_sensor(0x3400,0x00);

}
#endif
extern char Back_Camera_Name[256];
extern char Back_Camera_OTP [256];
/*************************************************************************
* FUNCTION
*	get_imgsensor_id
*
* DESCRIPTION
*	This function get the sensor ID 
*
* PARAMETERS
*	*sensorID : return the sensor ID 
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/
static kal_uint32 get_imgsensor_id(UINT32 *sensor_id) 
{
	kal_uint8 i = 0;
	kal_uint8 retry = 2;
	//sensor have two i2c address 0x6c 0x6d & 0x21 0x20, we should detect the module used i2c address
	while(imgsensor_info.i2c_addr_table[i] != 0xff) {
		spin_lock(&imgsensor_drv_lock);
		imgsensor.i2c_write_id = imgsensor_info.i2c_addr_table[i];
		spin_unlock(&imgsensor_drv_lock);
		do {
			*sensor_id = return_sensor_id();
			if (*sensor_id == imgsensor_info.sensor_id) {
				read_OTP();
	    #ifdef TARGET_BUILD_MMITEST
		if(vcm_id==0x0)
			{
			printk("i2c write id: 0x%x, sensor id: 0x%x\n", imgsensor.i2c_write_id,*sensor_id);
				sprintf(Back_Camera_Name,"S5k5e8yx:SEASONS:SUP2:5M_FF");
				if(OTP_vailiable_mini==1){
				sprintf(Back_Camera_OTP,"OTP:OK");
				}
				else{
				sprintf(Back_Camera_OTP,"OTP:NOK");
			      }
				return ERROR_NONE;
			}
		else{
			*sensor_id = 0xFFFFFFFF;
			if(OTP_vailiable_mini==1)
			{
				 sprintf(Back_Camera_OTP,"OTP:OK");
			}
			else{
			       sprintf(Back_Camera_OTP,"OTP:NOK");
			}
			      return ERROR_SENSOR_CONNECT_FAIL;	
		}
		#else
				if(OTP_vailiable==1){
				printk("i2c write id: 0x%x, sensor id: 0x%x\n", imgsensor.i2c_write_id,*sensor_id);
				sprintf(Back_Camera_Name,"S5k5e8yx:SEASONS:SUP2:5M_FF");
				sprintf(Back_Camera_OTP,"OTP:OK");
				return ERROR_NONE;
				}
				else{
				*sensor_id = 0xFFFFFFFF;
				if(OTP_vailiable_mini==1){
				sprintf(Back_Camera_OTP,"OTP:OK");
				}
				else{
				sprintf(Back_Camera_OTP,"OTP:NOK");
				}
				return ERROR_SENSOR_CONNECT_FAIL;
				}
		#endif
			}	
			printk("Read sensor id fail, id: 0x%x\n", imgsensor.i2c_write_id);
			retry--;
		} while(retry > 0);
		i++;
		retry = 2;
	}
	if (*sensor_id != imgsensor_info.sensor_id) {
		// if Sensor ID is not correct, Must set *sensor_id to 0xFFFFFFFF 
		*sensor_id = 0xFFFFFFFF;
		return ERROR_SENSOR_CONNECT_FAIL;
	}
	return ERROR_NONE;
}


/*************************************************************************
* FUNCTION
*	open
*
* DESCRIPTION
*	This function initialize the registers of CMOS sensor
*
* PARAMETERS
*	None
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/
static kal_uint32 open(void)
{
	//const kal_uint8 i2c_addr[] = {IMGSENSOR_WRITE_ID_1, IMGSENSOR_WRITE_ID_2};
	kal_uint8 i = 0;
	kal_uint8 retry = 2;
	kal_uint32 sensor_id = 0; 
	
	LOG_1;
	LOG_2;
	if(first_flag == 1){
		LOG_INF("modify device I2C address to 0x5a\n");
		write_cmos_sensor(0x0107,0x5a);
		first_flag = 0;
		}
	//sensor have two i2c address 0x20,0x5a  we should detect the module used i2c address
	while (imgsensor_info.i2c_addr_table[i] != 0xff) {
		spin_lock(&imgsensor_drv_lock);
		imgsensor.i2c_write_id = imgsensor_info.i2c_addr_table[i];
		spin_unlock(&imgsensor_drv_lock);
		do {
			sensor_id = return_sensor_id();
			if (sensor_id == imgsensor_info.sensor_id) {				
				LOG_INF("i2c write id: 0x%x, sensor id: 0x%x\n", imgsensor.i2c_write_id,sensor_id);	  
				break;
			}	
			LOG_INF("Read sensor id fail, id: 0x%x\n", imgsensor.i2c_write_id);
			retry--;
		} while(retry > 0);
		i++;
		if (sensor_id == imgsensor_info.sensor_id)
			break;
		retry = 2;
	}		 
	if (imgsensor_info.sensor_id != sensor_id)
		return ERROR_SENSOR_CONNECT_FAIL;
	
	/* initail sequence write in  */
	sensor_init();
#ifdef S5k5e8_OTP
	read_OTP();
	awb_gain_cal_set();
	apply_lsc_otp();

#endif
	spin_lock(&imgsensor_drv_lock);

	imgsensor.autoflicker_en= KAL_FALSE;
	imgsensor.sensor_mode = IMGSENSOR_MODE_INIT;
	imgsensor.shutter = 0x3D0;
	imgsensor.gain = 0x100;
	imgsensor.pclk = imgsensor_info.pre.pclk;
	imgsensor.frame_length = imgsensor_info.pre.framelength;
	imgsensor.line_length = imgsensor_info.pre.linelength;
	imgsensor.min_frame_length = imgsensor_info.pre.framelength;
	imgsensor.dummy_pixel = 0;
	imgsensor.dummy_line = 0;
	imgsensor.ihdr_en = 0;
	imgsensor.test_pattern = KAL_FALSE;
	imgsensor.current_fps = imgsensor_info.pre.max_framerate;
	spin_unlock(&imgsensor_drv_lock);

	return ERROR_NONE;
}	/*	open  */



/*************************************************************************
* FUNCTION
*	close
*
* DESCRIPTION
*	
*
* PARAMETERS
*	None
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/
static kal_uint32 close(void)
{
	LOG_INF("E\n");

	/*No Need to implement this function*/ 
	
	return ERROR_NONE;
}	/*	close  */


/*************************************************************************
* FUNCTION
* preview
*
* DESCRIPTION
*	This function start the sensor preview.
*
* PARAMETERS
*	*image_window : address pointer of pixel numbers in one period of HSYNC
*  *sensor_config_data : address pointer of line numbers in one period of VSYNC
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/
static kal_uint32 preview(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("E\n");

	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_PREVIEW;
	imgsensor.pclk = imgsensor_info.pre.pclk;
	//imgsensor.video_mode = KAL_FALSE;
	imgsensor.line_length = imgsensor_info.pre.linelength;
	imgsensor.frame_length = imgsensor_info.pre.framelength; 
	imgsensor.min_frame_length = imgsensor_info.pre.framelength;
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	preview_setting();
	set_mirror_flip(imgsensor.mirror);
	return ERROR_NONE;
}	/*	preview   */

/*************************************************************************
* FUNCTION
*	capture
*
* DESCRIPTION
*	This function setup the CMOS sensor in capture MY_OUTPUT mode
*
* PARAMETERS
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/
static kal_uint32 capture(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
						  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("E\n");
	spin_lock(&imgsensor_drv_lock);
	//imgsensor.current_fps = 240;
	imgsensor.sensor_mode = IMGSENSOR_MODE_CAPTURE;
	if (imgsensor.current_fps == imgsensor_info.cap.max_framerate){
		imgsensor.pclk = imgsensor_info.cap.pclk;
		imgsensor.line_length = imgsensor_info.cap.linelength;
		imgsensor.frame_length = imgsensor_info.cap.framelength;  
		imgsensor.min_frame_length = imgsensor_info.cap.framelength;
		imgsensor.autoflicker_en = KAL_FALSE;
		}
	else if (imgsensor.current_fps == imgsensor_info.cap1.max_framerate) {//PIP capture:15fps
		imgsensor.pclk = imgsensor_info.cap1.pclk;
		imgsensor.line_length = imgsensor_info.cap1.linelength;
		imgsensor.frame_length = imgsensor_info.cap1.framelength;  
		imgsensor.min_frame_length = imgsensor_info.cap1.framelength;
		imgsensor.autoflicker_en = KAL_FALSE;
	}else {//PIP capture: 24fps
		imgsensor.pclk = imgsensor_info.cap2.pclk;
		imgsensor.line_length = imgsensor_info.cap2.linelength;
		imgsensor.frame_length = imgsensor_info.cap2.framelength;  
		imgsensor.min_frame_length = imgsensor_info.cap2.framelength;
		imgsensor.autoflicker_en = KAL_FALSE;		
	}
	spin_unlock(&imgsensor_drv_lock);

	capture_setting(imgsensor.current_fps); 
	set_mirror_flip(imgsensor.mirror);
	
	
	return ERROR_NONE;
}	/* capture() */
static kal_uint32 normal_video(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("E\n");
	
	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_VIDEO;
	imgsensor.pclk = imgsensor_info.normal_video.pclk;
	imgsensor.line_length = imgsensor_info.normal_video.linelength;
	imgsensor.frame_length = imgsensor_info.normal_video.framelength;  
	imgsensor.min_frame_length = imgsensor_info.normal_video.framelength;
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	normal_video_setting(imgsensor.current_fps);
	set_mirror_flip(imgsensor.mirror);
	
	return ERROR_NONE;
}	/*	normal_video   */

static kal_uint32 hs_video(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
   
	LOG_INF("E\n");
	
	
	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_HIGH_SPEED_VIDEO;
	imgsensor.pclk = imgsensor_info.hs_video.pclk;
	//imgsensor.video_mode = KAL_TRUE;
	imgsensor.line_length = imgsensor_info.hs_video.linelength;
	imgsensor.frame_length = imgsensor_info.hs_video.framelength; 
	imgsensor.min_frame_length = imgsensor_info.hs_video.framelength;
	imgsensor.dummy_line = 0;
	imgsensor.dummy_pixel = 0;
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	hs_video_setting();
	set_mirror_flip(imgsensor.mirror);
	
	return ERROR_NONE;
}	/*	hs_video   */

static kal_uint32 slim_video(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("E\n");
	
	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_SLIM_VIDEO;
	imgsensor.pclk = imgsensor_info.slim_video.pclk;
	imgsensor.line_length = imgsensor_info.slim_video.linelength;
	imgsensor.frame_length = imgsensor_info.slim_video.framelength; 
	imgsensor.min_frame_length = imgsensor_info.slim_video.framelength;
	imgsensor.dummy_line = 0;
	imgsensor.dummy_pixel = 0;
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	slim_video_setting();
	set_mirror_flip(imgsensor.mirror);
	
	return ERROR_NONE;
}	/*	slim_video	 */



static kal_uint32 get_resolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *sensor_resolution)
{
	LOG_INF("E\n");
	sensor_resolution->SensorFullWidth = imgsensor_info.cap.grabwindow_width;
	sensor_resolution->SensorFullHeight = imgsensor_info.cap.grabwindow_height;
	
	sensor_resolution->SensorPreviewWidth = imgsensor_info.pre.grabwindow_width;
	sensor_resolution->SensorPreviewHeight = imgsensor_info.pre.grabwindow_height;

	sensor_resolution->SensorVideoWidth = imgsensor_info.normal_video.grabwindow_width;
	sensor_resolution->SensorVideoHeight = imgsensor_info.normal_video.grabwindow_height;		

	
	sensor_resolution->SensorHighSpeedVideoWidth	 = imgsensor_info.hs_video.grabwindow_width;
	sensor_resolution->SensorHighSpeedVideoHeight	 = imgsensor_info.hs_video.grabwindow_height;
	
	sensor_resolution->SensorSlimVideoWidth	 = imgsensor_info.slim_video.grabwindow_width;
	sensor_resolution->SensorSlimVideoHeight	 = imgsensor_info.slim_video.grabwindow_height;
	return ERROR_NONE;
}	/*	get_resolution	*/

static kal_uint32 get_info(MSDK_SCENARIO_ID_ENUM scenario_id,
					  MSDK_SENSOR_INFO_STRUCT *sensor_info,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("scenario_id = %d\n", scenario_id);

	
	//sensor_info->SensorVideoFrameRate = imgsensor_info.normal_video.max_framerate/10; /* not use */
	//sensor_info->SensorStillCaptureFrameRate= imgsensor_info.cap.max_framerate/10; /* not use */
	//imgsensor_info->SensorWebCamCaptureFrameRate= imgsensor_info.v.max_framerate; /* not use */

	sensor_info->SensorClockPolarity = SENSOR_CLOCK_POLARITY_LOW;
	sensor_info->SensorClockFallingPolarity = SENSOR_CLOCK_POLARITY_LOW; /* not use */
	sensor_info->SensorHsyncPolarity = SENSOR_CLOCK_POLARITY_LOW; // inverse with datasheet
	sensor_info->SensorVsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
	sensor_info->SensorInterruptDelayLines = 4; /* not use */
	sensor_info->SensorResetActiveHigh = FALSE; /* not use */
	sensor_info->SensorResetDelayCount = 5; /* not use */

	sensor_info->SensroInterfaceType = imgsensor_info.sensor_interface_type;
	sensor_info->MIPIsensorType = imgsensor_info.mipi_sensor_type;
	sensor_info->SettleDelayMode = imgsensor_info.mipi_settle_delay_mode;
	sensor_info->SensorOutputDataFormat = imgsensor_info.sensor_output_dataformat;

	sensor_info->CaptureDelayFrame = imgsensor_info.cap_delay_frame; 
	sensor_info->PreviewDelayFrame = imgsensor_info.pre_delay_frame; 
	sensor_info->VideoDelayFrame = imgsensor_info.video_delay_frame;
	sensor_info->HighSpeedVideoDelayFrame = imgsensor_info.hs_video_delay_frame;
	sensor_info->SlimVideoDelayFrame = imgsensor_info.slim_video_delay_frame;

	sensor_info->SensorMasterClockSwitch = 0; /* not use */
	sensor_info->SensorDrivingCurrent = imgsensor_info.isp_driving_current;
	
	sensor_info->AEShutDelayFrame = imgsensor_info.ae_shut_delay_frame; 		 /* The frame of setting shutter default 0 for TG int */
	sensor_info->AESensorGainDelayFrame = imgsensor_info.ae_sensor_gain_delay_frame;	/* The frame of setting sensor gain */
	sensor_info->AEISPGainDelayFrame = imgsensor_info.ae_ispGain_delay_frame;	
	sensor_info->IHDR_Support = imgsensor_info.ihdr_support;
	sensor_info->IHDR_LE_FirstLine = imgsensor_info.ihdr_le_firstline;
	sensor_info->SensorModeNum = imgsensor_info.sensor_mode_num;
	
	sensor_info->SensorMIPILaneNumber = imgsensor_info.mipi_lane_num; 
	sensor_info->SensorClockFreq = imgsensor_info.mclk;
	sensor_info->SensorClockDividCount = 3; /* not use */
	sensor_info->SensorClockRisingCount = 0;
	sensor_info->SensorClockFallingCount = 2; /* not use */
	sensor_info->SensorPixelClockCount = 3; /* not use */
	sensor_info->SensorDataLatchCount = 2; /* not use */
	
	sensor_info->MIPIDataLowPwr2HighSpeedTermDelayCount = 0; 
	sensor_info->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
	sensor_info->SensorWidthSampling = 0;  // 0 is default 1x
	sensor_info->SensorHightSampling = 0;	// 0 is default 1x 
	sensor_info->SensorPacketECCOrder = 1;

	switch (scenario_id) {
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
			sensor_info->SensorGrabStartX = imgsensor_info.pre.startx; 
			sensor_info->SensorGrabStartY = imgsensor_info.pre.starty;		
			
			sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount = imgsensor_info.pre.mipi_data_lp2hs_settle_dc;
			
			break;
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			sensor_info->SensorGrabStartX = imgsensor_info.cap.startx; 
			sensor_info->SensorGrabStartY = imgsensor_info.cap.starty;
				  
			sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount = imgsensor_info.cap.mipi_data_lp2hs_settle_dc; 

			break;	 
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			
			sensor_info->SensorGrabStartX = imgsensor_info.normal_video.startx; 
			sensor_info->SensorGrabStartY = imgsensor_info.normal_video.starty;
	   
			sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount = imgsensor_info.normal_video.mipi_data_lp2hs_settle_dc; 

			break;	  
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:			
			sensor_info->SensorGrabStartX = imgsensor_info.hs_video.startx; 
			sensor_info->SensorGrabStartY = imgsensor_info.hs_video.starty;
				  
			sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount = imgsensor_info.hs_video.mipi_data_lp2hs_settle_dc; 

			break;
		case MSDK_SCENARIO_ID_SLIM_VIDEO:
			sensor_info->SensorGrabStartX = imgsensor_info.slim_video.startx; 
			sensor_info->SensorGrabStartY = imgsensor_info.slim_video.starty;
				  
			sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount = imgsensor_info.slim_video.mipi_data_lp2hs_settle_dc; 

			break;
		default:			
			sensor_info->SensorGrabStartX = imgsensor_info.pre.startx; 
			sensor_info->SensorGrabStartY = imgsensor_info.pre.starty;		
			
			sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount = imgsensor_info.pre.mipi_data_lp2hs_settle_dc;
			break;
	}
	
	return ERROR_NONE;
}	/*	get_info  */


static kal_uint32 control(MSDK_SCENARIO_ID_ENUM scenario_id, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("scenario_id = %d\n", scenario_id);
	spin_lock(&imgsensor_drv_lock);
	imgsensor.current_scenario_id = scenario_id;
	spin_unlock(&imgsensor_drv_lock);
	switch (scenario_id) {
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
			preview(image_window, sensor_config_data);
			break;
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			capture(image_window, sensor_config_data);
			break;	
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			normal_video(image_window, sensor_config_data);
			break;	  
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
			hs_video(image_window, sensor_config_data);
			break;
		case MSDK_SCENARIO_ID_SLIM_VIDEO:
			slim_video(image_window, sensor_config_data);
			break;	  
		default:
			LOG_INF("Error ScenarioId setting");
			preview(image_window, sensor_config_data);
			return ERROR_INVALID_SCENARIO_ID;
	}
	return ERROR_NONE;
}	/* control() */



static kal_uint32 set_video_mode(UINT16 framerate)
{
	LOG_INF("framerate = %d\n ", framerate);
	// SetVideoMode Function should fix framerate
	if (framerate == 0)
		// Dynamic frame rate
		return ERROR_NONE;
	spin_lock(&imgsensor_drv_lock);
	if ((framerate == 300) && (imgsensor.autoflicker_en == KAL_TRUE))
		imgsensor.current_fps = 296;
	else if ((framerate == 150) && (imgsensor.autoflicker_en == KAL_TRUE))
		imgsensor.current_fps = 146;
	else
		imgsensor.current_fps = framerate;
	spin_unlock(&imgsensor_drv_lock);
	set_max_framerate(imgsensor.current_fps,1);

	return ERROR_NONE;
}

static kal_uint32 set_auto_flicker_mode(kal_bool enable, UINT16 framerate)
{
	LOG_INF("enable = %d, framerate = %d \n", enable, framerate);
	spin_lock(&imgsensor_drv_lock);
	if (enable) //enable auto flicker	  
		imgsensor.autoflicker_en = KAL_TRUE;
	else //Cancel Auto flick
		imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	return ERROR_NONE;
}


static kal_uint32 set_max_framerate_by_scenario(MSDK_SCENARIO_ID_ENUM scenario_id, MUINT32 framerate) 
{
	kal_uint32 frame_length;
  
	LOG_INF("scenario_id = %d, framerate = %d\n", scenario_id, framerate);

	switch (scenario_id) {
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
			frame_length = imgsensor_info.pre.pclk / framerate * 10 / imgsensor_info.pre.linelength;
			spin_lock(&imgsensor_drv_lock);
			imgsensor.dummy_line = (frame_length > imgsensor_info.pre.framelength) ? (frame_length - imgsensor_info.pre.framelength) : 0;
			imgsensor.frame_length = imgsensor_info.pre.framelength + imgsensor.dummy_line;
			imgsensor.min_frame_length = imgsensor.frame_length;
			spin_unlock(&imgsensor_drv_lock);
			//set_dummy();
			break;			
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			if(framerate == 0)
				return ERROR_NONE;
			frame_length = imgsensor_info.normal_video.pclk / framerate * 10 / imgsensor_info.normal_video.linelength;
			spin_lock(&imgsensor_drv_lock);
			imgsensor.dummy_line = (frame_length > imgsensor_info.normal_video.framelength) ? (frame_length - imgsensor_info.normal_video.framelength) : 0;			
			imgsensor.frame_length = imgsensor_info.normal_video.framelength + imgsensor.dummy_line;
			imgsensor.min_frame_length = imgsensor.frame_length;
			spin_unlock(&imgsensor_drv_lock);
			//set_dummy();
			break;
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
        	  if (imgsensor.current_fps == imgsensor_info.cap1.max_framerate) {
                frame_length = imgsensor_info.cap1.pclk / framerate * 10 / imgsensor_info.cap1.linelength;
                spin_lock(&imgsensor_drv_lock);
		            imgsensor.dummy_line = (frame_length > imgsensor_info.cap1.framelength) ? (frame_length - imgsensor_info.cap1.framelength) : 0;
		            imgsensor.frame_length = imgsensor_info.cap1.framelength + imgsensor.dummy_line;
		            imgsensor.min_frame_length = imgsensor.frame_length;
		            spin_unlock(&imgsensor_drv_lock);
            } else if(imgsensor.current_fps == imgsensor_info.cap2.max_framerate){
            	 frame_length = imgsensor_info.cap2.pclk / framerate * 10 / imgsensor_info.cap2.linelength;
                spin_lock(&imgsensor_drv_lock);
		            imgsensor.dummy_line = (frame_length > imgsensor_info.cap2.framelength) ? (frame_length - imgsensor_info.cap2.framelength) : 0;
		            imgsensor.frame_length = imgsensor_info.cap2.framelength + imgsensor.dummy_line;
		            imgsensor.min_frame_length = imgsensor.frame_length;
		            spin_unlock(&imgsensor_drv_lock);
            	}
			else{
				if (imgsensor.current_fps != imgsensor_info.cap.max_framerate)
                    LOG_INF("Warning: current_fps %d fps is not support, so use cap's setting: %d fps!\n",framerate,imgsensor_info.cap.max_framerate/10);
			frame_length = imgsensor_info.cap.pclk / framerate * 10 / imgsensor_info.cap.linelength;
			spin_lock(&imgsensor_drv_lock);
			imgsensor.dummy_line = (frame_length > imgsensor_info.cap.framelength) ? (frame_length - imgsensor_info.cap.framelength) : 0;
			imgsensor.frame_length = imgsensor_info.cap.framelength + imgsensor.dummy_line;
			imgsensor.min_frame_length = imgsensor.frame_length;
			spin_unlock(&imgsensor_drv_lock);
            }
			//set_dummy();
			break;	
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
			frame_length = imgsensor_info.hs_video.pclk / framerate * 10 / imgsensor_info.hs_video.linelength;
			spin_lock(&imgsensor_drv_lock);
			imgsensor.dummy_line = (frame_length > imgsensor_info.hs_video.framelength) ? (frame_length - imgsensor_info.hs_video.framelength) : 0;
			imgsensor.frame_length = imgsensor_info.hs_video.framelength + imgsensor.dummy_line;
			imgsensor.min_frame_length = imgsensor.frame_length;
			spin_unlock(&imgsensor_drv_lock);
			//set_dummy();
			break;
		case MSDK_SCENARIO_ID_SLIM_VIDEO:
			frame_length = imgsensor_info.slim_video.pclk / framerate * 10 / imgsensor_info.slim_video.linelength;
			spin_lock(&imgsensor_drv_lock);
			imgsensor.dummy_line = (frame_length > imgsensor_info.slim_video.framelength) ? (frame_length - imgsensor_info.slim_video.framelength): 0;	
			imgsensor.frame_length = imgsensor_info.slim_video.framelength + imgsensor.dummy_line;
			imgsensor.min_frame_length = imgsensor.frame_length;
			spin_unlock(&imgsensor_drv_lock);
			//set_dummy();
			break;		
		default:  //coding with  preview scenario by default
			frame_length = imgsensor_info.pre.pclk / framerate * 10 / imgsensor_info.pre.linelength;
			spin_lock(&imgsensor_drv_lock);
			imgsensor.dummy_line = (frame_length > imgsensor_info.pre.framelength) ? (frame_length - imgsensor_info.pre.framelength) : 0;
			imgsensor.frame_length = imgsensor_info.pre.framelength + imgsensor.dummy_line;
			imgsensor.min_frame_length = imgsensor.frame_length;
			spin_unlock(&imgsensor_drv_lock);
			//set_dummy();
			LOG_INF("error scenario_id = %d, we use preview scenario \n", scenario_id);
			break;
	}	
	return ERROR_NONE;
}


static kal_uint32 get_default_framerate_by_scenario(MSDK_SCENARIO_ID_ENUM scenario_id, MUINT32 *framerate) 
{
	LOG_INF("scenario_id = %d\n", scenario_id);

	switch (scenario_id) {
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
			*framerate = imgsensor_info.pre.max_framerate;
			break;
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			*framerate = imgsensor_info.normal_video.max_framerate;
			break;
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			*framerate = imgsensor_info.cap.max_framerate;
			break;		
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
			*framerate = imgsensor_info.hs_video.max_framerate;
			break;
		case MSDK_SCENARIO_ID_SLIM_VIDEO: 
			*framerate = imgsensor_info.slim_video.max_framerate;
			break;
		default:
			break;
	}

	return ERROR_NONE;
}

static kal_uint32 set_test_pattern_mode(kal_bool enable)
{
	LOG_INF("enable: %d\n", enable);
    enable = false;
	if (enable) {
		
		// 0x0601[2:0]; 0=no pattern,1=solid colour,2 = 100% colour bar ,3 = Fade to gray' colour bar
 		write_cmos_sensor(0x0601, 0x02);
	} else {
		write_cmos_sensor(0x0601, 0x00);
	}	 
	write_cmos_sensor(0x3200, 0x00);
	spin_lock(&imgsensor_drv_lock);
	imgsensor.test_pattern = enable;
	spin_unlock(&imgsensor_drv_lock);
	return ERROR_NONE;
}

static kal_uint32 feature_control(MSDK_SENSOR_FEATURE_ENUM feature_id,
							 UINT8 *feature_para,UINT32 *feature_para_len)
{
	UINT16 *feature_return_para_16=(UINT16 *) feature_para;
	UINT16 *feature_data_16=(UINT16 *) feature_para;
	UINT32 *feature_return_para_32=(UINT32 *) feature_para;
	UINT32 *feature_data_32=(UINT32 *) feature_para;
    unsigned long long *feature_data=(unsigned long long *) feature_para;
//    unsigned long long *feature_return_para=(unsigned long long *) feature_para;

	SENSOR_WINSIZE_INFO_STRUCT *wininfo;	
	MSDK_SENSOR_REG_INFO_STRUCT *sensor_reg_data=(MSDK_SENSOR_REG_INFO_STRUCT *) feature_para;
 
	//LOG_INF("feature_id = %d", feature_id);
	switch (feature_id) {
		case SENSOR_FEATURE_GET_PERIOD:
			*feature_return_para_16++ = imgsensor.line_length;
			*feature_return_para_16 = imgsensor.frame_length;
			*feature_para_len=4;
			break;
		case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ:
            //LOG_INF("feature_Control imgsensor.pclk = %d,imgsensor.current_fps = %d\n", imgsensor.pclk,imgsensor.current_fps);
			*feature_return_para_32 = imgsensor.pclk;
			*feature_para_len=4;
			break;
		case SENSOR_FEATURE_SET_ESHUTTER:
            set_shutter(*feature_data);
			break;
		case SENSOR_FEATURE_SET_NIGHTMODE:
			break;
		case SENSOR_FEATURE_SET_GAIN:
            set_gain((UINT16) *feature_data);
			break;
		case SENSOR_FEATURE_SET_FLASHLIGHT:
			break;
		case SENSOR_FEATURE_SET_ISP_MASTER_CLOCK_FREQ:
			break;
		case SENSOR_FEATURE_SET_REGISTER:
			if((sensor_reg_data->RegData>>8)>0)
			   write_cmos_sensor(sensor_reg_data->RegAddr, sensor_reg_data->RegData);
			else
				write_cmos_sensor_8(sensor_reg_data->RegAddr, sensor_reg_data->RegData);
			break;
		case SENSOR_FEATURE_GET_REGISTER:
			sensor_reg_data->RegData = read_cmos_sensor(sensor_reg_data->RegAddr);
			break;
		case SENSOR_FEATURE_GET_LENS_DRIVER_ID:
			// get the lens driver ID from EEPROM or just return LENS_DRIVER_ID_DO_NOT_CARE
			// if EEPROM does not exist in camera module.
			*feature_return_para_32=LENS_DRIVER_ID_DO_NOT_CARE;
			*feature_para_len=4;
			break;
		case SENSOR_FEATURE_SET_VIDEO_MODE:
            set_video_mode(*feature_data);
			break;
		case SENSOR_FEATURE_CHECK_SENSOR_ID:
			get_imgsensor_id(feature_return_para_32);
			break;
		case SENSOR_FEATURE_SET_AUTO_FLICKER_MODE:
			set_auto_flicker_mode((BOOL)*feature_data_16,*(feature_data_16+1));
			break;
		case SENSOR_FEATURE_SET_MAX_FRAME_RATE_BY_SCENARIO:
            set_max_framerate_by_scenario((MSDK_SCENARIO_ID_ENUM)*feature_data, *(feature_data+1));
			break;
		case SENSOR_FEATURE_GET_DEFAULT_FRAME_RATE_BY_SCENARIO:
            get_default_framerate_by_scenario((MSDK_SCENARIO_ID_ENUM)*(feature_data), (MUINT32 *)(uintptr_t)(*(feature_data+1)));
			break;
		case SENSOR_FEATURE_SET_TEST_PATTERN:
            set_test_pattern_mode((BOOL)*feature_data);
			break;
		case SENSOR_FEATURE_GET_TEST_PATTERN_CHECKSUM_VALUE: //for factory mode auto testing
			*feature_return_para_32 = imgsensor_info.checksum_value;
			*feature_para_len=4;
			break;
		case SENSOR_FEATURE_SET_FRAMERATE:
           // LOG_INF("current fps :%d\n", *feature_data);
			spin_lock(&imgsensor_drv_lock);
            imgsensor.current_fps = *feature_data;
			spin_unlock(&imgsensor_drv_lock);
			break;
		case SENSOR_FEATURE_SET_HDR:
			//LOG_INF("ihdr enable :%d\n", (BOOL)*feature_data_16);
			//LOG_INF("Warning! Not Support IHDR Feature");
			spin_lock(&imgsensor_drv_lock);
			//imgsensor.ihdr_en = (BOOL)*feature_data_16;
            imgsensor.ihdr_en = KAL_FALSE;
			spin_unlock(&imgsensor_drv_lock);
			break;
		case SENSOR_FEATURE_GET_CROP_INFO:
            //LOG_INF("SENSOR_FEATURE_GET_CROP_INFO scenarioId:%d\n", *feature_data);
            wininfo = (SENSOR_WINSIZE_INFO_STRUCT *)(uintptr_t)(*(feature_data+1));

			switch (*feature_data_32) {
				case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
					memcpy((void *)wininfo,(void *)&imgsensor_winsize_info[1],sizeof(SENSOR_WINSIZE_INFO_STRUCT));
					break;
				case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
					memcpy((void *)wininfo,(void *)&imgsensor_winsize_info[2],sizeof(SENSOR_WINSIZE_INFO_STRUCT));
					break;
				case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
					memcpy((void *)wininfo,(void *)&imgsensor_winsize_info[3],sizeof(SENSOR_WINSIZE_INFO_STRUCT));
					break;
				case MSDK_SCENARIO_ID_SLIM_VIDEO:
					memcpy((void *)wininfo,(void *)&imgsensor_winsize_info[4],sizeof(SENSOR_WINSIZE_INFO_STRUCT));
					break;
				case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
				default:
					memcpy((void *)wininfo,(void *)&imgsensor_winsize_info[0],sizeof(SENSOR_WINSIZE_INFO_STRUCT));
					break;
			}
            break;
		case SENSOR_FEATURE_SET_IHDR_SHUTTER_GAIN:
            //LOG_INF("SENSOR_SET_SENSOR_IHDR LE=%d, SE=%d, Gain=%d\n",(UINT16)*feature_data,(UINT16)*(feature_data+1),(UINT16)*(feature_data+2));
            ihdr_write_shutter_gain((UINT16)*feature_data,(UINT16)*(feature_data+1),(UINT16)*(feature_data+2));
			break;
		default:
			break;
	}
  
	return ERROR_NONE;
}	/*	feature_control()  */

static SENSOR_FUNCTION_STRUCT sensor_func = {
	open,
	get_info,
	get_resolution,
	feature_control,
	control,
	close
};

UINT32 S5K5E8YXFF_MIPI_RAW_SensorInit(PSENSOR_FUNCTION_STRUCT *pfFunc)
{
	/* To Do : Check Sensor status here */
	if (pfFunc!=NULL)
		*pfFunc=&sensor_func;
	return ERROR_NONE;
}	/*	S5K5E8YX_MIPI_RAW_SensorInit	*/
