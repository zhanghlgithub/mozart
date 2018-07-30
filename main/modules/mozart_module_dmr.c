#include <stdio.h>
#include <string.h>

#include "utils_interface.h"

#include "mozart_config.h"
#include "mozart_app.h"

#include "modules/mozart_module_dmr.h"

static bool dmr_started = false;

bool qplay_is_controler(void)
{
	control_point_info *info = mozart_render_get_controler();

	if (info && info->music_provider && strcmp(QPLAY_PROVIDER, info->music_provider) == 0)
		return true;
	else
		return false;
}

static void AVTransportAction_callback(char *ActionName, struct Upnp_Action_Request *ca_event)
{
    printf("Not support AVTransportAction: %s\n", ActionName);
    return;
}

static void ConnectionManagerAction_callback(char *ActionName, struct Upnp_Action_Request *ca_event)
{
    printf("Not support ConnectionManagerAction: %s\n", ActionName);
    return;
}

static void RenderingControlAction_callback(char *ActionName, struct Upnp_Action_Request *ca_event)
{
    printf("Not support RenderingControlAction: %s\n", ActionName);
    return;
}

bool dmr_is_started(void)
{
	return dmr_started;
}

void dmr_start(void)
{
	int ret = 0;
	render_device_info_t device;

	memset(&device, 0, sizeof(device));
	/*
	 * this is our default frendlyname rule in librender.so,
	 * and you can create your own frendlyname rule.
	 */
	char deviceName[64] = {};
	char macaddr[] = "255.255.255.255";
	memset(macaddr, 0, sizeof (macaddr));

	if (dmr_started) {
		stop(DMR);
		dmr_started = false;
	}

	//FIXME: replace "wlan0" with other way. such as config file.
	get_mac_addr("wlan0", macaddr, "");
	//修改于18年6月5号
	//sprintf(deviceName, "EGO-%s", macaddr + 4);
	sprintf(deviceName, "SmartAudio-%s", macaddr + 4);
	device.friendlyName = deviceName;

	if (mozart_render_action_callback(AVTransportAction_callback,
					     ConnectionManagerAction_callback,
					     RenderingControlAction_callback))
		printf("[warning] register render action callback failed.\n");

	ret = mozart_render_start(&device);
	
	if (ret) {
		printf("[Error] start render service failed.\n");
	}
	else
	{
		printf("=====================================================\n");
		printf("start render server success.\n");
	}

	dmr_started = true;
}
