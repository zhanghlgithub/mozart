#ifndef __MOZART_MODULE_LINEIN_H__
#define __MOZART_MODULE_LINEIN_H__

/**
 * @brief open linein
 *
 * @return linein dev fd.
 */
extern int  mozart_linein_open(void);

/**
 * @brief close linein
 * XXX: invoke mozart_linein_open()/mozart_linein_close() in the same process if cpu=M150
 */
extern void mozart_linein_close(void);

extern bool mozart_linein_is_in(void);

#endif	/* __MOZART_MODULE_LINEIN_H__ */
