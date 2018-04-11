#include <stdint.h>

typedef struct Slot {
	int fd;
	int threshold;
	uint64_t state;
} Slot;

struct Slot * slot_create();
void slot_destroy(Slot **s);
void slot_write(Slot *s, const char *data, int size);

#define RELEASED(state) ((int32_t)(state))
#define JOINED(state) (((state)>>32) & 0x7FFFFFFF)
#define CLOSE(state) ((state) | (1UL << 63))
#define IS_CLOSED(state) (((state)>>32) & 0x80000000)

