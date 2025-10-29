# Cloud Development: Bare Metal Deployment with C++

This comprehensive guide covers deploying C++ data engineering applications, including our SimpleDB database system, on bare metal infrastructure across various cloud providers using Infrastructure as Code (IaC) with Terraform.

---

## Table of Contents

1. [Introduction to Bare Metal Cloud Hosting](#introduction-to-bare-metal-cloud-hosting)
2. [Why Bare Metal for C++ Applications](#why-bare-metal-for-cpp-applications)
3. [Cloud Provider Overview](#cloud-provider-overview)
4. [Infrastructure as Code with Terraform](#infrastructure-as-code-with-terraform)
5. [Linode Bare Metal Deployment](#linode-bare-metal-deployment)
6. [DigitalOcean Bare Metal Deployment](#digitalocean-bare-metal-deployment)
7. [Weights & Biases ML Deployment](#weights--biases-ml-deployment)
8. [Deploying SimpleDB on Bare Metal](#deploying-simpledb-on-bare-metal)
9. [Monitoring and Observability](#monitoring-and-observability)
10. [Security Best Practices](#security-best-practices)
11. [Cost Optimization](#cost-optimization)
12. [Troubleshooting](#troubleshooting)

---

## Introduction to Bare Metal Cloud Hosting

Bare metal servers provide direct access to physical hardware without virtualization overhead, making them ideal for high-performance C++ applications. Unlike virtual machines (VMs), bare metal offers:

- **Predictable Performance**: No noisy neighbor problems
- **Full Resource Access**: All CPU cores, memory, and I/O bandwidth
- **Custom Kernel Configuration**: Complete control over the operating system
- **Hardware-Level Optimization**: Direct access to CPU instructions (AVX, SSE)
- **Lower Latency**: No hypervisor overhead

### Bare Metal vs. Virtual Machines

| Feature | Bare Metal | Virtual Machine |
|---------|-----------|-----------------|
| Performance | Predictable, maximum | Variable, shared |
| Cost | Higher | Lower |
| Provisioning | Minutes to hours | Seconds |
| Isolation | Physical | Logical |
| Flexibility | Lower | Higher |
| Use Case | High-performance computing | General workloads |

---

## Why Bare Metal for C++ Applications

C++ applications benefit significantly from bare metal deployment:

### 1. **Memory Management**
- Direct access to physical memory without virtualization overhead
- NUMA (Non-Uniform Memory Access) optimization
- Huge pages support for large datasets
- Custom memory allocators perform better

### 2. **CPU Performance**
- Access to all CPU cores without sharing
- Optimal cache utilization
- Hardware acceleration (AVX-512, SIMD instructions)
- CPU pinning and affinity control

### 3. **I/O Performance**
- Direct NVMe SSD access with maximum IOPS
- Network card optimization (DPDK, kernel bypass)
- Lower storage latency for database operations
- PCIe device passthrough

### 4. **Real-Time Requirements**
- Deterministic performance for latency-sensitive applications
- Real-time operating system (RTOS) support
- Precise timing control for trading systems, streaming

---

## Cloud Provider Overview

### Linode (Akamai Cloud Computing)
- **Strengths**: Simple pricing, excellent support, global presence
- **Bare Metal**: Dedicated CPU instances with guaranteed resources
- **Best For**: Production databases, high-performance computing
- **Pricing**: Predictable, competitive pricing
- **Locations**: 11 global data centers

### DigitalOcean
- **Strengths**: Developer-friendly, simple interface, good documentation
- **Bare Metal**: Dedicated Droplets with full CPU allocation
- **Best For**: Development, testing, medium-scale production
- **Pricing**: Transparent, hourly billing
- **Locations**: 13 data centers worldwide

### Weights & Biases (W&B)
- **Strengths**: Specialized ML platform, experiment tracking
- **Bare Metal**: GPU-accelerated compute instances
- **Best For**: Machine learning training, model deployment
- **Pricing**: Usage-based, focused on ML workloads
- **Integration**: Built-in experiment tracking and visualization

---

## Infrastructure as Code with Terraform

Terraform enables version-controlled, reproducible infrastructure deployments. All examples in this guide use Terraform to provision and configure bare metal resources.

### Prerequisites

```bash
# Install Terraform (Linux)
wget https://releases.hashicorp.com/terraform/1.6.0/terraform_1.6.0_linux_amd64.zip
unzip terraform_1.6.0_linux_amd64.zip
sudo mv terraform /usr/local/bin/

# Verify installation
terraform version

# Install additional tools
sudo apt-get update
sudo apt-get install -y git curl wget build-essential cmake
```

### Terraform Basics

```hcl
# Example Terraform structure
terraform {
  required_version = ">= 1.0"
  required_providers {
    linode = {
      source  = "linode/linode"
      version = "~> 2.0"
    }
  }
}

provider "linode" {
  token = var.linode_token
}

variable "linode_token" {
  description = "Linode API Token"
  type        = string
  sensitive   = true
}
```

---

## Linode Bare Metal Deployment

### Overview
Deploy the SimpleDB C++ database on Linode's dedicated CPU instances for maximum performance.

### Directory Structure

```
terraform/linode/
├── main.tf              # Main infrastructure configuration
├── variables.tf         # Input variables
├── outputs.tf           # Output values
├── simpledb.tf          # SimpleDB-specific configuration
├── networking.tf        # Network configuration
├── security.tf          # Firewall and security groups
├── monitoring.tf        # Monitoring setup
└── scripts/
    ├── setup.sh         # Initial server setup
    ├── install_deps.sh  # Install C++ dependencies
    └── deploy_db.sh     # Deploy SimpleDB
```

### Step 1: Create Terraform Configuration

**File: `terraform/linode/main.tf`**
```hcl
terraform {
  required_version = ">= 1.0"
  
  required_providers {
    linode = {
      source  = "linode/linode"
      version = "~> 2.5"
    }
  }
}

provider "linode" {
  token = var.linode_token
}

# Dedicated CPU Instance for SimpleDB
resource "linode_instance" "simpledb_server" {
  label           = "simpledb-production"
  region          = var.region
  type            = "g6-dedicated-8"  # 8 dedicated cores, 32GB RAM
  image           = "linode/ubuntu22.04"
  root_pass       = var.root_password
  authorized_keys = [var.ssh_public_key]
  
  tags = ["production", "database", "cpp"]
  
  # Enable backups
  backups_enabled = true
  
  # Private IP for internal communication
  private_ip = true
}

# Additional storage for database
resource "linode_volume" "simpledb_data" {
  label  = "simpledb-data-volume"
  region = var.region
  size   = 100  # 100 GB
}

resource "linode_volume_attachment" "simpledb_attachment" {
  volume_id   = linode_volume.simpledb_data.id
  linode_id   = linode_instance.simpledb_server.id
  config_path = "/dev/disk/by-id/scsi-0Linode_Volume_${linode_volume.simpledb_data.label}"
}
```

**File: `terraform/linode/variables.tf`**
```hcl
variable "linode_token" {
  description = "Linode API Token"
  type        = string
  sensitive   = true
}

variable "region" {
  description = "Linode region"
  type        = string
  default     = "us-east"
}

variable "root_password" {
  description = "Root password for Linode instance"
  type        = string
  sensitive   = true
}

variable "ssh_public_key" {
  description = "SSH public key for authentication"
  type        = string
}

variable "allowed_ips" {
  description = "IP addresses allowed to connect"
  type        = list(string)
  default     = []
}

variable "db_port" {
  description = "SimpleDB port"
  type        = number
  default     = 9999
}
```

**File: `terraform/linode/networking.tf`**
```hcl
# Firewall configuration
resource "linode_firewall" "simpledb_firewall" {
  label = "simpledb-firewall"
  
  # Inbound rules
  inbound {
    label    = "allow-ssh"
    action   = "ACCEPT"
    protocol = "TCP"
    ports    = "22"
    ipv4     = var.allowed_ips
  }
  
  inbound {
    label    = "allow-database"
    action   = "ACCEPT"
    protocol = "TCP"
    ports    = tostring(var.db_port)
    ipv4     = var.allowed_ips
  }
  
  inbound {
    label    = "allow-monitoring"
    action   = "ACCEPT"
    protocol = "TCP"
    ports    = "9090,3000"  # Prometheus, Grafana
    ipv4     = var.allowed_ips
  }
  
  # Outbound rules
  outbound {
    label    = "allow-all-outbound"
    action   = "ACCEPT"
    protocol = "TCP"
    ports    = "1-65535"
    ipv4     = ["0.0.0.0/0"]
  }
  
  outbound {
    label    = "allow-dns"
    action   = "ACCEPT"
    protocol = "UDP"
    ports    = "53"
    ipv4     = ["0.0.0.0/0"]
  }
  
  # Attach to instance
  linodes = [linode_instance.simpledb_server.id]
}
```

**File: `terraform/linode/outputs.tf`**
```hcl
output "server_ip" {
  description = "Public IP address of SimpleDB server"
  value       = linode_instance.simpledb_server.ip_address
}

output "server_id" {
  description = "Linode instance ID"
  value       = linode_instance.simpledb_server.id
}

output "private_ip" {
  description = "Private IP address"
  value       = linode_instance.simpledb_server.private_ip_address
}

output "ssh_command" {
  description = "SSH command to connect"
  value       = "ssh root@${linode_instance.simpledb_server.ip_address}"
}

output "volume_path" {
  description = "Path to attached volume"
  value       = linode_volume_attachment.simpledb_attachment.config_path
}
```

### Step 2: Deployment Scripts

**File: `terraform/linode/scripts/setup.sh`**
```bash
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
```

**File: `terraform/linode/scripts/deploy_db.sh`**
```bash
#!/bin/bash
set -e

echo "=== Deploying SimpleDB ==="

# Clone repository
cd /opt
git clone https://github.com/EdwardPlata/accelerated-data-engineering.git
cd accelerated-data-engineering/examples/database

# Build SimpleDB
echo "Building SimpleDB..."
mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)

# Create systemd service
cat > /etc/systemd/system/simpledb.service <<EOF
[Unit]
Description=SimpleDB High-Performance C++ Database
After=network.target

[Service]
Type=simple
User=root
WorkingDirectory=/opt/accelerated-data-engineering/examples/database/build
ExecStart=/opt/accelerated-data-engineering/examples/database/build/simple_db
Restart=always
RestartSec=10
StandardInput=tty-force
StandardOutput=journal
StandardError=journal

# Performance settings
LimitNOFILE=65536
LimitNPROC=32768

# Security settings
NoNewPrivileges=true
PrivateTmp=true

[Install]
WantedBy=multi-user.target
EOF

# Enable and start service
systemctl daemon-reload
systemctl enable simpledb
systemctl start simpledb

echo "=== SimpleDB deployed and running ==="
echo "Status: systemctl status simpledb"
echo "Logs: journalctl -u simpledb -f"
```

### Step 3: Deploy Infrastructure

```bash
# Navigate to Linode terraform directory
cd terraform/linode

# Initialize Terraform
terraform init

# Create terraform.tfvars file with your credentials
cat > terraform.tfvars <<EOF
linode_token    = "YOUR_LINODE_API_TOKEN"
root_password   = "YOUR_SECURE_PASSWORD"
ssh_public_key  = "YOUR_SSH_PUBLIC_KEY"
region          = "us-east"
allowed_ips     = ["YOUR_IP_ADDRESS/32"]
EOF

# Plan deployment
terraform plan

# Apply configuration
terraform apply

# Get outputs
terraform output
```

### Step 4: Connect and Verify

```bash
# Get server IP from Terraform output
SERVER_IP=$(terraform output -raw server_ip)

# SSH into server
ssh root@$SERVER_IP

# Check SimpleDB status
systemctl status simpledb

# Test database connection
cd /opt/accelerated-data-engineering/examples/database/build
./simple_db

# Run performance benchmarks
simpledb> CREATE TABLE benchmark (id int, data string, value double)
simpledb> INSERT INTO benchmark VALUES (1, test_data, 123.45)
simpledb> SELECT * FROM benchmark
```

### Performance Optimization for Linode

```bash
# CPU pinning for SimpleDB process
# Pin to specific cores for consistency
taskset -cp 0-3 $(pidof simple_db)

# Use huge pages for better memory performance
echo 1024 > /proc/sys/vm/nr_hugepages

# Monitor performance
# CPU usage
htop

# Disk I/O
iotop

# Network
iftop

# System statistics
sar -u 1 10  # CPU
sar -r 1 10  # Memory
sar -d 1 10  # Disk
```

---

## DigitalOcean Bare Metal Deployment

### Overview
Deploy SimpleDB on DigitalOcean's Dedicated CPU Droplets for development and production workloads.

### Step 1: DigitalOcean Terraform Configuration

**File: `terraform/digitalocean/main.tf`**
```hcl
terraform {
  required_version = ">= 1.0"
  
  required_providers {
    digitalocean = {
      source  = "digitalocean/digitalocean"
      version = "~> 2.30"
    }
  }
}

provider "digitalocean" {
  token = var.do_token
}

# Dedicated CPU Droplet for SimpleDB
resource "digitalocean_droplet" "simpledb_server" {
  name     = "simpledb-production"
  region   = var.region
  size     = "c-8"  # 8 dedicated vCPUs, 16GB RAM
  image    = "ubuntu-22-04-x64"
  
  ssh_keys = [digitalocean_ssh_key.default.id]
  
  tags = ["production", "database", "cpp"]
  
  # Enable monitoring
  monitoring = true
  
  # Enable backups
  backups = true
  
  # Enable IPv6
  ipv6 = true
  
  # User data for initial setup
  user_data = file("${path.module}/scripts/cloud-init.yaml")
}

# SSH key
resource "digitalocean_ssh_key" "default" {
  name       = "simpledb-key"
  public_key = var.ssh_public_key
}

# Block storage volume
resource "digitalocean_volume" "simpledb_data" {
  region                  = var.region
  name                    = "simpledb-data-volume"
  size                    = 100  # 100 GB
  initial_filesystem_type = "ext4"
  description             = "SimpleDB data volume"
}

resource "digitalocean_volume_attachment" "simpledb_attachment" {
  droplet_id = digitalocean_droplet.simpledb_server.id
  volume_id  = digitalocean_volume.simpledb_data.id
}

# VPC for private networking
resource "digitalocean_vpc" "simpledb_vpc" {
  name   = "simpledb-vpc"
  region = var.region
}
```

**File: `terraform/digitalocean/networking.tf`**
```hcl
# Cloud Firewall
resource "digitalocean_firewall" "simpledb_firewall" {
  name = "simpledb-firewall"
  
  droplet_ids = [digitalocean_droplet.simpledb_server.id]
  
  # SSH access
  inbound_rule {
    protocol         = "tcp"
    port_range       = "22"
    source_addresses = var.allowed_ips
  }
  
  # SimpleDB access
  inbound_rule {
    protocol         = "tcp"
    port_range       = tostring(var.db_port)
    source_addresses = var.allowed_ips
  }
  
  # Monitoring (Prometheus)
  inbound_rule {
    protocol         = "tcp"
    port_range       = "9090"
    source_addresses = var.allowed_ips
  }
  
  # Grafana
  inbound_rule {
    protocol         = "tcp"
    port_range       = "3000"
    source_addresses = var.allowed_ips
  }
  
  # Outbound - allow all
  outbound_rule {
    protocol              = "tcp"
    port_range            = "1-65535"
    destination_addresses = ["0.0.0.0/0", "::/0"]
  }
  
  outbound_rule {
    protocol              = "udp"
    port_range            = "1-65535"
    destination_addresses = ["0.0.0.0/0", "::/0"]
  }
  
  outbound_rule {
    protocol              = "icmp"
    destination_addresses = ["0.0.0.0/0", "::/0"]
  }
}

# Load balancer for high availability (optional)
resource "digitalocean_loadbalancer" "simpledb_lb" {
  name   = "simpledb-lb"
  region = var.region
  
  forwarding_rule {
    entry_port     = var.db_port
    entry_protocol = "tcp"
    
    target_port     = var.db_port
    target_protocol = "tcp"
  }
  
  healthcheck {
    port     = var.db_port
    protocol = "tcp"
  }
  
  droplet_ids = [digitalocean_droplet.simpledb_server.id]
}
```

**File: `terraform/digitalocean/variables.tf`**
```hcl
variable "do_token" {
  description = "DigitalOcean API Token"
  type        = string
  sensitive   = true
}

variable "region" {
  description = "DigitalOcean region"
  type        = string
  default     = "nyc3"
}

variable "ssh_public_key" {
  description = "SSH public key for authentication"
  type        = string
}

variable "allowed_ips" {
  description = "IP addresses allowed to connect"
  type        = list(string)
  default     = []
}

variable "db_port" {
  description = "SimpleDB port"
  type        = number
  default     = 9999
}
```

**File: `terraform/digitalocean/outputs.tf`**
```hcl
output "droplet_ip" {
  description = "Public IP address of SimpleDB droplet"
  value       = digitalocean_droplet.simpledb_server.ipv4_address
}

output "droplet_id" {
  description = "Droplet ID"
  value       = digitalocean_droplet.simpledb_server.id
}

output "private_ip" {
  description = "Private IP address"
  value       = digitalocean_droplet.simpledb_server.ipv4_address_private
}

output "volume_path" {
  description = "Path to attached volume"
  value       = "/dev/disk/by-id/scsi-0DO_Volume_${digitalocean_volume.simpledb_data.name}"
}

output "load_balancer_ip" {
  description = "Load balancer IP address"
  value       = digitalocean_loadbalancer.simpledb_lb.ip
}

output "ssh_command" {
  description = "SSH command to connect"
  value       = "ssh root@${digitalocean_droplet.simpledb_server.ipv4_address}"
}
```

### Step 2: Cloud-Init Configuration

**File: `terraform/digitalocean/scripts/cloud-init.yaml`**
```yaml
#cloud-config

package_update: true
package_upgrade: true

packages:
  - build-essential
  - cmake
  - git
  - curl
  - wget
  - htop
  - iotop
  - net-tools
  - sysstat
  - gcc-12
  - g++-12
  - clang-14
  - prometheus-node-exporter

write_files:
  - path: /etc/sysctl.d/99-simpledb.conf
    content: |
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

runcmd:
  - sysctl -p /etc/sysctl.d/99-simpledb.conf
  - update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-12 100
  - update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-12 100
  - mkdir -p /data/simpledb /data/logs /data/backups
  - echo "never" > /sys/kernel/mm/transparent_hugepage/enabled
  - systemctl enable prometheus-node-exporter
  - systemctl start prometheus-node-exporter
```

### Step 3: Deployment

```bash
# Navigate to DigitalOcean terraform directory
cd terraform/digitalocean

# Initialize Terraform
terraform init

# Create terraform.tfvars
cat > terraform.tfvars <<EOF
do_token       = "YOUR_DIGITALOCEAN_API_TOKEN"
ssh_public_key = "YOUR_SSH_PUBLIC_KEY"
region         = "nyc3"
allowed_ips    = ["YOUR_IP_ADDRESS/32"]
EOF

# Deploy
terraform plan
terraform apply

# Get droplet IP
DROPLET_IP=$(terraform output -raw droplet_ip)

# Connect and deploy SimpleDB
ssh root@$DROPLET_IP

# On the server:
cd /opt
git clone https://github.com/EdwardPlata/accelerated-data-engineering.git
cd accelerated-data-engineering/examples/database
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)

# Create and start service
# (Use the same systemd service from Linode example)
```

---

## Weights & Biases ML Deployment

### Overview
Deploy C++ machine learning applications with Weights & Biases for experiment tracking and model monitoring.

### Use Case: ML-Enhanced Database Query Optimization

This example shows how to use W&B with C++ applications for machine learning workloads.

**File: `terraform/wandb/main.tf`**
```hcl
terraform {
  required_version = ">= 1.0"
  
  required_providers {
    aws = {
      source  = "hashicorp/aws"
      version = "~> 5.0"
    }
  }
}

provider "aws" {
  region = var.region
}

# EC2 instance with GPU for ML workloads
resource "aws_instance" "wandb_ml_server" {
  ami           = "ami-0c55b159cbfafe1f0"  # Ubuntu 22.04 with GPU support
  instance_type = "g4dn.xlarge"  # GPU instance for ML
  
  key_name = aws_key_pair.deployer.key_name
  
  vpc_security_group_ids = [aws_security_group.wandb_sg.id]
  
  root_block_device {
    volume_size = 100
    volume_type = "gp3"
  }
  
  user_data = templatefile("${path.module}/scripts/setup-wandb.sh", {
    wandb_api_key = var.wandb_api_key
  })
  
  tags = {
    Name        = "wandb-ml-server"
    Environment = "production"
    Purpose     = "ml-training"
  }
}

resource "aws_key_pair" "deployer" {
  key_name   = "wandb-deployer-key"
  public_key = var.ssh_public_key
}

resource "aws_security_group" "wandb_sg" {
  name        = "wandb-security-group"
  description = "Security group for W&B ML server"
  
  ingress {
    from_port   = 22
    to_port     = 22
    protocol    = "tcp"
    cidr_blocks = var.allowed_ips
  }
  
  ingress {
    from_port   = 8080
    to_port     = 8080
    protocol    = "tcp"
    cidr_blocks = var.allowed_ips
  }
  
  egress {
    from_port   = 0
    to_port     = 0
    protocol    = "-1"
    cidr_blocks = ["0.0.0.0/0"]
  }
}
```

**File: `terraform/wandb/scripts/setup-wandb.sh`**
```bash
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
```

### C++ Integration with Weights & Biases

**File: `examples/wandb/ml_query_optimizer.cpp`**
```cpp
#include <iostream>
#include <string>
#include <cstdlib>
#include <chrono>
#include <fstream>

class WandBLogger {
private:
    std::string run_id;
    std::string project_name;
    bool enabled;
    
public:
    WandBLogger(const std::string& project, bool enable = true) 
        : project_name(project), enabled(enable) {
        if (enabled) {
            // Initialize W&B run via Python API
            std::string command = "python3 -c \"import wandb; "
                                "run = wandb.init(project='" + project + "'); "
                                "print(run.id)\"";
            FILE* pipe = popen(command.c_str(), "r");
            if (pipe) {
                char buffer[128];
                if (fgets(buffer, sizeof(buffer), pipe)) {
                    run_id = std::string(buffer);
                    run_id.erase(run_id.find_last_not_of("\n\r") + 1);
                }
                pclose(pipe);
            }
            std::cout << "W&B Run ID: " << run_id << std::endl;
        }
    }
    
    void log(const std::string& key, double value, int step = 0) {
        if (!enabled) return;
        
        std::string command = "python3 -c \"import wandb; "
                            "wandb.init(id='" + run_id + "', resume='allow'); "
                            "wandb.log({'" + key + "': " + std::to_string(value) + 
                            ", 'step': " + std::to_string(step) + "})\"";
        system(command.c_str());
    }
    
    void finish() {
        if (!enabled) return;
        system("python3 -c \"import wandb; wandb.finish()\"");
    }
};

// ML-based query optimizer example
class QueryOptimizer {
private:
    WandBLogger logger;
    
public:
    QueryOptimizer() : logger("simpledb-query-optimization") {}
    
    double optimize_query(const std::string& query) {
        auto start = std::chrono::high_resolution_clock::now();
        
        // Simulate query optimization with ML
        // In practice, this would use trained models
        double optimization_score = 0.85;
        
        // Simulate query execution
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        auto end = std::chrono::high_resolution_clock::now();
        double execution_time = std::chrono::duration<double, std::milli>(end - start).count();
        
        // Log metrics to W&B
        logger.log("execution_time_ms", execution_time);
        logger.log("optimization_score", optimization_score);
        logger.log("query_length", query.length());
        
        return optimization_score;
    }
    
    void train_model(int epochs) {
        std::cout << "Training query optimization model..." << std::endl;
        
        for (int epoch = 0; epoch < epochs; ++epoch) {
            // Simulate training
            double loss = 1.0 / (epoch + 1);  // Decreasing loss
            double accuracy = 1.0 - loss;
            
            logger.log("train_loss", loss, epoch);
            logger.log("train_accuracy", accuracy, epoch);
            
            std::cout << "Epoch " << epoch << ": loss=" << loss 
                     << ", accuracy=" << accuracy << std::endl;
        }
        
        logger.finish();
    }
};

int main() {
    QueryOptimizer optimizer;
    
    // Train the model
    optimizer.train_model(10);
    
    // Test optimization
    std::string test_query = "SELECT * FROM users WHERE age > 25";
    double score = optimizer.optimize_query(test_query);
    
    std::cout << "Optimization score: " << score << std::endl;
    
    return 0;
}
```

### Building and Running with W&B

```bash
# Build the ML optimizer
cd /opt/accelerated-data-engineering/examples/wandb
mkdir -p build && cd build
cmake ..
make

# Run with W&B tracking
export WANDB_API_KEY="your-api-key"
./ml_query_optimizer

# View results at https://wandb.ai/your-username/simpledb-query-optimization
```

---

## Deploying SimpleDB on Bare Metal

### Complete End-to-End Deployment Guide

This section provides a comprehensive, step-by-step guide to deploy SimpleDB on bare metal infrastructure.

### Architecture Overview

```
┌─────────────────────────────────────────────────────┐
│                   Load Balancer                      │
│              (Optional for HA setup)                 │
└──────────────────┬──────────────────────────────────┘
                   │
        ┌──────────┴──────────┐
        │                     │
┌───────▼─────────┐   ┌───────▼─────────┐
│  SimpleDB       │   │  SimpleDB       │
│  Primary Node   │   │  Replica Node   │
│                 │   │  (Optional)     │
└────────┬────────┘   └────────┬────────┘
         │                     │
         └──────────┬──────────┘
                    │
         ┌──────────▼──────────┐
         │  Monitoring Stack   │
         │  Prometheus/Grafana │
         └─────────────────────┘
```

### Deployment Steps

#### 1. Choose Your Cloud Provider

Based on your requirements:

- **Linode**: Best for predictable performance, simple pricing
- **DigitalOcean**: Best for developer experience, quick setup
- **AWS with W&B**: Best for ML workloads, GPU requirements

#### 2. Provision Infrastructure

```bash
# Clone the repository
git clone https://github.com/EdwardPlata/accelerated-data-engineering.git
cd accelerated-data-engineering

# Choose provider and navigate to terraform directory
cd terraform/linode  # or digitalocean, or wandb

# Configure variables
cp terraform.tfvars.example terraform.tfvars
# Edit terraform.tfvars with your credentials

# Deploy
terraform init
terraform plan
terraform apply -auto-approve

# Save outputs
terraform output > deployment_info.txt
```

#### 3. Initial Server Configuration

```bash
# Get server IP
SERVER_IP=$(terraform output -raw server_ip)

# SSH into server
ssh root@$SERVER_IP

# Verify system resources
free -h              # Memory
lscpu               # CPU
df -h               # Disk
ip addr             # Network

# Check performance settings
cat /sys/kernel/mm/transparent_hugepage/enabled  # Should be [never]
sysctl vm.swappiness  # Should be 10
```

#### 4. Build SimpleDB

```bash
# On the server:
cd /opt/accelerated-data-engineering/examples/database

# Create optimized build
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_CXX_FLAGS="-O3 -march=native -mtune=native" \
      ..
make -j$(nproc)

# Verify build
./simple_db --version
./simple_db --help
```

#### 5. Configure as System Service

```bash
# Create systemd service file
cat > /etc/systemd/system/simpledb.service <<'EOF'
[Unit]
Description=SimpleDB High-Performance C++ Database
After=network.target
Documentation=https://github.com/EdwardPlata/accelerated-data-engineering

[Service]
Type=simple
User=simpledb
Group=simpledb
WorkingDirectory=/opt/accelerated-data-engineering/examples/database/build

# Start command
# Note: Current simple_db implementation doesn't support these flags
# This is an example of what a production version would include
ExecStart=/opt/accelerated-data-engineering/examples/database/build/simple_db

# Restart policy
Restart=always
RestartSec=10

# Resource limits
LimitNOFILE=65536
LimitNPROC=32768
LimitMEMLOCK=infinity

# Security settings
NoNewPrivileges=true
PrivateTmp=true
ProtectSystem=strict
ProtectHome=true
ReadWritePaths=/data

# Performance settings
CPUSchedulingPolicy=fifo
CPUSchedulingPriority=99
IOSchedulingClass=realtime
IOSchedulingPriority=0

[Install]
WantedBy=multi-user.target
EOF

# Create simpledb user
useradd -r -s /bin/false simpledb
chown -R simpledb:simpledb /data/simpledb /data/logs

# Enable and start service
systemctl daemon-reload
systemctl enable simpledb
systemctl start simpledb

# Check status
systemctl status simpledb
journalctl -u simpledb -f
```

#### 6. Performance Tuning

```bash
# CPU affinity - pin to specific cores
systemctl set-property simpledb.service AllowedCPUs=0-7

# NUMA optimization (if applicable)
numactl --show
# Pin to NUMA node 0
systemctl set-property simpledb.service NUMAPolicy=bind NUMAMask=0

# I/O scheduler optimization
echo "deadline" > /sys/block/sda/queue/scheduler

# Network tuning for high-throughput
ethtool -G eth0 rx 4096 tx 4096
ethtool -K eth0 gro on
ethtool -K eth0 gso on
```

#### 7. Monitoring Setup

```bash
# Install Prometheus Node Exporter
apt-get install -y prometheus-node-exporter
systemctl enable prometheus-node-exporter
systemctl start prometheus-node-exporter

# Install Prometheus
wget https://github.com/prometheus/prometheus/releases/download/v2.45.0/prometheus-2.45.0.linux-amd64.tar.gz
tar xvf prometheus-2.45.0.linux-amd64.tar.gz
mv prometheus-2.45.0.linux-amd64 /opt/prometheus

# Configure Prometheus
cat > /opt/prometheus/prometheus.yml <<'EOF'
global:
  scrape_interval: 15s
  evaluation_interval: 15s

scrape_configs:
  - job_name: 'simpledb'
    static_configs:
      - targets: ['localhost:9999']
  
  - job_name: 'node'
    static_configs:
      - targets: ['localhost:9100']
EOF

# Start Prometheus
cd /opt/prometheus
./prometheus --config.file=prometheus.yml &

# Install Grafana
apt-get install -y software-properties-common
add-apt-repository "deb https://packages.grafana.com/oss/deb stable main"
wget -q -O - https://packages.grafana.com/gpg.key | apt-key add -
apt-get update
apt-get install -y grafana

systemctl enable grafana-server
systemctl start grafana-server

# Access Grafana at http://SERVER_IP:3000
# Default credentials: admin/admin
```

#### 8. Load Testing and Validation

```bash
# Create test script
cat > /tmp/load_test.sql <<'EOF'
CREATE TABLE users (id int, name string, email string, age int)
INSERT INTO users VALUES (1, Alice, alice@example.com, 30)
INSERT INTO users VALUES (2, Bob, bob@example.com, 25)
INSERT INTO users VALUES (3, Charlie, charlie@example.com, 35)
SELECT * FROM users
SELECT name, age FROM users WHERE age > 25
DROP TABLE users
EOF

# Run load test
time ./simple_db < /tmp/load_test.sql

# Benchmark with multiple connections
for i in {1..100}; do
    ./simple_db < /tmp/load_test.sql &
done
wait

# Monitor during load
htop
iotop
nethogs
```

#### 9. Backup Configuration

```bash
# Create backup script
cat > /usr/local/bin/backup-simpledb.sh <<'EOF'
#!/bin/bash
BACKUP_DIR="/data/backups"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
BACKUP_FILE="$BACKUP_DIR/simpledb_backup_$TIMESTAMP.tar.gz"

# Stop SimpleDB for consistent backup
systemctl stop simpledb

# Create backup
tar czf "$BACKUP_FILE" \
    /data/simpledb \
    /opt/accelerated-data-engineering/examples/database

# Restart SimpleDB
systemctl start simpledb

# Keep only last 7 days of backups
find "$BACKUP_DIR" -name "simpledb_backup_*.tar.gz" -mtime +7 -delete

echo "Backup completed: $BACKUP_FILE"
EOF

chmod +x /usr/local/bin/backup-simpledb.sh

# Schedule daily backups
cat > /etc/cron.d/simpledb-backup <<'EOF'
0 2 * * * root /usr/local/bin/backup-simpledb.sh >> /data/logs/backup.log 2>&1
EOF
```

#### 10. Health Checks and Monitoring

```bash
# Create health check script
cat > /usr/local/bin/simpledb-health.sh <<'EOF'
#!/bin/bash

# Check if process is running
if ! systemctl is-active --quiet simpledb; then
    echo "ERROR: SimpleDB is not running"
    systemctl start simpledb
    exit 1
fi

# Check if port is listening
if ! netstat -tuln | grep -q ":9999"; then
    echo "ERROR: SimpleDB port 9999 is not listening"
    exit 1
fi

# Check memory usage
MEMORY_USAGE=$(ps aux | grep simple_db | grep -v grep | awk '{print $4}')
if (( $(echo "$MEMORY_USAGE > 80" | bc -l) )); then
    echo "WARNING: High memory usage: ${MEMORY_USAGE}%"
fi

# Check disk space
DISK_USAGE=$(df -h /data | tail -1 | awk '{print $5}' | sed 's/%//')
if [ "$DISK_USAGE" -gt 80 ]; then
    echo "WARNING: High disk usage: ${DISK_USAGE}%"
fi

echo "SimpleDB health check: OK"
exit 0
EOF

chmod +x /usr/local/bin/simpledb-health.sh

# Run health check every 5 minutes
cat > /etc/cron.d/simpledb-health <<'EOF'
*/5 * * * * root /usr/local/bin/simpledb-health.sh >> /data/logs/health.log 2>&1
EOF
```

---

## Monitoring and Observability

### Metrics to Monitor

#### System Metrics
- **CPU**: Utilization, load average, context switches
- **Memory**: Usage, swap, cache, huge pages
- **Disk**: I/O operations, throughput, latency, queue depth
- **Network**: Bandwidth, packets, errors, connections

#### Application Metrics
- **Query Performance**: Execution time, throughput (queries/sec)
- **Database Size**: Number of tables, rows, memory usage
- **Connection Pool**: Active connections, wait time
- **Errors**: Failed queries, exceptions, crashes

### Grafana Dashboards

Create custom dashboards for SimpleDB monitoring:

```json
{
  "dashboard": {
    "title": "SimpleDB Performance",
    "panels": [
      {
        "title": "Query Throughput",
        "targets": [
          {
            "expr": "rate(simpledb_queries_total[5m])"
          }
        ]
      },
      {
        "title": "Query Latency",
        "targets": [
          {
            "expr": "histogram_quantile(0.95, rate(simpledb_query_duration_seconds_bucket[5m]))"
          }
        ]
      },
      {
        "title": "Memory Usage",
        "targets": [
          {
            "expr": "process_resident_memory_bytes{job=\"simpledb\"}"
          }
        ]
      }
    ]
  }
}
```

### Alerting Rules

```yaml
# Prometheus alerting rules
groups:
  - name: simpledb_alerts
    rules:
      - alert: HighMemoryUsage
        expr: process_resident_memory_bytes > 25000000000  # 25GB
        for: 5m
        annotations:
          summary: "High memory usage on SimpleDB"
          description: "Memory usage is above 25GB for 5 minutes"
      
      - alert: HighQueryLatency
        expr: histogram_quantile(0.95, rate(simpledb_query_duration_seconds_bucket[5m])) > 1
        for: 10m
        annotations:
          summary: "High query latency detected"
          description: "95th percentile query latency is above 1 second"
      
      - alert: SimpleDBDown
        expr: up{job="simpledb"} == 0
        for: 1m
        annotations:
          summary: "SimpleDB is down"
          description: "SimpleDB instance is not responding"
```

---

## Security Best Practices

### Network Security

```bash
# Configure firewall with UFW
ufw default deny incoming
ufw default allow outgoing
ufw allow from YOUR_IP to any port 22 proto tcp
ufw allow from YOUR_IP to any port 9999 proto tcp
ufw enable

# Disable root SSH login
sed -i 's/PermitRootLogin yes/PermitRootLogin no/' /etc/ssh/sshd_config
systemctl restart sshd

# Setup fail2ban for brute force protection
apt-get install -y fail2ban
systemctl enable fail2ban
systemctl start fail2ban
```

### Application Security

```bash
# Run SimpleDB as non-root user (already configured in systemd)
# Limit file permissions
chmod 750 /data/simpledb
chmod 640 /data/simpledb/*

# Enable SELinux or AppArmor (Ubuntu)
apt-get install -y apparmor apparmor-utils
aa-enforce /etc/apparmor.d/*

# Regular security updates
apt-get install -y unattended-upgrades
dpkg-reconfigure -plow unattended-upgrades
```

### Data Security

```bash
# Encrypt data at rest
apt-get install -y cryptsetup

# Encrypt volume
cryptsetup luksFormat /dev/sdb
cryptsetup open /dev/sdb simpledb_encrypted
mkfs.ext4 /dev/mapper/simpledb_encrypted
mount /dev/mapper/simpledb_encrypted /data

# Setup automatic unlock
echo "simpledb_encrypted /dev/sdb /root/.keyfile luks" >> /etc/crypttab

# Encrypt backups
gpg --output backup.tar.gz.gpg --encrypt --recipient your-email@example.com backup.tar.gz
```

---

## Cost Optimization

### Cloud Provider Cost Comparison

| Provider | Instance Type | vCPUs | RAM | Storage | Monthly Cost |
|----------|--------------|-------|-----|---------|--------------|
| Linode | Dedicated 8GB | 8 | 32GB | 640GB SSD | ~$240 |
| DigitalOcean | c-8 | 8 | 16GB | 200GB SSD | ~$336 |
| AWS EC2 | c6i.2xlarge | 8 | 16GB | 100GB EBS | ~$250 |

### Cost Optimization Strategies

1. **Right-sizing**: Start with smaller instances and scale up
2. **Reserved Instances**: Commit for 1-3 years for 30-50% discount
3. **Auto-scaling**: Scale down during off-peak hours
4. **Storage Optimization**: Use cheaper storage tiers for backups
5. **Data Transfer**: Minimize cross-region traffic
6. **Monitoring**: Track resource utilization to identify waste

### Example: Auto-scaling Configuration

```bash
# Scale down during night hours (00:00-06:00)
cat > /etc/cron.d/simpledb-autoscale <<'EOF'
0 0 * * * root systemctl set-property simpledb.service CPUQuota=50%
0 6 * * * root systemctl set-property simpledb.service CPUQuota=100%
EOF
```

---

## Troubleshooting

### Common Issues and Solutions

#### Issue 1: High Memory Usage

```bash
# Check memory consumption
ps aux --sort=-%mem | head -10
free -h

# Solution: Increase swap or reduce max memory
sysctl vm.swappiness=60
fallocate -l 8G /swapfile
chmod 600 /swapfile
mkswap /swapfile
swapon /swapfile
```

#### Issue 2: Poor Query Performance

```bash
# Check system load
uptime
top

# Check I/O wait
iostat -x 1 10

# Solution: Optimize disk I/O
echo "deadline" > /sys/block/sda/queue/scheduler
ionice -c1 -n0 -p $(pidof simple_db)
```

#### Issue 3: Network Connectivity Issues

```bash
# Test connectivity
ping SERVER_IP
telnet SERVER_IP 9999
nc -zv SERVER_IP 9999

# Check firewall
ufw status
iptables -L -n

# Solution: Update firewall rules
ufw allow from YOUR_IP to any port 9999
```

#### Issue 4: Build Failures

```bash
# Check compiler version
gcc --version
g++ --version

# Install dependencies
apt-get install -y build-essential cmake

# Clean build
cd /opt/accelerated-data-engineering/examples/database
rm -rf build
mkdir build && cd build
cmake .. && make clean && make
```

### Debug Mode

```bash
# Enable debug logging
export SIMPLEDB_LOG_LEVEL=DEBUG
systemctl restart simpledb

# View detailed logs
journalctl -u simpledb -f --all

# Run with gdb for crash debugging
gdb --args ./simple_db
(gdb) run
(gdb) bt  # backtrace on crash
```

---

## Conclusion

This guide provides a comprehensive, end-to-end solution for deploying C++ data engineering applications, specifically SimpleDB, on bare metal infrastructure across multiple cloud providers. Key takeaways:

1. **Bare metal** provides maximum performance for C++ applications
2. **Terraform** enables reproducible, version-controlled infrastructure
3. **Multiple providers** offer different trade-offs (cost, performance, features)
4. **Monitoring** is critical for production deployments
5. **Security** must be built in from the start
6. **Cost optimization** requires continuous monitoring and adjustment

### Next Steps

1. Deploy to staging environment first
2. Run comprehensive load tests
3. Set up monitoring and alerting
4. Implement backup and disaster recovery
5. Document runbooks for operations team
6. Plan capacity for growth

### Additional Resources

- [Terraform Documentation](https://www.terraform.io/docs)
- [Linode API Documentation](https://www.linode.com/docs/api/)
- [DigitalOcean API Documentation](https://docs.digitalocean.com/reference/api/)
- [Weights & Biases Documentation](https://docs.wandb.ai/)
- [Linux Performance Tools](https://www.brendangregg.com/linuxperf.html)
- [C++ Performance Optimization](https://www.agner.org/optimize/)

---

## Support and Contributing

For issues, questions, or contributions:
- GitHub: [EdwardPlata/accelerated-data-engineering](https://github.com/EdwardPlata/accelerated-data-engineering)
- Documentation: `/docs` directory
- Examples: `/examples` directory

