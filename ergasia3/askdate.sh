#!/bin/bash

for i in {1..30}; do
  host="linux$(printf "%02d" $i)"
  echo -n "$host: "
  ssh $host date
done
