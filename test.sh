#! /bin/bash

NUM_PLAYERS=$5
NUM_HOPS=$1

./ringmaster 5 1 &

sleep 2

for (( i=0; i < 5; i++ ))
do
    ./player $i &
done

