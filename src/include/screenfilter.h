/*
 * screenfilter.h:
 *
 * Copyright (c) 2026 Jacob Menke
 */

#ifndef __SCREENFILTER_H_ /* include guard */
#define __SCREENFILTER_H_

#include "conf.h"

#ifdef HAVE_REGCOMP

int screen_filter_set(char *s);

int screen_filter_match(char *s);

#endif

#endif /* __SCREENFILTER_H_ */
