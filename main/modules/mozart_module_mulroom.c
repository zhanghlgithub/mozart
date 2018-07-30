#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <sys/time.h>

#include "ini_interface.h"
#include "player_interface.h"
#include "mulroom_interface.h"

#include "modules/mozart_module_mulroom.h"

#define MODULE_MR_DEBUG

#ifdef MODULE_MR_DEBUG
#define pr_debug(fmt, args...)                  \
	printf("[Module.MultiRoom][Debug] -%d- %s. "fmt, __LINE__, __func__, ##args)
#define fpr_debug(fmt, args...)                 \
	fprintf(stderr, "[Module.MultiRoom][Debug] -%d- %s. "fmt, __LINE__, __func__, ##args)
#else
#define pr_debug(fmt, args...)
#define fpr_debug(fmt, args...)
#endif /* MODULE_MR_DEBUG */

#define pr_err(fmt, args...)                    \
	fprintf(stderr, "[Module.MultiRoom][Error] -%d- %s. "fmt, __LINE__, __func__, ##args)

#define pr_warn(fmt, args...)                   \
	printf("[Module.MultiRoom][Warn] -%d- %s. "fmt, __LINE__, __func__, ##args)

#define pr_info(fmt, args...)                   \
	printf("[Module.MultiRoom][Info] -%d- %s. "fmt, __LINE__, __func__, ##args)

static char *ao_mulroom_distributor = "stream:sockfile=/var/run/mulroom/stream.sock";
static char *ini_path = "/usr/data/player.ini";
static char ao_mulroom_normal[128];

static int normal_play_interface_get(char *ini_path, char *ao_iface)
{
	return mozart_ini_getkey(ini_path, "mplayer", "ao", ao_iface);
}

static int mulroom_change_audio_output(char *ao_iface)
{
	player_handler_t *handle;
	int err;

	handle = mozart_player_handler_get("multiroom", NULL, NULL);
	if (!handle)
		return -1;

	err = mozart_player_aoswitch(handle, ao_iface);
	mozart_player_handler_put(handle);

	return err;
}

int module_mulroom_audio_change(mulroom_ao_mode_t mode)
{
	int err;

	switch (mode) {
	case MR_AO_NORMAL:
		err = normal_play_interface_get(ini_path, ao_mulroom_normal);
		if (err) {
			pr_err("get ao normal interface failed\n");
			err = -1;
			break;
		}

		err = mulroom_change_audio_output(ao_mulroom_normal);
		if (err < 0)
			pr_err("change player ao normal failed");
		break;
	case MR_AO_DISTRIBUTOR:
		err = mulroom_change_audio_output(ao_mulroom_distributor);
		if (err < 0)
			pr_err("change player ao stream failed\n");
		break;
	default:
		pr_err("error mode code: %d\n", mode);
		err = -1;
	}

	return err;
}

static void double_to_tv(double d, struct timeval *tv)
{
	tv->tv_sec = (long)d;
	tv->tv_usec = (d - tv->tv_sec) * 1000000;
}

static int mulroom_update_time_change(double time_offset)
{
	struct timeval tv;
	enum mul_state m_state;
	int err = 0;

	m_state = mozart_mulroom_get_state(NULL);

	if (m_state != MUL_IDLE) {
		pr_debug("prepear to set mulroom change\n");

		double_to_tv(time_offset, &tv);
		err = mozart_mulroom_ntp_change(tv.tv_sec, tv.tv_usec);
		if (err < 0)
			pr_err("mulroom ntp change failed\n");
	}

	return err;
}

int module_mulroom_run_ntpd(void)
{
	char res_str[128] = {0};
	FILE *res_ptr;
	ssize_t res_len;

	char p_cmd[] = {
		"ntpd -nq 2>&1 | "
		"sed -n '$s/.*offset\\s\\([^0-9][0-9]\\+[^0-9][0-9]\\+\\)s)/\\1/p'"
	};

	pr_debug("run ntpd\n");

	res_ptr = popen(p_cmd, "re");
	if (!res_ptr) {
		pr_err("sync time popen: %s\n", strerror(errno));
		return -1;
	}

	res_len = fread(res_str, 1, 128, res_ptr);
	pclose(res_ptr);

	if (res_len) {
		double time_offset;

		pr_info("@@ %s\n", res_str);

		time_offset = strtod(res_str, NULL);
		if (mulroom_update_time_change(time_offset) < 0)
			return -1;
	} else {
		pr_info("ntpd has NOT changed system time\n");
	}

	return 0;
}
