#!/bin/bash -e

# install essential packages
sudo apt-get update
sudo apt-get install -y emacs24 dbus-x11 libgmp-dev libelf-dev
sudo apt-get install -y build-essential vim curl bison flex bc libcap-dev git cmake libboost-all-dev libncurses5-dev python-minimal python-pip unzip 
sudo apt-get install -y git subversion wget libprotobuf-dev python-protobuf protobuf-compiler libboost-all-dev
sudo apt-get install -y llvm-5.0 libclang-5.0-dev clang-5.0 tree 
sudo apt-get install -y python-pip
sudo apt-get install libssl-dev
sudo apt-get install libelf-dev
sudo apt-get install libgtk-3-dev
sudo apt-get install libncurses5-dev libncursesw5-dev

# rename llvm packages
sudo cp /usr/bin/clang-5.0 /usr/bin/clang
sudo cp /usr/bin/clang++-5.0 /usr/bin/clang++
sudo cp /usr/bin/llvm-link-5.0 /usr/bin/llvm-link
sudo cp /usr/bin/llvm-dis-5.0 /usr/bin/llvm-dis
sudo cp /usr/bin/llvm-ar-5.0 /usr/bin/llvm-ar
sudo cp /usr/bin/llvm-as-5.0 /usr/bin/llvm-as
sudo cp /usr/bin/llc-5.0 /usr/bin/llc
sudo cp /usr/bin/opt-5.0 /usr/bin/opt
sudo cp /usr/bin/llvm-nm-5.0 /usr/bin/llvm-nm

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
