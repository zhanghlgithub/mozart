#include <stdio.h>  
#include <stdlib.h>  
#include <fcntl.h>  
#include <string.h>  
#include <pthread.h>
#include <linux/i2c.h>  
#include <linux/i2c-dev.h>   
#include <unistd.h>
#include <errno.h>
#include <inttypes.h>
#include<termios.h>    
#include <sys/types.h>  
#include <sys/stat.h>  
#include <sys/mman.h>
#include <sys/ioctl.h>

#include "tips_interface.h"  //语音提示

#include "mozart_iic_touch.h" 
#include "mozart_musicplayer.h"
#include "modules/mozart_module_local_music.h"
#include "ingenicplayer.h"
#define I2C_DEV "/dev/i2c-0"						//i2c_dev为i2c　adapter创建的别名


static pthread_t iic_pthread;


int iic_read(int fd, unsigned char buff[], unsigned char reg_addr)  
{  
    int res;  
    if(write(fd,&reg_addr,1)==1)  {
	   res=read(fd,buff,1);//IF == 1   SUCCESEE;
   	   return res;  
	   }
    else
	return 44;
}  

int iic_write(int fd, unsigned char send_buff, unsigned char reg_addr)  
{  
        int res;  
       res=write(fd , &reg_addr , 1);  
	if (res ==1){
      		 res = write(fd, &send_buff,1);
		return res;
			}
	else
		return res;
}  
/*
static int  MPU6050_Init(int fd )
{
	unsigned char motion_address=0x69;
	unsigned char reg_address=0x01;
	int res;
	if(ioctl(fd, I2C_SLAVE, motion_address)<0) {    //set i2c address 
		printf("fail to set i2c device slave address!\n");
		close(fd);
		return -1;
	}
	printf("set slave address to 0x%x success!\n", motion_address);
	//write(fd, &motion_address, 1);
	//iic_write(fd, 0X00, 0x6B);    
	//write(fd, &motion_address, 1);
	//iic_write(fd, 0X07, 0x19); 
	//write(fd, &motion_address, 1);
	//iic_write(fd, 0X06, 0x1A); 
	//write(fd, &motion_address, 1);
	//iic_write(fd, 0x18, 0x1B); 
	//write(fd, &motion_address, 1);
	if(write(fd,&reg_address,1)==1)
		printf("\n write test success\n");
	else
		printf("write failed\n");
	res = iic_write(fd, 0X01, 0x1C); 
	if (res == 1)
	return(1);
	else	
	return(0);
}
*/

static void *pthread_iic(void *args){
	int fd;
	int res;
	
    unsigned char rec_buf[3]={0,0,0};
    unsigned char rec_buf_old[3]={0,0,0};
	int time=0;
	bool sign=false;
	   
//	unsigned char touch_rec_addr0=0x00;
//	unsigned char touch_rec_addr1=0x01;
	unsigned char touch_rec_addr2=0x02;	
/*	
	unsigned char motion_rec_addr1=0x75;
	unsigned char motion_rec_addr2=0x3A;*/
	unsigned char touch_address = 0x08; //10 

	
	printf(">>>>>>>>>This is a test of I2C<<<<<<<<<<<\n");
	fd = open( I2C_DEV ,O_RDWR);
	if (fd<0){
             perror("can't open i2c device!");
     	}
	
	res = ioctl(fd,I2C_TENBIT,0);
	if(res < 0){
	  	printf("I2C_TENBIT error, errno = %d, %s\n", errno, strerror(errno));
		close(fd);  		
	 }
	while(1)
	{
	

		res = ioctl(fd,I2C_SLAVE,touch_address);
		if(res < 0){
	  		printf("touch_SLAVE error, errno = %d, %s\n", errno, strerror(errno));
			close(fd);  		
	 	}

#if 0			
		res = iic_read(fd, rec_buf, touch_rec_addr1)  ;
		if (res==1)
		{
		//	printf(">>>>>>>touch_read address: 0x%x====value:0x%x<<<<\n",  touch_rec_addr0 , rec_buf[0] );
            if(rec_buf_old[0]!=0xff&&rec_buf[0]!=0xff)
           	{
           		//混动条设置音量
                mozart_musicplayer_set_volume(mozart_musicplayer_handler,mozart_musicplayer_get_volume(mozart_musicplayer_handler)+(rec_buf[0]-rec_buf_old[0])*0.2);
           	}	  
			rec_buf_old[0]=rec_buf[0];
		}
		else
			printf(">>>>>>>>>>>touch_read faild<<<<<<<<<<\n");

		res = iic_read(fd, rec_buf+1, touch_rec_addr0)  ;
		if (res==1)
		{
		//	printf(">>>>>>>touch_read address: 0x%x====value:0x%x<<<<\n",  touch_rec_addr1 , rec_buf[1] );
		    if(rec_buf_old[1]!=0xff&&rec_buf[1]!=0xff)
            {
            	/*此处修改灯光效果*/     
            }	  
			rec_buf_old[1]=rec_buf[1];
		}
		else
			printf(">>>>>>>>>>>touch_read faild<<<<<<<<<<\n");
#endif
		res = iic_read(fd, rec_buf+2, touch_rec_addr2)  ;
		if (res==1)
		{
		//	 printf(">>>>>>>touch_read address: 0x%x====value:0x%x<<<<\n",  touch_rec_addr2, rec_buf[2] );
			 
        	if(rec_buf[2]==0x01&&rec_buf_old[2]==0xff)
			sign=true;
			if(rec_buf[2]==0xff&&rec_buf_old[2]==0x01)
			{
				if(time>4)
			    {
                	if(mozart_mozart_mode == SLEEP)
                    {
                    	mozart_start_mode(FREE);
					    system("echo 1 > /sys/class/leds/led-sleep/brightness");
					    usleep(50000);
				        mozart_ingenicplayer_notify_work_status();
						mozart_play_key("exit_sleep_mode");
                    }
					else if(mozart_mozart_mode == FREE)
					{
                    	mozart_start_mode(SLEEP);
					    system("echo 0 > /sys/class/leds/led-sleep/brightness");
					    usleep(50000);
				        mozart_ingenicplayer_notify_work_status();
						//语音提示：
						mozart_play_key("enter_sleep_mode");
					 }
				}		
			  	time=0;
                sign=false;
			}
			  
			if(sign==true)
				time++;
			  
			rec_buf_old[2]=rec_buf[2] ;
		}
		else
			printf(">>>>>>>>>>>touch_read faild<<<<<<<<<<\n");
			
		usleep(1000*50);   //0.05秒
			/*
				if(MPU6050_Init( fd )==1)
		printf("MPU6050_SUCCESEE\n");

		res=iic_read(fd, rec_buf, motion_rec_addr1)  ;
		if (res==1)
			printf(">>>>>>>motion_read address: 0x%x====value:0x%x<<<<\n",  motion_rec_addr1 , rec_buf[0] );
		else
			printf(">>>>>>>>>>>motion_read faild<<<<<<<<<<\n");

	*/	
	}
	close(fd);
	return NULL;
}

void start_iic(void){
	int err;
	printf(">>>>>>>>>>>>>>>>>>dsc_start_iic<<<<<<<<<<<<<<<<<<<\n");
	err = pthread_create(&iic_pthread, NULL, pthread_iic, NULL);
	if (err != 0){
	printf("can't create thread: %s\n", strerror(err));
	}
	pthread_detach(iic_pthread);
	}
