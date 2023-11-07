#Makefile of OS project for checkboard game
all: start_game test clean

CC = gcc #compiler

CFLAGS = -std=c89 -pedantic #flags to compile in c89

TARGET = game

test: master.o chessboard.o common.o flag_list.o message_queue.o pawn.o pawn_list.o player.o player_list.o goal_list.o

# Should be equivalent to your list of C files, if you don't build selectively
# '$': identify a variable, with C_FILES variable we hold the names of all .c files in our project, use wildcard because we have a variable
C_FILES =	$(wildcard *.c)

#-lm to use math.h  #-g to use debugger
start_game:	$(C_FILES)
	$(CC) -o $@ $^ $(CFLAGS) -g -o $(TARGET) -lm

run_test:	$(TARGET)
			./$(TARGET) config_files/test_config.txt

run_hard:	$(TARGET)
			./$(TARGET)	config_files/hard.txt

run_easy:	$(TARGET)
			./$(TARGET)	config_files/easy.txt	

run_test_1_flag:	$(TARGET)
			./$(TARGET)	config_files/test_1_flag.txt
clean:	
	rm	-f	*.o  *.h.gch


