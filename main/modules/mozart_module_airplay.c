#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "mozart_config.h"
#include "utils_interface.h"
#include "sharememory_interface.h"
#include "ini_interface.h"

static char apname[64] = {};
static char ao_type[32] = {};
static char *cmdline[32] = {};

#define AIRPLAY_COMMAND_STOP	'4'
#define AIRPLAY_COMMAND_PAUSE	'5'
#define AIRPLAY_COMMAND_RESUME	'6'
#define AIRPLAY_COMMAND_EXIT	'7'

#define AIRPLAY_SERVER	"127.0.0.1"
#define AIRPLAY_PORT	13578

static int connect_to_tcp_server(char *ipaddr, int port)
{
	int sockfd = -1;
	struct sockaddr_in seraddr;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (-1 == sockfd) {
		printf("socket() error for connect to shairport failed: %s.\n", strerror(errno));
		return -1;
	}

	seraddr.sin_family = AF_INET;
	seraddr.sin_port = htons(port);
	seraddr.sin_addr.s_addr = inet_addr(ipaddr);

	if (connect(sockfd, (struct sockaddr *)&seraddr, sizeof(seraddr))) {
		printf("connect() error for connect to shairport failed: %s.\n", strerror(errno));
		close(sockfd);
		return -1;
	}

	return sockfd;
}

static int mozart_airplay_control(char cmd)
{
	int sockfd = -1;
	ssize_t wSize, rSize;
	char res;
	int err = 0;

	sockfd = connect_to_tcp_server(AIRPLAY_SERVER, AIRPLAY_PORT);
	if (sockfd < 0) {
		printf("connect to airplay error.\n");
		return -1;
	}

	wSize = send(sockfd, &cmd, 1, 0);
	if (wSize != 1) {
		printf("send stop command: %s\n", strerror(errno));
		err = -1;
		goto err_send;
	}

	rSize = recv(sockfd, &res, 1, 0);
	if (rSize != 1) {
		printf("recv res: %s\n", strerror(errno));
		err = -1;
	}

err_send:
	close(sockfd);

	return err;
}

int mozart_airplay_stop_playback(void)
{
	return mozart_airplay_control(AIRPLAY_COMMAND_STOP);
}

int mozart_airplay_pause(void)
{
	return mozart_airplay_control(AIRPLAY_COMMAND_PAUSE);
}

int mozart_airplay_resume(void)
{
	return mozart_airplay_control(AIRPLAY_COMMAND_RESUME);
}

int mozart_airplay_play_pause(void)
{
	module_status status;

	if (share_mem_get(AIRPLAY_DOMAIN, &status)) {
		printf("share_mem_get failure.\n");
		return -1;
	} else {
		if (status == STATUS_PLAYING)
			return mozart_airplay_pause();
		else if (status == STATUS_PAUSE)
			return mozart_airplay_resume();
	}

	return 0;
}

int mozart_airplay_init(void)
{
	int i = 0;
	int argc = 0;
	char mac[] = "00:11:22:33:44:55";
	char dev_playback[12] = "";

	// Parse playback dsp device.
	if(AUDIO_ALSA == get_audio_type()){
		strcpy(ao_type, "alsa");
	} else {
		strcpy(ao_type, "oss");
		strcat(ao_type, ":");
		if (mozart_ini_getkey("/usr/data/system.ini", "audio", "dev_playback", dev_playback)) {
			printf("[player/libaudio - OSS] Can't get record dsp device, force to /dev/dsp.\n");
			strcat(ao_type, "/dev/dsp");
		} else {
			strcat(ao_type, dev_playback);
		}
	}

	char *argv[] = {
		"shairport",
		"-o",
		ao_type,
		"-a",
		apname,
#if 0
		"-e",
		"/var/log/shairport.errlog",
#endif
#if 0 // soft volume set
		"-v",
		"soft",
#endif
		"-d",
		NULL
	};

	memset(mac, 0, sizeof(mac));
	memset(apname, 0, sizeof(apname));
	get_mac_addr("wlan0", mac, "");

	strcat(apname, "EGO-");
	strcat(apname, mac+4);

	argc = sizeof(argv)/sizeof(argv[0]);

	for (i = 0; i < argc; i++)
		cmdline[i] = argv[i];

	cmdline[0] = "shairport";
	cmdline[argc] = NULL;

	return 0;
}

int mozart_airplay_start_service(void)
{
	int i = 0;
	int ret = 0;
	char cmd[128] = {};

	ret = mozart_airplay_init();
	if (ret) {
		printf("mozart_airplay_init error.");
		return ret;
	}

#define CMDLINE_DEBUG 0
#if CMDLINE_DEBUG
	while (cmdline[i++])
		printf("argv[%d] is %s\n", i - 1, cmdline[i - 1]);
#endif

	i = 0;
	while (cmdline[i]) {
		strcat(cmd, cmdline[i]);
		strcat(cmd, " ");
		i++;
	}

#if CMDLINE_DEBUG
	printf("shairport cmd is `%s`\n", cmd);
#endif

	ret = mozart_system(cmd);

	return ret;
}

int mozart_airplay_shutdown(void)
{
	return mozart_airplay_control(AIRPLAY_COMMAND_EXIT);
}
