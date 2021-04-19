#!/bin/bash
if [ `uname –a` has string beaglebone in it ]
	gcc –o lab4b lab4b.c …
else
	gcc –o lab4b –DDUMMY lab4b.c 
endif 

