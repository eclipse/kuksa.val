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
    def versiontag="unknown"
    stage('Prepare') {
        sh 'ls -al artifacts || true'
        sh 'git submodule update --init'
        sh 'git clean -fdx || true'
        sh 'mkdir -p artifacts'
        versiontag=sh(returnStdout: true, script: "git tag --contains | head -n 1").trim()
        if (versiontag == "") { //not tagged, using commit
            versiontag = sh(returnStdout: true, script: "git rev-parse --short HEAD").trim()
        }
        echo "Using versiontag ${versiontag} for images";
    }
    stage('Build') {
        //Prepare for building test-client with default tokens
        parallel arm64: {
                stage('arm64') {
                    sh "docker buildx build --platform=linux/arm64 -f ./kuksa-val-server/docker/Dockerfile -t arm64/kuksa-val:${versiontag} --output type=docker,dest=./artifacts/kuksa-val-${versiontag}-arm64.tar ."
                    sh "docker buildx build --platform=linux/arm64 -f ./kuksa_viss_client/Dockerfile -t arm64/kuksa-client:${versiontag} --output type=docker,dest=./artifacts/kuksa-client-${versiontag}-arm64.tar ."
                    sh "docker buildx build --platform=linux/arm64 -t arm64/kuksa-gps-feeder:${versiontag} --output type=docker,dest=./artifacts/kuksa-gps-feeder-${versiontag}-arm64.tar kuksa_feeders/gps2val"
                }
                }, 
                amd64: {
                stage('amd64') {
                    sh "docker buildx build --platform=linux/amd64 -f ./kuksa-val-server/docker/Dockerfile -t amd64/kuksa-val:${versiontag} --output type=docker,dest=./artifacts/kuksa-val-${versiontag}-amd64.tar ."
                    sh "docker buildx build --platform=linux/amd64 -f ./kuksa_viss_client/Dockerfile -t amd64/kuksa-client:${versiontag} --output type=docker,dest=./artifacts/kuksa-client-${versiontag}-amd64.tar ."
                    sh "docker buildx build --platform=linux/amd64 -t amd64/kuksa-gps-feeder:${versiontag} --output type=docker,dest=./artifacts/kuksa-gps-feeder-${versiontag}-amd64.tar kuksa_feeders/gps2val"
                    
                    sh "docker build -t kuksa-val-dev-ubuntu20.04:${versiontag} -f ./kuksa-val-server/docker/Dockerfile.dev ."
                    sh "docker save kuksa-val-dev-ubuntu20.04:${versiontag}  > artifacts/kuksa-val-dev-ubuntu20.04:${versiontag}.tar"
            }
        }
    }
    stage('Test') {
        sh "docker run --rm  -w /kuksa.val/kuksa-val-server -v ${env.WORKSPACE}/artifacts:/out kuksa-val-dev-ubuntu20.04:${versiontag}  ./run_tests.sh -a /out"
        step([$class: 'XUnitPublisher',
                thresholds: [ skipped(failureThreshold: '0'), failed(failureThreshold: '0') ],
                tools: [ BoostTest(pattern: 'artifacts/results.xml') ]]
        )
        cobertura autoUpdateHealth: false, autoUpdateStability: false, coberturaReportFile: 'artifacts/coverage.xml', conditionalCoverageTargets: '70, 0, 0', enableNewApi: true, failUnhealthy: false, failUnstable: false, lineCoverageTargets: '80, 0, 0', maxNumberOfBuilds: 0, methodCoverageTargets: '80, 0, 0', onlyStable: false, sourceEncoding: 'ASCII', zoomCoverageChart: false
        //Cleaning up: After unittest we do not need dev docker in local registry
        sh "docker rmi kuksa-val-dev-ubuntu20.04:${versiontag}"
    }
    stage('Compress') {
        sh 'ls -al artifacts'
        sh 'cd artifacts && xz -T 0 ./*.tar'
    }
    stage ('Archive') {
        archiveArtifacts artifacts: 'artifacts/*.xz' 
    }
}
