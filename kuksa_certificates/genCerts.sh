#!/bin/sh


genCACert() {
    openssl genrsa -out CA.key 2048
    openssl req -key CA.key -new -out CA.csr -subj "/C=DE/ST=BW/L=Rng/O=Robert Bosch GmbH/OU=CR/CN=localhost-ca/emailAddress=CI.Hotline@de.bosch.com"
    openssl x509 -signkey CA.key -in CA.csr -req -days 365 -out CA.pem
}

genCert() {
    openssl genrsa -out $1.key 2048
    openssl req -new -key $1.key -out $1.csr -passin pass:"temp" -subj "/C=DE/ST=BW/L=Rng/O=Robert Bosch GmbH/OU=CR/CN=$1/emailAddress=CI.Hotline@de.bosch.com"
    openssl x509 -req -in $1.csr -CA CA.pem -CAkey CA.key -CAcreateserial -days 365 -out $1.pem
    openssl verify -CAfile CA.pem $1.pem
}

set -xe
# Check if the CA is available, else make CA certificates
if [ -f "CA.key" ]; then
    echo "Existing CA will be used"
else
    echo "No CA found"
    genCACert
    echo ""
fi


for i in Server Client;
do
    genCert $i
    echo ""
done
