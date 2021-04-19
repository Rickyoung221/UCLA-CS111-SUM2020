# NAME: Weikeng Yang
# EMAIL: weikengyang@gmail.com
# ID: 405346443

default: build

build:
	gcc -Wall -Wextra -g -o lab2_list -lpthread lab2_list.c SortedList.c

tests: default
	rm -f *.csv
	-./lab2b_test.sh 2>>/dev/null

profile:
	rm -f ./raw.gperf profile.out
	LD_PRELOAD=/w/home.25/cs/ugrad/weikeng/lab2b/libprofiler.so.0 \
	CPUPROFILE=./raw.gperf ./lab2_list --iterations=1000 --threads=12 --sync=s
	pprof --text ./lab2_list ./raw.gperf >> profile.out
	pprof --list=thread_routine ./lab2_list ./raw.gperf >> profile.out
	rm -f ./raw.gperf
	
graphs: tests
	rm -f *.png
	./lab2_list.gp

clean:
	rm -f lab2_list
	rm -f lab2b-405346443.tar.gz

dist: graphs clean
	tar -cvzf lab2b-405346443.tar.gz README Makefile lab2_list.c SortedList.h SortedList.c *.csv *.gp lab2b_test.sh profile.out *.png

