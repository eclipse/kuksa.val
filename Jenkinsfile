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
            git submodule update --init
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
            sudo docker save $(sudo docker images --filter "reference=amd64/kuksa-val*"  --format "{{.Repository}}:{{.Tag}}" | head -1) | xz -T 0 > artifacts/kuksa-val-amd64.tar.xz
			sudo docker save $(sudo docker images --filter "reference=arm64/kuksa-val*"  --format "{{.Repository}}:{{.Tag}}" | head -1) | xz -T 0 > artifacts/kuksa-val-arm64.tar.xz
			sudo docker save kuksa-val-dev:ubuntu20.04 | xz -T 0 > artifacts/kuksa-val-dev-ubuntu20.04.tar.xz
            '''
        }
        stage ('Archive') {
            archiveArtifacts artifacts: 'artifacts/*.xz' 
        }
    }
    docker.image('kuksa-val-dev:ubuntu20.04').inside("-v /var/run/docker.sock:/var/run/docker.sock"){ 
        stage('Test') {
            sh '''
                cd /kuksa.val/build
                # TODO and you may need sudo right for testing ctest --build-config Debug --output-on-failure --parallel 8
            '''
        }
    }
}
