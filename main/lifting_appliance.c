#include <stdio.h>        // for printf
#include <stdlib.h>        // for exit
#include <string.h>        // for bzero        
#include <unistd.h>        
#include <arpa/inet.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <termio.h>
#include <time.h>
#include <pthread.h>

#include "lifting_appliance.h"
#include "ingenicplayer.h"

#define Height_detection_1 "/sys/class/gpio/gpio96/value"
//#define Height_detection_2 "/sys/class/gpio/gpio98/value"
#define Height_detection_3 "/sys/class/gpio/gpio90/value"
#define Height_detection_4 "/sys/class/gpio/gpio91/value"
#define Height_detection_5 "/sys/class/gpio/gpio15/value"

bool led_state=false;
int fd_ttys1 = -1;
static pthread_t lifting_appliance_thread;
static pthread_t ttys1_read_thread;
int lifting_appliance_high=0;    	//当前的高度
int lifting_appliance_go_high=0;  	//要上升到的高度
int lifting_appliance_state=1;
char res_top[7]={0xAA,0XAA,0x02,0x02,0x02,0x04,0};    //磁悬浮处在最顶端
char res_suspension[7]={0xAA,0XAA,0x02,0x02,0x03,0x05,0}; //磁悬浮处于悬空状态
char res_bottom[7]={0xAA,0XAA,0x02,0x02,0x01,0x03,0};   //磁悬浮处于最低端
char res_drop[7]={0xAA,0XAA,0x02,0x02,0x04,0x06,0};    //磁悬浮脱落警告
char res_shift_drop[7]={0xAA,0XAA,0x02,0x02,0x05,0x07,0};    //磁悬浮偏移脱落警告

int set_com_config(int fd1,int baud_rate,int data_bits,char parity,int stop_bits)
{
	struct termios new_cfg,old_cfg;
	int speed;
	if(tcgetattr(fd1,&old_cfg)!=0)
	{
		perror("tcgetattr");
		return -1;
	}
	new_cfg=old_cfg;
	cfmakeraw(&new_cfg);
	new_cfg.c_cflag&=~CSIZE;
	switch(baud_rate)
	{
		case 9600:
		{
			speed=B9600;
		}
		break;
		default:
		case 115200:
		{
			speed=B115200;
        		}
		break;
	}
	cfsetispeed(&new_cfg,speed);
	cfsetospeed(&new_cfg,speed);
	switch(data_bits)
	{
		case 7:
		{
            new_cfg.c_cflag&=~CSIZE;
			new_cfg.c_cflag|=CS7;
		}
		break;	
        default:
		case 8:
		{
            new_cfg.c_cflag&=~CSIZE;
			new_cfg.c_cflag|=CS8;
                        
                        
		}
		break;		
	}
            
 	switch(parity)
	{
                        
		default:
		case 'n':
		case 'N':
		{
                       
			new_cfg.c_cflag&=~PARENB;
			//new_cfg.c_cflag&=~INPCK;  
		}	
		break;	
	}
	switch(stop_bits)
	{
		default:
		case 1:
		{
			new_cfg.c_cflag&=~CSTOPB;
		}		
		break;
		case 2:
		{
			new_cfg.c_cflag|=CSTOPB;
		}	
	}
	new_cfg.c_cc[VTIME]=1;
	new_cfg.c_cc[VMIN]=255;
	tcflush(fd1,TCIFLUSH);
	if(tcsetattr(fd1,TCSANOW,&new_cfg)!=0)
	{
		perror("tcsetattr");
		return -1;
	}
	return 0;
}


int open_port()
{
	int fd1;
	char *dev[]={"/dev/ttyS1"};
	fd1=open(dev[0],O_RDWR|O_NOCTTY|O_NONBLOCK|O_NDELAY);
	if(fd1<0)
	{
		perror("open serial port error");
		return -1;
	}	
	if(fcntl(fd1,F_SETFL,0)<0)
	{
		perror("fcntl F_SETFL\n");
	}	
	if(isatty(fd1)==0)
	{
		perror("This is not a terminal device");
	}	
	return fd1;
}

//上升操作初始化	
void lifting_appliance_init()
{
	fd_ttys1=open_port();

   	if(fd_ttys1<0)
   	{
		perror("Open port failed.\r\n");
    }

	if(set_com_config(fd_ttys1,115200,8,'n',1)<0)
   	{
		perror("set_com_config error");
  	}

	system("echo 15 > /sys/class/gpio/export");  
	system("echo 91 > /sys/class/gpio/export");  
	system("echo 90 > /sys/class/gpio/export");  
	system("echo 98 > /sys/class/gpio/export");  
	system("echo 96 > /sys/class/gpio/export");  

	system("echo in > /sys/class/gpio/gpio15/direction"); 
	system("echo in > /sys/class/gpio/gpio91/direction"); 
	system("echo in > /sys/class/gpio/gpio90/direction"); 
	system("echo in > /sys/class/gpio/gpio98/direction"); 
	system("echo in > /sys/class/gpio/gpio96/direction"); 

}

void lifting_appliance_control(int sign)
{
	int ret = -1;
	printf("???????????sign:%d\n",sign);  
  	char up[6] = {0xAA,0XAA,0x02,0x01,0x01,0x02};
  	char stop[6] = {0xAA,0XAA,0x02,0x01,0x03,0x04};
  	char down[6] = {0xAA,0XAA,0x02,0x01,0x02,0x03};
 	char serch[6] = {0xAA,0XAA,0x02,0x01,0x04,0x05};
	char power_on[6] = {0xAA,0XAA,0x02,0x01,0x05,0x06};	
 	char power_off[6] = {0xAA,0xAA,0x02,0x01,0x06,0x07};

   	if(sign == 0)
   	{
   		printf("\n up,fd_ttys1,%d\n",fd_ttys1);
   		ret = write(fd_ttys1,up,6);
		printf("\n up,ret: %d\n",ret);
   	}
   	if(sign == 1)
   	{
   		write(fd_ttys1,stop,6);
		printf("\n stop");
   	}
   	if(sign == 2)
   	{
   		write(fd_ttys1,down,6);
		printf("\n down");
   	}
   	if(sign == 3)
   	{
   		write(fd_ttys1,serch,6);
   	}
	if(sign == 4)
	{
		write(fd_ttys1,power_on,6);
	}
	if(sign == 5)
	{
		write(fd_ttys1,power_off,6);
	}   	
}

//功能：读取当前磁悬浮所处的位置，并通知app
static void *ttys1_read(void *arg)
{
   	int length = 0;
   	char buffer[7];
  	bzero(buffer,7);
  	while(1)
 	{
		//printf("\nhigh:%d",lifting_appliance_high);
    	length = read(fd_ttys1,buffer,7);		
    	if(length > 0)
    	{
  			//printf("\n%d %d %d %d %d %d %d",buffer[0],buffer[1],buffer[2],buffer[3],buffer[4],buffer[5],buffer[6]);
	
  			if(strcmp(buffer,res_top)==0)  //磁悬浮在最顶端
    		{
	  			lifting_appliance_high=6;
	  			mozart_ingenicplayer_notify_get_hight(22);
			//  suspensionlog(22);
    		}

			if(strcmp(buffer,res_bottom)==0)  
    		{
	  			lifting_appliance_high=0;
	 			mozart_ingenicplayer_notify_get_hight(21);
				// suspensionlog(21);
    		}
    		if(strcmp(buffer,res_drop)==0)  
   			{
				//  lifting_appliance_high=7;	
				mozart_ingenicplayer_notify_get_hight(23);
   			}
    		if(strcmp(buffer,res_shift_drop)==0)  
    		{
				//  lifting_appliance_high=7;	
				mozart_ingenicplayer_notify_get_hight(23);
				//suspensionlog(23);
    		}
    
    		 bzero(buffer,7);
    		 length = 0;
			//	 usleep(200000);
   		}
  	}
	return NULL;
}

//根据现在注意力集中的等级亮灯的个数，共五个每个代表一个等级
void lifting_appliance_led(int i)
{
 	switch(i){
 	 	case 0:
	 		if(!led_state)
	 		{
		 		system("echo 1 > /sys/class/leds/led-charge/brightness");
				system("echo 1 > /sys/class/leds/led-wifi1/brightness");	
				system("echo 1 > /sys/class/leds/led-bt1/brightness");
				system("echo 1 > /sys/class/leds/led-aux/brightness");	
				system("echo 1 > /sys/class/leds/led-usb/brightness");	
			
		        system("echo timer > /sys/class/leds/led-charge/trigger");	
	            system("echo 500 > /sys/class/leds/led-charge/delay_on");	
	            system("echo 500 > /sys/class/leds/led-charge/delay_off");

				system("echo timer > /sys/class/leds/led-wifi1/trigger"); 		
	            system("echo 500> /sys/class/leds/led-wifi1/delay_on");	
	            system("echo 500> /sys/class/leds/led-wifi1/delay_off");		

	            system("echo timer > /sys/class/leds/led-bt1/trigger");
	            system("echo 500 > /sys/class/leds/led-bt1/delay_on");	
	            system("echo 500 > /sys/class/leds/led-bt1/delay_off");

	            system("echo timer > /sys/class/leds/led-aux/trigger"); 
	            system("echo 500 > /sys/class/leds/led-aux/delay_on");	
	            system("echo 500 > /sys/class/leds/led-aux/delay_off");	

	            system("echo timer > /sys/class/leds/led-usb/trigger");
	            system("echo 500 > /sys/class/leds/led-usb/delay_on");	
	            system("echo 500 > /sys/class/leds/led-usb/delay_off"); 

				led_state=true;
	 	}
		break;

		case 1:
			if(led_state)	
			{
				system("echo timer > /sys/class/leds/led-charge/trigger");	
		        system("echo 0 > /sys/class/leds/led-charge/delay_on");	
	            system("echo 0 > /sys/class/leds/led-charge/delay_off");

	            system("echo timer > /sys/class/leds/led-wifi1/trigger"); 
		        system("echo 0 > /sys/class/leds/led-wifi1/delay_on");	
	            system("echo 0> /sys/class/leds/led-wifi1/delay_off");

	            system("echo timer > /sys/class/leds/led-bt1/trigger");
			    system("echo 0 > /sys/class/leds/led-bt1/delay_on");	
	            system("echo 0 > /sys/class/leds/led-bt1/delay_off");

	            system("echo timer > /sys/class/leds/led-aux/trigger"); 
			    system("echo 0 > /sys/class/leds/led-aux/delay_on");	
	            system("echo 0 > /sys/class/leds/led-aux/delay_off");

	            system("echo timer > /sys/class/leds/led-usb/trigger");
			    system("echo 0 > /sys/class/leds/led-usb/delay_on");	
	            system("echo 0 > /sys/class/leds/led-usb/delay_off");
	            led_state=false;
			}
			system("echo 0 > /sys/class/leds/led-charge/brightness");
			system("echo 1 > /sys/class/leds/led-wifi1/brightness");	
			system("echo 1 > /sys/class/leds/led-bt1/brightness");
			system("echo 1 > /sys/class/leds/led-aux/brightness");	
			system("echo 1 > /sys/class/leds/led-usb/brightness");	
			break;

		case 2:
			if(led_state)	
			{
				system("echo timer > /sys/class/leds/led-charge/trigger");	
	            system("echo 0 > /sys/class/leds/led-charge/delay_on");	
                system("echo 0 > /sys/class/leds/led-charge/delay_off");
                system("echo timer > /sys/class/leds/led-wifi1/trigger"); 
	            system("echo 0 > /sys/class/leds/led-wifi1/delay_on");	
                system("echo 0> /sys/class/leds/led-wifi1/delay_off");

                system("echo timer > /sys/class/leds/led-bt1/trigger");
		        system("echo 0 > /sys/class/leds/led-bt1/delay_on");	
                system("echo 0 > /sys/class/leds/led-bt1/delay_off");

                system("echo timer > /sys/class/leds/led-aux/trigger"); 
		      	system("echo 0 > /sys/class/leds/led-aux/delay_on");	
                system("echo 0 > /sys/class/leds/led-aux/delay_off");

                system("echo timer > /sys/class/leds/led-usb/trigger");
		      	system("echo 0 > /sys/class/leds/led-usb/delay_on");	
                system("echo 0 > /sys/class/leds/led-usb/delay_off");
                led_state=false;
			}
			system("echo 0 > /sys/class/leds/led-charge/brightness");
			system("echo 0 > /sys/class/leds/led-wifi1/brightness");	
			system("echo 1 > /sys/class/leds/led-bt1/brightness");
			system("echo 1 > /sys/class/leds/led-aux/brightness");	
			system("echo 1 > /sys/class/leds/led-usb/brightness");	
			break;

	  	case 3:
	  		if(led_state)	
			{
				system("echo timer > /sys/class/leds/led-charge/trigger");	
	            system("echo 0 > /sys/class/leds/led-charge/delay_on");	
                system("echo 0 > /sys/class/leds/led-charge/delay_off");

                system("echo timer > /sys/class/leds/led-wifi1/trigger"); 
	            system("echo 0 > /sys/class/leds/led-wifi1/delay_on");	
                system("echo 0> /sys/class/leds/led-wifi1/delay_off");

                system("echo timer > /sys/class/leds/led-bt1/trigger"); 
		       	system("echo 0 > /sys/class/leds/led-bt1/delay_on");	
                system("echo 0 > /sys/class/leds/led-bt1/delay_off");

                system("echo timer > /sys/class/leds/led-aux/trigger"); 
		      	system("echo 0 > /sys/class/leds/led-aux/delay_on");	
                system("echo 0 > /sys/class/leds/led-aux/delay_off");

                system("echo timer > /sys/class/leds/led-usb/trigger");
		      	system("echo 0 > /sys/class/leds/led-usb/delay_on");	
                system("echo 0 > /sys/class/leds/led-usb/delay_off");
                led_state=false;
			}
			system("echo 0 > /sys/class/leds/led-charge/brightness");
			system("echo 0 > /sys/class/leds/led-wifi1/brightness");	
			system("echo 0 > /sys/class/leds/led-bt1/brightness");
			system("echo 1 > /sys/class/leds/led-aux/brightness");	
			system("echo 1 > /sys/class/leds/led-usb/brightness");				
			break;

	  	case 4:
	  		if(led_state)	
			{
				system("echo timer > /sys/class/leds/led-charge/trigger");	
	            system("echo 0 > /sys/class/leds/led-charge/delay_on");	
                system("echo 0 > /sys/class/leds/led-charge/delay_off");

                system("echo timer > /sys/class/leds/led-wifi1/trigger"); 
	            system("echo 0 > /sys/class/leds/led-wifi1/delay_on");	
                system("echo 0> /sys/class/leds/led-wifi1/delay_off");

                system("echo timer > /sys/class/leds/led-bt1/trigger");
		       	system("echo 0 > /sys/class/leds/led-bt1/delay_on");	
                system("echo 0 > /sys/class/leds/led-bt1/delay_off");

                system("echo timer > /sys/class/leds/led-aux/trigger"); 
		     	system("echo 0 > /sys/class/leds/led-aux/delay_on");	
                system("echo 0 > /sys/class/leds/led-aux/delay_off");

                system("echo timer > /sys/class/leds/led-usb/trigger");
		      	system("echo 0 > /sys/class/leds/led-usb/delay_on");	
                system("echo 0 > /sys/class/leds/led-usb/delay_off");
                led_state=false;
			}
			system("echo 0 > /sys/class/leds/led-charge/brightness");
			system("echo 0 > /sys/class/leds/led-wifi1/brightness");	
			system("echo 0 > /sys/class/leds/led-bt1/brightness");
			system("echo 0 > /sys/class/leds/led-aux/brightness");	
			system("echo 1 > /sys/class/leds/led-usb/brightness");			
		break;

	   	case 5:
	   		if(led_state)	
			{
				system("echo timer > /sys/class/leds/led-charge/trigger");	
	            system("echo 0 > /sys/class/leds/led-charge/delay_on");	
                system("echo 0 > /sys/class/leds/led-charge/delay_off");

                system("echo timer > /sys/class/leds/led-wifi1/trigger"); 
	            system("echo 0 > /sys/class/leds/led-wifi1/delay_on");	
                system("echo 0> /sys/class/leds/led-wifi1/delay_off");

                system("echo timer > /sys/class/leds/led-bt1/trigger");
		       	system("echo 0 > /sys/class/leds/led-bt1/delay_on");	
                system("echo 0 > /sys/class/leds/led-bt1/delay_off");

                system("echo timer > /sys/class/leds/led-aux/trigger"); 
		      	system("echo 0 > /sys/class/leds/led-aux/delay_on");	
                system("echo 0 > /sys/class/leds/led-aux/delay_off");

                system("echo timer > /sys/class/leds/led-usb/trigger");
		      	system("echo 0 > /sys/class/leds/led-usb/delay_on");	
                system("echo 0 > /sys/class/leds/led-usb/delay_off");
                led_state=false;
			}
			system("echo 0 > /sys/class/leds/led-charge/brightness");
			system("echo 0 > /sys/class/leds/led-wifi1/brightness");	
			system("echo 0 > /sys/class/leds/led-bt1/brightness");
			system("echo 0 > /sys/class/leds/led-aux/brightness");	
			system("echo 0 > /sys/class/leds/led-usb/brightness");			
			break;

	    default:
	   		system("echo timer > /sys/class/leds/led-charge/trigger");	
	   		system("echo 0 > /sys/class/leds/led-charge/delay_on");	
            system("echo 0 > /sys/class/leds/led-charge/delay_off");
	   		system("echo 1 > /sys/class/leds/led-charge/brightness");

            system("echo timer > /sys/class/leds/led-wifi1/trigger"); 
	       	system("echo 0 > /sys/class/leds/led-wifi1/delay_on");	
            system("echo 0> /sys/class/leds/led-wifi1/delay_off");
			system("echo 1 > /sys/class/leds/led-wifi1/brightness");	

            system("echo timer > /sys/class/leds/led-bt1/trigger");
			system("echo 0 > /sys/class/leds/led-bt1/delay_on");	
            system("echo 0 > /sys/class/leds/led-bt1/delay_off");
			system("echo 1 > /sys/class/leds/led-bt1/brightness");

            system("echo timer > /sys/class/leds/led-aux/trigger"); 
			system("echo 0 > /sys/class/leds/led-aux/delay_on");	
            system("echo 0 > /sys/class/leds/led-aux/delay_off");
			system("echo 1 > /sys/class/leds/led-aux/brightness");	

            system("echo timer > /sys/class/leds/led-usb/trigger");
			system("echo 0 > /sys/class/leds/led-usb/delay_on");	
            system("echo 0 > /sys/class/leds/led-usb/delay_off");
			system("echo 1 > /sys/class/leds/led-usb/brightness");

			led_state=false;
	   		break;

	}
}
static void *lifting_appliance(void *arg)
{
	int fd1;
 //	int fd2;
 	int fd3;
	int fd4;
	int fd5;
	int high=0;
	int state=1;
	char value1[3],value3[3],value4[3],value5[3];


  	while(1)
  	{  
  		high=lifting_appliance_high;
  		fd1=  open(Height_detection_1, O_RDONLY);
  		//fd2=  open(Height_detection_2, O_RDONLY);
  		fd3=  open(Height_detection_3, O_RDONLY);
  		fd4=  open(Height_detection_4, O_RDONLY);
  		fd5=  open(Height_detection_5, O_RDONLY);

  	   	read(fd1,value1,3);
	   	//read(fd2,value2,3);
	   	read(fd3,value3,3);
	   	read(fd4,value4,3);
	   	read(fd5,value5,3);
		
      //printf("%s %s %d valuel[0]:%d..%d..%d..%d..\n",__FILE__, __func__, __LINE__,value1[0],value3[0],value4[0],value5[0]);
        if(value1[0]==49)
       	{
       		
       		high=1;
        }
	 //	if(value2[0]==49)
	 //	{
	 //		high=2;
	 //	}
	 	if(value3[0]==49)
	 	{
	 		high=3;
	 	}
        if(value4[0]==49)
        {
        	high=4;
        }
	 	if(value5[0]==49)
	 	{
	 		high=5;
	 	}		  
   		close(fd1);
   		//close(fd2);
   		close(fd3);
   		close(fd4);
   		close(fd5);	

        if(lifting_appliance_high!=high)
        {
        	// lifting_appliance_led(lifting_appliance_high,high);
	        lifting_appliance_high=high;
			mozart_ingenicplayer_notify_get_hight(20);	
        }
	//	printf("\n     %d     ->       %d",lifting_appliance_high,lifting_appliance_go_high);

		if(lifting_appliance_go_high>5)
			lifting_appliance_go_high=5;
        if(lifting_appliance_high<lifting_appliance_go_high)
			state=0;
		else if(lifting_appliance_high>lifting_appliance_go_high)
			state=2;
		else if(lifting_appliance_high==lifting_appliance_go_high)
			state=1;

	//	printf("\n lifting_appliance_state state:%d  %d",lifting_appliance_state,state);
		if(lifting_appliance_state!=state)
		{
			lifting_appliance_state=state;
            lifting_appliance_control(lifting_appliance_state);
		}
        usleep(300000);
  	}
	return NULL;
}

void lifting_appliance_start()
{
	lifting_appliance_control(2);
	if(0 != pthread_create(&lifting_appliance_thread, NULL,lifting_appliance, NULL))
		printf("Can't create lifting appliance!\n");
    pthread_detach(lifting_appliance_thread);

	if(0 != pthread_create(&ttys1_read_thread, NULL,ttys1_read, NULL))
		printf("Can't create ttys1_read!\n");
	pthread_detach(ttys1_read_thread);
}

