CC = gcc
LDFLAGS = -lm

SENDER_SRC = RUDP_Sender.c
RECEIVER_SRC = RUDP_Receiver.c
RUDP_API_SRC = RUDP_API.c
COMMON_OBJ = $(RUDP_API_SRC:.c=.o)
SENDER_OBJ = $(SENDER_SRC:.c=.o)
RECEIVER_OBJ = $(RECEIVER_SRC:.c=.o)

all: RUDP_Sender RUDP_Receiver

RUDP_Sender: $(COMMON_OBJ) $(SENDER_OBJ)
	$(CC) -o RUDP_Sender $(COMMON_OBJ) $(SENDER_OBJ) $(LDFLAGS)

RUDP_Receiver: $(COMMON_OBJ) $(RECEIVER_OBJ)
	$(CC) -o RUDP_Receiver $(COMMON_OBJ) $(RECEIVER_OBJ) $(LDFLAGS)

%.o: %.c
	$(CC) -c $< -o $@

clean:
	rm -f RUDP_Sender RUDP_Receiver *.o
