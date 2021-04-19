#!/bin/bash

# NAME: Weikeng Yang
# EMAIL: weikengyang@gmail.com
# UID: 405346443


##mutex
for thread in 1 2 4 8 12 16 24
do
    ./lab2_list --iterations=1000 --threads=$thread --sync=m >> lab2b_list.csv
done
##spin-lock
for thread in 1 2 4 8 12 16 24
do
    ./lab2_list --iterations=1000 --threads=$thread --sync=s >> lab2b_list.csv
done

##
for index in 1 2 4 8 16
do
    for thread in 1 4 8 12 16
    do
        ./lab2_list --iterations=$index --threads=$thread --yield=id --lists=4 >> lab2b_list.csv
    done
done

##
for index in 10 20 40 80
do
    for thread in 1 4 8 12 16
    do
        ./lab2_list --iterations=$index --threads=$thread --sync=s --yield=id >> lab2b_list.csv --lists=4
    done
done

for index in 10 20 40 80
do
    for thread in 1 2 4 8 12 16
    do
        ./lab2_list --iterations=$index --threads=$thread --sync=m --yield=id >> lab2b_list.csv --lists=4
    done
done

##

for thread in 1 2 4 8 12
do
    for list in 1 4 8 16
    do
        ./lab2_list --iterations=1000 --threads=$thread --sync=s --lists=$list >> lab2b_list.csv
    done
done

for thread in 1 2 4 8 12
do
    for list in 1 4 8 16
    do
        ./lab2_list --iterations=1000 --threads=$thread --sync=m --lists=$list >> lab2b_list.csv
    done
done
