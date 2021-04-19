# NAME: Weikeng Yang
# EMAIL: weikengyang@gmail.com
# ID: 405346443

default: build

build:
	gcc lab1a.c -o lab1a -g -Wall -Wextra


check:
	./P1A_check.sh 405436443

clean:
	-@rm -f lab1a
	-@rm -f lab1a-405346443.tar.gz
	-@rm -rf lab1a.dSYM

dist: clean build
	tar -cvzf lab1a-405346443.tar.gz lab1a.c Makefile README
