#!/bin/bash

#NAME: Weikeng Yang
#EMAIL: weikengyang@gmail.com
#ID: 405346443

###
for index in 10 100 1000 10000 100000
do
    for threads in 2 4 8 10
    do
        ./lab2_add --thread=$threads --iterations=$index
    done
done


###
for threads in 2 4 8 12
do
    for index in 10 20 40 80 100 1000 10000 100000
    do
        ./lab2_add --thread=$threads --iterations=$index --yield
    done
done

###
for threads in 1 2 4 8 12
do
    
    ./lab2_add --iterations=1000 --threads=$threads --sync=s --yield
    ./lab2_add --iterations=10000 --threads=$threads --sync=m --yield
    ./lab2_add --iterations=10000 --threads=$threads --sync=c --yield
    
    ./lab2_add --thread=$threads --iterations=10000 --sync=m
    ./lab2_add --thread=$threads --iterations=10000 --sync=s
    ./lab2_add --thread=$threads --iterations=10000 --sync=c
done





