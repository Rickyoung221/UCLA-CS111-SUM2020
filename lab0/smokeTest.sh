#!/bin/bash
#
# Name:    Weikeng Yang
# Email: weikengyang@gmail.com
# ID:    405346443

#1
touch input.txt
# Compare input and output files
echo "if copied" > input.txt
./lab0 --input_file="input.txt" --output_file="output.txt"
cmp "input.txt" "output.txt"
if [ $? -ne 0 ]; then
    echo "Test Fail. Both files do not match"
fi
rm -f input.txt output.txt

#check segfault and catching creation
./lab0 --segfault --catch
if [ $? -ne 4 ]; then
    echo "Test Fail: Did not create or catch segfault"
fi


# check if the input file is invalid
./lab0 --input=file_not_exist.txt
if [ $? -ne 2 ]; then
    echo "Invalid input Test Failed."
fi
rm -f file_not_exist.txt


# check if the output file is invalid
touch invalid_output.txt
chmod -w invalid_output.txt
./lab0 --input=input.txt --output=invalid_output.txt
if [ $? -ne 3 ]; then
    echo "Invalid output Test Failed."
fi;
rm -f invalid_output.txt


# Check invalid argument exit code
./lab0 --bogus
if [ $? -ne 1 ]; then
    echo "Invalid arguemnt test failed"
fi


