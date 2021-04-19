#!/bin/bash
# NAME: Weikeng Yang
# EMAIL: weikengyang@gmail.com
# ID: 405346443

#!/bin/bash

{ echo "START"; sleep 2; echo "STOP"; sleep 2; echo "OFF"; } | ./lab4b --log=log.txt
echo "...checking arguments"
./lab4b --bogus < /dev/tty > /dev/null 2>STDERR
if [ $? -ne 0 ]
then
    echo "Test Failed! Program should exit probably with 0. "
else
    echo "Test succeed! "
fi

if [ $? -ne 1 ]
then
    echo "Test Failed! The detecion failed. "
else
    echo "Test succeed! "
fi

echo "Testing Arguments to be passed in"
./lab4b --period=2 --scale=F --log=$logfile <<-EOF
SCALE=F
SCALE=C
PERIOD=3
STOP
START
LOG THIS IS A TEST
OFF
EOF

for c in START STOP OFF SHUTDOWN
    do
        grep "$c" log.txt > /dev/null
        if [ $? -ne 0 ]
        then
            echo "ERROR! Cannot log $c command"
        else
            echo "$c was logged successfully!"
        fi
    done

rm -f $logfile
