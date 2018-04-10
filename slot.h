typedef struct Slot {
	int size;
	int join;
	int threshold;
	int state;
} Slot;


struct Slot * slot_create();
void slot_release(Slot **s);
void slot_write(Slot *s, const char *data, int size);
