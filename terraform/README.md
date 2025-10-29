# Terraform Infrastructure for SimpleDB Cloud Deployment

This directory contains Terraform configurations for deploying SimpleDB and related C++ applications on bare metal infrastructure across multiple cloud providers.

## Directory Structure

```
terraform/
├── linode/              # Linode bare metal deployment
│   ├── main.tf
│   ├── variables.tf
│   ├── outputs.tf
│   ├── networking.tf
│   ├── terraform.tfvars.example
│   └── scripts/
│       ├── setup.sh
│       └── deploy_db.sh
├── digitalocean/        # DigitalOcean deployment
│   ├── main.tf
│   ├── variables.tf
│   ├── outputs.tf
│   ├── networking.tf
│   ├── terraform.tfvars.example
│   └── scripts/
│       └── cloud-init.yaml
└── wandb/               # W&B ML deployment (AWS)
    ├── main.tf
    ├── variables.tf
    ├── outputs.tf
    ├── terraform.tfvars.example
    └── scripts/
        └── setup-wandb.sh
```

## Prerequisites

### 1. Install Terraform

**Linux:**
```bash
wget https://releases.hashicorp.com/terraform/1.6.0/terraform_1.6.0_linux_amd64.zip
unzip terraform_1.6.0_linux_amd64.zip
sudo mv terraform /usr/local/bin/
terraform version
```

**macOS:**
```bash
brew tap hashicorp/tap
brew install hashicorp/tap/terraform
terraform version
```

**Windows:**
Download from https://www.terraform.io/downloads and add to PATH.

### 2. Get API Credentials

**Linode:**
1. Log in to Linode Cloud Manager
2. Navigate to API Tokens
3. Create a new Personal Access Token with read/write permissions

**DigitalOcean:**
1. Log in to DigitalOcean
2. Navigate to API → Tokens/Keys
3. Generate New Token with read and write scopes

**AWS (for W&B):**
1. Log in to AWS Console
2. Navigate to IAM → Users → Your User → Security Credentials
3. Create Access Key

**Weights & Biases:**
1. Sign up at https://wandb.ai
2. Navigate to Settings → API Keys
3. Copy your API key

### 3. Generate SSH Keys

```bash
# Generate new SSH key pair
ssh-keygen -t rsa -b 4096 -C "your-email@example.com" -f ~/.ssh/simpledb_key

# View public key
cat ~/.ssh/simpledb_key.pub
```

## Quick Start

### Option 1: Linode Deployment

```bash
# Navigate to Linode directory
cd terraform/linode

# Copy and configure variables
cp terraform.tfvars.example terraform.tfvars
# Edit terraform.tfvars with your credentials

# Initialize Terraform
terraform init

# Preview changes
terraform plan

# Deploy infrastructure
terraform apply

# Get connection info
terraform output
```

### Option 2: DigitalOcean Deployment

```bash
# Navigate to DigitalOcean directory
cd terraform/digitalocean

# Copy and configure variables
cp terraform.tfvars.example terraform.tfvars
# Edit terraform.tfvars with your credentials

# Initialize and deploy
terraform init
terraform plan
terraform apply

# Get connection info
terraform output
```

### Option 3: W&B ML Deployment

```bash
# Navigate to W&B directory
cd terraform/wandb

# Copy and configure variables
cp terraform.tfvars.example terraform.tfvars
# Edit terraform.tfvars with your credentials

# Set AWS credentials
export AWS_ACCESS_KEY_ID="your-access-key"
export AWS_SECRET_ACCESS_KEY="your-secret-key"

# Initialize and deploy
terraform init
terraform plan
terraform apply

# Get connection info
terraform output
```

## Configuration Details

### Linode Configuration

**Instance Type:** g6-dedicated-8
- 8 dedicated CPU cores
- 32GB RAM
- 640GB SSD storage
- Additional 100GB block storage volume

**Monthly Cost:** ~$240

**Best For:** Production databases requiring consistent performance

### DigitalOcean Configuration

**Instance Type:** c-8 (Dedicated CPU)
- 8 dedicated vCPUs
- 16GB RAM
- 200GB SSD storage
- Additional 100GB block storage volume
- Optional load balancer

**Monthly Cost:** ~$336 (without load balancer)

**Best For:** Developer-friendly deployments, rapid iteration

### W&B/AWS Configuration

**Instance Type:** g4dn.xlarge
- 4 vCPUs
- 16GB RAM
- NVIDIA T4 GPU
- 100GB GP3 storage

**Monthly Cost:** ~$390 (on-demand)

**Best For:** ML training workloads, GPU-accelerated applications

## Post-Deployment Steps

### 1. Connect to Server

```bash
# Get IP address from Terraform output
SERVER_IP=$(terraform output -raw server_ip)  # or droplet_ip, instance_ip

# SSH into server
ssh root@$SERVER_IP  # or ubuntu@$SERVER_IP for AWS
```

### 2. Deploy SimpleDB

```bash
# On the server:
cd /opt
git clone https://github.com/EdwardPlata/accelerated-data-engineering.git
cd accelerated-data-engineering/examples/database

# Build
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)

# Test
./simple_db
```

### 3. Setup as Service

For Linode/DigitalOcean:
```bash
# Run deployment script
bash /opt/accelerated-data-engineering/terraform/linode/scripts/deploy_db.sh

# Check status
systemctl status simpledb
```

### 4. Verify Deployment

```bash
# Check if SimpleDB is running
ps aux | grep simple_db

# Test connection
telnet localhost 9999

# View logs
journalctl -u simpledb -f
```

## Monitoring

### Access Monitoring Tools

**Prometheus:**
```
http://SERVER_IP:9090
```

**Grafana:**
```
http://SERVER_IP:3000
Default credentials: admin/admin
```

**Node Exporter:**
```
http://SERVER_IP:9100/metrics
```

## Maintenance

### Update Infrastructure

```bash
# Make changes to .tf files
# Plan changes
terraform plan

# Apply changes
terraform apply
```

### Backup State

```bash
# Backup Terraform state
cp terraform.tfstate terraform.tfstate.backup

# Use remote state (recommended)
terraform {
  backend "s3" {
    bucket = "my-terraform-state"
    key    = "simpledb/terraform.tfstate"
    region = "us-east-1"
  }
}
```

### Destroy Infrastructure

```bash
# Preview what will be destroyed
terraform plan -destroy

# Destroy all resources
terraform destroy

# Destroy specific resource
terraform destroy -target=linode_instance.simpledb_server
```

## Security Best Practices

1. **Never commit credentials:**
   ```bash
   # Add to .gitignore
   echo "*.tfvars" >> .gitignore
   echo ".terraform/" >> .gitignore
   echo "terraform.tfstate*" >> .gitignore
   ```

2. **Use environment variables:**
   ```bash
   export TF_VAR_linode_token="your-token"
   export TF_VAR_root_password="your-password"
   ```

3. **Restrict IP access:**
   ```hcl
   # In terraform.tfvars
   allowed_ips = ["YOUR_IP/32"]
   ```

4. **Enable encryption:**
   - Use encrypted volumes
   - Enable SSL/TLS for connections
   - Rotate credentials regularly

## Troubleshooting

### Common Issues

**Issue: Terraform init fails**
```bash
# Solution: Clear cache and reinitialize
rm -rf .terraform .terraform.lock.hcl
terraform init
```

**Issue: Provider authentication error**
```bash
# Solution: Verify credentials
terraform validate
# Check environment variables
env | grep TF_VAR
```

**Issue: Resource already exists**
```bash
# Solution: Import existing resource
terraform import linode_instance.simpledb_server INSTANCE_ID
```

**Issue: State lock error**
```bash
# Solution: Force unlock (use carefully)
terraform force-unlock LOCK_ID
```

### Enable Debug Logging

```bash
# Enable detailed logging
export TF_LOG=DEBUG
terraform apply

# Log to file
export TF_LOG_PATH=terraform-debug.log
terraform apply
```

## Cost Optimization

### 1. Use Reserved Instances

Save 30-50% by committing to 1-3 year terms.

### 2. Auto-scaling

```bash
# Scale down during off-hours
# Add to crontab
0 22 * * * terraform apply -auto-approve -var="instance_count=1"
0 6 * * * terraform apply -auto-approve -var="instance_count=3"
```

### 3. Spot Instances (AWS)

For non-critical workloads:
```hcl
resource "aws_spot_instance_request" "wandb_ml_spot" {
  ami           = var.ami_id
  instance_type = "g4dn.xlarge"
  spot_price    = "0.30"
  # ...
}
```

### 4. Monitor Usage

```bash
# Linode: View invoice
linode-cli account invoices-list

# DigitalOcean: View usage
doctl account get

# AWS: Enable cost explorer
# View at: https://console.aws.amazon.com/cost-management/
```

## Advanced Features

### Multi-Region Deployment

```hcl
# Deploy to multiple regions
module "us_east" {
  source = "./linode"
  region = "us-east"
}

module "eu_west" {
  source = "./linode"
  region = "eu-west"
}
```

### High Availability Setup

```hcl
# Create multiple instances
resource "linode_instance" "simpledb_cluster" {
  count = 3
  label = "simpledb-node-${count.index}"
  # ...
}
```

### Automated Backups

```hcl
# Linode backup schedule
resource "linode_instance" "simpledb_server" {
  backups_enabled = true
  backups {
    enabled  = true
    schedule {
      day    = "Saturday"
      window = "W22"
    }
  }
}
```

## References

- [Cloud Development Guide](../docs/cloud_development.md)
- [SimpleDB Documentation](../examples/database/README.md)
- [Terraform Documentation](https://www.terraform.io/docs)
- [Linode Provider](https://registry.terraform.io/providers/linode/linode/latest/docs)
- [DigitalOcean Provider](https://registry.terraform.io/providers/digitalocean/digitalocean/latest/docs)
- [AWS Provider](https://registry.terraform.io/providers/hashicorp/aws/latest/docs)

## Support

For issues or questions:
- GitHub Issues: [Create Issue](https://github.com/EdwardPlata/accelerated-data-engineering/issues)
- Documentation: [docs/](../docs/)
- Examples: [examples/](../examples/)
