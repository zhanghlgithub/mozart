#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <linux/soundcard.h>
#include <string.h>
#include <inttypes.h>
#include <sys/mman.h>

#include "linklist_interface.h"
#include "utils_interface.h"
#include "sharememory_interface.h"
#include "resample_interface.h"
#include "channels_interface.h"
#include "webrtc_aec.h"
#include "bluetooth_interface.h"
#include "mozart_config.h"
#include "localstream_interface.h"
#include "localstream_fmt.h"
#include "mozart_musicplayer.h"
#include "modules/mozart_module_bt.h"
#include "modules/mozart_module_local_music.h"
#include "player_interface.h"
#include "ini_interface.h"
#include "tips_interface.h"
#include "volume_interface.h"
#include "egoserver.h"
#include "ingenicplayer.h"
#include "battery_capacity.h"
//#include "bluetooth_interface.h"//6.15
  
#include<unistd.h>     
#include<sys/types.h>  
#include<sys/stat.h>   
#include<fcntl.h>      
#include<termios.h>    
#include<errno.h>      

#include <signal.h>


//int connect_flag =0;
//UINT8  notify_data[30];
struct info date_buffer;
struct info *info=&date_buffer;
static pthread_mutex_t bt_mutex = PTHREAD_MUTEX_INITIALIZER;

static List cmd_list;
static bool die_out_bt = false;
static pthread_t ble_thread;
static pthread_mutex_t ble_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t ble_var = PTHREAD_COND_INITIALIZER;

//新加通达打开状态标志位
static bool open_chancal = false;

unsigned char g_Tower_binnumber[20] = {0};	//新添加于2018.7.31号
#if (SUPPORT_BSA_BLE_HH == 1)
int hh_client_num = -1;
#endif

//注：此功能没有开，疑问：此功能做什么用
#if (SUPPORT_BSA_BLE_HH_DIALOG_AUDIO == 1)
#define wavOutFile_path 		"/mnt/sdcard/bleaudio.wav"
#define WAV_ID_RIFF			0x46464952 /* "RIFF" */
#define WAV_ID_WAVE			0x45564157 /* "WAVE" */
#define WAV_ID_FMT			0x20746d66 /* "fmt " */
#define WAV_ID_DATA			0x61746164 /* "data" */
#define WAV_ID_PCM			0x0001
#define WAV_ID_FLOAT_PCM		0x0003
#define WAV_ID_FORMAT_EXTENSIBLE 	0xfffe

typedef signed long long    int64_t;
typedef unsigned char       uint8_t;
typedef unsigned short      uint16_t;
typedef unsigned int        uint32_t;

static int dia_FilterCoefs[] = {
	-215, -236, 326, 1236, 1451, 471, -610, -373, 727, 770,
	-599, -1189, 380, 1816, 150, -2856, -1557, 5926, 13522, 13522,
	5926, -1557, -2856, 150, 1816, 380, -1189, -599, 770, 727,
	-373, -610, 471, 1451, 1236, 326, -236, -215
};

static int dia_indexTable[] = {
	-1, -1, -1, -1, 2, 4, 6, 8,
	-1, -1, -1, -1, 2, 4, 6, 8
};

static int dia_stepSizeTable[] = {
	7, 8, 9, 10, 11, 12, 13, 14,
	16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45, 50, 55, 60,
	66, 73, 80, 88, 97, 107, 118, 130, 143, 157, 173, 190, 209,
	230, 253, 279, 307, 337, 371, 408, 449, 494, 544, 598, 658,
	724, 796, 876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878,
	2066, 2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871,
	5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635,
	13899, 15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794,
	32767
};

extern BOOLEAN ble_client_connect_state;
extern int mozart_bluetooth_ble_client_connect_server(ble_client_connect_data *cl_connect_data);
/* Settings for Remote Control. Note instance is (report number - 1) */
static int  HID_STREAM_ENABLE_WRITE_INSTANCE		= 3;
static int  HID_STREAM_ENABLE_READ_INSTANCE		= 4;
static int  HID_STREAM_DATA_MIN_INSTANCE		= 5;
static int  HID_STREAM_DATA_MAX_INSTANCE		= 7;
static int  dia_imaIndex = 0;
static short dia_imaPredictedSample = 0;
static int dia_imaOr = 0;
static int DIALOG_FILT_LEN = 38;
static int dia_imaAnd = 0xF;
static int dia_imaSize = 4;
static int dia_blockLen = 40;
static int dia_prevInstance = 0;
static BOOLEAN dialog_doLiveAudio = true;
static BOOLEAN dia_doStoreAudio = false;
static BOOLEAN dia_doSpeechRecognition = false;
static BOOLEAN dia_doupSample = false;
static int dia_dsp_fd = -1;
static int dia_wav_fd = -1;
static int dia_nbytes = 0;

/*
 *  Set the Data Rate
 *  0: 64 Kbit/s = ima 4Bps, 16 Khz.
 *  1: 48 Kbit/s = ima 3Bps, 16 Khz.
 *  2: 32 Kbit/s = ima 2Bps, 16 Khz.
 *  3: 32 Kbit/s = ima 4Bps, 8 Khz (downsample).
 *  4: 24 Kbit/s = ima 3Bps, 8 Khz (downsample).
 */
static void dialog_setDecodeMode(int mode)
{
	dia_doupSample = false;

	/* 4 bits per pixel */
	if (mode == 0) 	{
		dia_imaOr = 0;
		dia_imaAnd = 0x0F;
		dia_imaSize = 4;
		dia_blockLen = 40;
	/* 3 bits per pixel */
	} else if (mode == 1) {
		dia_imaOr = 1;
		dia_imaAnd = 0x0E;
		dia_imaSize = 3;
		dia_blockLen = 53;
	/* 8Khz, 4 bits per pixel */
	} else if (mode == 2) {
		dia_imaOr = 0;
		dia_imaAnd = 0x0F;
		dia_imaSize = 4;
		dia_doupSample = true;
		dia_blockLen = 80;
	/* 8Khz, 3 bits per pixel */
	} else if (mode == 3) {
		dia_imaOr = 1;
		dia_imaAnd = 0x0E;
		dia_imaSize = 3;
		dia_doupSample = true;
		dia_blockLen = 106;
	}
	dia_imaIndex = 0;
	dia_imaPredictedSample = 0;
	printf("Codec new Mode: %d, dia_doupSample: %d, dia_imaSize: %d\n", mode, dia_doupSample, dia_imaSize);
}

static int config_device(int fd, int format, int channels, int speed)
{
	int ioctl_val;

	/* NOTE: config order is important: format, channel, speed */
	/* set bit format */
	ioctl_val = format;
	if (ioctl(fd, SNDCTL_DSP_SETFMT, &ioctl_val) == -1) {
		fprintf(stderr, "set format failed: %s\n",
				strerror(errno));
		return -1;
	}
	if (ioctl_val != format) {
		fprintf(stderr, "format not supported, changed by driver to 0x%08x\n",
				ioctl_val);
		return -1;
	}

	/*set channel */
	ioctl_val = channels;
	if ((ioctl(fd, SNDCTL_DSP_CHANNELS, &ioctl_val)) == -1) {
		fprintf(stderr, "set channels failed: %s\n",
				strerror(errno));
		return -1;
	}
	if (ioctl_val != channels) {
		fprintf(stderr, "%d channel is not supported, changed to %d by driver\n",
				channels, ioctl_val);
		return -1;
	}

	/*set speed */
	ioctl_val = speed;
	if (ioctl(fd, SNDCTL_DSP_SPEED, &ioctl_val) == -1) {
		fprintf(stderr, "set speed failed: %s\n",
				strerror(errno));
		return -1;
	}
	if (ioctl_val != speed) {
		fprintf(stderr, "speed %d is not supported, changed to %d by driver\n",
				speed, ioctl_val);
		return -1;
	}

	return 0;
}

static void hh_fput16le(uint16_t val, int fd)
{
	int ret = 0;
	uint8_t bytes[2] = {val, val >> 8};

	ret = write(fd, bytes, 2);
	if (ret != 2) {
		printf("Warinig: hh_fput16le write num %d not equa to return num %d!!\n", 2, ret);
	}
}

static void hh_fput32le(uint32_t val, int fd)
{
	int ret = 0;
	uint8_t bytes[4] = {val, val >> 8, val >> 16, val >> 24};

	ret = write(fd, bytes, 4);
	if (ret != 4) {
		printf("Warinig: hh_fput32le write num %d not equa to return num %d!!\n", 4, ret);
	}
}

static void hh_write_wave_header(int fd, uint32_t data_length)
{
	uint32_t fmt_chunk_size = 0x00000010;
	int samplerate = 16000;
	int channels = 0x0001;
	int format = 16;
	int bps = channels * samplerate * (format / 8);

	/* Master RIFF chunk */
	hh_fput32le(WAV_ID_RIFF, fd);
	/* RIFF chunk size: 'WAVE' + 'fmt ' + 4 + fmt_chunk_size + data chunk hdr (8) + data length */
	hh_fput32le((36 + data_length), fd);
	hh_fput32le(WAV_ID_WAVE, fd);	/* WAVE */

	/* Format chunk */
	hh_fput32le(WAV_ID_FMT, fd);	/* fmt */
	hh_fput32le(fmt_chunk_size, fd);   /* Subchunksize = 16a ->should be modified later.. */
	hh_fput16le(WAV_ID_PCM, fd);		/* PCM */
	hh_fput16le(channels, fd);		/* Mono */

	hh_fput32le(samplerate, fd);	/* Samplerate = 16000 HZ */
	hh_fput32le(bps, fd); 		/* ByteRate = channels * SampleRate * format/8 */
	hh_fput16le(channels * (format / 8), fd); /* BlockAlign */
	hh_fput16le(format, fd);			/* BitsPerSample */

	/* Data chunk */
	hh_fput32le(WAV_ID_DATA, fd);		/* DATA */
	hh_fput32le(data_length, fd);		/* SubChunk2size = dia_nbytes -> should be modified later.. */
}

static int audio_store_start()
{
	dia_wav_fd = open(wavOutFile_path, O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
	if (dia_wav_fd < 0) {
		printf("open wavfile (%s) error: %s.\n", wavOutFile_path, strerror(errno));
		return -1;
	}
	dia_nbytes = 0;  // Clear the buffer...
	hh_write_wave_header(dia_wav_fd, 0x800);

	return 0;
}

static void dialog_streamSetOnOff(BOOLEAN enable)
{
	int ret = 0, i = 0;
	ble_client_write_data cl_wdata;

	if (enable) {
		printf("Stream On !!\n");
		if (dia_doStoreAudio) {
			ret = audio_store_start();
			if (ret == -1) {
				printf("audio_store_start Failed !!\n");
			}
		}
		if (dia_doSpeechRecognition) {
			//speechRecGoogle.start();
		}
		dia_imaIndex = 0;
		dia_imaPredictedSample = 0;

		/* play audio pcm use dsp */
		if (dialog_doLiveAudio)
		{
			if (dia_dsp_fd == -1) {
				char buf[16] = {};
				int volume = 0;
				int hh_sample_rate = 16000;
				int hh_sample_bit = 16;
				int hh_sample_channel = 1;
				/* Parse playback dsp device. */
				if (mozart_ini_getkey("/usr/data/system.ini", "audio", "dev_playback", buf)) {
					printf("[app_avk - OSS] Can't get playback dsp device, force to /dev/dsp.\n");
					strcpy(buf, "/dev/dsp");
				}

				dia_dsp_fd = open(buf, O_WRONLY);
				if (dia_dsp_fd < 0) {
					printf("avk: open play dsp(%s) error: %s.\n", buf, strerror(errno));
					return;
				} else {
					if (mozart_ini_getkey("/usr/data/system.ini", "volume", "bt_music", buf)) {
						printf("failed to parse /usr/data/system.ini, set BT music volume to 60.\n");
						volume = 60;
					} else {
						volume = atoi(buf);
					}
					mozart_volume_set(volume, MUSIC_VOLUME);
					config_device(dia_dsp_fd, hh_sample_bit, hh_sample_channel, hh_sample_rate);
				}
			}
		}
	} else {
		printf("Stream Off !!\n");
		if (dia_doStoreAudio)
		{
			if (lseek(dia_wav_fd, 0, SEEK_SET) == 0) {
				hh_write_wave_header(dia_wav_fd, dia_nbytes);
				close(dia_wav_fd);
				dia_wav_fd = -1;
				printf("Close wav file, dia_nbytes is %d\n", dia_nbytes);
			} else {
				printf("Could not seek to start, WAV size headers not updated! \n");
				close(dia_wav_fd);
				close(dia_dsp_fd);
				dia_dsp_fd = -1;
				dia_wav_fd = -1;

				return;
			}
		}
		if (dia_doSpeechRecognition)
		{
			//speechRecGoogle.stop();
		}
		/* audioDecoder.stop(); */
		if (dialog_doLiveAudio)
		{
			close(dia_dsp_fd);
			dia_dsp_fd = -1;
		}
		printf("dia_nbytes = %d\n", dia_nbytes);
	}
	dia_prevInstance = 0;
	/* write enable to hid device */

	printf("enable = %d\n", enable);
	/* write connected ble device */
	cl_wdata.service_uuid = 0x1812;
	cl_wdata.char_uuid = 0x2a4d;
	cl_wdata.write_len = 20;
	cl_wdata.write_data[0] = enable;
	for (i = 1; i < cl_wdata.write_len; i++) {
		cl_wdata.write_data[i] = 0x00;
	}
	cl_wdata.client_num = hh_client_num;
	cl_wdata.is_primary = 1;
	cl_wdata.ser_inst_id = 0;
	cl_wdata.char_inst_id = HID_STREAM_ENABLE_WRITE_INSTANCE;
	cl_wdata.is_descript = 0;
	cl_wdata.descr_id = 0;
	cl_wdata.desc_value = 0;
	cl_wdata.write_type = 2;

	ret = mozart_bluetooth_ble_client_write(&cl_wdata);
	if (ret != 0) {
		printf("mozart_bluetooth_ble_client_write failed !\n");
		return;
	}
}

static void dialog_updatekeys(UINT8 *packet)
{
	int type = (int)packet[1];
	int i = 0;

	printf("Key Update type: %d\n", type);
	if (type == 0) {
		BOOLEAN enable = (packet[0] != 0);
		dialog_streamSetOnOff(enable);
	} else if (type == 2) {
		//keyReport.displayKeyPress(packet);
	} else if (type == 3) {
		int fifo_err = (int) packet[5];
		int spi_err  = (int) packet[6];
		int fifo_sz = ((int)packet[9]) & 0x000000FF;
		int wr = ((int)packet[7]) & 0x000000FF;
		int rd = ((int)packet[8]) & 0x000000FF;

		printf("Fifo error %d,  spi %d,  sz %d, wr %d, rd %d \n", fifo_err, spi_err, fifo_sz, rd, wr);
	} else if (type == 4) {
		int mode = (int) packet[5];
		int spi_sz = ((int)packet[6]) & 0x000000FF;
		int fifo_sz = ((int)packet[7]) & 0x000000FF;

		printf("Ima Mode Change %d, fsz %d, spi %d \n", mode, fifo_sz, spi_sz);
		for (i = 10; i < 20; i++) {
			int v = ((int)packet[i]) & 0x000000FF;
			printf("ImaMode = %d\n", v);
		}
	}
}

static void dialog_DecodeImaAdpcm(int len, UINT8 *inpBytes, UINT16 data_len, short *outSamples)
{
	int i;
	int predictedSample = (int)dia_imaPredictedSample;
	int index = dia_imaIndex;
	int stepSize;
	int inpBuf = 0;
	int inpMsb = 0;
	int inpIdx = 0;
	int shift = 4 - dia_imaSize;
	int bsize = data_len;

	for (i = 0; i < len; i++)
	{
		stepSize = dia_stepSizeTable[index];   /* Find new quantizer stepsize */
		int inp;

		/**
		 ** Unpack the bytes from 2 bytes (1 word)
		 */
		if (inpMsb < 8)
		{
			inpBuf = (int)(inpBuf << 8) & 0xFFFF;
			if (inpIdx < bsize)  inpBuf |= ((int)inpBytes[inpIdx++] & 0x00FF);
			inpMsb += 8;
		}
		inpMsb -= dia_imaSize;
		inp = ((inpBuf >> inpMsb) << shift ) | dia_imaOr;
		inp = inp & 0x0F;

		/* Compute predicted sample estimate                 */
		/* This part is different from regular IMA-ADPCM     */
		/* The prediction is calculated with full precision  */
		int diff = stepSize;  /* the 4 is for rounding */
		if ((inp & 4) != 0) {
			diff += (int)(stepSize << 3);
		}
		if ((inp & 2) != 0) {
			diff += (int)(stepSize << 2);
		}
		if ((inp & 1) != 0) {
			diff += (int)(stepSize << 1);
		}
		diff >>= 3;

		/* Adjust predictedSample based on calculated difference */
		//predictedSample += diff;
		if ((inp & 8) != 0)
			predictedSample -= diff;
		else
			predictedSample += diff;

		/* Saturate if there is overflow */
		if (predictedSample > 32767)
			predictedSample = 32767;
		if (predictedSample < -32768)
			predictedSample = -32768;

		/* 16-bit output sample can be stored at this point */
		outSamples[i] = (short)predictedSample;

		/* Compute new stepsize */
		/* Adjust index into dia_stepSizeTable using newIMA */
		index += dia_indexTable[inp];
		if (index < 0) index = 0;
		if (index > 88) index = 88;
	}
	dia_imaIndex = index;
	dia_imaPredictedSample = (short)predictedSample;
}

static short dialog_rndSat(int acc)
{
	if (acc > 32767)
		acc = 32767;
	else if (acc < -32768)
		acc = -32768;

	return (short)acc;
}

static void dialog_upSample(int len, short *inpSamples, short *outSamples)
{
	int i, j;
	short *filterTaps = malloc(38 * sizeof(short));

	for (i = 0; i < len; i++)
	{
		int acc1 = 0;
		int acc2 = 0;
		for (j = 0; j < (DIALOG_FILT_LEN / 2) - 1; j++) {
			acc1 += (int)dia_FilterCoefs[2*j] * (int)filterTaps[j];
			acc2 += (int)dia_FilterCoefs[2*j + 1] * (int)filterTaps[j];
			filterTaps[j] = filterTaps[j + 1];
		}
		filterTaps[(DIALOG_FILT_LEN/2) - 1] = inpSamples[i];
		acc1 += (int)dia_FilterCoefs[DIALOG_FILT_LEN - 2] * (int)filterTaps[(DIALOG_FILT_LEN/2) - 1];
		acc2 += (int)dia_FilterCoefs[DIALOG_FILT_LEN - 1] * (int)filterTaps[(DIALOG_FILT_LEN/2) - 1];
		outSamples[2*i] = (short)dialog_rndSat(acc2 >> 15);
		outSamples[2*i + 1] = (short)dialog_rndSat(acc1 >> 15);
	}
	free(filterTaps);
}

static void audio_decoder_process(UINT8 *inpBytes, UINT16 data_len, short *outSamples)
{
	if (dia_doupSample)
	{
		short *tmpSamples = malloc(dia_blockLen / 2 * (sizeof(short)));
		dialog_DecodeImaAdpcm(dia_blockLen / 2, inpBytes, data_len, tmpSamples);
		dialog_upSample(dia_blockLen / 2, tmpSamples, outSamples);
		free(tmpSamples);
	} else {
		dialog_DecodeImaAdpcm(dia_blockLen, inpBytes, data_len, outSamples);
	}
}

static void audio_addSampleData(short *samples)
{
	int ret = 0;
	int samples_length = dia_blockLen * sizeof(short);

	if (dialog_doLiveAudio)
	{
		if (dia_dsp_fd != -1) {
			ret = write(dia_dsp_fd, (char *)samples, samples_length);
			if (ret != samples_length) {
				printf("Warinig: audio_addSampleData write num %d not equa to return num %d!!\n", 4, ret);
			}
		}
	}

	if (dia_doStoreAudio)
	{
		dia_nbytes += samples_length;
		ret = write(dia_wav_fd, (char *)samples, samples_length);
	}
	if (dia_doSpeechRecognition)
	{
		//speechRecGoogle.addSampleData(samples);
	}
}

static void dialog_sendAudioData(UINT8 *data, UINT16 data_len)
{
	short *outSamples = malloc(dia_blockLen * sizeof(short));

	audio_decoder_process(data, data_len, outSamples);
	audio_addSampleData(outSamples);
	free(outSamples);
}

static int dialog_init()
{
	dia_imaIndex = 0;
	dia_imaPredictedSample = 0;
	dialog_setDecodeMode(0);  // Default mode

	return 0;
}
#endif /* SUPPORT_BSA_BLE_HH_DIALOG_AUDIO */

#if (SUPPORT_BT == BT_BCM)
extern int bsa_start_regular_enable;
extern int bt_link_state;
#endif

extern int bluetooth_cancel_auto_reconnect_pthread();

#if (SUPPORT_BSA_SPPS == 1)
static int mozart_bluetooth_dg_uipc_cback(UINT16 event, UINT8 *buf, UINT32 length)
{
	int i = 0;
	if (event == UIPC_RX_DATA_READY_EVT) {
		printf("length = %d\n", length);
		for (i = 0; i < length; i++) {
			printf("0x%02x ", buf[i]);
		}
		printf("\n");
	}

	return 0;
}
#endif /* SUPPORT_BSA_SPPS */

#if (SUPPORT_BSA_SPPC == 1)
enum {
	DG_IDLE,
	DG_CONNECTED,
	DG_CONNECT_FAILED,
	DG_DISCONNECTED,
};
extern int bt_dg_flag;
BOOLEAN sdpu_is_base_uuid(char *uuid)
{
	UINT16 i;
	static const char sdp_base_uuid[] = {0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB};

	for (i = 4; i < MAX_UUID_SIZE; i++) {
		if (uuid[i] != sdp_base_uuid[i]) {
			return (false);
		}
	}
	/* If here, matched */
	return (true);
}
#endif

#if (SUPPORT_BSA_BLE == 1)
tAPP_BLE_CB *bsa_ble_cb = NULL;
extern int bsa_ble_start_regular_enable;
int ble_service_create_enable = 0;
int ble_service_add_char_enable = 0;
int ble_service_start_enable = 0;
int ble_client_open_enable = 0;
int ble_client_open_states = 0;		//dsc_1120
int ble_client_read_enable;
int ble_client_write_enable = 0;
extern int ble_hh_add_dev_enable;
struct {
	ble_create_service_data service;
	ble_char_data 	measure_character;
	ble_char_notify measure_notif;
} ble_heart_rate;

pthread_mutex_t ble_conn_lock;
void mozart_ble_print_conn_id(int *conn_id)
{
	int i = 0;

	for (i = 0; i < NOTIFY_CONN_MAX_NUM; i++) {
		if (conn_id[i] != 0)
			printf("conn_id[%d] = %d\n", i, conn_id[i]);
	}
}

int mozart_ble_add_conn_id(int add_id, int *conn_id)
{
	pthread_mutex_lock(&ble_conn_lock);
	int i = 0;

	for (i = 0; i < NOTIFY_CONN_MAX_NUM; i++) {
		if (conn_id[i] == 0) {
			conn_id[i] = add_id;
			break;
		}
	}
	if (i >= NOTIFY_CONN_MAX_NUM) {
		printf("conn_id num: %d > NOTIFY_CONN_MAX_NUM: %d\n", i, NOTIFY_CONN_MAX_NUM);
		pthread_mutex_unlock(&ble_conn_lock);
		return -1;
	}
	pthread_mutex_unlock(&ble_conn_lock);

	return 0;
}

int mozart_ble_del_conn_id(int del_id, int *conn_id)
{
	pthread_mutex_lock(&ble_conn_lock);
	int i = 0, j = 0;

	for (i = 0; i < NOTIFY_CONN_MAX_NUM; i++) {
		if (conn_id[i] == del_id) {
			if (i < (NOTIFY_CONN_MAX_NUM - 1)) {
				for (j = i; j < (NOTIFY_CONN_MAX_NUM - 1); j++) {
					if (conn_id[j + 1] != 0)
						conn_id[j] = conn_id[j + 1];
					else {
						conn_id[j] = 0;
						break;
					}
				}
			} else if (i == (NOTIFY_CONN_MAX_NUM - 1)) {
				conn_id[i] = 0;
						break;
			}
			break;
		}
	}
	pthread_mutex_unlock(&ble_conn_lock);

	return 0;
}

int mozart_ble_send_indication()
{
	int i = 0;
	int server_num = 0;
	int char_attr_num = 0, value_len = 0;
	ble_server_indication ble_indication;

	server_num =  ble_heart_rate.service.server_num;
	char_attr_num = ble_heart_rate.measure_character.char_attr_num;
	value_len = bsa_ble_cb->ble_server[server_num].attr[char_attr_num].value_len;
	ble_indication.server_num = server_num;
	ble_indication.attr_num   = char_attr_num;
	ble_indication.length_of_data = value_len;

	for (i = 0; i < NOTIFY_CONN_MAX_NUM; i++) {
		if (ble_heart_rate.measure_notif.conn_id[i] != 0) {
			ble_indication.conn_id = ble_heart_rate.measure_notif.conn_id[i];
			/* send client indication */
			mozart_bluetooth_ble_server_send_indication(&ble_indication);
		} else
			break;
	}

	return 0;
}

#define PREFIX_LEN	4
#define PRO_LEN 	16
#define MAJOR_LEN	2
#define MINOR_LEN	2
#define TXPOWER_LEN	1
#define UUID_LEN	25  /* UUID_LEN = PREFIX_LEN + PRO_LEN + MAJOR_LEN + MINOR_LEN + TXPOWER_LEN */
int mozart_bluetooth_ble_parse_ibeacon_data(UINT8 * eir_data)
{
	int index = 0;
	bsa_manu_data manu_data = {0};;

	mozart_bluetooth_parse_eir_manuf_specific(eir_data, &manu_data);
	if (manu_data.data_length > 0) {
		printf("Manufacturer data length = %d\n", manu_data.data_length);
		printf("Manufacturer Specific CompanyId: 0x%04X\n", manu_data.company_id);
		if (manu_data.company_id == 0x004c) {
			printf("Apple Ibeacon!\n");
		}
		printf("Manufacturer data: \n");
		for (index = 0; index < manu_data.data_length; index++)
		{
			printf("0x%02X ", manu_data.p_manu[index]);
		}
		printf("\n");
		printf("Proximity UUID: ");
		for (index = 0; index < PRO_LEN; index++)
		{
			printf("0x%02X ", manu_data.p_manu[index+2]);
		}
		printf("\n");
		printf("Major: ");
		for (index = 0; index < MAJOR_LEN; index++)
		{
			printf("0x%02X ", manu_data.p_manu[manu_data.data_length - 5 + index]);
		}
		printf("\n");
		printf("Minor: ");
		for (index = 0; index < MINOR_LEN; index++)
		{
			printf("0x%02X ", manu_data.p_manu[manu_data.data_length - 3 + index]);
		}
		printf("\n");
		printf("Txpower: ");
		printf("0x%02X \n", manu_data.p_manu[manu_data.data_length-1]);
	}
	return 0;
}

#if (SUPPORT_BSA_BLE_SERVER == 1)
static int bsa_ble_server_profile_cback(tBSA_BLE_EVT event, tBSA_BLE_MSG *p_data)
{
	int attr_index, index;
	int i = 0;

	switch (event) {
		case BSA_BLE_SE_CREATE_EVT:
			printf("## BSA_BLE_SE_CREATE_EVT!\n");
			printf("server_if:%d status:%d service_id:%d\n",
					p_data->ser_create.server_if,
					p_data->ser_create.status,
					p_data->ser_create.service_id);
			ble_service_create_enable = 1;
			break;
		case BSA_BLE_SE_ADDCHAR_EVT:
			printf("## BSA_BLE_SE_ADDCHAR_EVT!\n");
			printf("status: %d\n", p_data->ser_addchar.status);
			printf("attr_id: 0x%x\n", p_data->ser_addchar.attr_id);
			ble_service_add_char_enable = 1;
			break;
		case BSA_BLE_SE_START_EVT:
			printf("## BSA_BLE_SE_START_EVT!\n");
			printf("status: %d\n", p_data->ser_start.status);
			ble_service_start_enable = 1;
			break;
		case BSA_BLE_SE_READ_EVT:
			printf("## BSA_BLE_SE_READ_EVT!\n");
			printf("status: %d\n", p_data->ser_read.status);
			printf("conn_id: %d\n", p_data->ser_read.conn_id);
			break;
		case BSA_BLE_SE_WRITE_EVT:
			printf("## BSA_BLE_SE_WRITE_EVT!\n");
			printf("status: %d\n", p_data->ser_write.status);
			printf("conn_id: %d, handle: %d\n", p_data->ser_write.conn_id, p_data->ser_write.handle);
			{
				for (index = 0; index < BSA_BLE_SERVER_MAX; index++)
				{
					if (bsa_ble_cb->ble_server[index].enabled)
					{
						for (attr_index = 0; attr_index < BSA_BLE_ATTRIBUTE_MAX; attr_index++)
						{
							if (bsa_ble_cb->ble_server[index].attr[attr_index].attr_UUID.uu.uuid16)
							{
								if (bsa_ble_cb->ble_server[index].attr[attr_index].attr_id == p_data->ser_write.handle) {
									printf("ble_server[%d].attr[%d].attr_id = %d\n",
											index, attr_index,
											bsa_ble_cb->ble_server[index].attr[attr_index].attr_id);
									printf("ble_server[%d].attr[%d].attr_UUID.uu.uuid16 = 0x%02x\n",
											index, attr_index,
											bsa_ble_cb->ble_server[index].attr[attr_index].attr_UUID.uu.uuid16);
									memset(bsa_ble_cb->ble_server[index].attr[attr_index].value, 0, BSA_BLE_MAX_ATTR_LEN);
									memcpy(bsa_ble_cb->ble_server[index].attr[attr_index].value, p_data->ser_write.value, p_data->ser_write.len);
									bsa_ble_cb->ble_server[index].attr[attr_index].value_len = p_data->ser_write.len;
									for (i = 0; i < p_data->ser_write.len; i++) {
										printf("value[%d] = 0x%x\n", i, p_data->ser_write.value[i]);
									}
									goto BLE_SE_WRITE_SendRSP;
								}
							}
						}
					}
				}
BLE_SE_WRITE_SendRSP:
				if (index >= BSA_BLE_SERVER_MAX) {
					printf("Error: not find Ble Server, please check!\n");
					break;
				}
				/* Judged which Characterisic Notify is be Writed */
				if ((index == ble_heart_rate.service.server_num) &&
						(attr_index == ble_heart_rate.measure_notif.char_data.char_attr_num)) {
					UINT8 *value = NULL;
					ble_server_indication ble_indication;
					int char_attr_num = 0, value_len = 0;

					char_attr_num = ble_heart_rate.measure_notif.char_data.char_attr_num;
					value = bsa_ble_cb->ble_server[index].attr[char_attr_num].value;
					/* Nofification opened */
					if (value[0] == 1) {
						mozart_ble_add_conn_id(p_data->ser_write.conn_id, ble_heart_rate.measure_notif.conn_id);
						char_attr_num = ble_heart_rate.measure_character.char_attr_num;
						value_len = bsa_ble_cb->ble_server[index].attr[char_attr_num].value_len;
						ble_indication.server_num = index;
						ble_indication.attr_num   = char_attr_num;
						ble_indication.length_of_data = value_len;
						ble_indication.conn_id = p_data->ser_write.conn_id;
						/* send client indication */
						mozart_bluetooth_ble_server_send_indication(&ble_indication);
					} else if (value[0] == 0) {
						mozart_ble_del_conn_id(p_data->ser_write.conn_id, ble_heart_rate.measure_notif.conn_id);
					}
				}
			}
			break;
		case BSA_BLE_SE_OPEN_EVT:
			printf("## BSA_BLE_SE_OPEN_EVT!\n");
			printf("open status: %d, conn_id: %d, bd_addr: %02x:%02x:%02x:%02x:%02x:%02x\n",
					p_data->ser_open.reason, p_data->ser_open.conn_id,
					p_data->ser_open.remote_bda[0],
					p_data->ser_open.remote_bda[1],
					p_data->ser_open.remote_bda[2],
					p_data->ser_open.remote_bda[3],
					p_data->ser_open.remote_bda[4],
					p_data->ser_open.remote_bda[5]);
			break;
		case BSA_BLE_SE_CLOSE_EVT:
			printf("## BSA_BLE_SE_CLOSE_EVT!\n");
			printf("status: %d\n", p_data->ser_close.reason);
			printf("conn_id: 0x%x\n", p_data->ser_close.conn_id);
			mozart_ble_del_conn_id(p_data->ser_close.conn_id, ble_heart_rate.measure_notif.conn_id);
			break;
		default:
			printf("%s not handle this event: %d\n", __func__, event);
			break;
	}

	return 0;
}

static int ble_server_set_ibeacon_data(UINT8 *p_val, UINT8 *proximity_uuid, UINT8 *major, UINT8 *minor, UINT8 *txpower)
{
	int len = 0, i = 0;
	UINT8 prefix_data[PREFIX_LEN] = {0x4C, 0x00, 0x02, 0x15};

	memcpy(p_val, prefix_data, PREFIX_LEN);
	len = PREFIX_LEN;
	memcpy(p_val + len, proximity_uuid, PRO_LEN);
	len += PRO_LEN;
	memcpy(p_val + len, major, MAJOR_LEN);
	len += MAJOR_LEN;
	memcpy(p_val + len, minor, MINOR_LEN);
	len += MINOR_LEN;(⊙o⊙)…
	memcpy(p_val + len, txpower, TXPOWER_LEN);
	len += TXPOWER_LEN;

	for (i = 0; i < UUID_LEN; i++) {
		printf("%02x ", p_val[i]);
		if(i == (UUID_LEN / 2))
			printf("\n");
	}
	printf("\n");

	return 0;
}

int ble_server_create_ibeacon_adv()
{
	int state = 0;
	tBSA_DM_BLE_ADV_CONFIG ble_config_adv = {0};

	/* 02 01 1a 1a ff 4c 00 02 15  # Apple's fixed iBeacon advertising prefix */
	ble_config_adv.flag = BTM_BLE_BREDR_NOT_SPT | BTM_BLE_GEN_DISC_FLAG;
	/* iBeacon profile uuid (128 bits) */
	UINT8 proximity_uuid[PRO_LEN] = {0xB9, 0x40, 0x7F, 0x30, 0xF5, 0xF8, 0x46, 0x6E,
		0xAF, 0xF9, 0x25, 0x55, 0x6B, 0x57, 0xFE, 0x6D};
	UINT8 major[MAJOR_LEN] = {0x00, 0x49};
	UINT8 minor[MINOR_LEN] = {0x00, 0x0A};
	UINT8 txpower[TXPOWER_LEN] = {0xC5};

	ble_config_adv.len = UUID_LEN;
	ble_config_adv.is_scan_rsp = 0; 	/* 0 is Active broadcast, 1 is Passive broadcast */
	ble_config_adv.is_part_service = 1; 	/* 1 is 0x02, 0 is 0x03 */
	ble_config_adv.adv_data_mask = BSA_DM_BLE_AD_BIT_FLAGS | BSA_DM_BLE_AD_BIT_MANU;
	ble_server_set_ibeacon_data(ble_config_adv.p_val, proximity_uuid, major, minor, txpower);
	state = mozart_bluetooth_ble_configure_adv_data(&ble_config_adv);
	if (state) {
		printf("mozart_bluetooth_ble_configure_adv_data failed, state = %d.\n", state);
		return -1;
	}
	return 0;
}

static int ble_service_config_adv_data()
{
	int state = 0;
	tBSA_DM_BLE_ADV_CONFIG ble_config_adv;
	tBSA_DM_BLE_ADV_PARAM ble_adv_param;
	/* First 2 byte is Company Identifier Eg: 0x000f refers to Broadcom Corporation, followed by 6 bytes of data*/
	UINT8 app_ble_adv_value[APP_BLE_ADV_VALUE_LEN] = { 0x0F, 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66};

	/* Active broadcast */
	/* Adv.Data should be < 31bytes,
	 * including Manufacturer data, Device Name,
	 * Appearance data, Services Info, etc.. */
	ble_config_adv.appearance_data	= BLE_ADV_APPEARANCE_DATA;
	ble_config_adv.num_service 	= 1;
	ble_config_adv.uuid_val[0]	= BSA_BLE_UUID_SERVCLASS_HEART_RATE;
	ble_config_adv.len		= APP_BLE_ADV_VALUE_LEN;
	ble_config_adv.flag		= BSA_DM_BLE_ADV_FLAG_MASK;
	ble_config_adv.is_scan_rsp 	= 0; 	/* 0 is Active broadcast, 1 is Passive broadcast */
	ble_config_adv.is_part_service 	= 1; 	/* 1 is 0x02, 0 is 0x03 */
	memcpy(ble_config_adv.p_val, app_ble_adv_value, APP_BLE_ADV_VALUE_LEN);
	/* All the masks/fields that are set will be advertised */
	ble_config_adv.adv_data_mask = BSA_DM_BLE_AD_BIT_FLAGS | BSA_DM_BLE_AD_BIT_MANU | BSA_DM_BLE_AD_BIT_DEV_NAME;
	state = mozart_bluetooth_ble_configure_adv_data(&ble_config_adv);
	if (state) {
		printf("mozart_bluetooth_ble_configure_adv_data failed, state = %d.\n", state);
		return -1;
	}

	/* Passive broadcast */
	memset(&ble_config_adv, 0, sizeof(tBSA_DM_BLE_ADV_CONFIG));
	/* start advertising */
	ble_config_adv.adv_data_mask = BSA_DM_BLE_AD_BIT_DEV_NAME;
	ble_config_adv.is_scan_rsp   = 1;
	state = mozart_bluetooth_ble_configure_adv_data(&ble_config_adv);
	if (state) {
		printf("mozart_bluetooth_ble_configure_adv_data failed, state = %d.\n", state);
		return -1;
	}

	/* set Advertising_Interval */
	/* Range: 0x0020 to 0x4000
	   Time = Range * 0.625 msec
	   Time Range: 20 ms to 10.24 sec */
	memset(&ble_adv_param, 0, sizeof(tBSA_DM_BLE_ADV_PARAM));
	ble_adv_param.adv_int_min = 0xa0;	/* 100 ms */
	ble_adv_param.adv_int_max = 0xa0;	/* 100 ms */
	mozart_bluetooth_ble_set_adv_param(&ble_adv_param);

	return 0;
}

static int ble_service_and_char_create()
{
	int state = 0;
	int try = 0, try_max = 30;
	ble_start_service_data ble_start_service;

	ble_heart_rate.service.service_uuid = BSA_BLE_UUID_SERVCLASS_HEART_RATE;
	ble_heart_rate.service.num_handle   = 10;
	ble_heart_rate.service.is_primary   = 1;
	ble_heart_rate.service.server_num   = mozart_bluetooth_ble_server_register(APP_BLE_MAIN_DEFAULT_APPL_UUID);
	if (ble_heart_rate.service.server_num < 0) {
		printf("mozart_bluetooth_ble_server_register failed, state = %d.\n", state);
		return -1;
	}

	state = mozart_bluetooth_ble_server_create_service(&ble_heart_rate.service);
	if (state) {
		printf("mozart_bluetooth_ble_create_service failed, state = %d.\n", state);
		return -1;
	}
	for (try = 0; try < try_max; try++) {
		if(ble_service_create_enable != 1)
			usleep(300*1000);
		else
			break;
	}
	if (ble_service_create_enable == 1)
		ble_service_create_enable = 0;
	else {
		printf("Error: not recived BSA_BLE_SE_CREATE_EVT, please checked!\n");
		return -1;
	}
	ble_heart_rate.measure_character.server_num    = ble_heart_rate.service.server_num;
	ble_heart_rate.measure_character.srvc_attr_num = ble_heart_rate.service.attr_num;
	ble_heart_rate.measure_character.char_uuid     = BSA_BLE_GATT_UUID_HEART_RATE_MEASUREMENT;
	ble_heart_rate.measure_character.attribute_permission    = BSA_GATT_PERM_READ | BSA_GATT_PERM_WRITE;
	ble_heart_rate.measure_character.characteristic_property = BSA_GATT_CHAR_PROP_BIT_READ | BSA_GATT_CHAR_PROP_BIT_WRITE |
									BSA_GATT_CHAR_PROP_BIT_NOTIFY | BSA_GATT_CHAR_PROP_BIT_INDICATE;
	state = mozart_bluetooth_ble_server_add_character(&ble_heart_rate.measure_character);
	if (state) {
		printf("mozart_bluetooth_ble_server_add_character failed, state = %d.\n", state);
		return -1;
	}
	for (try = 0; try < try_max; try++) {
		if(ble_service_add_char_enable != 1)
			usleep(300*1000);
		else
			break;
	}
	if (ble_service_add_char_enable == 1) {
		ble_service_add_char_enable = 0;
	} else {
		printf("Error: not recived BSA_BLE_SE_ADDCHAR_EVT, please checked!\n");
		return -1;
	}
	/* descripter: service notify */
	ble_heart_rate.measure_notif.char_data.server_num    = ble_heart_rate.service.server_num;
	ble_heart_rate.measure_notif.char_data.srvc_attr_num = ble_heart_rate.service.attr_num;
	ble_heart_rate.measure_notif.char_data.char_uuid = GATT_UUID_CHAR_CLIENT_CONFIG;
	ble_heart_rate.measure_notif.char_data.is_descript = true;
	ble_heart_rate.measure_notif.char_data.characteristic_property = 0;
	ble_heart_rate.measure_notif.char_data.attribute_permission = BSA_GATT_PERM_READ | BSA_GATT_PERM_WRITE;
	state = mozart_bluetooth_ble_server_add_character(&ble_heart_rate.measure_notif.char_data);
	if (state) {
		printf("mozart_bluetooth_ble_server_add_character failed, state = %d.\n", state);
		return -1;
	}
	for (try = 0; try < try_max; try++) {
		if(ble_service_add_char_enable != 1)
			usleep(300*1000);
		else
			break;
	}
	if (ble_service_add_char_enable == 1) {
		ble_service_add_char_enable = 0;
	} else {
		printf("Error: not recived BSA_BLE_SE_ADDCHAR_EVT, please checked!\n");
		return -1;
	}

	ble_start_service.server_num = ble_heart_rate.service.server_num;
	ble_start_service.attr_num   = ble_heart_rate.service.attr_num;
	state = mozart_bluetooth_ble_server_start_service(&ble_start_service);
	if (state) {
		printf("mozart_bluetooth_ble_server_tart_service failed, state = %d.\n", state);
		return -1;
	}
	for (try = 0; try < try_max; try++) {
		if(ble_service_start_enable != 1)
			usleep(300*1000);
		else
			break;
	}
	if (ble_service_start_enable == 1) {
		ble_service_start_enable = 0;
	} else {
		printf("Error: not recived BSA_BLE_SE_START_EVT, please checked!\n");
		return -1;
	}
	ble_service_config_adv_data();
	return 0;
}
#endif /* SUPPORT_BSA_BLE_SERVER */
int timer = 0;
int flag=0;
unsigned char x_data[36];
UINT16           data_len;

int client_num = 0;
int client_num2 = 0;
UINT8  cuband_mac[6] = {0x08, 0x7c, 0xbe, 0x00, 0x00, 0x11}; //my
UINT8  tower_mac[6] = {0x08, 0x7c, 0xbe, 0x00, 0x00, 0x01}; //my
UINT8  tower_eir[6]; //dsc1120
//UINT8  cuband_mac[6] = {0x08, 0x7c, 0xbe, 0x5e, 0xc5, 0x69}; //shiyou 
//UINT8  tower_mac[6] = {0x7e, 0xfd, 0x2a, 0x24, 0xdc, 0x1d}; //shiyou 
//UINT8  cuband_mac[6] = {0x08, 0x7c, 0xbe, 0x00, 0x00, 0xfe}; //shiyou2if
//UINT8  tower_mac[6] = {0x7e, 0xfd, 0x2a, 0x24, 0xdc, 0x1d}; //shiyou2if
char   ini_name[12][8]={"cub_1", "cub_2","cub_3","cub_4","cub_5","cub_6","cub_7","cub_8","cub_9","cub_10","cub_11","cub_12"};
char  light_list[100][13]={};
char  gif_list[100][13]={};
int  ii;

UINT8 cuband_flag = 0;		//ble有通知事件过来将其置1
UINT8 tower_flag = 0;		//ble有通知事件过来将其置1
UINT8 return_cuband_flag = 0;		//702
UINT8 return_tower_flag = 0;
UINT8 cuband_status=0;		//ble有打开事件将其置成1
UINT8 tower_status=0;		//ble有打开事件将其置成1

//int error;
//int flag=0;
//pthread_t p_tid_reconnect;
#if (SUPPORT_BSA_BLE_CLIENT == 1)
//获得头戴的连接状态
int  get_cuband_state(){
	if(cuband_status==1&&return_cuband_flag==1)
		return 1;
	else
	{	
		return 0;
	}
}
//tower与Base之间BLE的连接状态tatus"，布尔类型，1-设备连接正常，0-设备未连接
int  get_tower_state(){
	if(tower_status==1&&return_tower_flag==1)
		return 1;
	else
	{	
		return 0;
	}
}
int  new_get_tower_state(){
	int i = 0;
	if(tower_status==1&&return_tower_flag==1)
		return 1;
	else
	{	
		for (i = 0; i < 10; i++)
		{
			sleep(1);
			if(tower_status==1&&return_tower_flag==1)
				return 1;				
		}
		return 0;
	}
}


struct info  * get_meddata_form_ble(){
	return info;
}
#if 1
//将wifi信息传给tower
static void new_up_tower_wifi(){
	char ssid_buf[20] = {};
	char passwd_buf[20]={};
	if(!mozart_ini_getkey("/usr/data/system.ini", "base", "ssid", ssid_buf))
   	{
		Pack_write(Wifi_name, (unsigned char *)ssid_buf,strlen(ssid_buf));
		printf(".....................。。。。。。send success \n");
   	}
   	if(!mozart_ini_getkey("/usr/data/system.ini", "base", "passwd", passwd_buf))
   	{
	 	Pack_write(Wifi_passwd, (unsigned char *)passwd_buf,strlen(passwd_buf));
   	}
}
#endif

//注：这个需要重新确认传的数据情况
static int up_tower_info()
{
	char ssid_buf[20] = {};
  	char passwd_buf[20]={};
	char movie_med[100]={};
	char movie_sleep[100] = {};
	unsigned char DATA[2];
	char led_buf[8] = {};
	unsigned char da[2];
//	char zone[20] = {};
//	char time[20] = {};
//	char led_cmd[4] = {};
//	char led_id[4] = {};
	if(New_Pack_write(Weather,(unsigned char *)weather_data,4))	//天气信息
	{
		printf("pack_write_erron\n");
		return -1;
	}
	
   	if(!mozart_ini_getkey("/usr/data/system.ini", "base", "ssid", ssid_buf))   		//wifi信息——名字
   	{
		printf("ssid_buf>>>>>>>>>>%s\n",ssid_buf);
		if(New_Pack_write(Wifi_name, (unsigned char *)ssid_buf,strlen(ssid_buf))){
			printf("pack_write_erron\n");
			return -1;
		}
   	}
	
   	if(!mozart_ini_getkey("/usr/data/system.ini", "base", "passwd", passwd_buf))    //wifi信息--密码
   	{
   		printf("passwd_buf>>>>>>>>>>%s\n",passwd_buf);
	 	if(New_Pack_write(Wifi_passwd, (unsigned char *)passwd_buf,strlen(passwd_buf))){
			printf("pack_write_erron\n");
			return -1;
		}
   	}

	if(mozart_mozart_mode == FREE)
	{
		da[0]=0x01;
		da[1]=0x00;
		if(New_Pack_write(Base_station,da,2)) //新添加于2018.7.9号
		{
			printf("pack_write_erron\n");
			return -1;
		}
	}
	
	if(mozart_mozart_mode == MED)
	{
		da[0]=0x02;
		da[1]=0x00;
		if(New_Pack_write(Base_station,da,2)) 	//新添加于2018.7.9号
		{
			printf("pack_write_erron\n");
			return -1;
		}
		if(!mozart_ini_getkey("/usr/data/system.ini","med","movie",movie_med))
		{
			if(New_Pack_write(Select_gif, (unsigned char *)movie_med,strlen(movie_med)))
			{
				printf("pack_write_erron\n");
				return -1;
			}
		}
	}
	
	if(mozart_mozart_mode == SLEEP)
	{
		da[0]=0x03;
		da[1]=0x00;
		if(New_Pack_write(Base_station,da,2))	//新添加于2018.7.9号
		{
			printf("pack_write_erron\n");
			return -1;
		}
		if(!mozart_ini_getkey("/usr/data/system.ini", "sleep", "movie", movie_sleep)){
			if(movie_sleep == NULL){
				strcpy(movie_sleep,"0");
				New_Pack_write(Select_gif,(unsigned char *)movie_sleep,strlen(movie_sleep)+1);  //给tower发送选择指定动画文件指令
			}
			else{
    			New_Pack_write(Select_gif,(unsigned char *)movie_sleep,strlen(movie_sleep)+1);  //给tower发送选择指定动画文件指令
			}
		}
		if(!mozart_ini_getkey("/usr/data/system.ini", "sleep", "led", led_buf))		
 		{
			if(atoi(led_buf)==0)
			{
        		DATA[0] = 0;
				DATA[1] = 0;
				Pack_write(Light_effect,DATA,0);   //给tower发送灯光效果
			}
			else
			{
    			DATA[0] = 1;
				DATA[1] = atoi(led_buf);
				Pack_write(Light_effect,DATA,0);
			}
		}    
	}
	up_tower_sysc_time();
	return 0;
}

static void ble_response_cmd(struct BLE_INFO info)
{
	pthread_mutex_lock(&ble_mutex);
	
	/**************************2018.7.2号*************************************************/
	struct BLE_INFO *new_info = (struct BLE_INFO *)malloc(sizeof(struct BLE_INFO));
	memset(new_info,0,sizeof(struct BLE_INFO));
	memcpy(new_info,(struct BLE_INFO *)(&info),sizeof(struct BLE_INFO));
	
	/*************************************************************************************/
	list_insert_at_tail(&cmd_list, (struct BLE_INFO *)new_info);
	pthread_cond_signal(&ble_var);
	pthread_mutex_unlock(&ble_mutex);
}

//接受蓝牙发送来的数据，处理方式：保存到本地、直接回复给app
void Unpack(unsigned char *date,struct info *info,int total_length)
{
	int n=3,date_length,length;
	time_t timep;
	char original_brain_date[100];  //原始脑电数据
	char brain_date[200];           //脑电数据
	unsigned char crc=0x00;
	unsigned char crc_result=0x00;
	int i;
	//printf ("date_length=%d\n",length);
	for(i=0;i<total_length;i++)
	{
		//printf("date[%d]=%x\n",i,date[i]);
		if(date[i]==0xAA && date[i+1]==0xAA)
		{	
			//printf ("enter date\n");
			date_length=(int)date[i+2];
			if(total_length < date_length+4 || date_length > (32-i) )    //dsc 6.6
			{
				return ;
			}
			for(n=i+3 ; n<date_length+i+3 ; n++)
			{			
				crc=crc+date[n];				
			}
			crc_result=~crc;
			//printf("crc_result=%x\n",crc_result);
			//printf("crc_date=%x\n",date[date_length+i+3]);
			if (crc_result!=date[date_length+i+3])
			{
				//printf("crc error\n");					
			}				
			else
			{
				//printf("crc success\n");
				for(n=i+3 ; n<date_length+i+3 ; n++)
				{
					switch(date[i+n])
					{
						case 0x02:														//脑电质量(0x02)
						{													
							//printf("enter to %x, %x\n",date[i+n],date[i+n+1]);
							info->X_brain_quality = date[i+n+1];
							n=n+1;
							//printf("X_brain_quality=%x\n",info->X_brain_quality);
							break;
						}
						case 0x04:														//专注度参数(0x04)
						{
							info->X_brain_concentration=date[i+n+1];
							n=n+1;
							//printf("X_brain_concentration=%x\n",info->X_brain_concentration);
							break;
						}
						case 0x05:														//方松度参数(0x05)
						{
							info->X_brain_attention=date[i+n+1];
							//if(get_tower_state())
							//Pack_write(X_brain,&(info->X_brain_quality),0 );
							if(med_info.meding&&med_info.med_time>=5)
							{
								memset(brain_date, 0, 200);
								time(&timep);
								sprintf(brain_date, "%d/%d/%d,%d:%d:%d,A,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
								                     localtime(&timep)->tm_year+1900, localtime(&timep)->tm_mon+1,
				 	                                 localtime(&timep)->tm_mday,localtime(&timep)->tm_hour,
				 	                                 localtime(&timep)->tm_min,localtime(&timep)->tm_sec,
				 	                                 (int)info->X_brain_quality, (int)info->X_brain_concentration,
				 	                                 (int)info->X_brain_attention,
				 	                                 (int)info->brain_parameter.delta[0]*256*256+(int)info->brain_parameter.delta[1]*256+(int)info->brain_parameter.delta[2],
				 	                                 (int)info->brain_parameter.theta[0]*256*256+(int)info->brain_parameter.theta[1]*256+(int)info->brain_parameter.theta[2],
				 	                                 (int)info->brain_parameter.low_alpha[0]*256*256+(int)info->brain_parameter.low_alpha[1]*256+(int)info->brain_parameter.low_alpha[2],
				 	                                 (int)info->brain_parameter.high_alpha[0]*256*256+(int)info->brain_parameter.high_alpha[1]*256+(int)info->brain_parameter.high_alpha[2],
				 	                                 (int)info->brain_parameter.low_beta[0]*256*256+(int)info->brain_parameter.low_beta[1]*256+(int)info->brain_parameter.low_beta[2],
				 	                                 (int)info->brain_parameter.high_beta[0]*256*256+(int)info->brain_parameter.high_beta[1]*256+(int)info->brain_parameter.high_beta[2],
				 	                                 (int)info->brain_parameter.low_gamma[0]*256*256+(int)info->brain_parameter.low_gamma[1]*256+(int)info->brain_parameter.low_gamma[2],
				 	                                 (int)info->brain_parameter.high_gamma[0]*256*256+(int)info->brain_parameter.high_gamma[1]*256+(int)info->brain_parameter.high_gamma[2]
				 	                                 );

								fwrite(brain_date,1,strlen(brain_date),fp_B);
							}
							n=n+1;
							Pack_write(X_brain,&(info->X_brain_quality),0 );	
							printf("---------------------X_brain_attention=%x    X_brain_concentration=%x  \n",info->X_brain_attention,info->X_brain_concentration);
							break;
						}
						case 0x83:														//频谱参数(0x83)
						{
							memcpy(&(info->brain_parameter),&date[i+n+2],24);
							n=n+25;
							
							/*************************修改于5.29号************************************/
							//if (mozart_ingenicplayer_brain_data())
							//{
							//	printf("mozart_ingenicplayer_brain_data 错误\n");
							//}
							/************************************************************************/
							//printf("brain_parameter=%x,%x,%x\n",info->brain_parameter.delta[0],info->brain_parameter.delta[1],info->brain_parameter.delta[2]);
							//printf("brain_parameter2=%x,%x,%x\n",info->brain_parameter.theta[0],info->brain_parameter.theta[1],info->brain_parameter.theta[2]);
							break;
						}					    
						case 0x16:														//眨眼数据(0x16)
						{
							info->X_blink=date[i+n+1];
							n=n+1;
							//printf("X_blink=%x\n",info->X_blink);
							break;
						}	 
						case 0x20:														//X 的电量数据(0x20)
						{
							//修改（2.26号）
							if((int)date[i+n+1] > 128){
								info->X_charge = 1;
								info->X_power= date[i+n+1] - 0x80;
							}
							else{
								info->X_charge = 0;
								info->X_power= date[i+n+1];
							}
							n=n+1;
							//printf("power=%x\n",info->X_power);
							break;
						}	
						case 0x80:														//原始脑电数据(0x80)
						{
							memcpy(info->X_brain,&date[i+n+2],2);
							n=n+3;			  
							//printf("X_brain_heigh=%x\n",info->X_brain[0]);
							//printf("X_brain_low=%x\n",info->X_brain[1]);
							//int a = (int)(info->X_brain[0])*256+(int)(info->X_brain[1]);
							//printf("X_brain=%d\n",a);
							break;
						}	
						case 0x81:														//原始脑电组合数据(0x81)
			            	if(med_info.meding&&med_info.med_time>=5)
							{
								memset(original_brain_date, 0, 100);
								time(&timep);
								sprintf(original_brain_date, "%d/%d/%d,%d:%d:%d,W,%d,%d,%d,%d,%d,%d,%d\n",
								localtime(&timep)->tm_year+1900, localtime(&timep)->tm_mon+1,
				 	                      localtime(&timep)->tm_mday,localtime(&timep)->tm_hour,
				 	                      localtime(&timep)->tm_min,localtime(&timep)->tm_sec,
				 	                      (int)date[i+n+2]*256+(int)date[i+n+3], (int)date[i+n+4]*256+(int)date[i+n+5],
				 	                      (int)date[i+n+6]*256+(int)date[i+n+7], (int)date[i+n+8]*256+(int)date[i+n+9],
				 	                      (int)date[i+n+10]*256+(int)date[i+n+11], (int)date[i+n+12]*256+(int)date[i+n+13],
				 	                      (int)date[i+n+14]*256+(int)date[i+n+15]
				 	                      );
								fwrite(original_brain_date,1,strlen(original_brain_date),fp_R);
								printf("\noriginal_brain_date=%s\n",original_brain_date);
							}	
							n=n+15;	
							break;
						    /***********以下n=n+1;未测试***************************/
						case 0x90:										//X 的运动姿态数据(0x90)
						{
							memcpy(&(info->motion),&date[i+n+2],12);
							//if(get_tower_state())
							//Pack_write( motion ,(unsigned char *)&(info->motion) ,0);
							n=n+13;
							//printf("x_acc=%x\n",info->motion.x_acc);
							//printf("y_acc=%x\n",info->motion.y_acc);
							Pack_write( motion ,(unsigned char *)&(info->motion) ,0);
							//  Pack_write(X_brain_concentration,NULL,0);
							break;
						}	
						case 0x91:														//X 的设备型号(0x91)
						{
							length=(int)date[i+n+1];
							memset(info->X_model,0,20);
							memcpy(info->X_model,&date[i+n+2],length);
							n=n+length+1;
							break;
						}//长度不固定
						case 0x92:														//X 的设备序列号数据(0x92)
						{
							length=(int)date[i+n+1];
							memset(info->X_number,0,20);
							memcpy(info->X_number,&date[i+n+2],length);
							n=n+length+1;
							break;
						}//长度不固定
						case 0x93:														//X 的固件版本号数据(0x93)
						{
							length=(int)date[i+n+1];
							memset(info->X_binnumber,0,20);
							memcpy(info->X_binnumber,&date[i+n+2],length);
							n=n+length+1;
							break;
						}//长度不固定  	
						case 0x94:														//X 的 BLE 设备号数据(0x94)
						{
							length=(int)date[i+n+1];
							memset(info->X_BLEnumber,0,20);
							memcpy(info->X_BLEnumber,&date[i+n+2],length);
							n=n+length+1;
							break;
						}//长度不固定 
										   								
										   								
						//***********以下tower发送给base的数据处理***************************/
						case 0x21:														//数据请求指令 (0x21)	
			            {
							info->Tower_request=date[i+n+1];
							n=n+1;
							if(info->Tower_request==0x01)
							{
			                	new_up_tower_wifi();     //上传wifi信息
							}
							if(info->Tower_request==0x02)
							{
								int seconds;
								unsigned char DATA[7];
								char zone_buf[8] = {};
								char zone_buf_1[4] = {};
								char zone_buf_2[4] = {};
			                    char tformat_buf[8] = {};
							    seconds = time((time_t*)NULL);
				                DATA[3]  = seconds %256;
			                    seconds /=256;
			                    DATA[2]  = seconds %256;
			                    seconds /=256;
			                    DATA[1]  = seconds %256;
			                    seconds /=256;   
			                    DATA[0] = seconds;
			                    if(!mozart_ini_getkey("/usr/data/system.ini", "time", "zone", zone_buf))
								strncpy(zone_buf_1,zone_buf,2);
								strncpy(zone_buf_2,zone_buf+2,2);
			                   	DATA[5] = atoi(zone_buf_1);
								DATA[6] = atoi(zone_buf_2);
			                    if(!mozart_ini_getkey("/usr/data/system.ini", "time", "tformat", tformat_buf))
			                    DATA[4] =	atoi(tformat_buf);
					            Pack_write(Sync_time,DATA,6);
							}
							if(info->Tower_request==0x03)
							{
							}
							break;
						}						
						case 0x22:														//Tower的电量数据(0x22)
						{
							info->Tower_power=date[i+n+1];
							info->Tower_if_power = date[i+n+2];
							n=n+1;
							break;
						}	
						case 0xA0:                    //灯光效果文件列表（0xa0）
						{
							length=(int)date[i+n+1];		
							memset(light_list[date[i+n+3]],0,13);
							memcpy(light_list[date[i+n+3]],&date[i+n+4],length-2);
							printf("\n-----------------%d:%s",date[i+n+3],light_list[date[i+n+3]]);
			                if(date[i+n+3]==date[i+n+2])       //文件总个数等于第几个文件
							mozart_ingenicplayer_light_list(date[i+n+2]);
						}
						case 0xA1:														//当前存储的WIFI的SSID (0xA1)
						{
							length=(int)date[i+n+1];
							memset(info->Tower_WIFI_Name,0,20);
							memcpy(info->Tower_WIFI_Name,&date[i+n+2],length);
							n=n+length+1;
							break;
						}//长度不固定 
						case 0xA2:														//当前存储的 WIFI 的密码(0xA2) 
						{
							length=(int)date[i+n+1];
							memset(info->Tower_WIFI_Password,0,20);
							memcpy(info->Tower_WIFI_Password,&date[i+n+2],length);
							n=n+length+1;
							break;
						}//长度不固定
						case 0xA3:														//Tower的设备型号数据(0xA3)
						{
							length=(int)date[i+n+1];
							memset(info->Tower_type,0,20);
							memcpy(info->Tower_type,&date[i+n+2],length);
							n=n+length+1;
							break;
						}//长度不固定 
						case 0xA4:														//Tower的设备序列号数据(0xA4)
						{
							length=(int)date[i+n+1];
							memset(info->Tower_number,0,20);
							memcpy(info->Tower_number,&date[i+n+2],length);
							n=n+length+1;
							break;
						}//长度不固定
						case 0xA5:														//Tower的固件版本号数据(0xA5)
						{
							//修改：（4.26）
							length=(int)date[i+n+1];
							memset(info->Tower_binnumber,0,20);
							printf("info->Tower_binnumber:%x...........\n",date[i+n+2]);
							if(date[i+n+2] == 0x01){
								memcpy(info->Tower_binnumber,&date[i+n+3],6);
								printf("info->Tower_binnumber:%s...........\n",(char *)info->Tower_binnumber);
								memcpy(g_Tower_binnumber,info->Tower_binnumber,6);	//新添加于2018.7.31号

							}else if(date[i+n+2]  == 0x02){
								memcpy(info->Tower_os,&date[i+n+3],6);
								printf("info->Tower_os:%s...........\n",(char *)&date[i+n+2]);

							}else if(date[i+n+2]  == 0x03){
								memcpy(info->Tower_blever,&date[i+n+3],6);
								printf("info->Tower_blever:%s...........\n",(char *)&date[i+n+2]);
							}
							n=n+length+1;
							break;
						}//长度不固定 							
						case 0xA6:														//Tower的 BLE 设备号数据(0xA6)
						{
							length=(int)date[i+n+1];
							memset(info->Tower_BLEnumber,0,20);
							memcpy(info->Tower_BLEnumber,&date[i+n+2],length);
							n=n+length+1;
							break;
						}//长度不固定 	
						case 0xA7:
						{
							length=(int)date[i+n+1];
							memset(info->Tower_MAC,0,20);
							memcpy(info->Tower_MAC,&date[i+n+2],length);
							n=n+length+1;   //
							break;	
						}
						case 0xA8:														//Tower的 SD 卡容量数据(0xA8)
						{
							length=(int)date[i+n+1];
							info->Tower_sd_capacity=date[i+n+2]+date[i+n+3];  //修改于2018.7.13
							//info->Tower_sd_capacity=date[i+n+2]*256+date[i+n+3];
							n=n+length+1;
							printf("【%s】 %s %d info->Tower_sd_capacity：%d\n",__FILE__, __func__, __LINE__,(int)info->Tower_sd_capacity);
							break;
						}//长度不固定
						case 0xA9:														//Tower的 SD 卡剩余容量数据(0xA9)
						{
							length=(int)date[i+n+1];
							info->Tower_sd_rest=date[i+n+2]*256*256*256+date[i+n+3]*256*256+date[i+n+4]*256+date[i+n+5];
							n=n+length+1;
							printf("【%s】 %s %d info->Tower_sd_rest：%d\n",__FILE__, __func__, __LINE__,(int)info->Tower_sd_rest);
							break;
						}//长度不固定
						case 0xAA:											  		//Tower的动画播放状态(0xAA)  
						{
							length=(int)date[i+n+1];
							memset(info->Tower_gif_state,0,20);
							memcpy(info->Tower_gif_state,&date[i+n+2],length);
							mozart_ingenicplayer_gif_state(info->Tower_gif_state);
							n=n+length+1;
							break;
						}//长度不固定
						case 0xAB:											  		//Tower的灯光状态数据(0xAB)
						{
							length=3;
							memcpy(&(info->Tower_lighting_state),&date[i+n+2],3);
							mozart_ingenicplayer_light_state(info->Tower_lighting_state);
							n=n+length+1;
							break;
						}//长度不固定				
						case 0xAC:											  		//Tower的升级状态数据(0xAC)
						{
							length=2;
							memcpy(info->Tower_upgrade_state,&date[i+n+2],2);
							mozart_ingenicplayer_upgrade_state(info->Tower_upgrade_state);
							n=n+length+1;
							break;
						}//长度不固定		 										
						case 0xAD:											  		//Tower的WIFI连接状态信息(0xAD)
						{
							//修改（4.26）
							length=7;
							if(date[i+n+2] == 0x00){
								memcpy(info->Tower_WIFI_state,&date[i+n+2],1);

							}else if(date[i+n+2]  == 0x01){
								memcpy(info->Tower_WIFI_state,&date[i+n+2],1);

							}else if(date[i+n+2]  == 0x02){
								memcpy(info->Tower_WIFI_state,&date[i+n+2],1);
							}else if(date[i+n+2]  == 0x03){
								memcpy(info->Tower_WIFI_state,&date[i+n+2],1);
							}
							memcpy(info->Tower_wlanIp,&date[i+n+3],4);
							mozart_ingenicplayer_wifi_state(info->Tower_WIFI_state);
							n=n+length+1;
							break;
						}//长度不固定
						case 0xAE:											  		//Tower的动画文件列表数据(0xAE)
						{
							length=(int)date[i+n+1];		
							memset(gif_list[date[i+n+3]&0x7F],0,13)	;
							memcpy(gif_list[date[i+n+3]&0x7F],&date[i+n+4],length-2);
							printf("\n%d:%s\n",date[i+n+3],gif_list[date[i+n+3]]);		 
			                if((date[i+n+3]&0x7F)==date[i+n+2])
							mozart_ingenicplayer_gif_list(date[i+n+2]);
							n=n+length+1;
							break;
						}//长度不固定						
						case 0xAF:											  		//Tower的时钟设置状态数据(0xAF)
						{
							length=6;
							memcpy(info->Tower_clock_setting,&date[i+n+2],length);
							n=n+length+1;
							break;
						}//长度不固定								
						case 0xD9:
						{
							length = (int)date[i+n+1];
							memcpy(info->Tower_Beiyong,&date[i+n+2],length);
							reply_beiyong(info->Tower_Beiyong);
							n=n+length+1;
							break;
						}
						
					}
				}
			}

			i = i+date_length+4;
		}
	}
}

int  New_Pack_write(enum command command,unsigned char *date,int length)   //发送蓝牙对应指令
{	
	   if(!new_get_tower_state())
	   	{
	   		printf(".....this is erron.........................................this is erron\n");
	   		return -1;
	   	}
		ble_client_write_data cl_wdata;
		int state = 0;
		int i = 0;
		int num = 30;	
		
		unsigned char buffer[36];
		buffer[0]=0xAA;
		buffer[1]=0xAA;
		unsigned char crc=0x00;
		int date_length;
		
		switch(command){
				case X_brain:					//信号质量（0x02）
					{
						buffer[2]=0x20;
						buffer[3]=0x02;
						buffer[4]=date[0];
						buffer[5]=0X83;
						buffer[6]=0X18;
						memcpy(&buffer[7],&date[3],24);
						buffer[31]=0X04;
						buffer[32]=date[1];
						buffer[33]=0X05;
						buffer[34]=date[2];
						break;
						}
				case X_brain_concentration:	//专注度参数(0x04)
					{
						buffer[2]=0x20;
						buffer[3]=0x02;
						buffer[4]=0x23;
						buffer[5]=0X83;
						buffer[6]=0X18;
			
						buffer[7]=0x01;
						buffer[8]=0x02;
						buffer[9]=0x03;
						buffer[10]=0X04;
						buffer[11]=0X05;
						
						buffer[12]=0x06;
						buffer[13]=0x07;
						buffer[14]=0x08;
						buffer[15]=0X09;
						buffer[16]=0X0a;
						buffer[17]=0x0b;
						buffer[18]=0x0c;
						buffer[19]=0x0d;
						buffer[20]=0X0e;
						buffer[21]=0X0f;
						buffer[22]=0X10;
						buffer[23]=0X11;
						buffer[24]=0x12;
						buffer[25]=0x13;
						buffer[26]=0x14;
						buffer[27]=0X15;
						buffer[28]=0X16;
						buffer[29]=0X17;
						buffer[30]=0x18;
						buffer[31]=0X04;
						buffer[32]=0x22;
						buffer[33]=0X05;
						buffer[34]=0x33;
						
					//buffer[2]=0x02;
					//buffer[3]=0x04;
					//buffer[4]=date[0];
					break;
						}
				case X_brain_attention:			//注意力参数(0x05)
					{
						buffer[2]=0x02;
						buffer[3]=0x05;
						buffer[4]=date[0];
						break;
						}	
				case X_brain_parameter:				//频谱参数(0x83)	
					{
						buffer[2]=0x1A;						//报文长度26字节
						buffer[3]=0x83;
						buffer[4]=0x18;						//数据长度24字节
						memcpy(&buffer[5],&date[0],24);
						break;
						}
				case REQUEST_Name_passwd:			//请求发送 Tower 当前配置的 SSID 的名称和密码0x01 
					{	buffer[2]=0x02;
						buffer[3]=0x21;
						buffer[4]=0x01;
						break;
						}
				case REQUEST_Tower_type:		 //请求发送 Tower 设备型号0x03
					{
						buffer[2]=0x02;
						buffer[3]=0x21;
						buffer[4]=0x03;
						break;
						}	
			case REQUEST_Tower_number:			//请求发送 Tower 设备序列号0x04
					{
						buffer[2]=0x02;
						buffer[3]=0x21;
						buffer[4]=0x04;
						break;
						}	
			case REQUEST_Tower_binnumber:		  //请求发送 Tower 固件版本号0x05  
					{
						buffer[2]=0x02;
						buffer[3]=0x21;
						buffer[4]=0x05;
						break;
						}	
			case REQUEST_Tower_BLEnumber:			//请求发送 BLE 设备号0x06
					{
						buffer[2]=0x02;
						buffer[3]=0x21;
						buffer[4]=0x06;
						break;
						}	
			case REQUEST_Tower_sd:						//请求发送 SD 卡信息0x08
					{
						buffer[2]=0x02;
						buffer[3]=0x21;
						buffer[4]=0x08;
						break;
						}	
			case REQUEST_Tower_gif_state:			//请求发送动画播放状态0x10
					{
						buffer[2]=0x02;
						buffer[3]=0x21;
						buffer[4]=0x10;
						break;
						}			
			case REQUEST_Tower_lighting_state:	//请求发送灯光状态0x11
					{
						buffer[2]=0x02;
						buffer[3]=0x21;
						buffer[4]=0x11;
						break;
						}			
			case REQUEST_Tower_WIFI_state:			//请求发送 WIFI 连接状态0x12
					{
						buffer[2]=0x02;
						buffer[3]=0x21;
						buffer[4]=0x12;
						break;
						}	
			case Tower_on:							//Tower 的 WIFI 打开指令(0x22) 
					{
						buffer[2]=0x02;
						buffer[3]=0x22;
						buffer[4]=0x01;
						break;
						}	
			case Tower_off:							//Tower 的 WIFI 关闭指令(0x22)	
					{
						buffer[2]=0x02;
						buffer[3]=0x22;
						buffer[4]=0x00;
						break;
						}	
			case Get_gif_list:								//获取动画文件列表(0x23)	
					{
						printf("\n ----------------get gif list---------------------- \n");
						buffer[2]=0x02;
						buffer[3]=0x23;
						buffer[4]=date[0];
						break;
						}
			case Get_light_list:								//获取灯光效果文件（024）	
					{
						printf("\n ----------------get light list---------------------- \n");
						buffer[2]=0x02;
						buffer[3]=0x24;
						buffer[4]=0x00;
						break;
						}
			case Control_gif:								//动画播放控制指令(0x25)		
					{
						buffer[2]=0x02;
						buffer[3]=0x25;
						buffer[4]=date[0];				//[DATA]指令定义： 0-播放； 1-暂停； 2-上一首；3-下一首
						break;
						}	
			case Check_update:								//检查升级包指令(0x26)
					{
						buffer[2]=0x02;
						buffer[3]=0x26;
						buffer[4]=0x01;
						break;
						}				
			case Reset:								//恢复出厂配置指令(0x27)
					{
						buffer[2]=0x02;
						buffer[3]=0x27;
						buffer[4]=0x00;
						break;
						}								
			case Power_on:								//开机指令(0x28) date[0]=0x01打开
					{
						buffer[2]=0x02;
						buffer[3]=0x28;
						buffer[4]=0x01;
						break;
						}	
			case Power_off:								//关机指令(0x28) date[0]=0x00关闭
					{
						buffer[2]=0x02;
						buffer[3]=0x28;
						buffer[4]=0x00;
						break;
						}							
			case motion:								//运动姿态数据(0x90)
					{
						buffer[2]=0x0E;
						buffer[3]=0x90;
						buffer[4]=0x0C;
						memcpy(&buffer[5],&date[0],12);
						break;
						}	
						
						
			case Wifi_name:								//WIFI 的 SSID (0x91)
					{
						printf("\n进入了 wifi——name\n");
						buffer[2]=(char)(length+2);
						buffer[3]=0x91;
						buffer[4]=(char)length;
						memcpy(&buffer[5],&date[0],length);
						break;
						}	
			case Wifi_passwd:								//WIFI 的密码(0x92)
					{
						buffer[2]=(char)(length+2);
						buffer[3]=0x92;
						buffer[4]=(char)length;
						memcpy(&buffer[5],&date[0],length);
						break;
						}			
			case Sync_time:								//时钟同步数据(0x93)
					{	
						buffer[2]=0x08;
						buffer[3]=0x93;
						buffer[4]=0x07;
						memcpy(&buffer[5],&date[0],7);
						break;
						}	
			case Weather:								//天气数据(0x94)
					{	
						buffer[2]=0x06;
						buffer[3]=0x94;
						buffer[4]=(char)length;
						memcpy(&buffer[5],&date[0],length);
						break;
						}	
			case Light_effect:								//灯光效果指令(0x95)
					{	
						buffer[2]=0x04;
						buffer[3]=0x95;
						buffer[4]=0x02;
						memcpy(&buffer[5],&date[0],2);
						break;
						}	
			case Light_intensity:								//灯光亮度设置指令(0x96)
					{	
						buffer[2]=0x06;
						buffer[3]=0x96;
						buffer[4]=0x04;
						memcpy(&buffer[5],&date[0],4);
						break;
						}				
			case Base_station:								//Base 工作状态数据(0x97)
					{	
						buffer[2]=0x0C;
						buffer[3]=0x97;
						buffer[4]=0x0A;
						memcpy(&buffer[5],&date[0],2);
						break;
						}
			case Select_gif:								//选择指定动画文件指令(0x98)
					{	
						buffer[2]=0x0E;
						buffer[3]=0x98;
						buffer[4]=0x0C;
						memcpy(&buffer[5],&date[0],length);
						break;
						}				
			case Beiyong:								//閫塡D4\F1鎸嘰B6\A8\B6\AF\BB\AD\CE鍕焅FE鎸嘰C1\EE(0x98)
					{	
						buffer[2]=0x10;
						buffer[3]=0xd8;
						buffer[4]=0x0E;
						memcpy(&buffer[5],&date[0],length);
						break;
						}	
		}
							
		date_length=(int)(buffer[2]);			
		for(i=3;i<(date_length+3);i++)
		{
			crc=crc+buffer[i];
			}			
		buffer[date_length+3]=~crc;

	if(date_length>30){


				//cl_wdata.write_len = date_length + 4;
				cl_wdata.write_len = 18;
				//cl_wdata.service_uuid = BSA_BLE_UUID_SERVCLASS_HEART_RATE;
				//cl_wdata.char_uuid = BSA_BLE_GATT_UUID_HEART_RATE_MEASUREMENT;
				cl_wdata.service_uuid = 0x1807;
				cl_wdata.char_uuid = 0x2A25;
				memcpy(cl_wdata.write_data , buffer , 18);
				//cl_wdata.write_data[0] = 0x11;

				cl_wdata.client_num = client_num2;
				cl_wdata.is_primary = 1;
				cl_wdata.ser_inst_id = 0;
				cl_wdata.char_inst_id = 0;
				cl_wdata.is_descript = 0;
				cl_wdata.descr_id = 0;
				cl_wdata.desc_value = 0;
				cl_wdata.write_type = 1;
				state = mozart_bluetooth_ble_client_write(&cl_wdata);  //给tower发送数据cl_wdate；
				if (state != 0) {
					printf("mozart_bluetooth_ble_client_write111 failed !\n");
					return -1;
				}
				for (i = 0; i < num; i++) {
					if(ble_client_write_enable != 1)
						usleep(300*1000);
					else {
						ble_client_write_enable = 0;
						break;
					}
				}
				if (i >= num) {
					printf("ble_client_write_enable111 failed!\n");
					return -1;
				}
				//-------\B5璇橽FE\B0\FC\CA\FD\BE\DD-----------//
				memcpy(cl_wdata.write_data , &buffer[18] , 18);
				state = mozart_bluetooth_ble_client_write(&cl_wdata);
				if (state != 0) {
					printf("mozart_bluetooth_ble_client_write222 failed !\n");
					return -1;
				}
				for (i = 0; i < num; i++) {
					if(ble_client_write_enable != 1)
						usleep(300*1000);
					else {
						ble_client_write_enable = 0;
						break;
					}
				}
				if (i >= num) {
					printf("ble_client_write_enable222 failed!\n");
					return -1;
				}
	
	}
	else{
		cl_wdata.write_len = date_length + 4;
		//cl_wdata.service_uuid = BSA_BLE_UUID_SERVCLASS_HEART_RATE;
		//cl_wdata.char_uuid = BSA_BLE_GATT_UUID_HEART_RATE_MEASUREMENT;
		cl_wdata.service_uuid = 0x1807;    //服务端的uuid
		cl_wdata.char_uuid = 0x2A25;
	//	printf("\n");
	//	for(i=0;i<date_length + 4;i++)
//			{
	//		printf("%x  ",(int)buffer[i]);

	//		}
		memcpy(cl_wdata.write_data , buffer , date_length+4);
		//cl_wdata.write_data[0] = 0x11;

		cl_wdata.client_num = client_num2;
		cl_wdata.is_primary = 1;
		cl_wdata.ser_inst_id = 0;
		cl_wdata.char_inst_id = 0;
		cl_wdata.is_descript = 0;
		cl_wdata.descr_id = 0;
		cl_wdata.desc_value = 0;
		cl_wdata.write_type = 1;
		state = mozart_bluetooth_ble_client_write(&cl_wdata);   //将数据写到给蓝牙的server
		if (state != 0) {
			printf("mozart_bluetooth_ble_client_write failed !\n");
			return -1;
		}
		for (i = 0; i < num; i++) {
			
			if(ble_client_write_enable != 1)
			{
			//	printf("\n&&&&&&&&&&&&&&&&&&  蓝牙没写成功\n");
				usleep(300*1000);
			}
			else {
			//	printf("\n&&&&&&&&&&&&&&&&&&&  蓝牙写成功，状态位已改变\n");
				ble_client_write_enable = 0;
				break;
			}
		}
		if (i >= num) {
			printf("ble_client_write_enable failed!\n");
			return -1;
		}
	}
	return 0;

}


int  Pack_write(enum command command,unsigned char *date,int length)   //发送蓝牙对应指令
{	
	   if(!get_tower_state())
	   	{
	   	//	printf(".....this is erron.........................................this is erron\n");
	   		return -1;
	   	}
		ble_client_write_data cl_wdata;
		int state = 0;
		int i = 0;
		int num = 30;	
		
		unsigned char buffer[36];
		buffer[0]=0xAA;
		buffer[1]=0xAA;
		unsigned char crc=0x00;
		int date_length;
		
		switch(command){
				case X_brain:					//信号质量（0x02）
					{
						buffer[2]=0x20;
						buffer[3]=0x02;
						buffer[4]=date[0];
						buffer[5]=0X83;
						buffer[6]=0X18;
						memcpy(&buffer[7],&date[3],24);
						buffer[31]=0X04;
						buffer[32]=date[1];
						buffer[33]=0X05;
						buffer[34]=date[2];
						break;
						}
				case X_brain_concentration:	//专注度参数(0x04)
					{
						buffer[2]=0x20;
						buffer[3]=0x02;
						buffer[4]=0x23;
						buffer[5]=0X83;
						buffer[6]=0X18;
			
						buffer[7]=0x01;
						buffer[8]=0x02;
						buffer[9]=0x03;
						buffer[10]=0X04;
						buffer[11]=0X05;
						
						buffer[12]=0x06;
						buffer[13]=0x07;
						buffer[14]=0x08;
						buffer[15]=0X09;
						buffer[16]=0X0a;
						buffer[17]=0x0b;
						buffer[18]=0x0c;
						buffer[19]=0x0d;
						buffer[20]=0X0e;
						buffer[21]=0X0f;
						buffer[22]=0X10;
						buffer[23]=0X11;
						buffer[24]=0x12;
						buffer[25]=0x13;
						buffer[26]=0x14;
						buffer[27]=0X15;
						buffer[28]=0X16;
						buffer[29]=0X17;
						buffer[30]=0x18;
						buffer[31]=0X04;
						buffer[32]=0x22;
						buffer[33]=0X05;
						buffer[34]=0x33;
						
						//buffer[2]=0x02;
						//buffer[3]=0x04;
						//buffer[4]=date[0];
						break;
						}
				case X_brain_attention:			//注意力参数(0x05)
					{
						buffer[2]=0x02;
						buffer[3]=0x05;
						buffer[4]=date[0];
						break;
						}	
				case X_brain_parameter:				//频谱参数(0x83)	
					{
						buffer[2]=0x1A;						//报文长度26字节
						buffer[3]=0x83;
						buffer[4]=0x18;						//数据长度24字节
						memcpy(&buffer[5],&date[0],24);
						break;
						}
				case REQUEST_Name_passwd:			//请求发送 Tower 当前配置的 SSID 的名称和密码0x01 
					{	buffer[2]=0x02;
						buffer[3]=0x21;
						buffer[4]=0x01;
						break;
						}
				case REQUEST_Tower_type:		 //请求发送 Tower 设备型号0x03
					{
						buffer[2]=0x02;
						buffer[3]=0x21;
						buffer[4]=0x03;
						break;
						}	
				case REQUEST_Tower_number:			//请求发送 Tower 设备序列号0x04
					{
						buffer[2]=0x02;
						buffer[3]=0x21;
						buffer[4]=0x04;
						break;
						}	
				case REQUEST_Tower_binnumber:		  //请求发送 Tower 固件版本号0x05  
					{
						buffer[2]=0x02;
						buffer[3]=0x21;
						buffer[4]=0x05;
						break;
						}	
				case REQUEST_Tower_BLEnumber:			//请求发送 BLE 设备号0x06
					{
						buffer[2]=0x02;
						buffer[3]=0x21;
						buffer[4]=0x06;
						break;
						}	
				case REQUEST_Tower_sd:						//请求发送 SD 卡信息0x08
					{
						buffer[2]=0x02;
						buffer[3]=0x21;
						buffer[4]=0x08;
						break;
						}	
				case REQUEST_Tower_gif_state:			//请求发送动画播放状态0x10
					{
						buffer[2]=0x02;
						buffer[3]=0x21;
						buffer[4]=0x10;
						break;
						}			
				case REQUEST_Tower_lighting_state:	//请求发送灯光状态0x11
					{
						buffer[2]=0x02;
						buffer[3]=0x21;
						buffer[4]=0x11;
						break;
						}			
				case REQUEST_Tower_WIFI_state:			//请求发送 WIFI 连接状态0x12
					{
						buffer[2]=0x02;
						buffer[3]=0x21;
						buffer[4]=0x12;
						break;
						}	
				case Tower_on:							//Tower 的 WIFI 打开指令(0x22) 
					{
						buffer[2]=0x02;
						buffer[3]=0x22;
						buffer[4]=0x01;
						break;
						}	
				case Tower_off:							//Tower 的 WIFI 关闭指令(0x22)	
					{
						buffer[2]=0x02;
						buffer[3]=0x22;
						buffer[4]=0x00;
						break;
						}	
				case Get_gif_list:								//获取动画文件列表(0x23)	
					{
						printf("\n ----------------get gif list---------------------- \n");
						buffer[2]=0x02;
						buffer[3]=0x23;
						buffer[4]=date[0];
						break;
						}
				case Get_light_list:								//获取灯光效果文件（024）	
					{
						printf("\n ----------------get light list---------------------- \n");
						buffer[2]=0x02;
						buffer[3]=0x24;
						buffer[4]=0x00;
						break;
						}
				case Control_gif:								//动画播放控制指令(0x25)		
					{
						buffer[2]=0x02;
						buffer[3]=0x25;
						buffer[4]=date[0];				//[DATA]指令定义： 0-播放； 1-暂停； 2-上一首；3-下一首
						break;
						}	
				case Check_update:								//检查升级包指令(0x26)
					{
						buffer[2]=0x02;
						buffer[3]=0x26;
						buffer[4]=0x01;
						break;
						}				
				case Reset:								//恢复出厂配置指令(0x27)
					{
						buffer[2]=0x02;
						buffer[3]=0x27;
						buffer[4]=0x00;
						break;
						}								
				case Power_on:								//开机指令(0x28) date[0]=0x01打开
					{
						buffer[2]=0x02;
						buffer[3]=0x28;
						buffer[4]=0x01;
						break;
						}	
				case Power_off:								//关机指令(0x28) date[0]=0x00关闭
					{
						buffer[2]=0x02;
						buffer[3]=0x28;
						buffer[4]=0x00;
						break;
						}							
				case motion:								//运动姿态数据(0x90)
					{
						buffer[2]=0x0E;
						buffer[3]=0x90;
						buffer[4]=0x0C;
						memcpy(&buffer[5],&date[0],12);
						break;
						}	
						
						
				case Wifi_name:								//WIFI 的 SSID (0x91)
					{
						buffer[2]=(char)(length+2);
						buffer[3]=0x91;
						buffer[4]=(char)length;
						memcpy(&buffer[5],&date[0],length);
						break;
						}	
				case Wifi_passwd:								//WIFI 的密码(0x92)
					{
						buffer[2]=(char)(length+2);
						buffer[3]=0x92;
						buffer[4]=(char)length;
						memcpy(&buffer[5],&date[0],length);
						break;
						}			
				case Sync_time:								//时钟同步数据(0x93)
					{	
						buffer[2]=0x08;
						buffer[3]=0x93;
						buffer[4]=0x07;
						memcpy(&buffer[5],&date[0],7);
						break;
						}	
				case Weather:								//天气数据(0x94)
					{	
						printf("\n.....................................weather....................\n");
						buffer[2]=0x06;
						buffer[3]=0x94;
						buffer[4]=(char)length;
						memcpy(&buffer[5],&date[0],length);
						break;
						}	
				case Light_effect:								//灯光效果指令(0x95)
					{	
						buffer[2]=0x04;
						buffer[3]=0x95;
						buffer[4]=0x02;
						memcpy(&buffer[5],&date[0],2);
						break;
						}	
				case Light_intensity:								//灯光亮度设置指令(0x96)
					{	
						buffer[2]=0x06;
						buffer[3]=0x96;
						buffer[4]=0x04;
						memcpy(&buffer[5],&date[0],4);
						break;
						}				
				case Base_station:								//Base 工作状态数据(0x97)
					{	
						buffer[2]=0x0C;
						buffer[3]=0x97;
						buffer[4]=0x0A;
						memcpy(&buffer[5],&date[0],2);
						break;
						}
				case Select_gif:								//选择指定动画文件指令(0x98)
					{	
						buffer[2]=(char)(length + 2);
						buffer[3]=0x98;
						buffer[4]=(char)length;
						memcpy(&buffer[5],&date[0],length);
						break;
						}				
				case Beiyong:								//备用协议，帮助app给tower发送数据(0x98)
					{	
						buffer[2]=0x10;
						buffer[3]=0xd8;
						buffer[4]=0x0E;
						memcpy(&buffer[5],&date[0],length);
						break;
						}	
		}
							
		date_length=(int)(buffer[2]);			
		for(i=3;i<(date_length+3);i++)
		{
			crc=crc+buffer[i];
		}			
		buffer[date_length+3]=~crc;
		
		//机制决定每次最大只能发送不超过25
		if(date_length>30){    
			//cl_wdata.write_len = date_length + 4;
			cl_wdata.write_len = 18;
			//cl_wdata.service_uuid = BSA_BLE_UUID_SERVCLASS_HEART_RATE;
			//cl_wdata.char_uuid = BSA_BLE_GATT_UUID_HEART_RATE_MEASUREMENT;
			cl_wdata.service_uuid = 0x1807;    //固定死的，
			cl_wdata.char_uuid = 0x2A25;       //固定死的
			memcpy(cl_wdata.write_data , buffer , 18);
			//cl_wdata.write_data[0] = 0x11;

			cl_wdata.client_num = client_num2;
			//往下没改动
			cl_wdata.is_primary = 1;
			cl_wdata.ser_inst_id = 0;
			cl_wdata.char_inst_id = 0;
			cl_wdata.is_descript = 0;
			cl_wdata.descr_id = 0;
			cl_wdata.desc_value = 0;
			cl_wdata.write_type = 1;
			state = mozart_bluetooth_ble_client_write(&cl_wdata);  //给tower发送数据cl_wdate；
			if (state != 0) {
				printf("mozart_bluetooth_ble_client_write111 failed !\n");
					return -1;
			}
			for (i = 0; i < num; i++) {
				if(ble_client_write_enable != 1)
					usleep(300*1000);
				else {
					ble_client_write_enable = 0;
					break;
				}
			}
			if (i >= num) {
				printf("ble_client_write_enable111 failed!\n");
				return -1;
			}
			//-------从18位开始往后的字节发送-----------//
			memcpy(cl_wdata.write_data , &buffer[18] , 18);
			state = mozart_bluetooth_ble_client_write(&cl_wdata);
			if (state != 0) {
				printf("mozart_bluetooth_ble_client_write222 failed !\n");
				return -1;
			}
			for (i = 0; i < num; i++) {
				if(ble_client_write_enable != 1)
				usleep(300*1000);
				else {
					ble_client_write_enable = 0;
					break;
				}
			}
			if (i >= num) {
				printf("ble_client_write_enable222 failed!\n");
				return -1;
			}
		}
		else{
			cl_wdata.write_len = date_length + 4;
			//cl_wdata.service_uuid = BSA_BLE_UUID_SERVCLASS_HEART_RATE;
			//cl_wdata.char_uuid = BSA_BLE_GATT_UUID_HEART_RATE_MEASUREMENT;
			cl_wdata.service_uuid = 0x1807;
			cl_wdata.char_uuid = 0x2A25;
			//printf("\n");
			//for(i=0;i<date_length + 4;i++)
			//{
				//printf("%x  ",(int)buffer[i]);

			//}
			memcpy(cl_wdata.write_data , buffer , date_length+4);
			//cl_wdata.write_data[0] = 0x11;

			cl_wdata.client_num = client_num2;
			cl_wdata.is_primary = 1;
			cl_wdata.ser_inst_id = 0;
			cl_wdata.char_inst_id = 0;
			cl_wdata.is_descript = 0;
			cl_wdata.descr_id = 0;
			cl_wdata.desc_value = 0;
			cl_wdata.write_type = 1;
			state = mozart_bluetooth_ble_client_write(&cl_wdata);
			if (state != 0) {
				printf("mozart_bluetooth_ble_client_write failed !\n");
				return -1;
			}
			for (i = 0; i < num; i++) {
				if(ble_client_write_enable != 1)
				{
					//printf("\n&&&&&&&&&&&&&&&&&&  蓝牙没写成功\n");
					usleep(300*1000);
				}
				else {
					//printf("\n&&&&&&&&&&&&&&&&&&&  蓝牙写成功，状态位已改变\n");
					ble_client_write_enable = 0;
					break;
				}
			}
			if (i >= num) {
				printf("ble_client_write_enable failed!\n");
				return -1;
			}
		}
		return 0;
}

int ble_client_write(unsigned char *data)
{
	ble_client_write_data cl_wdata;
	int state = 0;
	int i = 0;
	int num = 30;	
	/* write connected ble device */
	cl_wdata.write_len = (int)data[2]+4;
	//cl_wdata.service_uuid = BSA_BLE_UUID_SERVCLASS_HEART_RATE;
	//cl_wdata.char_uuid = BSA_BLE_GATT_UUID_HEART_RATE_MEASUREMENT;
	cl_wdata.service_uuid = 0x1807;
	cl_wdata.char_uuid = 0x2A25;
	memcpy(cl_wdata.write_data , data , (int)data[2]+4);
	//cl_wdata.write_data[0] = 0x11;

	cl_wdata.client_num = client_num2;
	cl_wdata.is_primary = 1;
	cl_wdata.ser_inst_id = 0;
	cl_wdata.char_inst_id = 0;
	cl_wdata.is_descript = 0;
	cl_wdata.descr_id = 0;
	cl_wdata.desc_value = 0;
	cl_wdata.write_type = 1;
	state = mozart_bluetooth_ble_client_write(&cl_wdata);
	if (state != 0) {
		printf("mozart_bluetooth_ble_client_write failed !\n");
		return -1;
	}
	for (i = 0; i < num; i++) {
		if(ble_client_write_enable != 1)
			usleep(300*1000);
		else {
			ble_client_write_enable = 0;
			break;
		}
	}
	if (i >= num) {
		printf("ble_client_write_enable failed!\n");
		return -1;
	}
	return 0;
}

//使用原来创建的客户端重新连接 X 头戴
int ble_client_reconnect()
{
	printf("====>>>> CUBand_RECONNECT__6.1<<<<=====\n");
	int i = 0;
	int state = 0;
	//int disc_index = -1;//dsc1120
	//int client_num = 0;
	ble_client_connect_data cl_connect_data;
	//ble_client_read_data client_rdata;
	ble_client_write_data cl_wdata;
	BLE_CL_NOTIFREG cl_notireg;
	//char *connect_ble_name = "BSA_BLE";
	//char *connect_ble_name = "CUBand BLE";
	//UINT8  cuband_mac[6] = {0x08, 0x7c, 0xbe, 0x00, 0x00, 0x11}; 
	//UINT16 client_uuid = 0x4000;
	int num = 30;
	int duration = 5; /* Multiple of 1.28 seconds */
	
	tAPP_DISCOVERY_CB ble_buffer;//dsc1120
	tAPP_DISCOVERY_CB *ble_info=&ble_buffer;//dsc1120
	int number;//dsc1120
	int time;//dsc1120
	
	printf("====>>>> ble_client_reconnect <<<<=====\n");
	/* register ble client */

	printf("CONNECT---------------CUBand_mac:%x.%x.%x.%x.%x.%x\n",cuband_mac[0],cuband_mac[1],cuband_mac[2],cuband_mac[3],cuband_mac[4],cuband_mac[5]);
	for (i = 0; i < 4; i++) {
		/* discovery ble devices */
		state = mozart_bluetooth_ble_start_regular(duration);
		if (state != 0) {
			printf("ERROR: mozart_bluetooth_ble_start_regular failed !\n");//yizhizaizhechucuo
			return -1;
		}

		while (bsa_ble_start_regular_enable != 1) {
			usleep(300*1000);
		}
		bsa_ble_start_regular_enable = 0;
		
		ble_info = mozart_bluetooth_disc_get_device_info();		//dsc1120
		number  =  mozart_bluetooth_disc_get_device_count();	//dsc1120
		for(time = 0;time<number;time++){					//对照所有蓝牙设备名称，看是不是我们要找的CUB开头的头戴，是的话退出，此时下标time代表的是此时的信息
			if (ble_info->devs[time].device.name[0] == 'E'&&ble_info->devs[time].device.name[1] == 'G'&&ble_info->devs[time].device.name[2] == 'O'&&ble_info->devs[time].device.name[3] == '_'){
				break;
			}
		}
		if(time <number){
			printf("cuband find success\n");
			break;
		}
		
	}
	
	if (i >= 4) {
		printf("ERROR: mozart_bluetooth get cuband_info failed!\n");
		return -1;
	}
	
	/* connect ble server device */
	ble_client_open_enable = 0;
	ble_client_open_states = 1;		//dsc_1120
	cl_connect_data.device_index = 1;	/*use Device found in last discovery */

	/* Priority judged ble_name device, if ble_name is NULL, then judged bd_addr device */
	cl_connect_data.ble_name = NULL;
	memcpy(cl_connect_data.bd_addr , cuband_mac , 6);//0625
	cl_connect_data.client_num = client_num;
	cl_connect_data.direct = 1;
	mozart_bluetooth_ble_client_connect_server(&cl_connect_data);
	for (i = 0; i < num; i++) {
		if(ble_client_open_enable != 1)
			usleep(300*1000);
		else {
			ble_client_open_enable = 0;
			break;
		}
	}
	if (i > num) {
		printf("ble_client_open_enable failed!\n");
		return -1;
	}
	if(ble_client_open_states != 0){		//dsc_1120
		printf("ble_client_open_states failed!\n");
		return -1;
	}

	/* register notification */
	cl_notireg.client_num = client_num;
	//cl_notireg.service_id = BSA_BLE_UUID_SERVCLASS_HEART_RATE;
	//cl_notireg.character_id = BSA_BLE_GATT_UUID_HEART_RATE_MEASUREMENT;
	cl_notireg.service_id = 0x1806;
	cl_notireg.character_id = 0x2a20;
	cl_notireg.is_primary = 1;
	cl_notireg.ser_inst_id = 0;
	cl_notireg.char_inst_id = 0;
	state = mozart_bluetooth_ble_client_register_notification(&cl_notireg);
	printf("state = %d\n", state);

	/* open ble server notificaion */
	//cl_wdata.service_uuid = BSA_BLE_UUID_SERVCLASS_HEART_RATE;
	cl_wdata.service_uuid = 0x1806;
	cl_wdata.char_uuid = GATT_UUID_CHAR_CLIENT_CONFIG;
	cl_wdata.write_len = 2;
	cl_wdata.write_data[0] = 0x01;
	cl_wdata.write_data[1] = 0x00;
	cl_wdata.client_num = client_num;
	cl_wdata.is_primary = 1;
	cl_wdata.ser_inst_id = 0;
	cl_wdata.char_inst_id = 0;
	cl_wdata.is_descript = 0;
	cl_wdata.descr_id = 0;
	cl_wdata.desc_value = 0;
	cl_wdata.write_type = 1;
	state = mozart_bluetooth_ble_client_write(&cl_wdata);
	if (state != 0) {
		printf("mozart_bluetooth_ble_client_write failed !\n");
		return -1;
	}

	return 0;
}

//使用原来创建的信息，重新连接tower
int ble_client_reconnect2()
{
	printf("====>>>> TOWER_RECONNECT__6.1<<<<=====\n");
	int i = 0;
	int state = 0;
	//int disc_index = -1;//dsc1120
	//int client_num = 0;
	ble_client_connect_data cl_connect_data;
	//ble_client_read_data client_rdata;
	ble_client_write_data cl_wdata;
	BLE_CL_NOTIFREG cl_notireg;
	//char *connect_ble_name = "TOWER BLE";
	//char *connect_ble_name = "my Test Ble";//dsc1120
	//UINT8 tower_mac[6] = {0xac, 0xc1, 0xee, 0x1d, 0x76, 0xa4}; 
	//UINT8  tower_mac[6] = {0x08, 0x7c, 0xbe, 0x00, 0x00, 0x01}; 
	//unsigned char 
	//UINT16 client_uuid = 0x4001;
	int num = 30;
	int duration = 5; /* Multiple of 1.28 seconds */

	tAPP_DISCOVERY_CB ble_buffer;//dsc1120
	tAPP_DISCOVERY_CB *ble_info=&ble_buffer;//dsc1120
	int number;//dsc1120
	int time;//dsc1120

	printf("====>>>> %s,%d   0523 <<<<=====\n", __func__, __LINE__);
	/* register ble client */

	printf("CONNECT---------------TOWER_eir:%x,%x,%x,%x,%x,%x",tower_eir[0],tower_eir[1],tower_eir[2],tower_eir[3],tower_eir[4],tower_eir[5]);
	
	for (i = 0; i < 4; i++) {
		
		/* discovery ble devices */
		state = mozart_bluetooth_ble_start_regular(duration);
		if (state != 0) {
			printf("ERROR: mozart_bluetooth_ble_start_regular failed !\n");
			return -1;
		}

		while (bsa_ble_start_regular_enable != 1) {
			usleep(300*1000);
		}
		bsa_ble_start_regular_enable = 0;
		
		ble_info = mozart_bluetooth_disc_get_device_info();//dsc1120
		number  =  mozart_bluetooth_disc_get_device_count();//dsc1120
		for(time = 0;time<number;time++){//dsc1120
			if (ble_info->devs[time].device.name[0] == 'T'&&ble_info->devs[time].device.name[1] == 'O'&&ble_info->devs[time].device.eir_data[24]==tower_eir[0]&&ble_info->devs[time].device.eir_data[25]==tower_eir[1]&&ble_info->devs[time].device.eir_data[26]==tower_eir[2]&&ble_info->devs[time].device.eir_data[27]==tower_eir[3]&&ble_info->devs[time].device.eir_data[28]==tower_eir[4]&&ble_info->devs[time].device.eir_data[29]==tower_eir[5]){
				printf("tower find ok!\n");
				memcpy(tower_mac,ble_info->devs[time].device.bd_addr,6);
				break;
			}
		}
		if(time <number){
			printf("tower find success\n");
			break;
		}
	}
	if (i >= 4) {
		printf("ERROR: mozart_bluetooth get tower_info failed!\n");
		return -1;
	}
printf("hello_tower_mac:%x,%x,%x,%x,%x,%x\n",tower_mac[0],tower_mac[1],tower_mac[2],tower_mac[3],tower_mac[4],tower_mac[5]);
	/* connect ble server device */
	ble_client_open_enable = 0;
	ble_client_open_states = 1;		//dsc_1120
	cl_connect_data.device_index = 1;	/*use Device found in last discovery */

	/* Priority judged ble_name device, if ble_name is NULL, then judged bd_addr device */
	//cl_connect_data.ble_name =connect_ble_name;
	cl_connect_data.ble_name = NULL;
	memcpy(cl_connect_data.bd_addr , tower_mac , 6);//dsc1120
	cl_connect_data.client_num = client_num2;
	cl_connect_data.direct = 1;
	mozart_bluetooth_ble_client_connect_server(&cl_connect_data);
	for (i = 0; i < num; i++) {
		if(ble_client_open_enable != 1)
			usleep(300*1000);
		else {
			ble_client_open_enable = 0;
			break;
		}
	}
	if (i > num) {
		printf("ble_client_open_enable failed!\n");
		return -1;
	}
	if(ble_client_open_states != 0){		//dsc_1120
		printf("ble_client_open_states failed!\n");
		return -1;
	}

	/* register notification */
	cl_notireg.client_num = client_num2;
	//cl_notireg.service_id = BSA_BLE_UUID_SERVCLASS_HEART_RATE;
	//cl_notireg.character_id = BSA_BLE_GATT_UUID_HEART_RATE_MEASUREMENT;
	cl_notireg.service_id = 0x1807;
	cl_notireg.character_id = 0x2a22;
	cl_notireg.is_primary = 1;
	cl_notireg.ser_inst_id = 0;
	cl_notireg.char_inst_id = 0;
	state = mozart_bluetooth_ble_client_register_notification(&cl_notireg);
	printf("state = %d\n", state);
	if(state == 0)
		printf("----notify success----\n");

	/* open ble server notificaion */
	//cl_wdata.service_uuid = BSA_BLE_UUID_SERVCLASS_HEART_RATE;
	cl_wdata.service_uuid = 0x1807;
	cl_wdata.char_uuid = GATT_UUID_CHAR_CLIENT_CONFIG;
	cl_wdata.write_len = 2;
	cl_wdata.write_data[0] = 0x01;
	cl_wdata.write_data[1] = 0x00;
	cl_wdata.client_num = client_num2;
	cl_wdata.is_primary = 1;
	cl_wdata.ser_inst_id = 0;
	cl_wdata.char_inst_id = 0;
	cl_wdata.is_descript = 0;
	cl_wdata.descr_id = 0;
	cl_wdata.desc_value = 0;
	cl_wdata.write_type = 1;
	state = mozart_bluetooth_ble_client_write(&cl_wdata);
	if (state != 0) {
		printf("mozart_bluetooth_ble_client_write failed !\n");
		return -1;
	}
	else
		printf("-----write (open notify)success----\n");

	return 0;
}


//创建连接头戴的客户端
int ble_client_create()
{
	printf("====>>>> CUBand_CREATE__6.1<<<<=====\n");
	int i = 0;
	int state = 0;
	//int disc_index = -1;//dsc1120
	//int client_num = 0;
	ble_client_connect_data cl_connect_data;   //客户端连接的server的信息
	//ble_client_read_data client_rdata;
	ble_client_write_data cl_wdata;
	BLE_CL_NOTIFREG cl_notireg;					//通知的结构体
	//char *connect_ble_name = "BSA_BLE";
	//char *connect_ble_name = "CUBand BLE";
	//UINT8  cuband_mac[6] = {0x08, 0x7c, 0xbe, 0x00, 0x00, 0x11}; 
	UINT16 client_uuid = 0x4000;
	int num = 30;
	int duration = 5; /* Multiple of 1.28 seconds */  //??????
	
	tAPP_DISCOVERY_CB ble_buffer;			//dsc1120
	tAPP_DISCOVERY_CB *ble_info=&ble_buffer;		//dsc1120
	int number;			//dsc1120
	int time;			//dsc1120
	
	printf("====>>>> %s,%d   0523 <<<<=====\n", __func__, __LINE__);
	/* register ble client */
	client_num = state = mozart_bluetooth_ble_client_register(client_uuid);   //创建uuid为0x4000的客户端
	if (state < 0) {
		printf("ERROR: mozart_bluetooth_ble_client_register failed !\n");     //创建蓝牙客户端失败
		return -1;
	}
	for (i = 0; i < 4; i++) {
		/* discovery ble devices */
		state = mozart_bluetooth_ble_start_regular(duration);   //搜索蓝牙设备
		if (state != 0) {
			printf("ERROR: mozart_bluetooth_ble_start_regular failed !\n");
			return -1;
		}

		while (bsa_ble_start_regular_enable != 1) {
			usleep(300*1000);
		}
		bsa_ble_start_regular_enable = 0;
		
		ble_info = mozart_bluetooth_disc_get_device_info();			//dsc1120
		number  =  mozart_bluetooth_disc_get_device_count();		//dsc1120
		for(time = 0;time<number;time++){//dsc1120
			if (ble_info->devs[time].device.name[0] == 'E'&&ble_info->devs[time].device.name[1] == 'G'&&ble_info->devs[time].device.name[2] == 'O'&&ble_info->devs[time].device.name[3] == '_'){
				break;
			}
		}
		if(time <number){
			printf("cuband find success\n");
			break;
		}	
	}
	
	if (i >= 4) {
		printf("ERROR: mozart_bluetooth get cuband_info failed!\n");    //没有搜索到蓝牙设备
		return -1;
	}
	
	/* connect ble server device */
	ble_client_open_enable = 0;
	ble_client_open_states = 1;				//dsc_1120
	cl_connect_data.device_index = 1;		/*use Device found in last discovery */

	/* Priority judged ble_name device, if ble_name is NULL, then judged bd_addr device */
	//cl_connect_data.ble_name = connect_ble_name;
	cl_connect_data.ble_name =NULL;     //通过mac地址连接的话地址必须设置成NULL；
	memcpy(cl_connect_data.bd_addr , cuband_mac , 6);		//0625
	cl_connect_data.client_num = client_num;
	cl_connect_data.direct = 1;
	mozart_bluetooth_ble_client_connect_server(&cl_connect_data);   //如果成功会触发open事件，open成功表示已经连接成功
	for (i = 0; i < num; i++) {
		if(ble_client_open_enable != 1)
			usleep(300*1000);
		else {
			ble_client_open_enable = 0;
			break;
		}
	}
	if (i > num) {
		printf("ble_client_open_enable failed!\n");
		return -1;
	}
	if(ble_client_open_states != 0){			//dsc_1120
		printf("ble_client_open_states failed!\n");
		return -1;
	}

	/* register notification */
	cl_notireg.client_num = client_num;
	//cl_notireg.service_id = BSA_BLE_UUID_SERVCLASS_HEART_RATE;
	//cl_notireg.character_id = BSA_BLE_GATT_UUID_HEART_RATE_MEASUREMENT;
	cl_notireg.service_id = 0x1806;    //服务ID（定死的）
	cl_notireg.character_id = 0x2a20;   //特征ID（定死的）
	cl_notireg.is_primary = 1;
	cl_notireg.ser_inst_id = 0;
	cl_notireg.char_inst_id = 0;
	state = mozart_bluetooth_ble_client_register_notification(&cl_notireg);   //注册通知成功
	printf("state = %d\n", state);

	/* open ble server notificaion */
	//cl_wdata.service_uuid = BSA_BLE_UUID_SERVCLASS_HEART_RATE;
	//打开通知，通过给server发送data[0]和data[1]
	cl_wdata.service_uuid = 0x1806;
	cl_wdata.char_uuid = GATT_UUID_CHAR_CLIENT_CONFIG;
	cl_wdata.write_len = 2;
	cl_wdata.write_data[0] = 0x01;
	cl_wdata.write_data[1] = 0x00;
	cl_wdata.client_num = client_num;
	//以下参数没做改变
	cl_wdata.is_primary = 1;
	cl_wdata.ser_inst_id = 0;
	cl_wdata.char_inst_id = 0;
	cl_wdata.is_descript = 0;
	cl_wdata.descr_id = 0;
	cl_wdata.desc_value = 0;
	cl_wdata.write_type = 1;
	state = mozart_bluetooth_ble_client_write(&cl_wdata);   //有写的事件回复
	if (state != 0) {
		printf("mozart_bluetooth_ble_client_write failed !\n");
		return -1;
	}
	return 0;
}

//创建连接tower的客户端
int ble_client_create2()
{
	printf("====>>>> TOWER_CREATE__6.1<<<<=====\n");
	int i = 0;
	int state = 0;
	//int disc_index = -1;//dsc1120
	//int client_num = 0;
	ble_client_connect_data cl_connect_data;
	//ble_client_read_data client_rdata;
	ble_client_write_data cl_wdata;
	BLE_CL_NOTIFREG cl_notireg;
	//char *connect_ble_name = "BSA_BLE";
	//char *connect_ble_name = "TOWER BLE";
	//char *connect_ble_name = "my Test Ble";//dsc1120
	//UINT8 tower_mac[6] = {0xac, 0xc1, 0xee, 0x1d, 0x76, 0xa4}; 
	UINT16 client_uuid = 0x4001;
	int num = 30;
	int duration = 5; /* Multiple of 1.28 seconds */
	
	tAPP_DISCOVERY_CB ble_buffer;//dsc1120
	tAPP_DISCOVERY_CB *ble_info=&ble_buffer;//dsc1120
	int number;//dsc1120
	int time;//dsc1120

	printf("====>>>> %s,%d   0523 <<<<=====\n", __func__, __LINE__);
	/* register ble client */
	client_num2 = state = mozart_bluetooth_ble_client_register(client_uuid);   //创建tower的ble客户端
	if (state < 0) {
		printf("ERROR: mozart_bluetooth_ble_client_register failed !\n");  //创建ble客户端是吧
		return -1;
	}
	for (i = 0; i < 4; i++) {		
		/* discovery ble devices */
		state = mozart_bluetooth_ble_start_regular(duration);   //开启扫描
		if (state != 0) {
			printf("ERROR: mozart_bluetooth_ble_start_regular failed !\n");
			return -1;
		}

		while (bsa_ble_start_regular_enable != 1) {
			usleep(300*1000);
		}
		bsa_ble_start_regular_enable = 0;

		ble_info = mozart_bluetooth_disc_get_device_info();//dsc1120
		number  =  mozart_bluetooth_disc_get_device_count();//dsc1120
		for(time = 0;time<number;time++){//dsc1120
			if (ble_info->devs[time].device.name[0] == 'T'&&ble_info->devs[time].device.name[1] == 'O'&&ble_info->devs[time].device.eir_data[24]==tower_eir[0]&&ble_info->devs[time].device.eir_data[25]==tower_eir[1]&&ble_info->devs[time].device.eir_data[26]==tower_eir[2]&&ble_info->devs[time].device.eir_data[27]==tower_eir[3]&&ble_info->devs[time].device.eir_data[28]==tower_eir[4]&&ble_info->devs[time].device.eir_data[29]==tower_eir[5]){
				printf("tower find ok!\n");
				memcpy(tower_mac,ble_info->devs[time].device.bd_addr,6);   //因为每次tower的mac地址会变化，故每次扫描后保存mac
				printf("hello_tower_mac:%x,%x,%x,%x,%x,%x\n",ble_info->devs[time].device.bd_addr[0],ble_info->devs[time].device.bd_addr[1],ble_info->devs[time].device.bd_addr[2],ble_info->devs[time].device.bd_addr[3],ble_info->devs[time].device.bd_addr[4],ble_info->devs[time].device.bd_addr[5]);
				break;
			}
		}
		if(time <number){
			printf("tower find success\n");
			break;
		}
	}
	
	if (i >= 4) {
		printf("ERROR: mozart_bluetooth get tower_info failed!\n");
		return -1;
	}
	
	/* connect ble server device */
	ble_client_open_enable = 0;
	ble_client_open_states = 1;		//dsc_1120
	cl_connect_data.device_index = 1;	/*use Device found in last discovery */

	/* Priority judged ble_name device, if ble_name is NULL, then judged bd_addr device */
	cl_connect_data.ble_name = NULL;
	memcpy(cl_connect_data.bd_addr,tower_mac,6);//dsc1120
	cl_connect_data.client_num = client_num2;//dsc1120
	cl_connect_data.direct = 1;
	mozart_bluetooth_ble_client_connect_server(&cl_connect_data);
	for (i = 0; i < num; i++) {
		if(ble_client_open_enable != 1)
			usleep(300*1000);
		else {
			ble_client_open_enable = 0;
			break;
		}
	}
	if (i > num) {
		printf("ble_client_open_enable failed!\n");
		return -1;
	}
	if(ble_client_open_states != 0){		//dsc_1120
		printf("ble_client_open_states failed!\n");
		return -1;
	}

	/* register notification */
	cl_notireg.client_num = client_num2;
	//cl_notireg.service_id = BSA_BLE_UUID_SERVCLASS_HEART_RATE;
	//cl_notireg.character_id = BSA_BLE_GATT_UUID_HEART_RATE_MEASUREMENT;
	cl_notireg.service_id = 0x1807;
	cl_notireg.character_id = 0x2a22;
	cl_notireg.is_primary = 1;
	cl_notireg.ser_inst_id = 0;
	cl_notireg.char_inst_id = 0;
	state = mozart_bluetooth_ble_client_register_notification(&cl_notireg);
	printf("state = %d\n", state);

	/* open ble server notificaion */
	//cl_wdata.service_uuid = BSA_BLE_UUID_SERVCLASS_HEART_RATE;
	cl_wdata.service_uuid = 0x1807;
	cl_wdata.char_uuid = GATT_UUID_CHAR_CLIENT_CONFIG;
	cl_wdata.write_len = 2;
	cl_wdata.write_data[0] = 0x01;
	cl_wdata.write_data[1] = 0x00;
	cl_wdata.client_num = client_num2;
	cl_wdata.is_primary = 1;
	cl_wdata.ser_inst_id = 0;
	cl_wdata.char_inst_id = 0;
	cl_wdata.is_descript = 0;
	cl_wdata.descr_id = 0;
	cl_wdata.desc_value = 0;
	cl_wdata.write_type = 1;
	state = mozart_bluetooth_ble_client_write(&cl_wdata);
	if (state != 0) {
		printf("mozart_bluetooth_ble_client_write failed !\n");
		return -1;
	}

	return 0;
}

//蓝牙连接和重新连接
static void *ble_reconnect(void *args){
	int state = 0;
	
	int int_read_mac;
	int cuband_create_flag=0;
	int tower_create_flag=0;
	char char_read_mac[6];
	
	for(ii=0;ii<6;ii++){
		mozart_ini_getkey("/usr/data/system.ini", "ble", ini_name[ii], char_read_mac);
		int_read_mac = atoi(char_read_mac);
		cuband_mac[ii] = (unsigned char)int_read_mac;
		memset(char_read_mac,0,sizeof(char)*6);
		mozart_ini_getkey("/usr/data/system.ini", "ble", ini_name[6+ii], char_read_mac);
		int_read_mac = atoi(char_read_mac);
		tower_eir[ii] =(unsigned char)int_read_mac;
		memset(char_read_mac,0,sizeof(char)*6);
	}
	printf("config_tower_eir:%x,%x,%x,%x,%x,%x",tower_eir[0],tower_eir[1],tower_eir[2],tower_eir[3],tower_eir[4],tower_eir[5]);
	
	printf("CONNECT---------------TOWER_mac:%x.%x.%x.%x.%x.%x\n",tower_mac[0],tower_mac[1],tower_mac[2],tower_mac[3],tower_mac[4],tower_mac[5]);
	printf("CONNECT---------------CUBand_mac:%x.%x.%x.%x.%x.%x\n",cuband_mac[0],cuband_mac[1],cuband_mac[2],cuband_mac[3],cuband_mac[4],cuband_mac[5]);
	/* create ble client */
	while(tower_eir[0]==0x00&&tower_eir[1]==0x00&&tower_eir[2]==0x00&&cuband_mac[0]==0x00&&cuband_mac[1]==0x00&&cuband_mac[2]==0x00)
	{
		
	//	printf(">>>>>>>while_config-data:%x.%x.%x.%x.%x.%x\n",tower_eir[0],tower_eir[1],tower_eir[2],cuband_mac[0],cuband_mac[1],cuband_mac[2]);	
		sleep(3);
	}

	while(1)
	{
		return_cuband_flag = cuband_flag;
		return_tower_flag = tower_flag;
		cuband_flag = 0;
		tower_flag = 0;
		sleep(3);

	//	printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~while(1)~~~~~~~~~\n");

		pthread_mutex_lock(&bt_mutex);
		if(cuband_create_flag==0){
			if(!(cuband_mac[0]==0x00&&cuband_mac[1]==0x00&&cuband_mac[2]==0x00))
		       {
		      		cuband_create_flag =1;
			       state = ble_client_create();
				if (state != 0) {
					printf("ERROR: ble_client_create2 failed !\n");
				}
					
			}
		}
		pthread_mutex_unlock(&bt_mutex);
		
		sleep(1);
		
		pthread_mutex_lock(&bt_mutex);
		if(tower_create_flag==0){
			if(!(tower_eir[0]==0x00&&tower_eir[1]==0x00&&tower_eir[2]==0x00))
			{
				tower_create_flag=1;
				state = ble_client_create2();
				if (state != 0) 
				{
					printf("ERROR: ble_client_create failed !\n");
				}
				else
				{
					open_chancal = true;
					printf(">>>>>>start up_tower_wifi_message  strat 第一个 sec >>>>>>>>>>>\n");
				}
			}
		}
		pthread_mutex_unlock(&bt_mutex);

		pthread_mutex_lock(&bt_mutex);
		if (cuband_flag ==0&&cuband_create_flag == 1){//7.8
				return_cuband_flag = 0;
				if (cuband_status == 1){   //此时蓝牙连接存在
					printf("disconnect cuband\n");
					mozart_bluetooth_ble_client_disconnect_server(client_num);//断开蓝牙连接
				 	//sleep(1);
				}
				if(cuband_status == 0){      //此时蓝牙连接已断开      			  //修改于5.13
					if(!(cuband_mac[0]==0x00&&cuband_mac[1]==0x00&&cuband_mac[2]==0x00)){
						if(ble_client_reconnect()==0){   //重新连接
							open_chancal = true;
							//up_tower_info();
							printf("SUCCESS: ble_client_create succeed !\n");	
						}
						else{
							printf("ERROR: ble_client_create failed !\n");	
							//mozart_bluetooth_ble_client_disconnect_server(client_num);//\B6瀹勵摙AAcuband\C1\AC\BD\D3
						}
					}
				}
		}
		pthread_mutex_unlock(&bt_mutex);	
		
		//sleep(2);

		pthread_mutex_lock(&bt_mutex);
		if (tower_flag ==0&&tower_create_flag == 1){
				return_tower_flag = 0;
				printf("enter reconnect\n");
				if (tower_status == 1){
					printf("disconnect tower\n");
					mozart_bluetooth_ble_client_disconnect_server(client_num2);		//断开蓝牙连接
					//sleep(1);
				}
				if (tower_status == 0){   //修改于5.13
					if(!(tower_eir[0]==0x00&&tower_eir[1]==0x00&&tower_eir[2]==0x00)){
						if(ble_client_reconnect2()==0){
							open_chancal = true;
							//修改于（4.28号）
							printf("SUCCESS: ble_client_create succeed !\n");	
						}
						else{
							printf("ERROR: ble_client_create failed !\n");	
							//mozart_bluetooth_ble_client_disconnect_server(client_num2);//\B6瀹勵摙AAtower\C1\AC\BD\D3
						}
					}
				}
		}
		pthread_mutex_unlock(&bt_mutex);
	}
		return NULL;
}

//按键配置：搜索周围是否有tower和base，有记录mac地址到文件系统
int ble_config(){
	//int ble_config_fd = -1;
	int num = 0;
	int time;
	int try_time;
	player_status_t status;
	player_status_t status2;
	//int CUBand_num = 0;
	//int TOWER_num = 0;
	int CUB_state = 0;
	int TOWER_state = 0;
	unsigned char CUB_mac[6];
	unsigned char TOWER_eir[6];
	//int state = 0;
	//int duration = 3; /* Multiple of 1.28 seconds */
	tAPP_DISCOVERY_CB ble_buffer;
	tAPP_DISCOVERY_CB *ble_info=&ble_buffer;	
	//unsigned char config_buf[25];
	char mac[6];
	
	int state = 0;
	int duration = 3; /* Multiple of 1.28 seconds */    //？？？？
	
	
    status =  mozart_musicplayer_get_status(mozart_musicplayer_handler);
	if (status == PLAYER_PLAYING) 
	{     
		mozart_musicplayer_play_pause_pause(mozart_musicplayer_handler);
    }
    power_led_set(3);
    mozart_play_key("bleconfig");	//进入蓝牙配置模式
	
	pthread_mutex_lock(&bt_mutex);
	printf("\n===============enter==BLE==config====================\n");
	if (cuband_status == 1)     //判断发带是否已连接，cuband_status == 1表示已连接
		mozart_bluetooth_ble_client_disconnect_server(client_num);		//断开发带的蓝牙连接
	if (tower_status == 1)       //判断tower是否已连接，tower_status  == 1 表示已连接
		mozart_bluetooth_ble_client_disconnect_server(client_num2);		//断开已连接的tower
		
	for (try_time = 0; try_time < 4; try_time++) {		//尝试四次搜索设备
		/* discovery ble devices */
		state = mozart_bluetooth_ble_start_regular(duration);	//搜素蓝牙，duration表示每次搜索的时长
		if (state != 0) {
			printf("ERROR: mozart_bluetooth_ble_start_regular failed !\n");//yizhizaizhechucuo
			pthread_mutex_unlock(&bt_mutex);
			return -1;
		}

		while (bsa_ble_start_regular_enable != 1) {    //？？？？？？
			usleep(300*1000);
		}
		bsa_ble_start_regular_enable = 0;

			ble_info = mozart_bluetooth_disc_get_device_info();
			num =  mozart_bluetooth_disc_get_device_count();
			printf("@@@@@@@@@@@@num:%d@@@@@@@@@\n",num);
			for(time = 0;time<num;time++){
				if (ble_info->devs[time].device.name[0] == 'E' && ble_info->devs[time].device.name[1] == 'G' && ble_info->devs[time].device.name[2] == 'O' &&CUB_state ==0){
					memcpy(CUB_mac,ble_info->devs[time].device.bd_addr,6);   //把头戴的mac地址保存
					CUB_state = 1;
					//printf("&&&&&&&&&&&&&&CUBand_mac:%x.%x.%x.%x.%x.%x\n",ble_info->devs[time].device.bd_addr[0],ble_info->devs[time].device.bd_addr[1],ble_info->devs[time].device.bd_addr[2],ble_info->devs[time].device.bd_addr[3],ble_info->devs[time].device.bd_addr[4],ble_info->devs[time].device.bd_addr[5]);
					printf("&&&&&&&&&&&&&&CUBand_mac:%x.%x.%x.%x.%x.%x\n",CUB_mac[0],CUB_mac[1],CUB_mac[2],CUB_mac[3],CUB_mac[4],CUB_mac[5]);
				}
				else if (ble_info->devs[time].device.name[0] == 'T'&&ble_info->devs[time].device.name[1] == 'O'  && ble_info->devs[time].device.name[2] == 'W'  && TOWER_state ==0){
					//printf("eir_data:%x.%x.%x.%x.%x.%x\n",ble_info->devs[time].device.eir_data[0],ble_info->devs[time].device.eir_data[1],ble_info->devs[time].device.eir_data[2],ble_info->devs[time].device.eir_data[3],ble_info->devs[time].device.eir_data[4],ble_info->devs[time].device.eir_data[5]);
					memcpy(TOWER_eir , &ble_info->devs[time].device.eir_data[24] , 6);   //保存的是广播的物理的mac地址，此地址是固定的
					//memcpy(TOWER_mac,ble_info->devs[time].device.bd_addr,6);
					TOWER_state = 1;
					//printf("&&&&&&&&&&&&&&TOWER_mac:%x.%x.%x.%x.%x.%x\n",ble_info->devs[time].device.bd_addr[0],ble_info->devs[time].device.bd_addr[1],ble_info->devs[time].device.bd_addr[2],ble_info->devs[time].device.bd_addr[3],ble_info->devs[time].device.bd_addr[4],ble_info->devs[time].device.bd_addr[5]);
					printf("&&&&&&&&&&&&&&TOWER_eir:%x.%x.%x.%x.%x.%x\n",TOWER_eir[0],TOWER_eir[1],TOWER_eir[2],TOWER_eir[3],TOWER_eir[4],TOWER_eir[5]);
				}	
			}
			if (CUB_state == 1 && TOWER_state == 1){
				printf("==============mac save success================\n");
				break;
			}
			printf("<<<<<<<<<<<<<<<<<<<---try:%d--->>>>>>>>>>>>>>>>>>>>>\n",try_time);
			sleep(2);
	}

	status2 =  mozart_musicplayer_get_status(mozart_musicplayer_handler);
    if (status2 == PLAYER_PLAYING) 
	{     
		mozart_musicplayer_play_pause_pause(mozart_musicplayer_handler);
    }
	  
	if(CUB_state == 1 ){
		    mozart_play_key("xsuccess");	//发带配置成功
			memcpy(cuband_mac,CUB_mac,6);
			printf("&&&&&&&&&&&&&&config-data:%x.%x.%x.%x.%x.%x\n",tower_eir[0],tower_eir[1],tower_eir[2],cuband_mac[0],cuband_mac[1],cuband_mac[2]);	
			for(ii=0;ii<6;ii++){
				sprintf(mac,"%d",(int)CUB_mac[ii]);
				//itoa((int)CUB_mac[ii], mac, 10);
				mozart_ini_setkey("/usr/data/system.ini", "ble", ini_name[ii], mac);
				mozart_ini_setkey("/usr/data/system_beifen.ini", "ble", ini_name[ii], mac);
			}
			
	}
	else
		mozart_play_key("xfail");  //头带配置失败
	sleep(2);
	if(TOWER_state == 1){
		       mozart_play_key("towersuccess");  //tower配置成功
			memcpy(tower_eir,TOWER_eir,6);
			printf("&&&&&&&&&&&&&&config-data:%x.%x.%x.%x.%x.%x\n",tower_eir[0],tower_eir[1],tower_eir[2],cuband_mac[0],cuband_mac[1],cuband_mac[2]);	
			for(ii=0;ii<6;ii++)
			{
				sprintf(mac,"%d",(int)TOWER_eir[ii]);
				//itoa((int)TOWER_mac[ii], mac, 10);
				mozart_ini_setkey("/usr/data/system.ini", "ble", ini_name[6+ii],mac);
				mozart_ini_setkey("/usr/data/system_beifen.ini", "ble", ini_name[6+ii],mac);  //为什么备份？？
			}
	}
	else
		 mozart_play_key("towerfail");  		//tower配置失败

	pthread_mutex_unlock(&bt_mutex);

    power_led_set(5);
	if (status == PLAYER_PLAYING) 
	{    
		mozart_musicplayer_play_pause_play(mozart_musicplayer_handler);	
	}
	mozart_start_mode(FREE);
	return 0;
}


//两个蓝牙的ble数据事件回调函数(event蓝牙事件，p_data蓝牙传来的数据？？)
static int bsa_ble_client_profile_cback(tBSA_BLE_EVT event, tBSA_BLE_MSG *p_data)
{
	int i = 0;

	switch (event) {
		case BSA_BLE_CL_DEREGISTER_EVT:
			printf("## BSA_BLE_CL_DEREGISTER_EVT!\n");
			printf("cif = %d status = %d\n", p_data->cli_deregister.client_if,
					p_data->cli_deregister.status );
			break;
		case BSA_BLE_CL_OPEN_EVT:
			printf("## BSA_BLE_CL_OPEN_EVT!\n");
			//printf("client_if = %d status = %d open_conn_id=%d\n",
					//p_data->cli_open.client_if, p_data->cli_open.status ,p_data->cli_open.conn_id );
			if(p_data->cli_open.status == 0){
				ble_client_open_states=0;
				
				ble_client_open_enable = 1;
				if (cuband_mac[5]==p_data->cli_open.bd_addr[5]&&cuband_mac[4]==p_data->cli_open.bd_addr[4]&&cuband_mac[3]==p_data->cli_open.bd_addr[3]&&cuband_mac[2]==p_data->cli_open.bd_addr[2]&&cuband_mac[1]==p_data->cli_open.bd_addr[1]&&cuband_mac[0]==p_data->cli_open.bd_addr[0]){
					cuband_status = 1;	
					printf("^^open event:cuband \n");
					}			
				else{
					tower_status = 1;
					printf("^^open event:tower \n");
					}
				printf("ble_client_open_states success\n");
			}
			break;
		case BSA_BLE_CL_CLOSE_EVT:     //断开触发该事件
			printf("## BSA_BLE_CL_CLOSE_EVT!\n");
			printf("BDA: %02X:%02X:%02X:%02X:%02X:%02X\n",
					p_data->cli_close.remote_bda[0], p_data->cli_close.remote_bda[1],
					p_data->cli_close.remote_bda[2], p_data->cli_close.remote_bda[3],
					p_data->cli_close.remote_bda[4], p_data->cli_close.remote_bda[5]);
			printf("close_conn_id=%d \n",
					p_data->cli_close.conn_id );
			printf("close_reason=%d \n",
					p_data->cli_close.reason );
			if(p_data->cli_close.status == 0){
				if (cuband_mac[5]==p_data->cli_close.remote_bda[5]&&cuband_mac[4]==p_data->cli_close.remote_bda[4]&&cuband_mac[3]==p_data->cli_close.remote_bda[3]&&cuband_mac[2]==p_data->cli_close.remote_bda[2]&&cuband_mac[1]==p_data->cli_close.remote_bda[1]&&cuband_mac[0]==p_data->cli_close.remote_bda[0]){
					cuband_status = 0;
					printf("^^close event:cuband \n");
					}
				else{
					tower_status = 0;
					printf("^^close event:tower \n");
					}
			}
			break;
		case BSA_BLE_CL_READ_EVT:
			printf("## BSA_BLE_CL_READ_EVT!\n");
			printf("conn_id:%d len:%d\n",
					p_data->cli_read.conn_id, p_data->cli_read.len);
			printf("Read:\n");
			for (i = 0; i < p_data->cli_read.len; i++) {
				printf("value[%d] = 0x%x\n", i, p_data->cli_read.value[i]);
			}
			ble_client_read_enable = 1;
			break;
		case BSA_BLE_CL_WRITE_EVT:   //写成功事件
			printf("################################################# BSA_BLE_CL_WRITE_EVT!\n");    //如果执行，则说明给服务器发送数据成功
			printf("status: %d，ID：%d\n", p_data->cli_write.status,p_data->cli_write.conn_id);
			printf(" %d，%d, %d\n", p_data->cli_write.srvc_id.id.uuid.uu.uuid16,p_data->cli_write.char_id.uuid.uu.uuid16,p_data->cli_write.descr_type.uuid.uu.uuid16);

			ble_client_write_enable = 1;
			break;
		case BSA_BLE_CL_NOTIF_EVT:   //通知事件
			//printf("## BSA_BLE_CL_NOTIF_EVT!\n");
			if(cuband_mac[5]==p_data->cli_notif.bda[5]&&cuband_mac[4]==p_data->cli_notif.bda[4]&&cuband_mac[3]==p_data->cli_notif.bda[3]&&cuband_mac[2]==p_data->cli_notif.bda[2]&&cuband_mac[1]==p_data->cli_notif.bda[1]&&cuband_mac[0]==p_data->cli_notif.bda[0]){
				if(cuband_flag == 0){
					cuband_flag = 1;
				}
			}
			else{
				tower_flag = 1;
			}
			ble_response_cmd(*((struct BLE_INFO *)&(p_data->cli_notif)));	//将蓝牙传来的数据存入到链表			
			break;
		default:
			printf("BLE Client not handle this event:%d\n", event);
			break;
	}
	return 0;
}

#endif /* SUPPORT_BSA_BLE_CLIENT */

#if (SUPPORT_BSA_BLE_HH == 1)
static int mozart_bluetooth_hh_uipc_cback(UINT8 *data, UINT16 data_len)
{
#if (SUPPORT_BSA_BLE_HH_DIALOG_AUDIO == 1)
	/* compare instance, data[0] is report id */
	int instance = data[0] -1;

	data++;
	if ((instance >= HID_STREAM_DATA_MIN_INSTANCE) && (instance <= HID_STREAM_DATA_MAX_INSTANCE)) {
		dialog_sendAudioData(data, data_len - 1);
		if (dia_prevInstance == 0)
			dia_prevInstance = instance;
		dia_prevInstance++;
		if (dia_prevInstance > HID_STREAM_DATA_MAX_INSTANCE)
			dia_prevInstance = HID_STREAM_DATA_MIN_INSTANCE;
		if (dia_prevInstance != instance)
		{
			printf("Warning, packet sequence interruption, expected %d, received %d\n", dia_prevInstance, instance);
			dia_prevInstance = instance;
		}
	} else if (instance == HID_STREAM_ENABLE_READ_INSTANCE) {
		printf("-------->>> HID_STREAM_ENABLE_READ_INSTANCE!\n");
		dialog_updatekeys(data);
		if ((data[0] == 0) && (data[1] == 4)) {
			int mode = (int)data[2];
			dialog_setDecodeMode(mode);
		}
	}
#endif
	return 0;
}

static int ble_hh_create()
{
	int i = 0;
	int disc_index = -1;
	ble_hh_connect_data hh_connect_data;
	ble_client_connect_data cl_connect_data;
	char *connect_ble_name = "RemoteB008";
	int num = 120;
	int state = 0;
	int duration = 3; /* Multiple of 1.28 seconds */

	UINT16 client_uuid = 0x4000;
	/* register ble client */
	hh_client_num = state = mozart_bluetooth_ble_client_register(client_uuid);
	if (state < 0) {
		printf("ERROR: mozart_bluetooth_ble_client_register failed !\n");
		return -1;
	}
	mozart_bluetooth_hh_start();

	for (i = 0; i < 5; i++) {
		/* discovery ble devices */
		state = mozart_bluetooth_ble_start_regular(duration);
		if (state != 0) {
			printf("ERROR: mozart_bluetooth_ble_start_regular failed !\n");
			return -1;
		}

		while (bsa_ble_start_regular_enable != 1) {
			usleep(300*1000);
		}
		bsa_ble_start_regular_enable = 0;

		/* find connect_ble_name from discovered ble devices */
		disc_index = mozart_bluetooth_disc_find_device_name(connect_ble_name);
		if (disc_index != -1) {
			printf("mozart_bluetooth_disc_find_device_name %s success!\n", connect_ble_name);
			break;
		} else {
			printf("not find ble device: %s, rediscovery ble device!\n", connect_ble_name);
		}
	}
	if ((disc_index == -1) && (i >= 5)) {
		printf("ERROR: mozart_bluetooth_disc_find_device_name failed!\n");
		return -1;
	}

	/* connect ble server device */
	cl_connect_data.device_index = DEVICE_FROM_DISCOVERY;	/* use Device found in last discovery */
	/* Priority judged ble_name device, if ble_name is NULL, then judged bd_addr device */
	cl_connect_data.ble_name = connect_ble_name;
	cl_connect_data.client_num = hh_client_num;
	cl_connect_data.direct = 1;
	printf("mozart_bluetooth_ble_client_connect_server !!\n");
	state = mozart_bluetooth_ble_client_connect_server(&cl_connect_data);
	if (state != 0) {
		printf("mozart_bluetooth_ble_client_connect_server failed !\n");
		return -1;
	} else {
		printf("mozart_bluetooth_ble_client_connect_server succeed !\n");
	}
	for (i = 0; i < num; i++) {
		if(ble_client_open_enable != 1)
			usleep(300*1000);
		else {
			ble_client_open_enable = 0;
			break;
		}
	}
	if (i >= num) {
		printf("ble_client_open_enable failed!\n");
		return -1;
	}

	/* hh connect */
	hh_connect_data.device_index = DEVICE_FROM_DISCOVERY;	/* use Device found in last discovery */
	hh_connect_data.disc_index = disc_index;
	hh_connect_data.sec_type = SEC_NONE;
	hh_connect_data.mode = BSA_HH_PROTO_RPT_MODE;
	state = mozart_bluetooth_hh_connect(&hh_connect_data);
	if (state != 0) {
		printf("mozart_bluetooth_hh_connect failed !\n");
		return -1;
	}

	for (i = 0; i < num; i++) {
		if(ble_hh_add_dev_enable != 1)
			usleep(500*1000);
		else {
			ble_hh_add_dev_enable = 0;
			break;
		}
	}
	if (i >= num) {
		printf("ble_hh_add_dev_enable failed!\n");
		return -1;
	}

	return 0;
}
#endif /* SUPPORT_BSA_BLE_HH */
#endif /* SUPPORT_BSA_BLE */

static void bt_info_init(bt_init_info *bt_info, char *bt_name)
{
	bt_info->bt_name = bt_name;
	bt_info->bt_ble_name = "BSA_BLE";
	bt_info->discoverable = 0;
	bt_info->connectable = 0;
	memset(bt_info->out_bd_addr, 0, sizeof(bt_info->out_bd_addr));
}

#if (SUPPORT_BSA_A2DP_SOURCE == 1)
extern int bt_av_link_state;
#define DEFAULT_SAMPLE_RATE	48000
#define DEFAULT_CHANNELS	2
#define DEFAULT_FORMAT		16
#define DEFAULT_SOCKET          "/var/run/bt-av-stream.socket"
#define BURSTSIZE		4096

static char *local_buf = NULL;

typedef struct {
	int     channels;       /* number of audio channels */
	int     bytes;          /* bytes per sample */
	int     format;         /* oss format */
	int     rate;           /* samples per second (in a single channel) */
} ao_format_t;

static int lostream_init_callback(struct domain_init_hdr *hdr, void *data)
{
	tBSA_AV_MEDIA_FEED_CFG_PCM pcm_config;

	printf("Audio ori samplerate: %d\n", hdr->info.samplerate);
	printf("Audio ori channels: %d\n", hdr->info.channels);
	printf("Audio ori format: %d\n", hdr->info.format);

	/* Final sample rate */
	hdr->info.samplerate     = DEFAULT_SAMPLE_RATE;
	hdr->info.channels 	 = DEFAULT_CHANNELS;
	hdr->info.format 	 = AF_FORMAT_S16_LE;
	printf("Audio cfg samplerate: %d\n", hdr->info.samplerate);
	printf("Audio cfg channels: %d\n", hdr->info.channels);
	printf("Audio cfg format: %d\n", hdr->info.format);

	local_buf = malloc(BURSTSIZE);
	if (local_buf == NULL) {
		perror("malloc failed\n");
		return -1;
	}
	pcm_config.sampling_freq = DEFAULT_SAMPLE_RATE;
	pcm_config.num_channel = DEFAULT_CHANNELS;
	pcm_config.bit_per_sample = DEFAULT_FORMAT;
	mozart_bluetooth_av_start_stream(&pcm_config);

	return 0;
}

static int lostream_get_buffer(struct loStreamBuffer *s_buf, void *data)
{
	if (local_buf != NULL) {
		memset(local_buf, 0, sizeof(local_buf));
		s_buf->buf = local_buf;
		s_buf->size = BURSTSIZE;
	} else {
		printf("ERROR: local_buf == NULL, please checked!!\n");
		return -1;
	}
	return 0;
}

static void lostream_getspace_callback(struct domain_getspace_hdr *hdr, void *data)
{
	hdr->space = BURSTSIZE;
	return;
}

static int lostream_stream_callback(struct domain_stream_reply *hdr, int size, void *data)
{
	mozart_bluetooth_av_send_stream(local_buf, size);

	return 0;
}

static int lostream_stop_callback(void *header, void *data)
{
	if (local_buf != NULL)
		free(local_buf);
	mozart_bluetooth_av_stop_stream();

	return 0;
}

static struct loStream_callback localstream_cback = {
	.initStream     = lostream_init_callback,
	.getBuffer      = lostream_get_buffer,
	.getSpace       = lostream_getspace_callback,
	.handleRecv     = lostream_stream_callback,
	.stopStream     = lostream_stop_callback,
};

int avk_source_set_audio_output(char *ao_iface)
{
	player_handler_t *av_handle = NULL;

	av_handle = mozart_player_handler_get("bsa", NULL, NULL);
	if (!av_handle) {
		printf("Error: mozart_player_handler_get failed!\n");
		return -1;
	}
	mozart_player_aoswitch(av_handle, ao_iface);
	mozart_player_handler_put(av_handle);

	return 0;
}

struct localInStream  *loStream;
int mozart_avk_source_stream_init()
{
	printf("avk source init create socket: %s\n", DEFAULT_SOCKET);
	loStream = mozart_local_stream_create(DEFAULT_SOCKET, BURSTSIZE);
	mozart_local_stream_init(loStream, &localstream_cback, NULL);

	return 0;
}
#endif /* SUPPORT_BSA_A2DP_SOURCE */

#if (SUPPORT_BSA_HFP_HF == 1)
static int mozart_hs_aec_init(hs_sample_param *aec, int sample_time)
{
	/* webrtc support:
	 * rate: 8KHZ and 16KHZ
	 * channel: 1
	 * bits: 16 bit
	 * sample_time: 10 ms
	 * */
	aec->actual.rate	= aec->hope.rate;
	aec->actual.channel	= aec->hope.channel;
	aec->actual.bits	= aec->hope.bits;
	if (ingenic_apm_init(&aec->actual.rate, &aec->actual.channel, &aec->actual.bits, sample_time) < 0) {
		printf("Error: ingenic_apm_init failed!!\n");
		ingenic_apm_destroy();
	}
	return 0;
}
static int mozart_hs_aec_get_buf_length()
{
	return webrtc_aec_get_buffer_length();
}
static int mozart_hs_aec_calculate(hs_aec_param *hs_aec, char *ref_buf, char *record_buf, int sample_time)
{
	/* AEC calculate */
	if (!hs_aec->outbuf) {
		hs_aec->outbuf = malloc(hs_aec->outlen);
		if (!hs_aec->outbuf) {
			perror("aec_calculate malloc failed");
			return -1;
		}
	}
	webrtc_aec_calculate(record_buf, ref_buf, hs_aec->outbuf, sample_time);
	if (!hs_aec->outbuf) {
		printf("Error: hs_aec.outbuf is NULL, please checked!!\n");
		return -1;
	}

	return 0;
}
static int mozart_hs_aec_uninit()
{
	ingenic_apm_destroy();
	return 0;
}
#else /* SUPPORT_WEBRTC */
static inline int mozart_hs_aec_init(hs_sample_param *aec, int sample_time) { return 0; }
static inline int mozart_hs_aec_uninit() { return 0; }
#endif /* SUPPORT_WEBRTC */

#if (SUPPORT_BSA_HFP_HF == 1)
static int mozart_hs_init_sample(hs_stream_msg *hs_msg)
{
	hs_msg->sample_time		= 10; /* ms */
	/* config dsp */
	hs_msg->dsp.hope.rate		= hs_msg->hs.hope.rate;
	hs_msg->dsp.hope.channel	= hs_msg->hs.hope.channel;
	hs_msg->dsp.hope.bits 		= hs_msg->hs.hope.bits;

	/* config ref */
	//hs_msg->ref.hope.rate		= hs_msg->hs.hope.rate;
	hs_msg->ref.hope.rate		= 16000;
	hs_msg->ref.hope.channel	= hs_msg->hs.hope.channel;
	hs_msg->ref.hope.bits		= hs_msg->hs.hope.bits;
	hs_msg->ref.volume		= 50;

	/* config record */
	//hs_msg->record.hope.rate	= hs_msg->hs.hope.rate;
	hs_msg->record.hope.rate	= 16000;
	hs_msg->record.hope.channel	= hs_msg->hs.hope.channel;
	hs_msg->record.hope.bits	= hs_msg->hs.hope.bits;
	hs_msg->record.volume		= 50;

	/* config aec */
	hs_msg->aec.hope.rate		= hs_msg->hs.hope.rate;
	hs_msg->aec.hope.channel	= hs_msg->hs.hope.channel;
	hs_msg->aec.hope.bits		= hs_msg->hs.hope.bits;

	return 0;
}

static int mozart_hs_uninit_resample_param(hs_resample_param *param)
{
	if (param->res.out_buf) {
		free(param->res.out_buf);
		param->res.out_buf = NULL;
	}
	if (param->rec.out_buf) {
		free(param->rec.out_buf);
		param->rec.out_buf = NULL;
	}
	if (param->res.resample_t) {
		mozart_resample_uninit(param->res.resample_t);
		param->res.resample_t = NULL;
	}
	if (param->rec.channel_t) {
		mozart_channels_uninit(param->rec.channel_t);
		param->rec.channel_t = NULL;
	}
	memset(param, 0, sizeof(hs_resample_param));

	return 0;
}

static int mozart_hs_uninit_resample()
{
#if (SUPPORT_WEBRTC == 1)
	mozart_hs_uninit_resample_param(&hs_ref);
	mozart_hs_uninit_resample_param(&hs_aec.aec);
	if(hs_aec.outbuf) {
		free(hs_aec.outbuf);
		hs_aec.outbuf = NULL;
	}
	memset(&hs_aec, 0, sizeof(hs_aec_param));
#endif
	mozart_hs_uninit_resample_param(&hs_bt);
	mozart_hs_uninit_resample_param(&hs_record);

	return 0;
}

static int mozart_hs_resample(hs_sample_param *source, hs_sample_param *dst, hs_resample_info *res)
{
	if (res->resample_t == NULL) {
		res->resample_t = mozart_resample_init(source->actual.rate, source->actual.channel, source->actual.bits >> 3, dst->actual.rate);
		if (res->resample_t == NULL) {
			printf("Error: %s resample init failed\n", __func__);
			return -1;
		}
	}
	res->out_len = mozart_resample_get_outlen(res->in_len, source->actual.rate, source->actual.channel, source->actual.bits, dst->actual.rate);
	if (res->out_buf == NULL) {
		printf("hs malloc outbuf!\n");
		res->out_buf   = malloc(res->out_len);
		res->max_outlen = res->out_len;
	} else {
		if (res->out_len > res->max_outlen) {
			printf("hs realloc outbuf!\n");
			res->out_buf     = realloc(res->out_buf, res->out_len);
			res->max_outlen = res->out_len;
		}
	}
	return mozart_resample(res->resample_t, source->actual.channel, source->actual.bits, res->in_buf, res->in_len, res->out_buf);
}

static int mozart_hs_rechannel(hs_sample_param *source, hs_sample_param *dst, hs_rechannel_info *rec)
{
	if (rec->channel_t == NULL) {
		rec->channel_t = mozart_channels_init(source->actual.channel, dst->actual.channel);
		if (rec->channel_t == NULL) {
			printf("Error: func: %s line: %d mozart channel int failed\n", __func__, __LINE__);
			return -1;
		}
	}
	rec->out_len = mozart_channels_get_outlen(rec->channel_t, rec->in_len);
	if (rec->out_buf == NULL) {
		printf("hs channel_outbuf == NULL !\n");
		rec->out_buf    = malloc(rec->out_len);
		rec->max_outlen = rec->out_len;
	} else {
		if (rec->out_len > rec->max_outlen) {
			printf("hs realloc channel_outlen!\n");
			rec->out_buf    = realloc(rec->out_buf, rec->out_len);
			rec->max_outlen = rec->out_len;
		}
	}
	mozart_channels(rec->channel_t, source->actual.bits>>3, 1, 0, rec->in_buf, rec->in_len, rec->out_buf);

	return 0;
}

static int mozart_hs_event_cback(tBSA_HS_EVT event, void *p_data)
{
	switch (event) {
		case BSA_HS_AUDIO_OPEN_EVT:
			printf("BSA_HS_AUDIO_OPEN_EVT !\n");
			device_prope *DevPrope = (device_prope *)p_data;

			/* hs ori sample param
			 * rate:	8\16 KHZ
			 * channel:	1
			 * bits:	16 bit
			 * */
			printf("BT sample rate: %d, channel: %d, bits: %d\n",
					DevPrope->hs_msg.hs.hope.rate, DevPrope->hs_msg.hs.hope.channel, DevPrope->hs_msg.hs.hope.bits);

			mozart_hs_init_sample(&DevPrope->hs_msg);
			mozart_hs_aec_init(&DevPrope->hs_msg.aec, DevPrope->hs_msg.sample_time);
			break;

		case BSA_HS_AUDIO_CLOSE_EVT:
			printf("BSA_HS_AUDIO_CLOSE_EVT !\n");
			mozart_hs_aec_uninit();
			mozart_hs_uninit_resample();
			break;

		default:
			printf("%s Not handle this event!\n", __func__);
			break;
	}

	return 0;
}

static char *mozart_hs_data_sample(hs_sample_param *sourc, hs_sample_param *dst,
		hs_resample_param *hs_param, int *outlen)
{
	char *resample_outbuf = NULL;
	int resample_outlen = 0;
	char *rechannel_outbuf = NULL;
	int rechannel_outlen = 0;
	int inlen = hs_param->res.in_len;
	char *inbuf = hs_param->res.in_buf;

	/* No resample */
	if ((sourc->actual.rate == dst->actual.rate)
			&& (sourc->actual.channel == dst->actual.channel)
			&& (sourc->actual.bits == dst->actual.bits)) {
		rechannel_outbuf = inbuf;
		rechannel_outlen = inlen;
	/* Resample */
	} else {
		/* Resample rate */
		if (sourc->actual.rate != dst->actual.rate) {
			resample_outlen = mozart_hs_resample(sourc, dst, &hs_param->res);
			resample_outbuf = hs_param->res.out_buf;
		} else {
			resample_outlen = inlen;
			resample_outbuf = inbuf;
		}

		/* Resample channel */
		if (sourc->actual.channel != dst->actual.channel) {
			hs_param->rec.in_len = resample_outlen;
			hs_param->rec.in_buf = resample_outbuf;
			mozart_hs_rechannel(sourc, dst, &hs_param->rec);
			rechannel_outbuf = hs_param->rec.out_buf;
			rechannel_outlen = hs_param->rec.out_len;
		} else {
			rechannel_outbuf = resample_outbuf;
			rechannel_outlen = resample_outlen;
		}

		/* Resample bits */
		if (sourc->actual.bits != dst->actual.bits) {
			printf("Error: %s not handle format, recv_bits: %d, cfg_bits: %d !!\n",
					__func__, sourc->actual.bits, dst->actual.bits);
			return NULL;
		}
	}
	if (rechannel_outlen <= 0) {
		printf("Error: rechannel_outlen <= 0, please checked!!\n");
		return NULL;
	}
	if (!rechannel_outbuf) {
		printf("Error: rechannel_outbuf is NULL, please checked!!\n");
		return NULL;
	}

	*outlen = rechannel_outlen;
	return rechannel_outbuf;
}

/* Data: BT ===>> DSP */
static int mozart_hs_data_in_cback(hs_stream_msg *hs_msg)
{
	char *channel_outbuf = NULL;
	int  channel_outlen = 0;

	hs_bt.res.in_len = hs_msg->in.ilen;
	hs_bt.res.in_buf = hs_msg->in.ibuf;
	channel_outbuf = mozart_hs_data_sample(&hs_msg->hs, &hs_msg->dsp, &hs_bt, &channel_outlen);
	if (!channel_outbuf) {
		printf("Error: channel_outbuf is NULL, please checked!!!\n");
		return -1;
	}
	hs_msg->in.olen = channel_outlen;
	hs_msg->in.obuf = channel_outbuf;

	return 0;
}

/* Data: Record + Ref ===>> AEC ===>> BT */
static int mozart_hs_data_out_cback(hs_stream_msg *hs_msg)
{
	char *record_channel_outbuf = NULL;
	int  record_channel_outlen = 0;

#if (SUPPORT_WEBRTC == 1)
	int ret = 0;
	char *ref_channel_outbuf = NULL;
	int  ref_channel_outlen = 0;
	char *aec_channel_outbuf = NULL;
	int  aec_channel_outlen = 0;

	/* Ref resample */
	hs_ref.res.in_len   = hs_msg->out.ref_len;
	hs_ref.res.in_buf   = hs_msg->out.ref_buf;
	ref_channel_outbuf = mozart_hs_data_sample(&hs_msg->ref, &hs_msg->aec, &hs_ref, &ref_channel_outlen);
	if (!ref_channel_outbuf) {
		printf("Error: ref_channel_outbuf is NULL, please checked!!!\n");
		return -1;
	}

	/* Record resample */
	hs_record.res.in_len   = hs_msg->out.record_len;
	hs_record.res.in_buf   = hs_msg->out.record_buf;
	record_channel_outbuf = mozart_hs_data_sample(&hs_msg->record, &hs_msg->aec, &hs_record, &record_channel_outlen);
	if (!record_channel_outbuf) {
		printf("Error: record_channel_outbuf is NULL, please checked!!!\n");
		return -1;
	}

	/* AEC calculate */
	if (!hs_aec.outlen) {
		hs_aec.outlen = mozart_hs_aec_get_buf_length();
		if(hs_aec.outlen <= 0) {
			printf("Error: hs_aec.outlen <= 0, please checked!\n");
			return -1;
		}
	}
	if ((record_channel_outlen != hs_aec.outlen) || (ref_channel_outlen != hs_aec.outlen)) {
		printf("Error: aec_outlen: %d, record_channel_outlen: %d, ref_channel_outlen: %d, please checked\n",
				hs_aec.outlen, record_channel_outlen, ref_channel_outlen);
		return -1;
	}
	ret = mozart_hs_aec_calculate(&hs_aec, ref_channel_outbuf, record_channel_outbuf, hs_msg->sample_time);
	if (ret < 0) {
		printf("Error: mozart_hs_aec_calculate failed!\n");
		return -1;
	}

	/* AEC resample */
	hs_aec.aec.res.in_len  = hs_aec.outlen;
	hs_aec.aec.res.in_buf  = hs_aec.outbuf;
	aec_channel_outbuf = mozart_hs_data_sample(&hs_msg->aec, &hs_msg->hs, &hs_aec.aec, &aec_channel_outlen);
	if (!aec_channel_outbuf) {
		printf("Error: aec_channel_outbuf is NULL, please checked!!!\n");
		return -1;
	}
	hs_msg->out.out_buf = aec_channel_outbuf;
	hs_msg->out.out_len = aec_channel_outlen;
#else /* SUPPORT_WEBRTC */
	/* Record resample */
	hs_record.res.in_len   = hs_msg->out.record_len;
	hs_record.res.in_buf   = hs_msg->out.record_buf;
	record_channel_outbuf = mozart_hs_data_sample(&hs_msg->record, &hs_msg->hs, &hs_record, &record_channel_outlen);
	if (!record_channel_outbuf) {
		printf("Error: record_channel_outbuf is NULL, please checked!!!\n");
		return -1;
	}
	hs_msg->out.out_buf = record_channel_outbuf;
	hs_msg->out.out_len = record_channel_outlen;
#endif /* SUPPORT_WEBRTC */

	return 0;
}
#endif /* SUPPORT_BSA_HFP_HF */

#if (SUPPORT_BSA_A2DP_SYNK == 1)
static af_resample_t *avk_resample_t;
static af_channels_t *avk_channel_t;
static char *avk_resample_outbuf;
static char *avk_channel_outbuf;
static int  avk_resample_max_outlen;
static int  avk_channel_max_outlen;
static int  avk_resample_outlen;
static int  avk_channel_outlen;

static void mozart_avk_event_cback(tBSA_AVK_EVT event, void *p_data)
{
	switch (event) {
		case BSA_AVK_STOP_EVT:
			/* uninit resample */
			if(avk_resample_t != NULL) {
				mozart_resample_uninit(avk_resample_t);
				avk_resample_t = NULL;
			}
			if(avk_channel_t != NULL) {
				mozart_channels_uninit(avk_channel_t);
				avk_channel_t = NULL;
			}

			/* free out buffer */
			if (avk_resample_outbuf != NULL) {
				free(avk_resample_outbuf);
				avk_resample_outbuf = NULL;
			}
			if (avk_channel_outbuf != NULL) {
				free(avk_channel_outbuf);
				avk_channel_outbuf = NULL;
			}
			avk_resample_outlen 	= 0;
			avk_channel_outlen 	= 0;
			avk_resample_max_outlen = 0;
			avk_channel_max_outlen 	= 0;
			break;

		default:
			printf("[AVK] %s Not handle this event: %d\n", __func__, event);
			break;
	}
}

static void mozart_avk_data_cback(avk_data_cback_msg *avk_msg)
{
	/* No Resample */
	if ((avk_msg->recv_rate == avk_msg->cfg_rate)
			&& (avk_msg->recv_channel == avk_msg->cfg_channel)
			&& (avk_msg->recv_bits == avk_msg->cfg_bits)) {
		avk_msg->out_len    = avk_msg->in_len;
		avk_msg->out_buffer = avk_msg->in_buffer;
		return;
	/* Resample */
	} else {
		/* resample rate */
		
		if (avk_msg->recv_rate != avk_msg->cfg_rate) {
			
                             if (avk_resample_t == NULL) {
				avk_resample_t = mozart_resample_init(avk_msg->recv_rate, avk_msg->recv_channel, (avk_msg->recv_bits >> 3), avk_msg->cfg_rate);
				if (avk_resample_t == NULL) {
					printf("Error: mozart resample init rate failed\n");
					return;
				}
			}

			avk_resample_outlen = mozart_resample_get_outlen(avk_msg->in_len, avk_msg->recv_rate, avk_msg->recv_channel, avk_msg->recv_bits, avk_msg->cfg_rate);
			if (avk_resample_outbuf == NULL) {
				printf("malloc avk_resample_outbuf!\n");
				avk_resample_outbuf     = malloc(avk_resample_outlen);
				avk_resample_max_outlen = avk_resample_outlen;
			} else {
				if (avk_resample_outlen > avk_resample_max_outlen) {
					printf("realloc avk_resample_outbuf!\n");
					avk_resample_outbuf     = realloc(avk_resample_outbuf, avk_resample_outlen);
					avk_resample_max_outlen = avk_resample_outlen;
				}
			}
			avk_resample_outlen = mozart_resample(avk_resample_t, avk_msg->recv_channel, avk_msg->recv_bits, avk_msg->in_buffer, avk_msg->in_len, avk_resample_outbuf);

			/* resample channel */
			if (avk_msg->recv_channel != avk_msg->cfg_channel) {
				if (avk_channel_t == NULL) {
					avk_channel_t = mozart_channels_init(avk_msg->recv_channel, avk_msg->cfg_channel);
					if (avk_channel_t == NULL) {
						printf("Error: mozart resample init channel failed\n");
						return;
					}
				}
				avk_channel_outlen = mozart_channels_get_outlen(avk_channel_t, avk_resample_outlen);
				if (avk_channel_outbuf == NULL) {
					printf("malloc avk_channel_outbuf!\n");
					avk_channel_outbuf     = malloc(avk_channel_outlen);
					avk_channel_max_outlen = avk_channel_outlen;
				} else {
					if (avk_channel_outlen > avk_channel_max_outlen) {
						printf("realloc avk_channel_outbuf!\n");
						avk_channel_outbuf     = realloc(avk_channel_outbuf, avk_channel_outlen);
						avk_channel_max_outlen = avk_channel_outlen;
					}
				}
				mozart_channels(avk_channel_t, avk_msg->recv_bits>>3, 1, 0, avk_resample_outbuf, avk_resample_outlen, avk_channel_outbuf);
				avk_msg->out_len    = avk_channel_outlen;
				avk_msg->out_buffer = avk_channel_outbuf;
			} else {
				avk_msg->out_len    = avk_resample_outlen;
				avk_msg->out_buffer = avk_resample_outbuf;
			}
		} else {
			/* resample channel */
			if (avk_msg->recv_channel != avk_msg->cfg_channel) {
				if (avk_channel_t == NULL) {
					avk_channel_t = mozart_channels_init(avk_msg->recv_channel, avk_msg->cfg_channel);
					if (avk_channel_t == NULL) {
						printf("Error: mozart resample init channel failed\n");
						return;
					}
				}
				avk_channel_outlen = mozart_channels_get_outlen(avk_channel_t, avk_msg->in_len);
				if (avk_channel_outbuf == NULL) {
					printf("avk_channel_outbuf == NULL !\n");
					avk_channel_outbuf     = malloc(avk_channel_outlen);
					avk_channel_max_outlen = avk_channel_outlen;
				} else {
					if (avk_channel_outlen > avk_channel_max_outlen) {
						printf("realloc !\n");
						avk_channel_outbuf     = realloc(avk_channel_outbuf, avk_channel_outlen);
						avk_channel_max_outlen = avk_channel_outlen;
					}
				}
				mozart_channels(avk_channel_t, avk_msg->recv_bits>>3, 1, 0, avk_msg->in_buffer, avk_msg->in_len, avk_channel_outbuf);
				avk_msg->out_len    = avk_channel_outlen;
				avk_msg->out_buffer = avk_channel_outbuf;
			}
		}

		/* resample format */
		if (avk_msg->recv_bits != avk_msg->cfg_bits) {
			printf("Resample format: recv_format: %d, cfg_format: %d\n", avk_msg->recv_bits, avk_msg->cfg_bits);
			printf("Error: not handle format !!\n");
		}
	}
}
#endif /* SUPPORT_BSA_A2DP_SYNK */

void avk_start()
{
#if (SUPPORT_BSA_A2DP_SYNK == 1)
	avk_cback_init_data avk_cback;
	avk_cback.mozart_avk_evt_cback  = mozart_avk_event_cback;
	avk_cback.mozart_avk_data_cback = mozart_avk_data_cback;
	mozart_bluetooth_avk_init_callback(&avk_cback);
	if (mozart_bluetooth_avk_start_service()) {
		printf("avk service start failed.\n");
	}
#endif /* SUPPORT_BSA_A2DP_SYNK */
	mozart_bluetooth_set_visibility(true, true);

}

static void *thr_fn(void *args)
{
	pthread_t p_tid_reconnect;
	//pthread_t ble_config;
	int err;
	int i = 0;
	bt_init_info bt_info;
	char mac[] = "00:11:22:33:44:55";
	char bt_name[64] = {};
	char bt_avk_name[25] = "/var/run/bt-avk-fifo";
	char bt_socket_name[30] = "/var/run/bt-daemon-socket";
			
	if (pthread_mutex_init(&bt_mutex, NULL) != 0){           //全局变量初始化了，重复进行初始化
		printf("bt_mutex init error\n");
	}
		
	for(i = 0; i < 100; i++) {
		if(!access(bt_socket_name, 0) && !access(bt_avk_name, 0)) {
			break;
		} else {
			usleep(50000);
		}
	}

	if(access(bt_socket_name, 0) || access(bt_avk_name, 0)) {
		printf("%s or %s not exists, please check !!\n",
				bt_avk_name, bt_socket_name);
		goto err_exit;
	}

	memset(mac, 0, sizeof(mac));
	memset(bt_name, 0, sizeof(bt_name));
	get_mac_addr("wlan0", mac, "");
	strcat(bt_name, "EGO-");
	strcat(bt_name, mac+4);
	bt_info_init(&bt_info, bt_name);
#if (SUPPORT_BT == BT_RTK)
	system("bt_enable &");
#elif (SUPPORT_BT == BT_BCM)
	printf("Bluetooth name is: %s\n", bt_name);
	if (mozart_bluetooth_init(&bt_info)) {
		printf("bluetooth init failed.\n");
		goto err_exit;
	}
#if (SUPPORT_BSA_HFP_HF == 1)
	if (mozart_bluetooth_hs_start_service()) {
		printf("hs service start failed.\n");
		goto err_exit;
	}
	hs_cback_init_data hs_cback;
	hs_cback.mozart_hs_evt_cback      = mozart_hs_event_cback;
	hs_cback.mozart_hs_data_in_cback  = mozart_hs_data_in_cback;
	hs_cback.mozart_hs_data_out_cback = mozart_hs_data_out_cback;
	mozart_bluetooth_hs_init_callback(&hs_cback);
#endif /* SUPPORT_BSA_HFP_HF */

#if (SUPPORT_BSA_PBC == 1)
	mozart_bluetooth_pbc_start();
#endif /* SUPPORT_BSA_PBC */


#if (SUPPORT_BSA_A2DP_SOURCE == 1)
	int j = 0;
	int num = 50;
	int disc_num = -1;
	char *playlist_patch = "/mnt/sdcard";
	char *disc_name = "EGO-a23a13e1";
	av_open_param av_param;
	int duration = 3; /* Multiple of 1.28 seconds */

	mozart_avk_source_stream_init();
	mozart_bluetooth_av_start_service(playlist_patch);
	for (j = 0; j < 5; j++) {
		mozart_bluetooth_disc_start_regular(duration);
		for (i = 0; i < num; i++) {
			if(bsa_start_regular_enable != 1)
				usleep(200 * 1000);
			else {
				bsa_start_regular_enable = 0;
				break;
			}
		}
		if (i > num) {
			printf("mozart_bluetooth_disc_start_regular failed!\n");
			return NULL;
		}
		if (disc_num == -1) {
			disc_num = mozart_bluetooth_disc_find_device_name(disc_name);
			if (disc_num >= 0) {
				printf("find disc name: %s !!\n", disc_name);
				break;
			} else {
				printf("not find disc_name: %s\n", disc_name);
			}
		}
	}
	for (j = 0; j < 5; j++) {
		if (disc_num != -1) {
			av_param.select_num = 1;  /* Device found in last discovery */
			av_param.device_index = disc_num;
			mozart_bluetooth_av_open(&av_param);
			for (i = 0; i < num; i++) {
				if(bt_av_link_state == BT_LINK_CONNECT_FAILED) {
					printf("mozart_bluetooth_av_open failed!\n");
					break;
				} else if (bt_av_link_state == BT_LINK_CONNECTED) {
					printf("av link successful!\n");
					goto link_connected;
				} else if (bt_av_link_state == BT_LINK_DISCONNECTED)
					usleep(200*1000);
				else {
					printf("Not handle bt_av_link_state: %d\n", bt_av_link_state);
				}
			}
		}
	}
link_connected:
#endif /* SUPPORT_BSA_A2DP_SOURCE */

#if (SUPPORT_BSA_OPS == 1)
	ops_accept_param ops_param;
	ops_param.ops_push_dir = "/mnt/sdcard/";
	ops_param.ops_pull_file = "/mnt/sdcard/test.txt";
	mozart_bluetooth_ops_start();
	mozart_bluetooth_ops_auto_accept(&ops_param);
#endif /* SUPPORT_BSA_OPS */

#if (SUPPORT_BSA_OPC == 1)
	char *disc_name = "EGO-a23a144d";
	int disc_num = -1;
	opc_push_param opc_param;
	int j = 0;
	int num = 50;
	int duration = 3; /* Multiple of 1.28 seconds */

	mozart_bluetooth_opc_start();
	for (j = 0; j < 5; j++) {
		mozart_bluetooth_disc_start_regular(duration);
		for (i = 0; i < num; i++) {
			if(bsa_start_regular_enable != 1)
				usleep(200 * 1000);
			else {
				bsa_start_regular_enable = 0;
				break;
			}
		}
		if (i >= num) {
			printf("mozart_bluetooth_disc_start_regular failed!\n");
			return NULL;
		}
		if (disc_num == -1) {
			disc_num = mozart_bluetooth_disc_find_device_name(disc_name);
			if (disc_num >= 0) {
				printf("find disc name: %s !!\n", disc_name);
				break;
			} else {
				printf("not find disc_name: %s\n", disc_name);
			}
		}
	}
	opc_param.select_num = 1; /* Device found in last discovery */
	opc_param.device_index = disc_num;
	opc_param.file_path = "/mnt/sdcard/test_file.txt";
	mozart_bluetooth_opc_push_file(&opc_param);
#endif /* SUPPORT_BSA_OPC */

#if (SUPPORT_BSA_SPPS == 1)
	char *service_name = "BluetoothChatSecure";
	dg_start_service_paramters dg_service;
	memset(&dg_service, 0, sizeof(dg_start_service_paramters));
	char *spps_uuid = "00001101-0000-1000-8000-00805F9B34FB"; /* standard 128bit SPP UUID (16bit: 0x1101) */
	//char *spps_uuid = "FA87C0D0-AFAC-11DE-8A39-0800200C9A66"; /* for Android App BluetoothChatSecure */
	sscanf (spps_uuid, "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
			&dg_service.uuid[0], &dg_service.uuid[1], &dg_service.uuid[2], &dg_service.uuid[3],
			&dg_service.uuid[4], &dg_service.uuid[5], &dg_service.uuid[6], &dg_service.uuid[7],
			&dg_service.uuid[8], &dg_service.uuid[9], &dg_service.uuid[10], &dg_service.uuid[11],
			&dg_service.uuid[12], &dg_service.uuid[13], &dg_service.uuid[14], &dg_service.uuid[15]);
	dg_service.param.service = BSA_SPP_SERVICE_ID;
	memcpy(dg_service.param.service_name, service_name, strlen(service_name));
	mozart_bluetooth_dg_start();
	mozart_bluetooth_dg_start_server(&dg_service);
	mozart_bluetooth_dg_callback(mozart_bluetooth_dg_uipc_cback);
#else
	mozart_bluetooth_dg_callback(NULL);
#endif /* SUPPORT_BSA_SPPS */
#if (SUPPORT_BSA_SPPC == 1)
	int j = 0;
	int num = 50;
	int duration = 3; /* Multiple of 1.28 seconds */
	int disc_num = -1;
	//char *disc_name = "EGO-a23a144d";
	char *disc_name = "Printer003";

	mozart_bluetooth_dg_start();
#if 1
	for (j = 0; j < 5; j++) {
		mozart_bluetooth_disc_start_services(BSA_SPP_SERVICE_MASK, duration);
		while(bsa_start_regular_enable != 1) {
			usleep(200 * 1000);
		}
		bsa_start_regular_enable = 0;
		if (disc_num == -1) {
			disc_num = mozart_bluetooth_disc_find_device_name(disc_name);
			if (disc_num >= 0) {
				printf("find disc name: %s !!\n", disc_name);
				break;
			} else {
				printf("not find disc_name: %s\n", disc_name);
			}
		}
	}
	if (disc_num != -1) {
		int i = 0;
		BOOLEAN flag = 0;
		char connect_uuid[] = {0x00, 0x00, 0x11, 0x01, 0x00, 0x00, 0x10, 0x00,
			0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB}; /* standard 128bit SPP UUID (16bit: 0x1101) */
		/* char connect_uuid[] = {0xFA, 0x87, 0xC0, 0xD0, 0xAF, 0xAC, 0x11, 0xDE,
			0x8A, 0x39, 0x08, 0x00, 0x20, 0x0C, 0x9A, 0x66}; */ /* for Android App BluetoothChatSecure */

		dg_open_paramters dg_parm = {};
		dg_parm.connect_type = DEVICE_FROM_DISCOVERY;
		dg_parm.device_index = disc_num;
		dg_parm.param.service = BSA_SPP_SERVICE_ID;
		flag = sdpu_is_base_uuid(connect_uuid);
		if (flag) {
			printf("uuid is base uuid !!\n");
			dg_parm.is_128uuid = false;
		} else {
			char *connect_name = "BluetoothChatSecure";
			printf("uuid is not base uuid !!\n");
			for (i = 0; i < MAX_UUID_SIZE; i++) {
				dg_parm.uuid[i] = connect_uuid[i];
			}
			memcpy(dg_parm.param.service_name, connect_name, strlen(connect_name));
			dg_parm.is_128uuid = true;
		}
		dg_parm.param.sec_mask = BSA_SEC_NONE;
		for (j = 0; j < 5; j++) {
			bt_dg_flag = DG_IDLE;
			mozart_bluetooth_dg_open_connection(&dg_parm);
			for (i = 0; i < num; i++) {
				if(bt_dg_flag == DG_IDLE)
					usleep(200 * 1000);
				else if (bt_dg_flag == DG_CONNECTED) {
					goto connect_success;
				} else if (bt_dg_flag == DG_CONNECT_FAILED) {
					break;
				}
			}
		}
connect_success:
		bt_dg_flag = DG_IDLE;
	}
#endif
#endif /* SUPPORT_BSA_SPPC */
#if (SUPPORT_BSA_BLE == 1)
	int state = 0;
	state = mozart_bluetooth_ble_start();
	if (state) {
		printf("mozart_bluetooth_ble_start failed, state = %d.\n", state);
		goto err_exit;
	}
#if (SUPPORT_BSA_BLE_SERVER == 1)
	/* create ble service */
	state = ble_service_and_char_create();
	if (state != 0) {
		printf("ERROR: ble_service_and_char_create failed !\n");
		goto err_exit;
	}
#endif	/* SUPPORT_BSA_BLE_SERVER */
//	mozart_bluetooth_ble_set_visibility(discoverable, connectable);
#if (SUPPORT_BSA_BLE_CLIENT == 1)
#if (SUPPORT_BSA_BLE_HH == 1)
#if (SUPPORT_BSA_BLE_HH_DIALOG_AUDIO == 1)
	dialog_init();
#endif /* SUPPORT_BSA_BLE_HH_DIALOG_AUDIO */
	mozart_bluetooth_hh_callback(mozart_bluetooth_hh_uipc_cback);
	/* create ble_hid */
	state = ble_hh_create();
	if (state != 0) {
		printf("ERROR: ble_hh_create failed !\n");
	}
#else
	//6.1
	err = pthread_create(&p_tid_reconnect, NULL, ble_reconnect, NULL);
	if (err != 0)
	printf("can't create thread: %s\n", strerror(err));
	pthread_detach(p_tid_reconnect);
	#if 0
	err = pthread_create(&ble_config, NULL, ble_config_fun, NULL);
	if (err != 0)
	printf("can't create thread: %s\n", strerror(err));
	pthread_detach(p_tid_log_manage);	//6.2
	//6.1
	#endif
#endif /* SUPPORT_BSA_BLE_HH */
#endif /* SUPPORT_BSA_BLE_CLIENT */
#endif /* SUPPORT_BSA_BLE */
#else  /* SUPPORT_BT */
	printf("Bt funcs are closed.\n");
	goto err_exit;
#endif /* SUPPORT_BT */

err_exit:
	return NULL;
}

static void free_cmd_list(void *cmd_list_info)
{
	struct BLE_INFO *ble_info = (struct BLE_INFO *)cmd_list_info;
	if (ble_info) {
		free(ble_info);
	}
}

//ble与数据接收后，对列表的数据的处理
static void *ble_func(void *arg)
{
	pthread_mutex_lock(&ble_mutex);
	while (die_out_bt) {
		
		if (!is_empty(&cmd_list)) {
			struct BLE_INFO *ble_info;
			ble_info= list_delete_at_index(&cmd_list, 0);
			
			if (ble_info && ble_info->len <= 36){
				pthread_mutex_unlock(&ble_mutex);
			
				if ( ble_info->value[2]==0x20 && ble_info->value[0]==0xaa && ble_info->value[1]==0xaa && ble_info->len <= 36 ){
					memcpy(x_data,ble_info->value,ble_info->len);
					data_len=ble_info->len;
					flag=1;				
				}
				else if(flag==1 && ble_info->value[0] != 0xaa  && ble_info->value[1] != 0xaa && ble_info->len <= (36-data_len)){
					memcpy(&x_data[data_len],ble_info->value,ble_info->len);
					data_len=data_len+ble_info->len;

					Unpack(x_data ,info,data_len);   	//逻辑：从蓝牙传来的数据链表中把数据一个个的取出来处理	
					flag=0;
				}
				else{
					Unpack(ble_info->value ,info,ble_info->len);
					flag = 0;				
				}
				
				pthread_mutex_lock(&ble_mutex);
			}
			
			/**********************2018.7.2号**************************************/
			free_cmd_list(ble_info);
			/********************************************************************/
		}
		else {	
			pthread_cond_wait(&ble_var, &ble_mutex);
		}
	}

	list_destroy(&cmd_list, free_cmd_list);
	pthread_mutex_unlock(&ble_mutex);

	return NULL;
}

static void *up_info(void *arg)
{
	while(1){
		//base和tower 一建立连接就发送数据
		if(open_chancal == true){		
			while(1){
				printf("..............start up tower info\n");
				if(up_tower_info() == -1);
				else
					break;
				sleep(1);
			}
			open_chancal = false;
		}
		
		sleep(1);
	}
	return NULL;
}

int start_bt(void)
{
	int err;
	pthread_t p_tid;
	pthread_t upInfo_thread;
	
	system("/usr/fs/etc/init.d/S04bsa.sh start");
#if (SUPPORT_BSA_BLE == 1)
	bt_ble_callback ble_cback = {};
	bsa_ble_cb = mozart_bluetooth_ble_get_cb();
#if (SUPPORT_BSA_BLE_CLIENT == 1)
	ble_cback.ble_client_profile_cback = bsa_ble_client_profile_cback;
#endif
#if (SUPPORT_BSA_BLE_SERVER == 1)
	ble_cback.ble_server_profile_cback = bsa_ble_server_profile_cback;
#endif
	mozart_bluetooth_ble_callback(&ble_cback);
#else
	mozart_bluetooth_ble_callback(NULL);
#endif
	err = pthread_create(&p_tid, NULL, thr_fn, NULL);
	if (err != 0)
	printf("can't create thread: %s\n", strerror(err));

	pthread_detach(p_tid);
	
	pthread_mutex_lock(&ble_mutex);
	if (die_out_bt) {
		pthread_mutex_unlock(&ble_mutex);
		return 0;
	}

	die_out_bt = true;
	list_init(&cmd_list);

	if (pthread_create(&ble_thread, NULL, ble_func, NULL)) {
		return -1;
	}
	pthread_detach(ble_thread);

	if (pthread_create(&upInfo_thread, NULL, up_info, NULL)) {
		printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>create pthread error\n");
		return -1;
	}
	pthread_detach(upInfo_thread);
	
	pthread_mutex_unlock(&ble_mutex);
	
	return 0;
}

int stop_bt(void)
{
	bluetooth_cancel_auto_reconnect_pthread();
	mozart_bluetooth_disconnect(USE_HS_AVK);
#if (SUPPORT_BSA_HFP_HF == 1)
	mozart_bluetooth_hs_stop_service();
#endif

#if (SUPPORT_BSA_A2DP_SYNK == 1)
	mozart_bluetooth_avk_stop_service();
#endif

#if (SUPPORT_BSA_PBC == 1)
	mozart_bluetooth_pbc_stop();
#endif
	mozart_bluetooth_uninit();
	system("/usr/fs/etc/init.d/S04bsa.sh stop");

	pthread_mutex_lock(&ble_mutex);
	if (die_out_bt) {
		die_out_bt = false;
		pthread_cond_signal(&ble_var);
	}
	pthread_mutex_unlock(&ble_mutex);

	return 0;
}
