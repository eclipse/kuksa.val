/**  
* Copyright Robert Bosch GmbH, 2020. Part of the Eclipse Kuksa Project.
*
* All rights reserved. This configuration file is provided to you under the
* terms and conditions of the Eclipse Distribution License v1.0 which
* accompanies this distribution, and is available at
* http://www.eclipse.org/org/documents/edl-v10.php
*
**/

node('docker') {
    checkout scm
    	def buildImage = docker.build("my-image:${env.BUILD_ID}", "-f docker/Dockerfile-Jenkins-Build-Env .")
        buildImage.inside(" -v /var/run/docker.sock:/var/run/docker.sock " ){
        stage('Prepare') {
        sh '''
            mkdir -p artifacts
            rm -f artifacts/*
            '''
        }
        stage('Build') {
			sh '''
			 ./docker/build.sh amd64
            '''
            sh './docker/build.sh arm64'
            sh 'sudo docker build -t kuksa-val-dev:ubuntu20.04 -f docker/Dockerfile.dev .'
        }
        stage('Collect') {
			sh '''
            sudo docker save $(sudo docker images --filter "reference=amd64/kuksa-val*"  --format "{{.Repository}}:{{.Tag}}" | head -1) | bzip2 -9 > artifacts/kuksa-val-amd64.tar.bz2
			sudo docker save $(sudo docker images --filter "reference=amd64/kuksa-val*"  --format "{{.Repository}}:{{.Tag}}" | head -1) | bzip2 -9 > artifacts/kuksa-val-arm64.tar.bz2
			sudo docker save kuksa-val-dev:ubuntu20.04 | bzip2 -9 > artifacts/kuksa-val-dev-ubuntu20.04.tar.bz2
            '''
        }
        stage ('Archive') {
            archiveArtifacts artifacts: 'artifacts/*.bz2' 
        }
    }
}
