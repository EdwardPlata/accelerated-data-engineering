#!/bin/bash
set -e

echo "=== SimpleDB Bare Metal Setup on Linode ==="

# Update system
apt-get update
apt-get upgrade -y

# Install essential tools
apt-get install -y \
    build-essential \
    cmake \
    git \
    curl \
    wget \
    htop \
    iotop \
    net-tools \
    sysstat \
    linux-tools-common \
    linux-tools-generic

# Install modern GCC and C++ tools
apt-get install -y \
    gcc-12 \
    g++-12 \
    clang-14 \
    lldb-14 \
    gdb

# Set default compiler
update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-12 100
update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-12 100

# Install monitoring tools
apt-get install -y \
    prometheus-node-exporter \
    grafana

# Configure performance settings
echo "Configuring system performance..."

# Disable transparent huge pages (better for databases)
echo never > /sys/kernel/mm/transparent_hugepage/enabled
echo never > /sys/kernel/mm/transparent_hugepage/defrag

# Optimize network settings
cat >> /etc/sysctl.conf <<EOF
# Network optimizations
net.core.rmem_max = 134217728
net.core.wmem_max = 134217728
net.ipv4.tcp_rmem = 4096 87380 67108864
net.ipv4.tcp_wmem = 4096 65536 67108864
net.core.netdev_max_backlog = 5000

# Memory optimizations
vm.swappiness = 10
vm.dirty_ratio = 15
vm.dirty_background_ratio = 5
EOF

sysctl -p

# Setup data volume
DATA_VOLUME="/dev/disk/by-id/scsi-0Linode_Volume_simpledb-data-volume"
if [ -b "$DATA_VOLUME" ]; then
    echo "Formatting and mounting data volume..."
    mkfs.ext4 -F "$DATA_VOLUME"
    mkdir -p /data
    mount "$DATA_VOLUME" /data
    
    # Add to fstab
    echo "$DATA_VOLUME /data ext4 defaults,noatime 0 2" >> /etc/fstab
    
    # Create application directories
    mkdir -p /data/simpledb
    mkdir -p /data/logs
    mkdir -p /data/backups
fi

echo "=== Setup complete ==="
