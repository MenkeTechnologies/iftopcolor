/*
 * ui.h:
 *
 *
 */

#ifndef __UI_H_ /* include guard */
#define __UI_H_


#define HOSTNAME_LENGTH 256

void ui_print(void);

void ui_loop(void);

void ui_finish(void);

void ui_tick(int);

void sprint_host(char *line, int af, struct in6_addr *addr, unsigned int port, unsigned int protocol, int buflen);

void analyse_data(void);

void ui_init(void);

#endif /* __UI_H_ */
