#!/bin/bash
set -e

echo "=== Setting up Weights & Biases ML Environment ==="

# Update system
apt-get update
apt-get upgrade -y

# Install NVIDIA drivers and CUDA
apt-get install -y ubuntu-drivers-common
ubuntu-drivers autoinstall

# Install CUDA toolkit
wget https://developer.download.nvidia.com/compute/cuda/repos/ubuntu2204/x86_64/cuda-keyring_1.0-1_all.deb
dpkg -i cuda-keyring_1.0-1_all.deb
apt-get update
apt-get install -y cuda

# Install C++ build tools
apt-get install -y \
    build-essential \
    cmake \
    git \
    gcc-12 \
    g++-12

# Install Python for W&B
apt-get install -y python3-pip python3-dev
pip3 install --upgrade pip
pip3 install wandb numpy torch

# Login to W&B
wandb login ${wandb_api_key}

# Clone and build application
cd /opt
git clone https://github.com/EdwardPlata/accelerated-data-engineering.git

# Setup W&B experiment tracking
mkdir -p /data/experiments
mkdir -p /data/models

echo "=== W&B setup complete ==="
