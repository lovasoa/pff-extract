#!/usr/bin/env bash

final=0
for f in files/*
do
  echo -e "\n========= Testing '$f' ==========="
  ../pff-extract "$f" 2>&1 >output.txt
  code=$?
  tail output.txt
  rm output.txt
  echo "Exit code for '$f' : $code"
  if [ $code -gt 100 ]
  then
    final=$code
  fi
done
exit $final
