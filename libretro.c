#include "defines.h"
#include "libretro.h"
#include "conf.h"
#include "tty.h"
#include "libretro-defs.h"

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <assert.h>

static const char *const focal10rom = "FOCAL10.ROM";
static const char *const basic10rom = "BASIC10.ROM"; 

retro_log_printf_t log_cb;
retro_video_refresh_t video_cb;
struct retro_vfs_interface *vfs_interface;

static retro_audio_sample_t audio_cb;
static retro_audio_sample_batch_t audio_batch_cb;

retro_environment_t environ_cb;
static retro_input_poll_t input_poll_cb;
retro_input_state_t input_state_cb;
static int joystick_enabled;

float frame_time = 0;
int breakpoint = -1;
extern unsigned short framebuf[512][512];
static int joystick_cur_state = 0;
static double aspect_ratio = 1;

void platform_joystick_init() {
}

d_word platform_joystick_get_state() {
  return joystick_cur_state;
}


static void fallback_log(enum retro_log_level level, const char *fmt, ...)
{
   va_list va;

   (void)level;

   va_start(va, fmt);
   vfprintf(stderr, fmt, va);
   va_end(va);
}

void retro_init(void)
{
}

void retro_deinit(void)
{
}

unsigned retro_api_version(void)
{
   return RETRO_API_VERSION;
}

static void
set_input_descs(void)
{
	   	static struct retro_input_descriptor desc[] = {
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,  "Left" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,    "Up" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,  "Down" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "Right" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A, "1" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B, "2" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X, "3" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y, "4" },

		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,  "Left" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,    "Up" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,  "Down" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "Right" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A, "1" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B, "2" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X, "3" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y, "4" },
		{ 0 },
	};

	environ_cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, desc);
}

void retro_set_controller_port_device(unsigned port, unsigned device)
{
	(void)port;
	(void)device;
	set_input_descs();
}

void retro_get_system_info(struct retro_system_info *info)
{
   memset(info, 0, sizeof(*info));
   info->library_name     = "bk";
#ifndef GIT_VERSION
#define GIT_VERSION ""
#endif
   info->library_version  = "v1.0" GIT_VERSION;
   info->need_fullpath    = false;
   info->valid_extensions = "bin|img|dsk|bkd";
}

#define FPS 50
#define SAMPLE_RATE io_sound_freq

void retro_get_system_av_info(struct retro_system_av_info *info)
{
   info->timing.fps = FPS;
   info->timing.sample_rate = SAMPLE_RATE;

   info->geometry.base_width   = 512;
   info->geometry.base_height  = 512;
   info->geometry.max_width    = 512;
   info->geometry.max_height   = 512;
   info->geometry.aspect_ratio = aspect_ratio;
}

enum {
	SUBSYSTEM_2_FLOPPIES = 2,
	SUBSYSTEM_3_FLOPPIES,
	SUBSYSTEM_4_FLOPPIES,
};

void set_options_v1(retro_environment_t cb) {
	static struct retro_variable variables[] =
	{
		{
			"bk_model",
			"Model (restart); BK-0010|BK-0010.01|BK-0010.01 + FDD|BK-0011M + FDD|Terak 8510/a|Slow BK-0011M",
		},
		{
			"bk_peripheral",
			"Peripheral (UP port, restart); none|covox|ay_3_8910|mouse_high|mouse_low|joystick",
		},
		{
			"bk_layout",
			"Keyboard layout; qwerty|jcuken",
		},
		{
			"bk_doublespeed",
			"Double CPU speed; disabled|enabled",
		},
		{
			"bk_color",
			"Use color display; enabled|disabled",
		},
		{
			"bk_keyboard_type",
			"Keyboard type (restart); poll|callback",
		},
		{
			"bk_aspect_ratio",
			"Aspect ratio; 1:1|4:3",
		},
		{ "input_repeat", "", },
		{ "input_kt", "", },
		{ "input_r2", "", },
		{ "input_l1", "", },
		{ "input_r1", "", },
		{ "input_indsu", "", },
		{ "input_block", "", },
		{ "input_step", "", },
		{ "input_reset", "", },
		{ "input_tab", "", },
		{ "input_vs", "", },
		{ "input_rus", "", },
		{ "input_lat", "", },
		{ "input_colormode", "", },
		{ NULL, NULL },
	};

	char buf[20][1024];
	char* inp_opt = "Tilde|Insert|Delete|Home|End|PageUP|PageDown|Tab|LShift|RShift|LAlt|RAlt|LCtrl|RCtrl|Keypad_0|Keypad_1|Keypad_2|Keypad_3|Keypad_4|Keypad_5|Keypad_6|Keypad_7|Keypad_8|Keypad_9|Keypad_Divide|Keypad_Multiply|Keypad_Add|Keypad_Substract|Keypad_Dot|Keypad_Enter|F1|F2|F3|F4|F5|F6|F7|F8|F9|F10|F11|F12";
	//Defaults
	snprintf(buf[0], sizeof(buf[0]), "Input -> Repeat; F1|%s", inp_opt); variables[7].value = buf[0];
	snprintf(buf[1], sizeof(buf[1]), "Input -> KT; F2|%s", inp_opt);     variables[8].value = buf[1];
	snprintf(buf[2], sizeof(buf[2]), "Input -> |--->; F5|%s", inp_opt);  variables[9].value = buf[2];
	snprintf(buf[3], sizeof(buf[3]), "Input -> |<---; F4|%s", inp_opt);  variables[10].value = buf[3];
	snprintf(buf[4], sizeof(buf[4]), "Input -> -|-->; F3|%s", inp_opt);  variables[11].value = buf[4];
	snprintf(buf[5], sizeof(buf[5]), "Input -> Ind Su; F6|%s", inp_opt); variables[12].value = buf[5];
	snprintf(buf[6], sizeof(buf[6]), "Input -> Block; F7|%s", inp_opt);  variables[13].value = buf[6];
	snprintf(buf[7], sizeof(buf[7]), "Input -> Step; F8|%s", inp_opt);   variables[14].value = buf[7];
	snprintf(buf[8], sizeof(buf[8]), "Input -> Reset; F9|%s", inp_opt);  variables[15].value = buf[8];
	snprintf(buf[9], sizeof(buf[9]), "Input -> Tab; Tab|%s", inp_opt);   variables[16].value = buf[9];
	snprintf(buf[10], sizeof(buf[10]), "Input -> Vs; Home|%s", inp_opt); variables[17].value = buf[10];
	snprintf(buf[11], sizeof(buf[11]), "Input -> Rus; F11|%s", inp_opt); variables[18].value = buf[11];
	snprintf(buf[12], sizeof(buf[12]), "Input -> Lat; F12|%s", inp_opt); variables[19].value = buf[12];
	snprintf(buf[13], sizeof(buf[13]), "Key -> Hotkey -> Color Mode; Keypad_Multiply|%s", inp_opt); variables[20].value = buf[13];
	cb(RETRO_ENVIRONMENT_SET_VARIABLES, variables);
	//Real options
	snprintf(buf[0], sizeof(buf[0]), "Input -> Repeat; %s", inp_opt); variables[7].value = buf[0];
	snprintf(buf[1], sizeof(buf[1]), "Input -> KT; %s", inp_opt);     variables[8].value = buf[1];
	snprintf(buf[2], sizeof(buf[2]), "Input -> |--->; %s", inp_opt);  variables[9].value = buf[2];
	snprintf(buf[3], sizeof(buf[3]), "Input -> |<---; %s", inp_opt);  variables[10].value = buf[3];
	snprintf(buf[4], sizeof(buf[4]), "Input -> -|-->; %s", inp_opt);  variables[11].value = buf[4];
	snprintf(buf[5], sizeof(buf[5]), "Input -> Ind Su; %s", inp_opt); variables[12].value = buf[5];
	snprintf(buf[6], sizeof(buf[6]), "Input -> Block; %s", inp_opt);  variables[13].value = buf[6];
	snprintf(buf[7], sizeof(buf[7]), "Input -> Step; %s", inp_opt);   variables[14].value = buf[7];
	snprintf(buf[8], sizeof(buf[8]), "Input -> Reset; %s", inp_opt);  variables[15].value = buf[8];
	snprintf(buf[9], sizeof(buf[9]), "Input -> Tab; %s", inp_opt);    variables[16].value = buf[9];
	snprintf(buf[10], sizeof(buf[10]), "Input -> Vs; %s", inp_opt);   variables[17].value = buf[10];
	snprintf(buf[11], sizeof(buf[11]), "Input -> Rus; %s", inp_opt);  variables[18].value = buf[11];
	snprintf(buf[12], sizeof(buf[12]), "Input -> Lat; %s", inp_opt);  variables[19].value = buf[12];
	snprintf(buf[13], sizeof(buf[13]), "Key -> Hotkey -> Color Mode; %s", inp_opt); variables[20].value = buf[13];
	cb(RETRO_ENVIRONMENT_SET_VARIABLES, variables);
}
void set_options_v2(retro_environment_t cb) {
	struct retro_core_option_v2_category option_cats[] = {
		{
			"system",
			"System",
			"Configure system parameters."
		},
		{
			"input",
			"Input",
			"Remap keyboard system keys."
		},
		{ NULL, NULL, NULL },
	};
	struct retro_core_option_v2_definition option_defs[] = {
		{
			"bk_model",						//Key
			"Model (restart)",				//Description
			NULL,							//descr_categorized
			"Model of the emulated computer.",
			NULL,							//info categorized
			"system",						//category_key
			{
				{ "BK-0010", "BK-0010" },
				{ "BK-0010.01", "BK-0010.01" },
				{ "BK-0010.01 + FDD", "BK-0010.01 + FDD" },
				{ "BK-0011M + FDD", "BK-0011M + FDD" },
				{ "Terak 8510/a", "Terak 8510/a" },
				{ "Slow BK-0011M", "Slow BK-0011M" },
				{ NULL, NULL },
			},
			"BK-0010",						//default_value
		},
		{
			"bk_peripheral",
			"Peripheral (UP port, restart)",
			NULL,
			NULL,
			NULL,
			"system",
			{
				{ "none", "None" },
				{ "covox", "Covox" },
				{ "ay_3_8910", "AY 3 8910" },
				{ "mouse_high", "Mouse (high)" },
				{ "mouse_low", "Mouse (low)" },
				{ "joystick", "Joystick" },
				{ NULL, NULL },
			},
			"none"
		},
		{
			"bk_doublespeed",
			"Double CPU speed",
			NULL,
			NULL,
			NULL,
			"system",
			{
				{ "disabled", NULL },
				{ "enabled", NULL },
				{ NULL, NULL },
			},
			"disabled"
		},
		{
			"bk_color",
			"Use color display",
			NULL,
			NULL,
			NULL,
			"system",
			{
				{ "enabled", NULL },
				{ "disabled", NULL },
				{ NULL, NULL },
			},
			"enabled"
		},
		{
			"bk_aspect_ratio",
			"Aspect ratio",
			NULL,
			NULL,
			NULL,
			"system",
			{
				{ "1:1", NULL },
				{ "4:3", NULL },
				{ NULL, NULL },
			},
			"1:1"
		},
		{
			"bk_layout",
			"Keyboard Layout",
			NULL,
			NULL,
			NULL,
			"input",
			{
				{ "qwerty", NULL },
				{ "jcuken", NULL },
				{ NULL, NULL },
			},
			"qwerty"
		},
		{
			"bk_keyboard_type",
			"Keyboard type (restart)",
			NULL,
			NULL,
			NULL,
			"input",
			{
				{ "poll", NULL },
				{ "callback", NULL },
				{ NULL, NULL },
			},
			"poll"
		},

		{ "input_repeat",	"Key -> Repeat",	NULL, NULL, NULL, "input", {{NULL,NULL}}, "F1" },
		{ "input_kt",		"Key -> KT",		NULL, NULL, NULL, "input", {{NULL,NULL}}, "F2" },
		{ "input_r2",		"Key -> |--->",		NULL, NULL, NULL, "input", {{NULL,NULL}}, "F5" },
		{ "input_l1",		"Key -> |<---",		NULL, NULL, NULL, "input", {{NULL,NULL}}, "F4" },
		{ "input_r1",		"Key -> -|-->",		NULL, NULL, NULL, "input", {{NULL,NULL}}, "F3" },
		{ "input_indsu",	"Key -> Ind Su",	NULL, NULL, NULL, "input", {{NULL,NULL}}, "F6" },
		{ "input_block",	"Key -> Block",		NULL, NULL, NULL, "input", {{NULL,NULL}}, "F7" },
		{ "input_step",		"Key -> Step",		NULL, NULL, NULL, "input", {{NULL,NULL}}, "F8" },
		{ "input_reset",	"Key -> Reset",		NULL, NULL, NULL, "input", {{NULL,NULL}}, "F9" },
		{ "input_tab",		"Key -> Tab",		NULL, NULL, NULL, "input", {{NULL,NULL}}, "Tab" },
		{ "input_vs",		"Key -> Vs",		NULL, NULL, NULL, "input", {{NULL,NULL}}, "Home" },
		{ "input_rus",		"Key -> Rus",		NULL, NULL, NULL, "input", {{NULL,NULL}}, "F11" },
		{ "input_lat",		"Key -> Lat",		NULL, NULL, NULL, "input", {{NULL,NULL}}, "F12" },
		{ "input_colormode",	"Key -> Hotkey -> Color Mode",	NULL, NULL, NULL, "input", {{NULL,NULL}}, "Keypad_Multiply" },

		{ NULL, NULL, NULL, NULL, NULL, NULL, {{0}}, NULL },
	};

	static const struct retro_core_option_value input_keys_values[RETRO_NUM_CORE_OPTION_VALUES_MAX] = {
		{ "Tilde", NULL },
		{ "Insert", NULL },
		{ "Delete", NULL },
		{ "Home", NULL },
		{ "End", NULL },
		{ "PageUP", NULL },
		{ "PageDown", NULL },
		{ "Tab", NULL },
		{ "LShift", NULL },
		{ "RShift", NULL },
		{ "LAlt", NULL },
		{ "RAlt", NULL },
		{ "LCtrl", NULL },
		{ "RCtrl", NULL },
		{ "Keypad_0", NULL },
		{ "Keypad_1", NULL },
		{ "Keypad_2", NULL },
		{ "Keypad_3", NULL },
		{ "Keypad_4", NULL },
		{ "Keypad_5", NULL },
		{ "Keypad_6", NULL },
		{ "Keypad_7", NULL },
		{ "Keypad_8", NULL },
		{ "Keypad_9", NULL },
		{ "Keypad_Divide", NULL },
		{ "Keypad_Multiply", NULL },
		{ "Keypad_Add", NULL },
		{ "Keypad_Substract", NULL },
		{ "Keypad_Dot", NULL },
		{ "Keypad_Enter", NULL },
		{ "F1", NULL },
		{ "F2", NULL },
		{ "F3", NULL },
		{ "F4", NULL },
		{ "F5", NULL },
		{ "F6", NULL },
		{ "F7", NULL },
		{ "F8", NULL },
		{ "F9", NULL },
		{ "F10", NULL },
		{ "F11", NULL },
		{ "F12", NULL },
		{ NULL, NULL },
	};
	for (int n = 7; n <= 20; n++)
		memcpy(&option_defs[n].values, &input_keys_values, sizeof input_keys_values);

	struct retro_core_options_v2 options_v2 = { option_cats, option_defs };
	cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2, &options_v2);
}

void retro_set_environment(retro_environment_t cb)
{
	struct retro_log_callback logging;
	bool no_rom = true;

	environ_cb = cb;

	cb(RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME, &no_rom);

	if (cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &logging))
		log_cb = logging.log;
	else
		log_cb = fallback_log;

	unsigned version = 0;
	if (!cb(RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION, &version)) version = 0;
	if (version >= 2)
		set_options_v2(cb);
	else
		set_options_v1(cb);

	struct retro_vfs_interface_info vfs_interface_info;
	vfs_interface_info.required_interface_version = 1;
	vfs_interface_info.iface = NULL;
	if (cb(RETRO_ENVIRONMENT_GET_VFS_INTERFACE, &vfs_interface_info))
		vfs_interface = vfs_interface_info.iface;

	set_input_descs();

	static const struct retro_controller_description port_user[] = {
		{ "None",              RETRO_DEVICE_NONE },
		{ "Joystick",          RETRO_DEVICE_JOYPAD },
		{ "Mouse",             RETRO_DEVICE_MOUSE },
		{ 0 },
	};

	static const struct retro_controller_description port_kbd[] = {
		{ "Keyboard",         RETRO_DEVICE_KEYBOARD },
		{ "Joystick",          RETRO_DEVICE_JOYPAD },
		{ 0 },
	};

	static struct retro_controller_info ci[] =
	{
		{
			.types = port_kbd,
			.num_types = 2
		},
		{
			.types = port_user,
			.num_types = 3
		},
		{
			NULL, 0
		}
	};

	static struct retro_subsystem_rom_info floppies[] = {
		{"Floppy A", "img|dsk|bkd", false, false, false, NULL, 0},
		{"Floppy B", "img|dsk|bkd", false, false, false, NULL, 0},
		{"Floppy C", "img|dsk|bkd", false, false, false, NULL, 0},
		{"Floppy D", "img|dsk|bkd", false, false, false, NULL, 0},
	};

	static struct retro_subsystem_info subsys[] = {
		{"2 floppies", "floppy2", floppies, 2, SUBSYSTEM_2_FLOPPIES },
		{"3 floppies", "floppy3", floppies, 3, SUBSYSTEM_3_FLOPPIES },
		{"4 floppies", "floppy4", floppies, 4, SUBSYSTEM_4_FLOPPIES },
		{ NULL, NULL, NULL, 0, 0 }
	};

	environ_cb(RETRO_ENVIRONMENT_SET_CONTROLLER_INFO, &ci);

	environ_cb(RETRO_ENVIRONMENT_SET_SUBSYSTEM_INFO, &subsys);
}

void retro_set_audio_sample(retro_audio_sample_t cb)
{
   audio_cb = cb;
}

void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb)
{
   audio_batch_cb = cb;
}

void retro_set_input_poll(retro_input_poll_t cb)
{
   input_poll_cb = cb;
}

void retro_set_input_state(retro_input_state_t cb)
{
   input_state_cb = cb;
}

void retro_set_video_refresh(retro_video_refresh_t cb)
{
   video_cb = cb;
}

#define MAX_SAMPLES_PER_FRAME 5000
static const int16_t zero_samples[MAX_SAMPLES_PER_FRAME * 2];

static void * game_data;
static size_t game_size;
static void *disks_buffer;
static size_t disks_total_size;
static size_t disk_size[4];

static void update_variables(bool startup)
{
	struct retro_variable var;
	if (startup) {	
		var.key = "bk_model";
		var.value = NULL;

		if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
		{
			if (strcmp (var.value, "BK-0010") == 0)
				bkmodel = 0;
			else if (strcmp (var.value, "BK-0010.01") == 0)
				bkmodel = 1;
			else if (strcmp (var.value, "BK-0010.01 + FDD") == 0)
				bkmodel = 2;
			else if (strcmp (var.value, "BK-0011M + FDD") == 0)
				bkmodel = 3;
			else if (strcmp (var.value, "Slow BK-0011M") == 0)
				bkmodel = 4;
			else if (strcmp (var.value, "Terak 8510/a") == 0) {
				bkmodel = 9;
				// Terak has no sound yet, turn sound off
				nflag = 0;
			} else
				bkmodel = 3;
		} else
			bkmodel = 3;
	}

	var.key = "bk_aspect_ratio";
	var.value = NULL;
	if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value != NULL && strcmp (var.value, "4:3") == 0) {
		aspect_ratio = 4.0/3.0;
	} else {
		aspect_ratio = 1.0;
	}

	tty_set_keymap();

	int old_cflag = cflag;
	var.key = "bk_color";
	var.value = NULL;
	if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value != NULL && strcmp (var.value, "disabled") == 0) {
		cflag = 0;
	} else
		cflag = 1;

	if (!startup && cflag != old_cflag) {
		scr_mark_dirty ();
	}
	
	if (bkmodel == 3 || bkmodel == 9)
		TICK_RATE = 4000000;
	else
		TICK_RATE = 3000000;

	var.key = "bk_doublespeed";
	var.value = NULL;
	if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value != NULL && strcmp (var.value, "enabled") == 0) {
		TICK_RATE *= 2;
	}

	/* Starting frame rate */ 
	frame_delay = TICK_RATE/25;
	half_frame_delay = TICK_RATE/50;
}

void retro_run(void)
{
	bool updated = false;
	if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &updated) && updated)
		update_variables(false);

	input_poll_cb();

	if (game_data && framectr > 2)
	{
                load_and_run_bin(game_data, game_size);
		free(game_data);
		game_data = NULL;
		game_size = 0;
	}

	if (mouseflag) {
		relx += input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_X);
		rely += input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_Y);
		mouse_button_state = mouse_but0 * (
			(input_state_cb(0, RETRO_DEVICE_MOUSE, 0,
					RETRO_DEVICE_ID_MOUSE_LEFT) ? 1 : 0)
			| (input_state_cb(0, RETRO_DEVICE_MOUSE, 0,
					  RETRO_DEVICE_ID_MOUSE_RIGHT) ? 2 : 0));
	}

	if (joystick_enabled) {
		int new_state = 0;
		for (int pad = 0; pad < 2; pad++) {
			new_state |= !!input_state_cb(pad, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A);
			new_state |= !!input_state_cb(pad, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B) << 1;
			new_state |= !!input_state_cb(pad, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X) << 2;
			new_state |= !!input_state_cb(pad, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y) << 3;
			new_state |= !!input_state_cb(pad, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT) << 4;
			new_state |= !!input_state_cb(pad, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN) << 5;
			new_state |= !!input_state_cb(pad, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT) << 9;
			new_state |= !!input_state_cb(pad, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP) << 10;
		}
		joystick_cur_state = new_state;
	}

	tty_poll();

  run_cpu_until(&pdp, ++framectr * TICK_RATE/FPS);

  scr_flush();

  video_cb(framebuf, 512, 512, 1024);
  if (!nflag) {
	  int samplegoal = SAMPLE_RATE / FPS;
	  if (samplegoal > MAX_SAMPLES_PER_FRAME)
		  samplegoal = MAX_SAMPLES_PER_FRAME;
	  audio_batch_cb(zero_samples, samplegoal);
  }
}

static int game_init_pixelformat(void)
{
   enum retro_pixel_format fmt = RETRO_PIXEL_FORMAT_RGB565;
   if (!environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt))
   {
      if (log_cb)
         log_cb(RETRO_LOG_INFO, "RGB565 is not supported.\n");
      return 0;
   }

   return 1;
}

static bool load_game_real(const char *image_path,
			   const void *bin, size_t bin_size,
			   const struct retro_game_info *floppy_info, size_t floppy_num) {
	const char *dir;
	int i;

	bool is_basic = false;
	bool is_focal = false;
	if (image_path) {
                char *slash = strrchr(image_path, '/');
                if (!slash) slash = strrchr(image_path, '\\');
                if (slash) {
                        tape_prefix = strdup(image_path);
                        tape_prefix[slash - image_path + 1] = '\0';
						tape_suffix = slash + 1;
						//fprintf(stderr, "Tape suffix is: <%s>\n", tape_suffix);
                }
				is_basic = strstr(strtoupper(image_path), "BASIC") != NULL;
				is_focal = strstr(strtoupper(image_path), "FOCAL") != NULL;
        }
	
	if (bin && !is_basic && !is_focal) {
		void *gd = malloc(bin_size);
		if (!gd)
			return false;
                memcpy (gd, bin, bin_size);
		game_data = gd;
		game_size = bin_size;
	}

	disks_total_size = 0;
	for (i = 0; i < floppy_num; i++)
		disks_total_size += (floppy_info[i].size && floppy_info[i].data) ? floppy_info[i].size : 0;
	disks_buffer = malloc(disks_total_size);
	if (!disks_buffer)
		return false;

	char *diskptr = disks_buffer;
	for (i = 0; i < floppy_num; i++) {
		size_t sz = (floppy_info[i].size && floppy_info[i].data) ? floppy_info[i].size : 0;
		if (sz) {
			memcpy (diskptr, floppy_info[i].data, sz);
		}
		diskptr += sz;
		disk_size[i] = sz;
	}
	for (; i < 4; i++) {
		disk_size[i] = 0;
	}

	nflag = 1;		/* enable sound */
	/* nothing is connected to the port by default, use ~/.bkrc */

	/* Set ROM configuration */

	if (environ_cb(RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY, &dir) && dir && dir[0])
	{
		char *cd = malloc (strlen(dir) + 20);
		assert(cd != NULL);
		strcpy(cd, dir);
		strcat(cd, "/bk");
		romdir = cd;
	}

	update_variables(true);
	
	if (is_basic) bkmodel = 1;
	else if (is_focal) bkmodel = 0;

	switch( bkmodel ) {
	case 0: /* BK0010 */
		rompath10 = monitor10rom;
		rompath12 = focal10rom;
		rompath16 = 0;
		break;
	case 1: /* BK0010.01 */
		rompath10 = monitor10rom;
		rompath12 = basic10rom;
		rompath16 = 0;
		break;
	case 2: /* BK0010.01+FDD */
		rompath10 = monitor10rom;
		rompath12 = 0;
		rompath16 = diskrom;
		break;
	case 3:	/* BK-0011M */
	case 9: /* Terak 8510/a */
		rompath10 = rompath12 = rompath16 = 0;
		break;
	case 4: /* Slow BK-0011M */
		rompath10 = rompath12 = rompath16 = 0;
		break;
	default: /* Unknown ROM configuration */
	  log_cb(RETRO_LOG_ERROR, "Unknown BK model. Bailing out.\n");
		exit( -1 );
	}

	/* Convert BK model to 0010/0011 flag */
	fake_disk = bkmodel >= 2;
	terak = bkmodel == 9;
	bkmodel = bkmodel >= 3;
	tty_open();             /* initialize the tty stuff */
	ev_init();		/* initialize the event system */
	sim_init();		/* ...the simulated cpu */
	mem_init();		/* ...main memory */
	bk_scr_init();		/* video display */
	if (!boot_init(1))
	  return false;		/* ROM blocks */
	q_reset();             /* ...any devices */

	mouseflag = 0;
	joystick_enabled = 0;
	if (!terak) {
		struct retro_variable var;
		var.key = "bk_peripheral";
		var.value = NULL;
		if (!environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) || var.value == NULL) {
			var.value = "none";
		}
		if (strcmp (var.value, "covox") == 0)		
			plug_covox();
		if (strcmp(var.value, "ay_3_8910") == 0)
			plug_synth();
		if (strcmp(var.value, "mouse_low") == 0)
			mouseflag = 1;
		if (strcmp(var.value, "mouse_high") == 0)
			mouseflag = 2;
		if (mouseflag) {
			mouse_init();
			plug_mouse();
		}
		if (strcmp(var.value, "joystick") == 0) {
		       joystick_enabled = 1;
		       plug_joystick();
		}
	}

	if (terak) {
		pdp.regs[PC] = 0173000;
	} else {
		lc_word(0177716, &pdp.regs[PC]);
		pdp.regs[PC] &= 0177400;
	}

	if (!game_init_pixelformat())
		return false;

	return true;
}

bool retro_load_game_special(unsigned type, const struct retro_game_info *info, size_t num)
{
	if (!info)
		return load_game_real(NULL, NULL, 0, NULL, 0);

	return load_game_real(info->path, NULL, 0, info, num);
}

bool retro_load_game(const struct retro_game_info *info)
{
	int isdisk = 0;
	const struct retro_game_info_ext *info_ext = NULL;
	const char *extension = NULL;

	if (!info)
		return load_game_real(NULL, NULL, 0, NULL, 0);

	if (environ_cb(RETRO_ENVIRONMENT_GET_GAME_INFO_EXT, &info_ext) &&
	    info_ext && (info_ext->file_in_archive ? info_ext->archive_file : info_ext->full_path)) {
		extension = strrchr(info_ext->file_in_archive ? info_ext->archive_file : info_ext->full_path, '.');
	} else if (info->path) {
		extension = strrchr(info->path, '.');
	}

	isdisk = (extension && (strcasecmp(extension, ".img") == 0
				|| strcasecmp(extension, ".dsk") == 0
				 || strcasecmp(extension, ".bkd") == 0));

	if (isdisk)
		return load_game_real(info->path, NULL, 0, info, 1);

	return load_game_real(info->path, info->data, info->size, NULL, 0);
}

void retro_unload_game(void)
{
}

void retro_reset(void)
{
	load_game_real(NULL, NULL, 0, NULL, 0);
}

unsigned retro_get_region(void)
{
   return RETRO_REGION_NTSC;
}

size_t retro_serialize_size(void)
{
	return sizeof (current_state);
}

bool retro_serialize(void *data_, size_t size)
{
	memcpy(data_, &current_state, sizeof (current_state));
	return true;
}

bool retro_unserialize(const void *data_, size_t size)
{
	if (size < sizeof (current_state))
		return false;

	memcpy(&current_state, data_, sizeof (current_state));
	scr_mark_dirty();
	return true;
}

void *retro_get_memory_data(unsigned id)
{
       switch(id)
       {
       case RETRO_MEMORY_SAVE_RAM: // SRAM
               return disks_total_size ? disks_buffer : NULL;
       case RETRO_MEMORY_SYSTEM_RAM:
	       return current_state._ram;
       }
       return NULL;
}

size_t retro_get_memory_size(unsigned id)
{
       switch(id)
       {
       case RETRO_MEMORY_SAVE_RAM: // SRAM
               return disks_total_size;
       case RETRO_MEMORY_SYSTEM_RAM:
	       return sizeof(current_state._ram);
       }
       return 0;
}

void retro_cheat_reset(void)
{}

void retro_cheat_set(unsigned index, bool enabled, const char *code)
{
   (void)index;
   (void)enabled;
   (void)code;
}

void platform_sound_flush() {
}

void sound_write_sample(short val) {
	soundctr++;
	audio_cb (val, val);
}

void sound_discard() {
}

void platform_sound_init() {
}

void platform_disk_init(disk_t *disks) {
	char *diskptr = disks_buffer;
	for (int i = 0; i < 4; i++) {
		if (disk_size[i]) {
			disks[i].length = disk_size[i];
			disks[i].ro = 0;
			disks[i].image = diskptr;
		} else {
			disks[i].length = 0;
			disks[i].ro = 0;
			disks[i].image = NULL;
		}
		diskptr += disk_size[i];
	}
}

struct libretro_handle
{
	FILE *stdio;
	struct retro_vfs_file_handle *lr;
};

struct libretro_handle *
libretro_vfs_open(const char *filename, const char *mode)
{
	if (!vfs_interface) {
		FILE *f = fopen(filename, mode);
		if (!f)
			return NULL;
		struct libretro_handle *ret = malloc(sizeof(*ret));
		assert(ret != NULL);
		ret->stdio = f;
		ret->lr = NULL;
		return ret;
	}

	assert((mode[0] == 'r' || mode[0] == 'w') && mode[1] == '\0');

	struct retro_vfs_file_handle *lr =
		vfs_interface->open(filename, mode[0] == 'r'
				    ? RETRO_VFS_FILE_ACCESS_READ
				    : RETRO_VFS_FILE_ACCESS_WRITE,
				    RETRO_VFS_FILE_ACCESS_HINT_NONE);
	if (!lr)
		return NULL;
	struct libretro_handle *ret = malloc(sizeof(*ret));
	assert(ret != NULL);
	ret->stdio = NULL;
	ret->lr = lr;
	return ret;	
}

void libretro_vfs_close(struct libretro_handle *h)
{
	if (h->lr)
		vfs_interface->close(h->lr);
	if(h->stdio)
		fclose(h->stdio);
	free (h);
}

int libretro_vfs_getc(struct libretro_handle *h)
{
	if (h->lr) {
		unsigned char c = 0;
		int r;
		r = vfs_interface->read(h->lr, &c, 1);
		if (r != 1)
			return -1;
		return c;
	}

	return fgetc(h->stdio);
}

void libretro_vfs_putc(int c, struct libretro_handle *h)
{
	if (h->lr) {
		unsigned char c0 = c;
		vfs_interface->write(h->lr, &c0, 1);
		return;
	}

	fputc(c, h->stdio);
}

void libretro_vfs_flush(struct libretro_handle *h)
{
	if (h->lr) {
		vfs_interface->flush(h->lr);
		return;
	}

	fflush(h->stdio);
}

int64_t libretro_vfs_get_size(struct libretro_handle *h)
{
	if (h->lr) {
		return vfs_interface->size(h->lr);
	}

	long old_pos = ftell(h->stdio);
	fseek(h->stdio, 0, SEEK_END);
	long ret = ftell(h->stdio);
	fseek(h->stdio, old_pos, SEEK_SET);
	return ret;
}

void libretro_vfs_read(struct libretro_handle *h, void *s, uint64_t len)
{
	if (h->lr) {
		vfs_interface->read(h->lr, s, len);
		return;
	}

	fread(s, 1, len, h->stdio);
}

void *load_rom_file(const char * rompath, size_t *sz, size_t min_sz, size_t max_sz)
{
	char *path = malloc(strlen(romdir)+strlen(rompath)+2);

	if (!path) {
		log_cb(RETRO_LOG_ERROR, "No memory");
		environ_cb(RETRO_ENVIRONMENT_SHUTDOWN, NULL);
		return NULL;
	}

	/* If rompath is a real path, do not apply romdir to it */
	if (*romdir && !strchr(rompath, '/'))
		sprintf(path, "%s/%s", romdir, rompath);
	else
		strcpy(path, rompath);

	log_cb(RETRO_LOG_INFO, "Loading %s...\n", path);

	struct libretro_handle *romf = libretro_vfs_open(path, "r");
	if (!romf) {
		char *optr;
		const char *iptr;
		if (*romdir && !strchr(rompath, '/'))
			sprintf(path, "%s/", romdir);
		for (optr = path + strlen(path), iptr = rompath; *iptr; optr++, iptr++)
			*optr = tolower(*iptr);
		*optr = '\0';
		log_cb(RETRO_LOG_INFO, "Attempting to load %s...\n", path);
		romf = libretro_vfs_open(path, "r");
	}

	if (!romf) {
		log_cb(RETRO_LOG_ERROR, "Couldn't open file.\n");
		environ_cb(RETRO_ENVIRONMENT_SHUTDOWN, NULL);
		return NULL;
	}

	size_t fsz = libretro_vfs_get_size(romf);
	if (fsz > max_sz)
		fsz = max_sz;

	if (fsz < min_sz) {
		log_cb(RETRO_LOG_ERROR, "Incomplete or damaged file.\n");
		environ_cb(RETRO_ENVIRONMENT_SHUTDOWN, NULL);
		return NULL;
	}

	char *ret = malloc (fsz + 1);
	libretro_vfs_read(romf, ret, fsz);
	libretro_vfs_close(romf);
	ret[fsz] = '\0';
		
	*sz = fsz;

	free(path);

	return ret;
}
