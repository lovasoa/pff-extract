#!/usr/bin/env bash

final=0
for f in files/*
do
  echo -e "\n========= Testing '$f' ==========="
  ../pff-extract "$f" /dev/null 2>&1 | tail -n 5
  code=$?
  echo "Exit code for '$f' : $code"
  if [ $code -gt 100 ]
  then
    final=$code
  fi
done
exit $final
