#!/bin/bash

rm -rf output/*

cd src/Database
make
mv database ../../output

cd ../Door
make
mv door ../../output

cd ../Gateway
make
mv gateway ../../output

cd ../KeyChain
make
mv keychain ../../output

cd ../Motion
make
mv motion ../../output

cd ../SecuritySystem
make
mv securitysystem ../../output
