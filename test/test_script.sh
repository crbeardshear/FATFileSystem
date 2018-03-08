#!/bin/sh

# Functions:
# compares outputs of reference program and our library
cmp_output()
{
    echo ""
    echo "Test $testnum: $1 $2 $3 $4"
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
	echo "REF: $REF_STDOUT"
	echo "LIB: $LIB_STDOUT"
    else
        echo "Stdout outputs match!"
    fi

    # compare stderr
    if [ "$REF_STDERR" != "$LIB_STDERR" ]; then
        echo "Stderr outputs don't match..."
        diff -u ref.stderr lib.stderr
	echo "REF: $REF_STDERR"
	echo "LIB: $LIB_STDERR"
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
./fs_make.x refdisk.fs 50
./fs_make.x libdisk.fs 50
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
    echo "test" >> test1
    num=$(($num+1))
done

./fs_ref.x add refdisk.fs test1 >ref.stdout 2>ref.stderr
./test_fs.x add libdisk.fs test1 >lib.stdout 2>lib.stderr
cmp_output write test1 5,000 bytes

rm test1

# add file which doesn't exist
./fs_ref.x add refdisk.fs test10 >ref.stdout 2>ref.stderr
./test_fs.x add libdisk.fs test10 >lib.stdout 2>lib.stderr
cmp_output write nonexistent test10

# add test2: length 50000
num=0
while [ $num -lt 10000 ]
do
    echo "test" >> test2
    num=$(($num+1))
done

./fs_ref.x add refdisk.fs test2 >ref.stdout 2>ref.stderr
./test_fs.x add libdisk.fs test2 >lib.stdout 2>lib.stderr

# compare fs_ref.x to test_fs.x
cmp_output write test2 50,000 bytes

rm test2

# ls the file system, should have the two files we just wrote
./fs_ref.x ls refdisk.fs >ref.stdout 2>ref.stderr
./test_fs.x ls libdisk.fs >lib.stdout 2>lib.stderr
cmp_output ls

# read the test2 we wrote
./fs_ref.x cat refdisk.fs test2 >ref.stdout 2>ref.stderr
./test_fs.x cat libdisk.fs test2 >lib.stdout 2>lib.stderr
cmp_output cat test2

# stat the test2
./fs_ref.x stat refdisk.fs test2 >ref.stdout 2>ref.stderr
./test_fs.x stat libdisk.fs test2 >lib.stdout 2>lib.stderr
cmp_output stat test2

# info after the two files were added
./fs_ref.x info refdisk.fs >ref.stdout 2>ref.stderr
./test_fs.x info libdisk.fs >lib.stdout 2>lib.stderr
cmp_output info after adds

# add file over size of the file system (it must be truncated to fit)
# the string is 23 characters (with end of line char)
# times 10000 is 230,000 bytes
# file system is 50 blocks * 4096 = 204,800 bytes
num=0
while [ $num -lt 10000 ]
do
    echo "test number 3 asdf asdf" >> test3
    num=$(($num+1))
done

./fs_ref.x add refdisk.fs test3 >ref.stdout 2>ref.stderr
./test_fs.x add libdisk.fs test3 >lib.stdout 2>lib.stderr
cmp_output write test3 230,000 bytes
rm test3

# stat the test3
./fs_ref.x stat refdisk.fs test3 >ref.stdout 2>ref.stderr
./test_fs.x stat libdisk.fs test3 >lib.stdout 2>lib.stderr
cmp_output stat test3

# read the truncated test3 we wrote
./fs_ref.x cat refdisk.fs test3 >ref.stdout 2>ref.stderr
./test_fs.x cat libdisk.fs test3 >lib.stdout 2>lib.stderr
cmp_output cat test3

# add file with conflicting name
echo "test" > test1
./fs_ref.x add refdisk.fs test1 >ref.stdout 2>ref.stderr
./test_fs.x add libdisk.fs test1 >lib.stdout 2>lib.stderr
cmp_output add test1 conflict

rm test1

# check if the file has the correct value after attempting to write conflicting
# names
./fs_ref.x cat refdisk.fs test1 >ref.stdout 2>ref.stderr
./test_fs.x cat libdisk.fs test1 >lib.stdout 2>lib.stderr
cmp_output correct read after conflict

# remove one of the files (test1)
./fs_ref.x rm refdisk.fs test1 >ref.stdout 2>ref.stderr
./test_fs.x rm libdisk.fs test1 >lib.stdout 2>lib.stderr
cmp_output rm test1

# ls after the removal to see if the file was removed correctly
./fs_ref.x ls refdisk.fs >ref.stdout 2>ref.stderr
./test_fs.x ls libdisk.fs >lib.stdout 2>lib.stderr
cmp_output ls after rm

# info after conflict
./fs_ref.x info refdisk.fs >ref.stdout 2>ref.stderr
./test_fs.x info libdisk.fs >lib.stdout 2>lib.stderr
cmp_output info after conflict

# remove one of the files (test2)
./fs_ref.x rm refdisk.fs test2 >ref.stdout 2>ref.stderr
./test_fs.x rm libdisk.fs test2 >lib.stdout 2>lib.stderr
cmp_output rm test2

# remove one of the files (test3)
./fs_ref.x rm refdisk.fs test3 >ref.stdout 2>ref.stderr
./test_fs.x rm libdisk.fs test3 >lib.stdout 2>lib.stderr
cmp_output rm test3

# remove last of the files (test4)
./fs_ref.x rm refdisk.fs test4 >ref.stdout 2>ref.stderr
./test_fs.x rm libdisk.fs test4 >lib.stdout 2>lib.stderr
cmp_output rm test4

# Write two files of size 96,000 bytes
num=0
while [ $num -lt 2999 ]
do
    echo "test six, 96,000 bytes aaaaaaaa" >> test6
    num=$(($num+1))
done

./fs_ref.x info refdisk.fs
./test_fs.x info libdisk.fs

./fs_ref.x add refdisk.fs test6 >ref.stdout 2>ref.stderr
./test_fs.x add libdisk.fs test6 >lib.stdout 2>lib.stderr
cmp_output write test6 96,000 bytes

cat test6 > test7

./fs_ref.x add refdisk.fs test7 >ref.stdout 2>ref.stderr
./test_fs.x add libdisk.fs test7 >lib.stdout 2>lib.stderr
cmp_output write test7 96,000 bytes

echo "adsdf" > test8
cat test8 > test9

./fs_ref.x add refdisk.fs test8 >ref.stdout 2>ref.stderr
./test_fs.x add libdisk.fs test8 >lib.stdout 2>lib.stderr
cmp_output write test8 6 bytes

./fs_ref.x info refdisk.fs
./test_fs.x info libdisk.fs

./fs_ref.x add refdisk.fs test9 >ref.stdout 2>ref.stderr
./test_fs.x add libdisk.fs test9 >lib.stdout 2>lib.stderr
cmp_output write test9 6 bytes

rm test6 test7 test8 test9





# clean
rm refdisk.fs libdisk.fs