build:chats chatc

	gcc chats.c -o chats
	gcc chatc.c -o chatc
    
clean:

	-rm -f chats chatc
