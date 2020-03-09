node('docker') {
    checkout scm
    	def buildImage = docker.build("my-image:${env.BUILD_ID}", "-f docker/Dockerfile-Jenkins-Build-Env .")
        buildImage.inside(" -v /var/run/docker.sock:/var/run/docker.sock " ){
        stage('Build') {
			sh '''
			 # need this to use host docker. putting it in the correct group with correct gid doesn't help
             # but strangely this works. Don't try at home
            sudo chmod ugo+rwx /var/run/docker.sock
            ./docker/build.sh amd64
            '''
            sh './docker/build.sh arm64'
        }
        stage('Collect') {
			sh '''
            mkdir -p artifacts
            rm -f artifacts/*
            docker save $(docker images --filter "reference=amd64/kuksa-val*" -q | head -1) | bzip2 -9 > artifacts/kuksa-val-amd64.tar.bz2
			docker save $(docker images --filter "reference=arm64/kuksa-val*" -q | head -1) | bzip2 -9 > artifacts/kuksa-val-arm64.tar.bz2
			#docker save  amd64/kuksa-val:0.1.1 | bzip2 -9 > artifacts/kuksa-val-amd64.tar.bz2
            #docker save  arm64/kuksa-val:0.1.1 | bzip2 -9 > artifacts/kuksa-val-arm64.tar.bz2
            '''
        }
        stage ('Archive') {
            archiveArtifacts artifacts: 'artifacts/*.bz2' 
        }
    }
}


