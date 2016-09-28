def startVM(name) {
  lock("create-vm") {
    echo "Creating /Users/Shared/Jenkins/vagrant/electron-vagrant ${name}"
    //    withEnv(["VAGRANT_DOTFILE_PATH=.${env.BUILD_TAG}", "VAGRANT_CWD=/Users/Shared/Jenkins/vagrant/electron-vagrant"]) {
      withEnv(["VAGRANT_CWD=/Users/Shared/Jenkins/vagrant/electron-vagrant"]) {

      sh "vagrant up ${name}"
    }
  }
}

def stopVM(name) {
  stage("Stop VM") {
    echo "Stopping /Users/Shared/Jenkins/vagrant/electron-vagrant ${name}"
    //    withEnv(["VAGRANT_DOTFILE_PATH=.${env.BUILD_TAG}", "VAGRANT_CWD=/Users/Shared/Jenkins/vagrant/electron-vagrant"]) {
      withEnv(["VAGRANT_CWD=/Users/Shared/Jenkins/vagrant/electron-vagrant"]) {
      sh "vagrant halt ${name}"
    }
  }
}

def restoreVM(name) {
  stage("Restore VM") {
    echo "Restoring /Users/Shared/Jenkins/vagrant/electron-vagrant ${name}"
//    withEnv(["VAGRANT_DOTFILE_PATH=.${env.BUILD_TAG}", "VAGRANT_CWD=/Users/Shared/Jenkins/vagrant/electron-vagrant"]) {
      withEnv(["VAGRANT_CWD=/Users/Shared/Jenkins/vagrant/electron-vagrant"]) {
      sh "vagrant snapshot restore ${name} build-v1"
    }
  }
}

def destroyVM(name) {
  stage("Destroy VM") {
    echo "Destroying /Users/Shared/Jenkins/vagrant/electron-vagrant ${name}"
    //    withEnv(["VAGRANT_DOTFILE_PATH=.${env.BUILD_TAG}", "VAGRANT_CWD=/Users/Shared/Jenkins/vagrant/electron-vagrant"]) {
      withEnv(["VAGRANT_CWD=/Users/Shared/Jenkins/vagrant/electron-vagrant"]) {
      sh "vagrant destroy -f ${name}"
    }
  }
}

// TODO(bridiver) - use file so these aren't visible in the logs
def sshEnv() {
  return ["export TARGET_ARCH=${env.TARGET_ARCH}", "export ELECTRON_S3_BUCKET=${env.ELECTRON_S3_BUCKET}",
    "export LIBCHROMIUMCONTENT_MIRROR=${env.LIBCHROMIUMCONTENT_MIRROR}", "export CI=${env.CI}",
    "export ELECTRON_RELEASE=${env.ELECTRON_RELEASE}", "export GYP_DEFINES=${env.GYP_DEFINES}",
    "export ELECTRON_S3_SECRET_KEY=${env.ELECTRON_S3_SECRET_KEY}",
    "export ELECTRON_S3_ACCESS_KEY=${env.ELECTRON_S3_ACCESS_KEY}",
    "export ELECTRON_GITHUB_TOKEN=${env.ELECTRON_GITHUB_TOKEN}",
    "export PYTHONUNBUFFERED=1"].join('; ')
}

def vmSSH(name, command) {
  def envVars = sshEnv()
  //    withEnv(["VAGRANT_DOTFILE_PATH=.${env.BUILD_TAG}", "VAGRANT_CWD=/Users/Shared/Jenkins/vagrant/electron-vagrant"]) {
      withEnv(["VAGRANT_CWD=/Users/Shared/Jenkins/vagrant/electron-vagrant"]) {
    sh "vagrant ssh ${name} -c '(${envVars}; ${command})'"
  }
}

def buildElectron() {
  lock('build-electron') {
    retry(2) {
      stage('Clean') {
        sh "git clean -ffdx"
        deleteDir()
      }
      stage('Checkout') {
        checkout scm
      }
      stage('Build') {
        sh "script/cibuild"
      }
    }
  }
}

// git config http.postBuffer 524288000

def installNode(name) {
  stage('install node') {
    retry(3) {
      vmSSH(name, "curl -sL https://deb.nodesource.com/setup_6.x | sudo -E bash -")
    }
    vmSSH(name, "sudo apt-get install -y nodejs")
  }
}

def setLinuxDisplay(name) {
  vmSSH(name, "export DISPLAY=:99.0")
}

// def ia32symstorePath = 'export PATH=$PATH:/cygdrive/c/Program\\ Files\\ \\(x86\\)/Windows\\ Kits/10/Debuggers/x86'

timestamps {
  withEnv([
    'ELECTRON_S3_BUCKET=brave-laptop-binaries',
    'LIBCHROMIUMCONTENT_MIRROR=https://s3.amazonaws.com/brave-laptop-binaries/libchromiumcontent',
    'ELECTRON_RELEASE=1',
    'CI=1',
    'PYTHONUNBUFFERED=1'
]) {
    // LIBCHROMIUMCONTENT_COMMIT parameter?
    parallel (
      mac: {
        node {
          withEnv(['TARGET_ARCH=x64', 'PLATFORM=darwin']) {
            buildElectron()
          }
        }
      },
      winx64: {
        node {
          withEnv(['TARGET_ARCH=x64', 'PLATFORM=win']) {
            lock('win-x64') {
              try {
                retry(2) {
                  restoreVM('win-x64')
    //              lock('build-electron') {
                  // startVM('win-x64')
                }
                retry (2) {
                  timeout(10) {
                    vmSSH('win-x64', "rm -rf electron && git clone https://github.com/brave/electron.git")
                  }
                }
                retry (2) {
                  vmSSH('win-x64', "cd electron && script/clean.py && script/cibuild")
                }
  //              }
  //              }
              } finally {
                stopVM('win-x64')
              }
            }
          }
        }
      },
      winia32: {
        node {
          withEnv(['TARGET_ARCH=ia32', 'PLATFORM=win']) {
            lock('win-ia32') {
              try {
                retry(2) {
                  restoreVM('win-ia32')
  //                lock('build-electron') {
                  // startVM('win-ia32')
                }
                retry (2) {
                  timeout(10) {
                    vmSSH('win-ia32', "rm -rf electron && git clone https://github.com/brave/electron.git")
                  }
                }
                retry (2) {
                  vmSSH('win-ia32', "cd electron && script/clean.py && script/cibuild")
                }
  //                }
  //              }
              } finally {
                stopVM('win-ia32')
              }
            }
          }
        }
      }//,
//      linuxx64: {
//        node {
//          withEnv(['TARGET_ARCH=x64', 'PLATFORM=linux']) {
//            try {
//              retry(3) {
//                destroyVM('linux-x64')
//                lock('build-electron') {
//                  startVM('linux-x64')
//                  installNode('linux-x64')
//                  setLinuxDisplay('linux-x64')
//                  vmSSH('linux-x64', "git clone https://github.com/brave/electron.git && cd electron && script/cibuild")
//                }
//              }
//            } finally {
//              destroyVM('linux-x64')
//            }
//          }
//        }
//      }
    )
  }
}
