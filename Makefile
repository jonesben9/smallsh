CC=gcc --std=gnu99 -g
#CFLAGS := -std=c++11

smallsh: main.c 
	$(CC) $(CFLAGS) -o smallsh main.c 