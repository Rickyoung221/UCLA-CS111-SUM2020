# NAME: Weikeng Yang
# EMAIL: weikengyang@gmail.com
# ID: 405346443

default: build

build: 
	gcc lab2_add.c -o lab2_add -g -Wall -Wextra -pthread
	gcc -Wall -Wextra -g -o lab2_list -lpthread lab2_list.c SortedList.c


check:
	 ./P2A_check.sh 405346443


tests: build
	rm -f lab2_add.csv lab2_list.csv 
	-./test_add.sh > lab2_add.csv
	-./test_list.sh > lab2_list.csv

graphs: tests
	./lab2_add.gp
	./lab2_list.gp 
	

clean:
	-@rm -f lab2_add
	-@rm -f lab2_list
	-@rm -rf lab2_add.dSYM
	-@rm -rf lab2_list.dSYM
	-@rm -f lab2a-405346443.tar.gz

dist: clean build
	tar -cvzf lab2a-405346443.tar.gz README Makefile lab2_add.c lab2_list.c SortedList.h SortedList.c lab2_add.csv lab2_list.csv lab2_add.gp lab2_list.gp lab2_add-1.png lab2_add-2.png lab2_add-3.png lab2_add-4.png lab2_add-5.png lab2_list-1.png lab2_list-2.png lab2_list-3.png lab2_list-4.png test_add.sh test_list.sh
