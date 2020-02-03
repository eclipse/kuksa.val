#!/bin/bash
     
    mkdir -p build
    rm -rf build/*
    
    #Generate the public key from private key in demo-certs directory
    openssl rsa -in ./certificates/Client.key -pubout -outform PEM -out ./certificates/jwt.pub.key

    cd build
    cmake ..

    #for time being use native boost lib which is v1.67 
    #cmake -DBOOST_ROOT=/home/tspreck/kuksa.libs/libs-built/boost1.67 ..
    
    #boost v1.72.0 not curently compatible with project!
    #cmake -DBOOST_ROOT=/home/tspreck/kuksa.libs/libs-built/boost1.72.0 ..

