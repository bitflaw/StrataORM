#!/bin/sh

scope="$1"

if [[ -z "$scope" ]];then
  echo "Usage: sudo ./install.sh <how to install: global or local>"
  exit
fi

if [[ "$scope" == "global" ]]; then
  echo "[INFO] Installing liborm++ system-wide."
  rm -rf /usr/include/orm++ &&
  cp -r orm++ /usr/include/ &&
  install -Cv build/liborm++.so /usr/lib/ &&
  echo "[INFO] Install done"
  exit
elif [[ "$scope" == "local" ]]; then
  echo "[INFO] Installing liborm++ locally."
  rm -rf /usr/local/include/orm++ &&
  cp -r orm++ /usr/local/include/ &&
  install -Cv build/liborm++.so /usr/local/lib/ &&
  echo "[INFO] Install done"
  exit
else
  echo "No option such as the one you specified:'$scope'"
  exit
fi
