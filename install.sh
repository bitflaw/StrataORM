#This is to be used to install system-wide. Tweak accordingly.

rm -rf /usr/include/orm++
rm -f /usr/lib/liborm++.so
cp -r orm++ /usr/include/
cp build/liborm++.so /usr/lib/
