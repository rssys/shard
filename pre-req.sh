#!/bin/bash -e

# download go v1.11.2
wget https://dl.google.com/go/go1.11.2.linux-amd64.tar.gz

# extract go installation
sudo tar -C /usr/local -xzf go1.11.2.linux-amd64.tar.gz

# set the environment variables
export GOPATH=$HOME/go
export PATH=$PATH:/usr/local/go/bin:$GOPATH/bin

# go get the github link
go get github.com/SRI-CSL/gllvm/cmd/...

# add go binaries to /usr/local/bin
sudo cp $GOPATH/bin/* /usr/local/bin

# install LLVM 5.0
sudo apt-get install -y llvm-5.0 libclang-5.0-dev clang-5.0 tree

# Export the path
echo "Please add the following to ~/.bashrc"
echo "export SHARD_PATH="`pwd`
