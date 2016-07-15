/*
 * Part of the android-ndk-profiler library.
 * Copyright (C) Richard Quirk
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "read_maps.h"

#ifdef ANDROID
#include <android/log.h>    /* for __android_log_print, ANDROID_LOG_INFO, etc */
#define LOGI(...)  __android_log_print(ANDROID_LOG_INFO, "PROFILING", __VA_ARGS__)
#else
#define LOGI(...)  do { printf(__VA_ARGS__) ; printf("\n"); } while (0)
#endif

static char s_line[256];
extern int opt_is_shared_lib;

void free_maps(struct proc_map *s)
{
	struct proc_map *next = s->next;
	while (next != NULL) {
		struct proc_map *tmp = next;
		next = next->next;
		free(tmp);
	}
	free(s);
}

struct proc_map *read_maps(FILE *fp, const char *lname)
{
	struct proc_map *results = NULL;
	struct proc_map *current = NULL;
	size_t namelen = strlen(lname);

	while (fgets(s_line, sizeof(s_line), fp) != NULL) {
		size_t len = strlen(s_line);
		len--;
		s_line[len] = 0;

		int dev_major = 0, dev_minor = 0, inode = 0;
		char perm[4] = { 0 };
		char path[512] = { 0 };
		int lo = 0, base = 0, hi = 0;
		sscanf(s_line, "%x-%x %4c %x %u:%u %u %s", &lo, &hi, perm, &base, &dev_major, &dev_minor, &inode, path);

		/* ignore anonymously mapped regions, ie, regions that don't have an inode defined. */
		if (inode == 0) { continue; }

		if (results == NULL) {
			current = malloc(sizeof(struct proc_map));
			if (!current) {
				LOGI("error allocating memory");
				return NULL;
			}
			current->next = NULL;
			results = current;
		} else {
			current->next = malloc(sizeof(struct proc_map));
			current = current->next;
			if (!current) {
				LOGI("error allocating memory");
				return NULL;
			}
			current->next = NULL;
		}

		LOGI("source '%s', base = 0x%x, lo = 0x%x, hi = 0x%x", path, base, lo, hi);

		current->base = base;
		current->lo = lo;
		current->hi = hi;

	}
	return results;
}

unsigned int get_real_address(const struct proc_map *maps,
			      unsigned int fake)
{
	const struct proc_map *mp = maps;
	while (mp) {
		if (fake >= mp->lo && fake <= mp->hi) {
			unsigned int real = fake - mp->lo;
			/* if real > fake, assume integer underflow and return fake instead. */
			return (real > fake) ? fake : real;
		}
		mp = mp->next;
	}
	return fake;
}
