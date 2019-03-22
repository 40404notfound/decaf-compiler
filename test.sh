#!/bin/bash
#for i in $(ls tests/*.decaf); do
#	y=${i%.decaf}
#	f=${y##*/}
#	echo $f
#	./dcc < $i
#done
$(hostname | grep cane)
if [ $? -eq 0 ]
then
    echo @@testing on samples/\*
    for i in $(ls samples/*.decaf | grep -v 't5\|badlink\|black\|fib\|sort'); do
        echo @@@ $i @@@
        y=${i%.decaf}
        f=${y##*/}
        ./run samples/$f.decaf | tail -n +4 &> out/$f.my.out
        diff -w out/$f.my.out samples/$f.out
    done
exit 0
fi


echo @@testing on samples/\*
for i in $(ls samples/*.decaf | grep -v 't5\|badlink\|black\|fib\|sort'); do
    echo @@@ $i @@@
    y=${i%.decaf}
    f=${y##*/}
    # ./solution/dcccaenupdate < $i 2> samples/$f.out
    ./dcc < $i > out/$f.my.s
    # spim -f out/$f.my.s < samples/$f.in | tail -n +6 > out/$f.my.out
    spim -f out/$f.my.s | tail -n +6 > out/$f.my.out
    cat samples/$f.out | tail -n +2 > out/$f.cor.out
    diff -w out/$f.my.out out/$f.cor.out
done

for i in $(ls samples/*.decaf | grep 't5\|black\|fib\|sort'); do
    echo @@@ $i @@@
    y=${i%.decaf}
    f=${y##*/}
    # ./solution/dcccaenupdate < $i 2> samples/$f.out
    ./dcc < $i > out/$f.my.s
    # spim -f out/$f.my.s < samples/$f.in | tail -n +6 > out/$f.my.out
    spim -f out/$f.my.s < samples/$f.in | tail -n +6 > out/$f.my.out
    cat samples/$f.out | tail -n +2 > out/$f.cor.out
    diff -w out/$f.my.out out/$f.cor.out
done


echo @@testing on tests/\*
for i in $(ls tests/*.decaf); do
    echo @@@ $i @@@
    y=${i%.decaf}
    f=${y##*/}
    # ./solution/dcccaenupdate < $i 2> samples/$f.out
    ./dcc < $i > out/$f.my.s
    ./solution/dcc-caen < $i > out/$f.cor.s
    # spim -f out/$f.my.s < samples/$f.in | tail -n +6 > out/$f.my.out
    # spim -f out/$f.my.s < samples/$f.in | tail -n +6 > out/$f.my.out
    # cat samples/$f.out | tail -n +2 > out/$f.cor.out
    diff -w out/$f.my.s out/$f.cor.s
done
# for i in $(ls samples/*.decaf | grep 'badlink\'); do
#     echo @@@ $i @@@
#     y=${i%.decaf}
#     f=${y##*/}
#     # ./solution/dcccaenupdate < $i 2> samples/$f.out
#     ./dcc < $i &> out/$f.my.out
#     # spim -f out/$f.my.s < samples/$f.in | tail -n +6 > out/$f.my.out
#     # spim -f out/$f.my.s < samples/$f.in | tail -n +6 > out/$f.my.out
#     # cat samples/$f.out | tail -n +2 > out/$f.cor.out
#     diff -w out/$f.my.out samples/$f.out
# done
# echo @@tesing on tests/\*

# for i in $(ls tests/*.decaf); do
#     echo @@@ $i @@@
#     y=${i%.decaf}
#     f=${y##*/}
#     ./solution/dcccaenupdate < $i 2> tests/$f.out
#     ./dcc < $i 2> tests/$f.my
#     diff -w tests/$f.my tests/$f.out
# done

# for i in $(ls tests_2/*.decaf); do
#     echo @@@ $i @@@
#     y=${i%.decaf}
#     f=${y##*/}
#     ./solution/dcccaenupdate < $i 2> tests/$f.out
#     ./dcc < $i 2> tests/$f.my
#     diff -w tests/$f.my tests/$f.out
# done

exit 0


# TEST_DIR=tests
# # echo === generate correct files ===
# cd $TEST_DIR
# for i in $(ls *.in); do
# 	y=${i%.in}
# 	f=${y##*/}
# 	./dcc < $i &> $f.sol
# done
# # echo === done ===

# cd ..
# for i in $(ls $TEST_DIR/*.in); do
# 	y=${i%.in}
# 	f=${y##*/}
# 	echo $f
# 	./dcc < $i &> $TEST_DIR/out/$f.out
# 	diff -w $TEST_DIR/out/$f.out $TEST_DIR/$f.sol
# done


# TEST_DIR=more_tests
# echo generate correct files
# cd $TEST_DIR
# for i in $(ls *.in); do
# 	y=${i%.in}
# 	f=${y##*/}
# 	./dcc < $i &> $f.sol
# done

# cd ..
# for i in $(ls $TEST_DIR/*.in); do
# 	y=${i%.in}
# 	f=${y##*/}
# 	echo $f
# 	./dcc < $i &> $TEST_DIR/out/$f.out
# 	diff -w $TEST_DIR/out/$f.out $TEST_DIR/$f.sol
# done


