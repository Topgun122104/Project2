#!/bin/bash

cp output/run.sh ./

rm -rf output/*

cp run.sh output/run.sh

cd src/DatabaseP
make
mv databaseP ../../output

cd ../DatabaseS
make
mv databaseS ../../output

cd ../Door
make
mv door ../../output

cd ../GatewayP
make
mv gatewayP ../../output

cd ../GatewayS
make
mv gatewayS ../../output

cd ../KeyChain
make
mv keychain ../../output

cd ../Motion
make
mv motion ../../output

cd ../SecuritySystem
make
mv securitysystem ../../output
