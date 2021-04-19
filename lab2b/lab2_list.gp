#! /usr/bin/gnuplot
#	
# NAME: Weikeng Yang
# EMAIL: weikengyang@gmail.com
# ID: 405346443
#
# input: lab2_list.csv
#	1. test name
#	2. # threads
#	3. # iterations per thread
#	4. # lists
#	5. # operations performed (threads x iterations x (ins + lookup + delete))
#	6. run time (ns)
#	7. run time per operation (ns)
#

# general plot parameters
set terminal png
set datafile separator ","

##### lab2b_1.png ########
set title "1: Throughput vs. Number of Threads (1000 interations)"
set xlabel "Number of Threads"
set logscale x 2
set xrange [0.75:]
set ylabel "Throughput (operations per second)"
set logscale y 10
set output 'lab2b_1.png'

# non-yield resul, single thread, unprotected
plot \
     "< grep 'list-none-s,[0-9]*,1000,1,' lab2b_list.csv" using ($2):(1000000000/($7)) \
    title 'Spin-lock' with linespoints lc rgb 'green', \
     "< grep 'list-none-m,[0-9]*,1000,1,' lab2b_list.csv" using ($2):(1000000000/($7)) \
    title 'Mutex' with linespoints lc rgb 'red'
    
##### lab2b_2.png ########
set title "2: Average Waiting Time for Lock vs. Number of Threads"
set xlabel "Number of Threads"
set logscale x 2
set xrange [0.75:]
set ylabel "Average Time (ns)"
set logscale y 10
set output 'lab2b_2.png'

plot \
     "< grep 'list-none-m,[0-9]*,1000,1,' lab2b_list.csv" using ($2):($7) \
    title 'Operation time' with linespoints lc rgb 'red', \
     "< grep 'list-none-m,[0-9]*,1000,1,' lab2b_list.csv" using ($2):($8) \
    title 'Waiting for lock time' with linespoints lc rgb 'green'
    
    
##### lab2b_3.png #######
set title "3: Protected and unprotected iterations that run without failure"
set xlabel "Threads"
set ylabel "Successful Iterations"
set logscale x 2
set xrange [0.75:]
set yrange [0.75:]
set logscale y 10
set output 'lab2b_3.png'

plot \
    "< grep 'list-id-none,[0-9]*,[0-9]*,4,' lab2b_list.csv" using ($2):($3) \
    title 'Unprotected' with points lc rgb 'blue', \
    "< grep 'list-id-s,[0-9]*,[0-9]*,4,' lab2b_list.csv" using ($2):($3) \
    title 'Spin-Lock' with points lc rgb 'red', \
    "< grep 'list-id-m,[0-9]*,[0-9]*,4,' lab2b_list.csv" using ($2):($3) \
    title 'Mutex' with points lc rgb 'green'

    
##### lab2b_4.png ############
set title "4: Throughput vs. Number of Threads with Mutex"
set xlabel "Number of Threads"
set logscale x 2
set xrange [0.75:]
set ylabel "Throughput (operations per second)"
set logscale y 10
set output 'lab2b_4.png'


plot \
     "< grep 'list-none-m,[0-9]*,1000,1,' lab2b_list.csv" using ($2):(1000000000/($7)) \
    title '1 list' with linespoints lc rgb 'blue', \
     "< grep 'list-none-m,[0-9]*,1000,4,' lab2b_list.csv" using ($2):(1000000000/($7)) \
    title '4 lists' with linespoints lc rgb 'red', \
     "< grep 'list-none-m,[0-9]*,1000,8,' lab2b_list.csv" using ($2):(1000000000/($7)) \
    title '8 lists' with linespoints lc rgb 'green', \
     "< grep 'list-none-m,[0-9]*,1000,16,' lab2b_list.csv" using ($2):(1000000000/($7)) \
    title '16 lists' with linespoints lc rgb 'orange'
    
    
##### lab2b_5.png ###########
set title "5: Throughput vs. Number of Threads with Spin-Lock   "
set xlabel "Number of Threads"
set logscale x 2
set logscale y 10
set xrange [0.75:]
set ylabel "Aggregated Throughput (operations per second)"
set output 'lab2b_5.png'


plot \
     "< grep 'list-none-s,[0-9]*,1000,1,' lab2b_list.csv" using ($2):(1000000000/($7)) \
    title '1 list' with linespoints lc rgb 'blue', \
     "< grep 'list-none-s,[0-9]*,1000,4,' lab2b_list.csv" using ($2):(1000000000/($7)) \
    title '4 lists' with linespoints lc rgb 'red', \
     "< grep 'list-none-s,[0-9]*,1000,8,' lab2b_list.csv" using ($2):(1000000000/($7)) \
    title '8 lists' with linespoints lc rgb 'green', \
     "< grep 'list-none-s,[0-9]*,1000,16,' lab2b_list.csv" using ($2):(1000000000/($7)) \
    title '16 lists' with linespoints lc rgb 'orange'
