/* linux/arch/arm/mach-msm/board-pico-panel.c
 *
 * Copyright (c) 2011 HTC.
 * Copyright (c) 2010-2011, Code Aurora Forum.
 *
 * Copyright (c) 2014 Marin Spajic <marinspajic17@hotmail.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <asm/io.h>
#include <asm/mach-types.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/init.h>
#include <linux/leds.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/spi/spi.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <mach/msm_fb.h>
#include <mach/msm_iomap.h>
#include <mach/panel_id.h>
#include <mach/msm_bus_board.h>
#include <mach/debug_display.h>

#include "../devices.h"
#include "../board-pico.h"
#include "../devices-msm8x60.h"
#include "../../../../drivers/video/msm/mdp_hw.h"

extern int panel_type;
static int mipi_power_save_on = 1;

void mdp_color_enhancement(struct mdp_reg *reg_seq, int size);

struct mdp_reg pico_color_enhancement[] = {
	{0x93400, 0x00024C, 0x0},
	{0x93404, 0xFFFFDA, 0x0},
	{0x93408, 0xFFFFDA, 0x0},
	{0x9340C, 0xFFFFDA, 0x0},
	{0x93410, 0x00024C, 0x0},
	{0x93414, 0xFFFFDA, 0x0},
	{0x93418, 0xFFFFDA, 0x0},
	{0x9341C, 0xFFFFDA, 0x0},
	{0x93420, 0x00024C, 0x0},
	{0x93600, 0x000000, 0x0},
	{0x93604, 0x0000FF, 0x0},
	{0x93608, 0x000000, 0x0},
	{0x9360C, 0x0000FF, 0x0},
	{0x93610, 0x000000, 0x0},
	{0x93614, 0x0000FF, 0x0},
	{0x93680, 0x000000, 0x0},
	{0x93684, 0x0000FF, 0x0},
	{0x93688, 0x000000, 0x0},
	{0x9368C, 0x0000FF, 0x0},
	{0x93690, 0x000000, 0x0},
	{0x93694, 0x0000FF, 0x0},
	{0x90070, 0x08, 0x0},
};

int pico_mdp_color_enhancement(void)
{
//	mdp_color_enhancement(pico_color_enhancement, ARRAY_SIZE(pico_color_enhancement));
	
	return 0;
}

static void pico_panel_power(int on)
{

	PR_DISP_INFO("%s: power %s.\n", __func__, on ? "on" : "off");

	if (on) {
		gpio_set_value(PICO_GPIO_LCM_2v85_EN, 1);
		hr_msleep(20);
		gpio_set_value(PICO_GPIO_LCM_1v8_EN, 1);

		gpio_set_value(PICO_GPIO_LCD_RST_N, 0);
		hr_msleep(10);
		gpio_set_value(PICO_GPIO_LCD_RST_N, 1);
		hr_msleep(5);
	} else {
		gpio_set_value(PICO_GPIO_LCD_RST_N, 0);

		gpio_set_value(PICO_GPIO_LCM_2v85_EN, 0);

		gpio_set_value(PICO_GPIO_LCM_1v8_EN, 0);
	}
}

static int mipi_panel_power(int on)
{
	int flag_on = !!on;

	if (mipi_power_save_on == flag_on)
		return 0;

	mipi_power_save_on = flag_on;

	pico_panel_power(on);	

	return 0;
}

enum {
	DSI_SINGLE_LANE = 1,
	DSI_TWO_LANES,
};

static int msm_fb_get_lane_config(void)
{
	int rc = DSI_SINGLE_LANE;

	PR_DISP_INFO("DSI Single Lane\n");

	return rc;
}

static struct mipi_dsi_platform_data mipi_dsi_pdata = {
	.vsync_gpio		= 97,
	.dsi_power_save	= mipi_panel_power,
	.get_lane_config	= msm_fb_get_lane_config,
};

#define BRI_SETTING_MIN                 30
#define BRI_SETTING_DEF                 142
#define BRI_SETTING_MAX                 255

static unsigned char pico_shrink_pwm(int val)
{
	unsigned int pwm_min, pwm_default, pwm_max;
	unsigned char shrink_br = BRI_SETTING_MAX;

	if (panel_type == PANEL_ID_PIO_AUO) {
		pwm_min = 10;
		pwm_default = 115;
		pwm_max = 217;
	} else {
		pwm_min = 10;
		pwm_default = 133;
		pwm_max = 255;
	}

	if (val <= 0) {
		shrink_br = 0;
	} else if (val > 0 && (val < BRI_SETTING_MIN)) {
			shrink_br = pwm_min;
	} else if ((val >= BRI_SETTING_MIN) && (val <= BRI_SETTING_DEF)) {
			shrink_br = (val - BRI_SETTING_MIN) * (pwm_default - pwm_min) /
		(BRI_SETTING_DEF - BRI_SETTING_MIN) + pwm_min;
	} else if (val > BRI_SETTING_DEF && val <= BRI_SETTING_MAX) {
			shrink_br = (val - BRI_SETTING_DEF) * (pwm_max - pwm_default) /
		(BRI_SETTING_MAX - BRI_SETTING_DEF) + pwm_default;
	} else if (val > BRI_SETTING_MAX)
			shrink_br = pwm_max;


	return shrink_br;
}

static struct msm_panel_common_pdata mipi_pico_panel_data = {

};

static struct platform_device mipi_dsi_cmd_hvga_panel_device = {
	.name = "mipi_novatek",
	.id = 0,
	.dev = {
		.platform_data = &mipi_pico_panel_data,
	}
};

static int msm_fb_detect_panel(const char *name)
{
	if (panel_type == PANEL_ID_PIO_AUO) {
		if (!strcmp(name, "mipi_cmd_novatek_hvga"))
			return 0;
	} else if ((panel_type == PANEL_ID_PIO_SAMSUNG) | (panel_type == PANEL_ID_PIO_SAMSUNG_C2)) {
		if (!strcmp(name, "mipi_cmd_samsung_hvga"))
			return 0;
	}

	PR_DISP_WARN("%s: not supported '%s'\n", __func__, name);
	return -ENODEV;
}

static struct msm_fb_platform_data msm_fb_pdata = {
	.detect_client = msm_fb_detect_panel,
	.width = 48,
	.height = 72,
};

static struct resource msm_fb_resources[] = {
	{
		.start = MSM_FB_BASE,
		.end = MSM_FB_BASE + MSM_FB_SIZE - 1,
		.flags = IORESOURCE_MEM,
	},
};

static struct platform_device msm_fb_device = {
	.name   = "msm_fb",
	.id     = 0,
	.num_resources     = ARRAY_SIZE(msm_fb_resources),
	.resource          = msm_fb_resources,
	.dev.platform_data = &msm_fb_pdata,
};

static struct msm_panel_common_pdata mdp_pdata = {
	.cont_splash_enabled = 0x00,
	.gpio = 97,
	.mdp_rev = MDP_REV_303,
};

static void __init msm_fb_add_devices(void)
{
	PR_DISP_INFO("panel ID= 0x%x\n", panel_type);
	msm_fb_register_device("mdp", &mdp_pdata);

	if (panel_type != PANEL_ID_NONE)
		msm_fb_register_device("mipi_dsi", &mipi_dsi_pdata);
}

extern int mipi_status;
#define DEFAULT_BRIGHTNESS 143
extern int bl_level_prevset;
extern char ptype[60];

static struct mipi_dsi_panel_platform_data *mipi_novatek_pdata;
static struct mipi_dsi_panel_platform_data *mipi_samsung_pdata;

static struct dsi_buf novatek_tx_buf;
static struct dsi_buf novatek_rx_buf;
static struct dsi_buf samsung_tx_buf;
static struct dsi_buf samsung_rx_buf;

static int mipi_novatek_lcd_init(void);
static int mipi_samsung_lcd_init(void);

static unsigned char bkl_enable_cmds[] = {0x53, 0x24};/* DTYPE_DCS_WRITE1 */
static unsigned char bkl_disable_cmds[] = {0x53, 0x00};/* DTYPE_DCS_WRITE1 */

static char sw_reset[2] = {0x01, 0x00}; /* DTYPE_DCS_WRITE */
static char enter_sleep[2] = {0x10, 0x00}; /* DTYPE_DCS_WRITE */
static char exit_sleep[2] = {0x11, 0x00}; /* DTYPE_DCS_WRITE */
static char display_off[2] = {0x28, 0x00}; /* DTYPE_DCS_WRITE */
static char display_on[2] = {0x29, 0x00}; /* DTYPE_DCS_WRITE */


/* Pico panel initial setting */
	if (panel_type == PANEL_ID_PIO_AUO) {
static char pio_f0[] = {
	/* DTYPE_DCS_LWRITE */
	0xF0, 0xAA, 0x55, 0x52
};
static char pio_f6[] = {
	/* DTYPE_DCS_LWRITE */
	0xF6, 0x05, 0x70, 0x65
};
static char pio_b4[] = {
	/* DTYPE_DCS_LWRITE */
	0xB4, 0x03, 0x03, 0x03
};
static char pio_bf[] = {
	/* DTYPE_DCS_LWRITE */
	0xBF, 0x03, 0x08, 0x00
};
static char pio_b8[] = {
	/* DTYPE_DCS_LWRITE */
	0xB8, 0xEF, 0x02, 0xFF
};
static char pio_b0[] = {
	/* DTYPE_DCS_LWRITE */
	0xB0, 0x00, 0x7A, 0xFF
};
static char pio_b1[] = {
	/* DTYPE_DCS_LWRITE */
	0xB1, 0xB7, 0x01, 0x28
};
static char pio_b2[] = {
	/* DTYPE_DCS_LWRITE */
	0xB2, 0xB7, 0x01, 0x28
};
static char pio_b3[] = {
	/* DTYPE_DCS_LWRITE */
	0xB3, 0xB7, 0x01, 0x28
};
static char pio_c0[] = {
	/* DTYPE_DCS_LWRITE */
	0xC0, 0x78, 0x78, 0x00,
	0x00, 0x00, 0x00, 0x00
};
static char pio_c1[] = {
	/* DTYPE_DCS_LWRITE */
	0xC1, 0x46, 0x46, 0x46,
	0xF2, 0x02, 0x00, 0xFF
};
static char pio_c2[] = {
	/* DTYPE_DCS_LWRITE */
	0xC2, 0x01, 0x04, 0x01
};
static char pio_26[2] = {0x26, 0x10}; /* DTYPE_DCS_WRITE1 */
static char pio_e0[20] = {
	/* DTYPE_DCS_LWRITE */
	0xE0, 0x45, 0x48, 0x51, 0x5A, 0x17, 0x29, 0x5A, 0x3D, 0x1E,
	0x28, 0x7E, 0x19, 0x3E, 0x4C, 0x66, 0x99, 0x30, 0x71, 0xFF
};
static char pio_e1[20] = {
	/* DTYPE_DCS_LWRITE */
	0xE1, 0x45, 0x48, 0x51, 0x5A, 0x17, 0x29, 0x5A, 0x3D, 0x1E,
	0x28, 0x7E, 0x19, 0x3E, 0x4C, 0x66, 0x99, 0x30, 0x71, 0xFF
};
static char pio_e2[20] = {
	/* DTYPE_DCS_LWRITE */
	0xE2, 0x21, 0x27, 0x39, 0x49, 0x20, 0x34, 0x61, 0x42, 0x1D,
	0x27, 0x82, 0x1C, 0x46, 0x55, 0x64, 0x89, 0x2A, 0x71, 0xFF
};
static char pio_e3[20] = {
	/* DTYPE_DCS_LWRITE */
	0xE3, 0x21, 0x27, 0x39, 0x49, 0x20, 0x34, 0x61, 0x42, 0x1D,
	0x27, 0x82, 0x1C, 0x46, 0x55, 0x64, 0x89, 0x2A, 0x71, 0xFF
};
static char pio_e4[20] = {
	/* DTYPE_DCS_LWRITE */
	0xE4, 0x40, 0x52, 0x6A, 0x77, 0x20, 0x33, 0x60, 0x50, 0x1C,
	0x24, 0x86, 0x0D, 0x29, 0x35, 0xF4, 0xF0, 0x70, 0x7F, 0xFF
};
static char pio_e5[20] = {
	/* DTYPE_DCS_LWRITE */
	0xE5, 0x40, 0x52, 0x6A, 0x77, 0x20, 0x33, 0x60, 0x50, 0x1C,
	0x24, 0x86, 0x0D, 0x29, 0x35, 0xF4, 0xF0, 0x70, 0x7F, 0xFF
};
static char pio_pwm2[2] = {0x53, 0x24}; /* DTYPE_DCS_WRITE1 */
static char pio_cb[] = {
	/* DTYPE_DCS_LWRITE */
	0xCB, 0x80, 0x04, 0xFF
};
static char pio_fe[2] = {0xFE, 0x08};/* DTYPE_DCS_WRITE1 */
static char pio_enable_te[2] = {0x35, 0x00};/* DTYPE_DCS_WRITE */
static char peripheral_on[2] = {0x00, 0x00}; /* DTYPE_DCS_READ */

} else {

static char pio_f0[] = {
	/* DTYPE_DCS_LWRITE */
	0xF0, 0x5A, 0x5A, 0x00
};
static char pio_f1[] = {
	/* DTYPE_DCS_LWRITE */
	0xF1, 0x5A, 0x5A, 0x00
};
static char pio_f2[] = {
	/* DTYPE_DCS_LWRITE */
	0xF2, 0x3C, 0x7E, 0x03,
	0x18, 0x18, 0x02, 0x10,
	0x00, 0x2F, 0x10, 0xC8,
	0x5D, 0x5D, 0x00, 0x00
};
static char pio_fc[] = {
	/* DTYPE_DCS_LWRITE */
	0xFC, 0xA5, 0xA5, 0x00
};
static char pio_fd[] = {
	/* DTYPE_DCS_LWRITE */
	0xFD, 0x00, 0x00, 0x00,
	0x02, 0x4F, 0x92, 0x41,
	0x54, 0x49, 0x0C, 0x4C,
	0x8C, 0x70, 0xBF, 0x15
};
static char pio_f4[] = {
	/* DTYPE_DCS_LWRITE */
	0xF4, 0x02, 0xAE, 0x43,
	0x43, 0x43, 0x43, 0x50,
	0x32, 0x22, 0x54, 0x51,
	0x20, 0x2A, 0x2A, 0x66
};
static char pio_C2_f4[] = {
	/* DTYPE_DCS_LWRITE */
	0xF4, 0x02, 0xAE, 0x43,
	0x43, 0x43, 0x43, 0x50,
	0x32, 0x13, 0x54, 0x51,
	0x20, 0x2A, 0x2A, 0x62
};
static char pio_f5[] = {
	/* DTYPE_DCS_LWRITE */
	0xF5, 0x4C, 0x75, 0x00
};
static char pio_C2_f5[] = {
	/* DTYPE_DCS_LWRITE */
	0xF5, 0x4A, 0x75, 0x00
};
static char pio_f6[] = {
	/* DTYPE_DCS_LWRITE */
	0xF6, 0x29, 0x02, 0x0F,
	0x00, 0x04, 0x77, 0x55,
	0x15, 0x00, 0x00, 0x00
};
static char pio_C2_f6[] = {
	/* DTYPE_DCS_LWRITE */
	0xF6, 0x2C, 0x02, 0x0F,
	0x00, 0x03, 0x22, 0x1D,
	0x00, 0x00, 0x00, 0x00
};
static char pio_f7[] = {
	/* DTYPE_DCS_LWRITE */
	0xF7, 0x01, 0x80, 0x12,
	0x02, 0x00, 0x00, 0x00
};
static char pio_f8[] = {
	/* DTYPE_DCS_LWRITE */
	0xF8, 0x55, 0x00, 0x14,
	0x09, 0x40, 0x00, 0x04,
	0x0A, 0x00, 0x00, 0x00
};
static char pio_C2_f8[] = {
	/* DTYPE_DCS_LWRITE */
	0xF8, 0x55, 0x00, 0x14,
	0x19, 0x40, 0x00, 0x04,
	0x0A, 0x00, 0x00, 0x00
};
static char pio_ed[2] = {0xED, 0x17}; /* DTYPE_DCS_WRITE1 */

static char pio_f9_1[2] = {0xF9, 0x04}; /* DTYPE_DCS_WRITE1 */
static char pio_fa_1[] = {
	/* DTYPE_DCS_LWRITE */
	0xFA, 0x09, 0x29, 0x04,
	0x15, 0x18, 0x0C, 0x0F,
	0x0E, 0x29, 0x22, 0x21,
	0x01, 0x0B, 0x00, 0x00
};
static char pio_C2_fa_1[] = {
	/* DTYPE_DCS_LWRITE */
	0xFA, 0x09, 0x29, 0x04,
	0x15, 0x18, 0x14, 0x15,
	0x10, 0x29, 0x22, 0x21,
	0x01, 0x0B, 0x00, 0x00
};
static char pio_fb_1[] = {
	/* DTYPE_DCS_LWRITE */
	0xFB, 0x03, 0x29, 0x00,
	0x0F, 0x16, 0x0E, 0x11,
	0x12, 0x25, 0x22, 0x21,
	0x01, 0x0B, 0x00, 0x00
};
static char pio_C2_fb_1[] = {
	/* DTYPE_DCS_LWRITE */
	0xFB, 0x03, 0x29, 0x00,
	0x0F, 0x16, 0x12, 0x14,
	0x12, 0x25, 0x22, 0x21,
	0x01, 0x0B, 0x00, 0x00
};
static char pio_f9_2[2] = {0xF9, 0x02}; /* DTYPE_DCS_WRITE1 */
static char pio_fa_2[] = {
	/* DTYPE_DCS_LWRITE */
	0xFA, 0x1A, 0x2B, 0x06,
	0x15, 0x17, 0x08, 0x0C,
	0x0A, 0x2C, 0x24, 0x1E,
	0x20, 0x20, 0x00, 0x00
};
static char pio_C2_fa_2[] = {
	/* DTYPE_DCS_LWRITE */
	0xFA, 0x1A, 0x2B, 0x06,
	0x15, 0x17, 0x0F, 0x0F,
	0x0A, 0x2A, 0x24, 0x1E,
	0x20, 0x20, 0x00, 0x00
};
static char pio_fb_2[] = {
	/* DTYPE_DCS_LWRITE */
	0xFB, 0x15, 0x2B, 0x00,
	0x0F, 0x15, 0x0A, 0x0E,
	0x0E, 0x28, 0x24, 0x1E,
	0x20, 0x20, 0x00, 0x00
};
static char pio_C2_fb_2[] = {
	/* DTYPE_DCS_LWRITE */
	0xFB, 0x15, 0x2B, 0x00,
	0x0F, 0x15, 0x0E, 0x0E,
	0x0E, 0x29, 0x24, 0x1E,
	0x20, 0x20, 0x00, 0x00
};
static char pio_f9_3[2] = {0xF9, 0x01}; /* DTYPE_DCS_WRITE1 */
static char pio_fa_3[] = {
	/* DTYPE_DCS_LWRITE */
	0xFA, 0x30, 0x2D, 0x08,
	0x14, 0x15, 0x00, 0x03,
	0x00, 0x35, 0x29, 0x08,
	0x00, 0x00, 0x00, 0x00
};
static char pio_C2_fa_3[] = {
	/* DTYPE_DCS_LWRITE */
	0xFA, 0x30, 0x2D, 0x08,
	0x14, 0x10, 0x00, 0x00,
	0x00, 0x35, 0x29, 0x08,
	0x00, 0x00, 0x00, 0x00
};
static char pio_fb_3[] = {
	/* DTYPE_DCS_LWRITE */
	0xFB, 0x2A, 0x2D, 0x00,
	0x0C, 0x11, 0x00, 0x05,
	0x06, 0x2F, 0x29, 0x08,
	0x00, 0x00, 0x00, 0x00
};
static char pio_C2_fb_3[] = {
	/* DTYPE_DCS_LWRITE */
	0xFB, 0x2A, 0x2D, 0x00,
	0x0C, 0x11, 0x00, 0x00,
	0x00, 0x38, 0x29, 0x08,
	0x00, 0x00, 0x00, 0x00
};
static char pio_c3[] = {
	/* DTYPE_DCS_LWRITE */
	0xC3, 0xD8, 0x00, 0x28
};
static char pio_c0[] = {
	/* DTYPE_DCS_LWRITE */
	0xC0, 0x80, 0x80, 0x00
};
}

static struct dsi_cmd_desc pico_auo_cmd_on_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 10,
		sizeof(sw_reset), sw_reset},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_f0), pio_f0},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_b4), pio_b4},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_bf), pio_bf},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_b8), pio_b8},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_b1), pio_b1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_b2), pio_b2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_b3), pio_b3},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_c0), pio_c0},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_c1), pio_c1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_c2), pio_c2},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,
		sizeof(pio_26), pio_26},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_e0), pio_e0},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_e1), pio_e1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_e2), pio_e2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_e3), pio_e3},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_e4), pio_e4},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_e5), pio_e5},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,
		sizeof(pio_pwm2), pio_pwm2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_cb), pio_cb},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,
		sizeof(pio_enable_te), pio_enable_te},
	{DTYPE_DCS_WRITE, 1, 0, 0, 120,
		sizeof(exit_sleep), exit_sleep},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 20,
		sizeof(pio_fe), pio_fe},
	{DTYPE_DCS_WRITE, 1, 0, 0, 0,
		sizeof(display_on), display_on},
};

static struct dsi_cmd_desc novatek_display_on_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 0,
		sizeof(display_on), display_on},
};

static struct dsi_cmd_desc novatek_display_off_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 0,
		sizeof(display_off), display_off},
	{DTYPE_DCS_WRITE, 1, 0, 0, 120,
		sizeof(enter_sleep), enter_sleep}
};

static struct dsi_cmd_desc pico_samsung_cmd_on_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 10,
		sizeof(sw_reset), sw_reset},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_f0), pio_f0},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_f1), pio_f1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_f2), pio_f2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_f4), pio_f4},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_f5), pio_f5},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_f6), pio_f6},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_f7), pio_f7},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_f8), pio_f8},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,
		sizeof(pio_ed), pio_ed},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,
		sizeof(pio_f9_1), pio_f9_1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_fa_1), pio_fa_1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_fb_1), pio_fb_1},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,
		sizeof(pio_f9_2), pio_f9_2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_fa_2), pio_fa_2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_fb_2), pio_fb_2},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,
		sizeof(pio_f9_3), pio_f9_3},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_fa_3), pio_fa_3},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_fb_3), pio_fb_3},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_c3), pio_c3},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,
		sizeof(bkl_cabc_cmds), bkl_cabc_cmds},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,
		sizeof(rgb_888), rgb_888},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_c0), pio_c0},
	{DTYPE_DCS_WRITE, 1, 0, 0, 120,
		sizeof(exit_sleep), exit_sleep},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,
		sizeof(bkl_enable_cmds), bkl_enable_cmds},
	{DTYPE_DCS_WRITE, 1, 0, 0, 0,
		sizeof(display_on), display_on},
};

static struct dsi_cmd_desc pico_samsung_C2_cmd_on_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 10,
		sizeof(sw_reset), sw_reset},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_f0), pio_f0},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_f1), pio_f1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_f2), pio_f2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_fc), pio_fc},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_fd), pio_fd},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_C2_f4), pio_C2_f4},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_C2_f5), pio_C2_f5},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_C2_f6), pio_C2_f6},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_f7), pio_f7},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_C2_f8), pio_C2_f8},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,
		sizeof(pio_ed), pio_ed},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,
		sizeof(pio_f9_1), pio_f9_1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_C2_fa_1), pio_C2_fa_1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_C2_fb_1), pio_C2_fb_1},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,
		sizeof(pio_f9_2), pio_f9_2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_C2_fa_2), pio_C2_fa_2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_C2_fb_2), pio_C2_fb_2},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,
		sizeof(pio_f9_3), pio_f9_3},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_C2_fa_3), pio_C2_fa_3},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_C2_fb_3), pio_C2_fb_3},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_c3), pio_c3},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(pio_c0), pio_c0},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,
		sizeof(bkl_cabc_cmds), bkl_cabc_cmds},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,
		sizeof(rgb_888), rgb_888},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,
		sizeof(enable_te), enable_te},
	{DTYPE_DCS_WRITE, 1, 0, 0, 120,
		sizeof(exit_sleep), exit_sleep},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,
		sizeof(bkl_enable_cmds), bkl_enable_cmds},
	{DTYPE_DCS_WRITE, 1, 0, 0, 0,
		sizeof(display_on), display_on},
};

static struct dsi_cmd_desc samsung_display_off_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 0,
		sizeof(display_off), display_off},
	{DTYPE_DCS_WRITE, 1, 0, 0, 120,
		sizeof(enter_sleep), enter_sleep}
};


static int pico_lcd_on(struct platform_device *pdev)
{

	struct msm_fb_data_type *mfd;
	struct mipi_panel_info *mipi;
	struct msm_panel_info *pinfo;
	struct dcs_cmd_req cmdreq;

	mfd = platform_get_drvdata(pdev);
	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

	pinfo = &mfd->panel_info;
	
	mipi  = &mfd->panel_info.mipi;
	
			switch (panel_type) {
				case PANEL_ID_PIO_AUO:
			mipi_dsi_cmds_tx(&novatek_tx_buf, pico_auo_cmd_on_cmds,
				ARRAY_SIZE(pico_auo_cmd_on_cmds));
				case PANEL_ID_PIO_SAMSUNG:
			mipi_dsi_cmds_tx(&samsung_tx_buf, pico_samsung_cmd_on_cmds,
				ARRAY_SIZE(pico_samsung_cmd_on_cmds));
				case PANEL_ID_PIO_SAMSUNG_C2:
			mipi_dsi_cmds_tx(&samsung_tx_buf, pico_samsung_C2_cmd_on_cmds,
				ARRAY_SIZE(pico_samsung_C2_cmd_on_cmds));
				}

	return 0;
}

static int pico_lcd_off(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;
	struct dcs_cmd_req cmdreq;

	mfd = platform_get_drvdata(pdev);

	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;
		
			switch (panel_type) {
				case PANEL_ID_PIO_AUO:
			mipi_dsi_cmds_tx(&novatek_tx_buf, novatek_display_off_cmds,
				ARRAY_SIZE(novatek_display_off_cmds));
				case PANEL_ID_PIO_SAMSUNG:
				case PANEL_ID_PIO_SAMSUNG_C2:
			mipi_dsi_cmds_tx(&samsung_tx_buf, samsung_display_off_cmds,
				ARRAY_SIZE(samsung_display_off_cmds));
				}



	return 0;
}

static int pico_lcd_late_init(struct platform_device *pdev)
{
	return 0;
}

DEFINE_LED_TRIGGER(bkl_led_trigger);

static char led_pwm1[2] = {0x51, 0x0};	/* DTYPE_DCS_WRITE1 */
static struct dsi_cmd_desc backlight_cmd[] = {
	DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(led_pwm1), led_pwm1};


static void pico_set_backlight(struct msm_fb_data_type *mfd)
{
			switch (panel_type) {
				case PANEL_ID_PIO_AUO:
	if (mipi_novatek_pdata && mipi_novatek_pdata->gpio_set_backlight)
		mipi_novatek_pdata->gpio_set_backlight(mfd->bl_level);
		
	if ((mipi_novatek_pdata->enable_wled_bl_ctrl) && (wled_trigger_initialized))
		led_trigger_event(bkl_led_trigger, mfd->bl_level);
		
		led_pwm1[1] = (unsigned char)mfd->bl_level;
		
		mipi_dsi_cmds_tx(&novatek_tx_buf, backlight_cmd,
				ARRAY_SIZE(backlight_cmd));
				case PANEL_ID_PIO_SAMSUNG:
				case PANEL_ID_PIO_SAMSUNG_C2:
	if (mipi_samsung_pdata && mipi_samsung_pdata->gpio_set_backlight)
		mipi_samsung_pdata->gpio_set_backlight(mfd->bl_level);			
		
	if ((mipi_samsung_pdata->enable_wled_bl_ctrl) && (wled_trigger_initialized))
		led_trigger_event(bkl_led_trigger, mfd->bl_level);
		
		led_pwm1[1] = (unsigned char)mfd->bl_level;
		
		mipi_dsi_cmds_tx(&samsung_tx_buf, samsung_cmd_backlight_cmds,
				ARRAY_SIZE(samsung_cmd_backlight_cmds));
				}

			}
		
static struct platform_driver this_driver = {
	.probe  = mipi_novatek_lcd_probe,
	.driver = {
		.name   = "mipi_novatek",
	},
};

static struct msm_fb_panel_data novatek_panel_data = {
	.on			= pico_lcd_on,
	.off			= pico_lcd_off,
	.late_init		= pico_lcd_late_init,
	.set_backlight		= pico_set_backlight,
};

/*
TODO:
1.find a better way to handle msm_fb_resources, to avoid passing it across file.
2.error handling
 */
int __init pico_init_panel(void)
{
	int ret=0;

	if ((panel_type == PANEL_ID_PIO_SAMSUNG) | (panel_type == PANEL_ID_PIO_SAMSUNG_C2))
		mipi_dsi_cmd_hvga_panel_device.name = "mipi_samsung";
	else if (panel_type == PANEL_ID_PIO_AUO)
		mipi_dsi_cmd_hvga_panel_device.name = "mipi_novatek";
	else
		pico_panel_power(0);

	PR_DISP_INFO("panel_type= 0x%x\n", panel_type);
	PR_DISP_INFO("%s: %s\n", __func__, mipi_dsi_cmd_hvga_panel_device.name);


	ret = platform_device_register(&msm_fb_device);
	ret = platform_device_register(&mipi_dsi_cmd_hvga_panel_device);

	msm_fb_add_devices();

	return 0;
}
