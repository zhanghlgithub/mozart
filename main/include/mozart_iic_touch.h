#ifndef __MOZART_IIC_TOUCH_H__
#define __MOZART_IIC_TOUCH_H__
#include <stdbool.h>
extern void start_iic(void);
extern int iic_write(int fd, unsigned char send_buff, unsigned char reg_addr );
#endif /* __MOZART_IIC_TOUCH_H__ */
