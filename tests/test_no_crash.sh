#!/usr/bin/env bash

errors=0
total=0
for f in files/*
do
  echo -e "\n========= Testing '$f' ==========="
  ../pff-extract "$f" >output.txt 2>&1
  code=$?
  tail output.txt
  rm output.txt
  echo "Exit code for '$f' : $code"
  if [ $code -gt 100 ]
  then
    errors=$(($errors+1))
  fi
  total=$(($total+1))
done

echo "$errors/$total test failures."
exit $errors
