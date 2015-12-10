#!/bin/bash

rm -rf output/*

cd src/Database1
make
mv database1 ../../output

cd ../Database2
make
mv database2 ../../output

cd ../Door
make
mv door ../../output

cd ../Gateway1
make
mv gateway1 ../../output

cd ../Gateway2
make
mv gateway2 ../../output

cd ../KeyChain
make
mv keychain ../../output

cd ../Motion
make
mv motion ../../output

cd ../SecuritySystem
make
mv securitysystem ../../output
