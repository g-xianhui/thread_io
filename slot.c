#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "slot.h"

Slot * slot_create(int max_len, int threshold) {
	Slot *s = malloc(sizeof(Slot) + max_len);
	s->state = 0;
	s->threshold = threshold;
	s->fd = 0;
	return s;
}

void slot_destroy(Slot **s) {
	free(*s);
	*s = NULL;
}

void _dump_state(uint64_t state, const char *reason) {
	int join, release, close;
	join = JOINED(state);
	release = RELEASED(state);
	close = IS_CLOSED(state) ? 1 : 0;
	printf("[%lu] [%s] current state: %x: %d, %d, %d\n", pthread_self(), reason, state, join, release, close);
}

uint64_t _slot_join(uint64_t state, int size, int *offset) {
	int old_join, new_join;
	uint64_t new_state;

	old_join = JOINED(state);
	*offset = old_join;
	new_join = old_join + size;
	new_state = ((uint64_t)new_join << 32) | RELEASED(state);
	return new_state;
}

uint64_t _slot_release(uint64_t state, int size) {
	int new_release;
	uint64_t new_state;

	new_release = RELEASED(state) + size;
	new_state = (((uint64_t)state >> 32) << 32) | new_release;
	return new_state;
}

void slot_write(Slot *s, const char *buf, int size) {
	uint64_t old_state, new_state;
	int offset, old_join, new_join;
read_state:
	old_state = s->state;
	if (IS_CLOSED(old_state))
		goto read_state;

	new_state = _slot_join(old_state, size, &offset);
	if (JOINED(new_state) >= s->threshold)
		new_state = CLOSE(new_state);

	while (__sync_val_compare_and_swap(&s->state, old_state, new_state) != old_state) {
		goto read_state;
	}

	memcpy((char *)(s + 1) + offset, buf, size);

read_release:
	old_state = s->state;
	new_state = _slot_release(old_state, size);
	while (__sync_val_compare_and_swap(&s->state, old_state, new_state) != old_state) {
		goto read_release;
	}

	if (IS_CLOSED(new_state) && JOINED(new_state) == RELEASED(new_state)) {
		write(s->fd, (char *)(s + 1), JOINED(new_state));
		s->state = 0;
	}
}
