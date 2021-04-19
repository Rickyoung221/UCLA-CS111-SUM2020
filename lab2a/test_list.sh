#!/bin/bash

#NAME: Weikeng Yang
#EMAIL: weikengyang@gmail.com
#ID: 405346443


for index in 10 100 1000 10000 20000
do
    ./lab2_list --iterations=$index --threads=1
done


for index in 1 10 100 1000
do
    for threads in 2 4 8 12
    do
        ./lab2_list --iterations=$index --thread=$threads
    done
done



for index in 1 2 4 8 16 32
do
    for threads in 2 4 8 12
    do
        ./lab2_list --iterations=$index --threads=$threads --yield=i
        ./lab2_list --iterations=$index --threads=$threads --yield=il
        ./lab2_list --iterations=$index --threads=$threads --yield=dl
        ./lab2_list --iterations=$index --threads=$threads --yield=l
        ./lab2_list --iterations=$index --threads=$threads --yield=d
        ./lab2_list --iterations=$index --threads=$threads --yield=id
        ./lab2_list --iterations=$index --threads=$threads --yield=idl
    done
done



for threads in 1 2 4 8 12 16 24
do
    ./lab2_list --sync=s --iterations=$1000 --threads=$threads
    ./lab2_list --sync=m --iterations=$1000 --threads=$threads
done


for index in 1 2 4 8 16 32
do
    for threads in 2 4 8 12
    do
    ./lab2_list --iterations=$index --threads=$threads --sync=m --yield=i
    ./lab2_list --iterations=$index --threads=$threads --sync=m --yield=d
    ./lab2_list --iterations=$index --threads=$threads --sync=m --yield=il
    ./lab2_list --iterations=$index --threads=$threads --sync=m --yield=dl
    ./lab2_list --iterations=$index --threads=$threads --sync=m --yield=idl
    ./lab2_list --iterations=$index --threads=$threads --sync=m --yield=l

    ./lab2_list --iterations=$index --threads=$threads --sync=s --yield=i
    ./lab2_list --iterations=$index --threads=$threads --sync=s --yield=d
    ./lab2_list --iterations=$index --threads=$threads --sync=s --yield=il
    ./lab2_list --iterations=$index --threads=$threads --sync=s --yield=dl
    ./lab2_list --iterations=$index --threads=$threads --sync=s --yield=idl
    ./lab2_list --iterations=$index --threads=$threads --sync=s --yield=l
    done
done
