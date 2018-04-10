#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include "slot.h"

Slot *slot;
pthread_mutex_t slot_lock;
int fd;
int max_line;

void * thread_simple(void *arg) {
	long i = (long)arg;
	int j = 0;
	for (j = 0; j < max_line; j++) {
		dprintf(fd, "thread_simple: %ld - %d\n", i, j);
	}
	return NULL;
}

void * thread_with_lock(void *arg) {
	long i = (long)arg;
	int j = 0;
	int buf_size = 0;
	char buf[64] = {0};
	for (j = 0; j < max_line; j++) {
		snprintf(buf, sizeof(buf), "thread_with_lock: %ld - %d\n", i, j);
		pthread_mutex_lock(&slot_lock);
		buf_size = strlen(buf);
		memcpy((char *)(slot + 1) + slot->join, buf, buf_size);
		slot->join += buf_size;
		if (slot->join > slot->threshold) {
			write(fd, (char *)(slot +1), slot->join);
			slot->join = 0;
		}
		pthread_mutex_unlock(&slot_lock);
	}
	return NULL;
}

void * thread_without_lock(void *arg) {
	long i = (long)arg;
	int j = 0;
	int buf_size = 0;
	char buf[64] = {0};
	for (j = 0; j < max_line; j++) {
		snprintf(buf, sizeof(buf), "thread_without_lock: %ld - %d\n", i, j);
		slot_write(slot, buf, strlen(buf));
	}
	return NULL;
}

void test_thread() {
	pthread_t tids[10];
	long i;
	void *tret;

	for (i = 0; i < 10; i++) {
		// pthread_create(&tids[i], NULL, thread_simple, (void *)i);
		// pthread_create(&tids[i], NULL, thread_with_lock, (void *)i);
		pthread_create(&tids[i], NULL, thread_without_lock, (void *)i);
	}

	for (i = 0; i < 10; i++) {
		pthread_join(tids[i], &tret);
	}
}

int main(int argc, char **argv) {
	fd = open("t.txt", O_CREAT | O_TRUNC | O_WRONLY, 0664);
	slot = slot_create();
	pthread_mutex_init(&slot_lock, NULL);

	max_line = atoi(argv[1]);
	time_t t1 = time(NULL);
	test_thread();
	time_t t2 = time(NULL);
	/*
	if (slot->join > 0)
		write(fd, (char *)(slot + 1), slot->join);
	*/
	if (slot->size > 0)
		write(fd, (char *)(slot + 1), slot->size);
	printf("test_thread: %ld\n", t2 - t1);
	return 0;
}
