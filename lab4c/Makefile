# NAME: Weikeng Yang
# EMAIL: weikengyang@gmail.com
# ID: 405346443

default:
	gcc -o lab4c_tcp -Wall -Wextra -lm -lmraa -g lab4c_tcp.c
	gcc -o lab4c_tls -Wall -Wextra -lm -lmraa -g -lssl -lcrypto lab4c_tls.c



build: default
	tar -czvf lab4c-405346443.tar.gz Makefile README *.c


clean:
	-@rm -f lab4c
	-@rm -f lab4c-405346443.tar.gz
	-@rm -f *.dSYM
	-@rm -f dummy
	rm -f lab4c_tcp lab4c_tls

check:
	./P4C_check.sh 405346443

dist: clean build 
