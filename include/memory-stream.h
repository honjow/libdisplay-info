#ifndef MEMORY_STREAM_H
#define MEMORY_STREAM_H

/**
 * Utility functions for memory streams.
 */

#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>

struct memory_stream {
	FILE *fp;
	char *str;
	size_t str_len;
};

bool
memory_stream_open(struct memory_stream *m);

char *
memory_stream_close(struct memory_stream *m);

#endif
