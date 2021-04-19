# NAME: Weikeng Yang
# EMAIL: weikengyang@gmail.com
# ID: 405346443

default:
	gcc -Wall -Wextra -o lab4b -lm -lmraa lab4b.c 

lab4b: lab4b.c
	./compile.sh

build: default
	tar -czvf lab4b-405346443.tar.gz Makefile README lab4b.c smoketest.sh *.h

grade:
	./P4B_check.sh 405346443

clean:
	-@rm -f lab4b
	-@rm -f lab4b-405346443.tar.gz
	-@rm -f *.dSYM
	-@rm -f dummy

check:
	chmod u+x smoketest.sh
	./smoketest.sh

dist: clean build check
