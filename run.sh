#!/usr/bin/bash

if [ "$#" -ne 1 ]; then
  echo "Illegal number of parameters"
  exit -1
fi

DIR="experiments"
if [ -d "$DIR" ]; then
  echo "Error: Directory ${DIR} exists!"
  exit -1
fi

mkdir -p experiments

dir="experiments-${1//.in}"
current_time=$(date "+%H:%M:%S_%Y-%m-%d")
new_dir="${dir}_${current_time}"

cd build && make -j20 && cd ..

for input in patterns/*; do
  echo "$input"
  while IFS= read -r line
  do
    echo "$line"
    ./build/main $1 "$line" faster
  done < "$input"
done;

for input in patterns/*; do
  echo "$input"
  while IFS= read -r line
  do
    echo "$line"
    ./build/main $1 "$line" standard
  done < "$input"
done;

mv experiments ${new_dir}