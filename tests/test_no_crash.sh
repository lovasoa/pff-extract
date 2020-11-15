#!/usr/bin/env bash

errors=0
total=0

for type in "malformed" "correct"
do
  for f in files/$type/*
  do
    echo -e "\n========= Testing '$f' ==========="
    ../pff-extract "$f" >output.txt 2>&1
    code=$?
    echo "Exit code: $code"
    if [ $code -gt 100 ]
    then
      tail output.txt
      echo "TEST FAILED ! Program crashed"
      errors=$(($errors+1))
    fi
    if [ $code -ne 0 ] && [ "$type" = "correct" ]
    then
      tail output.txt
      echo "Program returned an error"
      errors=$(($errors+1))
    fi
    rm output.txt
    total=$(($total+1))
  done
done

echo "$errors/$total test failures."
exit $errors
