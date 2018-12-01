#!/bin/bash
args=""
for i in {1..6}; do
  args1=""
  for j in 44 48 88 96; do
    if test  $i -ge 5  -a  $j -ge 88 ; then continue; fi
    for k in 16 24; do
      args1="$args1 a${i}_${k}_${j}.wav"
    done
  done
  args="$args -g $args1"
done
echo  $args
