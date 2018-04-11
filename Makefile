main: main.o slot.o
	gcc -o $@ $^ -lpthread

clean:
	rm main *.o *.txt
