#!/bin/sh

echo "[INFO] Starting build..."
cd build &&
  cmake .. &&
  make > ../complogs.txt 2>&1 &&
  head -n 10 ../complogs.txt &&
  cd .. &&
  echo "[INFO] Finished build."
