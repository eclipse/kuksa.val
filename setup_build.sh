#!/bin/bash
     
    #Generate the public key from private key in demo-certs directory
    openssl rsa -in ./kuksa_certificates/Client.key -pubout -outform PEM -out ./kuksa_certificates/jwt.pub.key

    rm -rf build
    mkdir -p build

    cd build
    cmake -DBUILD_TEST_CLIENT=ON .. 

    #for time being use native boost lib which is v1.67 
    #cmake -DBOOST_ROOT=/home/tspreck/kuksa.libs/libs-built/boost1.67 ..
    
    #boost v1.72.0 not curently compatible with project!
    #cmake -DBOOST_ROOT=/home/tspreck/kuksa.libs/libs-built/boost1.72.0 ..

