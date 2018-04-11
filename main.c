#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include "slot.h"

typedef struct Buf {
	pthread_mutex_t lock;
	int join;
	int threshold;
	int fd;
} Buf;

typedef struct Pair {
	void *a;
	void *b;
} Pair;

int max_line;

void * thread_simple(void *arg) {
	Pair *pair = (Pair *)arg;
	long i = (long)pair->a;
	int fd = (int)(long)pair->b;
	int j = 0;

	for (j = 0; j < max_line; j++) {
		dprintf(fd, "thread_simple: %ld - %d\n", i, j);
	}
	return NULL;
}

void * thread_with_lock(void *arg) {
	Pair *pair = (Pair *)arg;
	long i = (long)pair->a;
	Buf *slot = (Buf *)pair->b;
	int j = 0;
	int buf_size = 0;
	char buf[64] = {0};

	for (j = 0; j < max_line; j++) {
		snprintf(buf, sizeof(buf), "thread_with_lock: %ld - %d\n", i, j);
		pthread_mutex_lock(&slot->lock);
		buf_size = strlen(buf);
		memcpy((char *)(slot + 1) + slot->join, buf, buf_size);
		slot->join += buf_size;
		if (slot->join > slot->threshold) {
			write(slot->fd, (char *)(slot +1), slot->join);
			slot->join = 0;
		}
		pthread_mutex_unlock(&slot->lock);
	}

	return NULL;
}

void * thread_without_lock(void *arg) {
	Pair *pair = (Pair *)arg;
	long i = (long)pair->a;
	Slot *slot = (Slot *)pair->b;
	int j = 0;
	char buf[64] = {0};
	int ret;

	for (j = 0; j < max_line; j++) {
		snprintf(buf, sizeof(buf), "thread_without_lock: %ld - %d\n", i, j);
		slot_write(slot, buf, strlen(buf));
	}

	return NULL;
}

void test_thread(int max_thread) {
	pthread_t tids[max_thread];
	long i;
	void *tret;
	time_t t1, t2;
	Pair *pair;

	/*
	 * test simple thread
	 */
	t1 = time(NULL);

	int fd = open("c.txt", O_CREAT | O_TRUNC | O_WRONLY, 0664);

	for (i = 0; i < max_thread; i++) {
		pair = malloc(sizeof(Pair));
		pair->a = (void *)i;
		pair->b = (void *)(long)fd;

		pthread_create(&tids[i], NULL, thread_simple, (void *)pair);
	}

	for (i = 0; i < max_thread; i++) {
		pthread_join(tids[i], &tret);
	}

	t2 = time(NULL);
	printf("test_thread_simple: %ld\n", t2 - t1);

	/*
	 * test lock thread
	 */
	t1 = time(NULL);

	Buf *buf = malloc(sizeof(Buf) + 2048);
	buf->join = 0;
	buf->threshold = 512;
	pthread_mutex_init(&buf->lock, NULL);
	buf->fd = open("b.txt", O_CREAT | O_TRUNC | O_WRONLY, 0664);

	for (i = 0; i < max_thread; i++) {
		pair = malloc(sizeof(Pair));
		pair->a = (void *)i;
		pair->b = buf;

		pthread_create(&tids[i], NULL, thread_with_lock, (void *)pair);
	}

	for (i = 0; i < max_thread; i++) {
		pthread_join(tids[i], &tret);
	}

	if (buf->join > 0)
		write(buf->fd, (char *)(buf + 1), buf->join);

	t2 = time(NULL);
	printf("test_thread_with_lock: %ld\n", t2 - t1);

	/*
	 * test lock free thread
	 */
	t1 = time(NULL);

	Slot *slot = slot_create(2048, 512);
	slot->fd = open("a.txt", O_CREAT | O_TRUNC | O_WRONLY, 0664);

	for (i = 0; i < max_thread; i++) {
		pair = malloc(sizeof(Pair));
		pair->a = (void *)i;
		pair->b = slot;

		pthread_create(&tids[i], NULL, thread_without_lock, (void *)pair);
	}

	for (i = 0; i < max_thread; i++) {
		pthread_join(tids[i], &tret);
	}

	if (JOINED(slot->state) > 0)
		write(slot->fd, (char *)(slot + 1), JOINED(slot->state));

	t2 = time(NULL);
	printf("test_thread_without_lock: %ld\n", t2 - t1);
}

int main(int argc, char **argv) {
	max_line = atoi(argv[1]);
	int max_thread = atoi(argv[2]);
	test_thread(max_thread);
	return 0;
}
