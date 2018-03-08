#!/bin/sh

# Functions:
# compares outputs of reference program and our library
cmp_output()
{
    echo "Test $testnum: $1"
    echo "Comparison for test $testnum:"

    # put output files into variables
    REF_STDOUT=$(cat ref.stdout)
    REF_STDERR=$(cat ref.stderr)

    LIB_STDOUT=$(cat lib.stdout)
    LIB_STDERR=$(cat lib.stderr)

    # compare stdout
    if [ "$REF_STDOUT" != "$LIB_STDOUT" ]; then
        echo "Stdout outputs don't match..."
        diff -u ref.stdout lib.stdout
    else
        echo "Stdout outputs match!"
    fi

    # compare stderr
    if [ "$REF_STDERR" != "$LIB_STDERR" ]; then
        echo "Stderr outputs don't match..."
        diff -u ref.stderr lib.stderr
    else
        echo "Stderr outputs match!"
    fi

    # cleanup for next test
    testnum=$(($testnum+1))
    rm ref.stdout ref.stderr
    rm lib.stdout lib.stderr
}

# Start of script:

# make fresh virtual disk, separate ones for fs_ref.x and test_fs.x
./fs_make.x refdisk.fs 2048
./fs_make.x libdisk.fs 2048
# initialize variables
testnum=1

# Compare the info command

# get fs_info from reference lib
./fs_ref.x info refdisk.fs >ref.stdout 2>ref.stderr

# get fs_info from my lib
./test_fs.x info libdisk.fs >lib.stdout 2>lib.stderr

# compare fs_ref.x to test_fs.x
cmp_output info

# Compare the add command: first create a file of length 5000

num=0
while [ $num -lt 1000 ]
do
    echo "test" >> ln5000
    num=$(($num+1))
done

./fs_ref.x add refdisk.fs ln5000 >ref.stdout 2>ref.stderr
./test_fs.x add libdisk.fs ln5000 >lib.stdout 2>lib.stderr

# compare fs_ref.x to test_fs.x
cmp_output write5000

rm ln5000

#
# unfinished!
#

# clean
rm refdisk.fs libdisk.fs
