#!/bin/sh

set -eu

for r in 2 4 8 16 32; do
  for try in $(seq 1 5); do
    for population in 1 2 3 5; do
      echo "=== num remove/add/rewire: $r === try: $try === population: $population ==="
      ./shannon2 30x30.json 30x30_states_${population}.txt --num_remove $r &
      ./shannon2 30x30.json 30x30_states_${population}.txt --num_rewire $r &
      ./shannon2 30x30.json 30x30_states_${population}.txt --num_add $r &
    done
    wait
  done
done
