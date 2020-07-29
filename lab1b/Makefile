# NAME: Weikeng Yang
# EMAIL: weikengyang@gmail.com
# ID: 405346443

default: build

build:
	gcc -Wall -Wextra -lz -o lab1b-client lab1b-client.c
	gcc -Wall -Wextra -lz -o lab1b-server lab1b-server.c


check:
        ./P1B_check.sh 405346443

client:
	gcc -Wall -Wextra -lz -o lab1b-client lab1b-client.c

server: 
	gcc -Wall -Wextra -lz -o lab1b-server lab1b-server.c

clean:
	-@rm -f lab1b-client
	-@rm -f lab1b-server
	-@rm -f lab1b-405346443.tar.gz
	-@rm -rf lab1b-client.dSYM
	-@rm -rf lab1b-server.dSYM

dist: clean build
	tar -cvzf lab1b-405346443.tar.gz lab1b-server.c lab1b-client.c README Makefile
