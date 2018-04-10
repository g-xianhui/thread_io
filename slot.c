#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "slot.h"

extern int fd;

Slot * slot_create() {
	Slot *s = malloc(sizeof(Slot) + 2048);
	s->size = 0;
	s->join = 0;
	s->state = 0;
	s->threshold = 256;
	return s;
}

void slot_release(Slot **s) {
	free(*s);
	*s = NULL;
}

void slot_write(Slot *s, const char *buf, int size) {
	int old_join, new_join, new_size;
read_join:
	if (s->size > s->threshold) {
		goto read_join;
	}

	old_join = s->join;
	new_join = old_join + size;
	while (__sync_val_compare_and_swap(&s->join, old_join, new_join) != old_join) {
		goto read_join;
	}

	new_size = __sync_add_and_fetch(&s->size, size);
	if (new_size > s->threshold) {
		s->state = 1;
	}

	memcpy((char *)(s + 1) + new_join, buf, size);

read_release:
	old_join = s->join;
	new_join = old_join - size;
	while (__sync_val_compare_and_swap(&s->join, old_join, new_join) != old_join) {
		goto read_release;
	}

	if (s->state == 1 && new_join == 0) {
		write(fd, (char *)(s +1), s->size);
		s->join = 0;
		s->size = 0;
		s->state = 0;
	}
}
