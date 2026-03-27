/*
 * export.h:
 *
 * Non-interactive JSON/CSV export mode.
 */

#ifndef __EXPORT_H_ /* include guard */
#define __EXPORT_H_

void export_init(void);

void export_print(void);

void export_loop(void);

void export_finish(void);

#endif /* __EXPORT_H_ */
