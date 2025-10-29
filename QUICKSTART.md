# Quick Start Guide: Deploying SimpleDB on Bare Metal

This quick reference guide helps you deploy the SimpleDB C++ database on bare metal infrastructure.

## 1. Choose Your Cloud Provider

| Provider | Best For | Monthly Cost | CPU | RAM |
|----------|----------|-------------|-----|-----|
| **Linode** | Production, consistent performance | ~$240 | 8 cores | 32GB |
| **DigitalOcean** | Development, quick setup | ~$336 | 8 vCPUs | 16GB |
| **AWS + W&B** | ML workloads, GPU compute | ~$390 | 4 vCPUs + GPU | 16GB |

## 2. Prerequisites Checklist

- [ ] API token from your chosen provider
- [ ] SSH key pair generated
- [ ] Terraform installed (v1.0+)
- [ ] Your public IP address for firewall rules
- [ ] Git installed

## 3. Five-Minute Deployment

### Step 1: Clone Repository
```bash
git clone https://github.com/EdwardPlata/accelerated-data-engineering.git
cd accelerated-data-engineering
```

### Step 2: Configure Provider

**Linode:**
```bash
cd terraform/linode
cp terraform.tfvars.example terraform.tfvars
# Edit terraform.tfvars with your credentials
```

**DigitalOcean:**
```bash
cd terraform/digitalocean
cp terraform.tfvars.example terraform.tfvars
# Edit terraform.tfvars with your credentials
```

**AWS + W&B:**
```bash
cd terraform/wandb
cp terraform.tfvars.example terraform.tfvars
# Edit terraform.tfvars with your credentials
export AWS_ACCESS_KEY_ID="your-key"
export AWS_SECRET_ACCESS_KEY="your-secret"
```

### Step 3: Deploy
```bash
terraform init
terraform apply -auto-approve
```

### Step 4: Connect
```bash
# Get server IP
SERVER_IP=$(terraform output -raw server_ip)

# SSH into server
ssh root@$SERVER_IP
```

### Step 5: Deploy SimpleDB
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

## 4. Production Setup

### Setup as System Service
```bash
# Create service file
sudo tee /etc/systemd/system/simpledb.service > /dev/null <<EOF
[Unit]
Description=SimpleDB C++ Database
After=network.target

[Service]
Type=simple
User=root
WorkingDirectory=/opt/accelerated-data-engineering/examples/database/build
ExecStart=/opt/accelerated-data-engineering/examples/database/build/simple_db --daemon --port=9999
Restart=always

[Install]
WantedBy=multi-user.target
EOF

# Enable and start
sudo systemctl daemon-reload
sudo systemctl enable simpledb
sudo systemctl start simpledb

# Check status
sudo systemctl status simpledb
```

### Enable Monitoring
```bash
# Access Grafana
http://SERVER_IP:3000
# Default: admin/admin

# View metrics
http://SERVER_IP:9090
```

## 5. Common Operations

### View Logs
```bash
journalctl -u simpledb -f
```

### Restart Service
```bash
sudo systemctl restart simpledb
```

### Backup Data
```bash
tar czf simpledb-backup-$(date +%Y%m%d).tar.gz /data/simpledb
```

### Update Infrastructure
```bash
cd terraform/[provider]
terraform plan
terraform apply
```

### Destroy Infrastructure
```bash
terraform destroy
```

## 6. Performance Tuning

### Check CPU Usage
```bash
htop
```

### Check Memory
```bash
free -h
```

### Check Disk I/O
```bash
iotop
```

### Optimize for Production
```bash
# Pin to specific CPUs
taskset -cp 0-7 $(pidof simple_db)

# Enable huge pages
echo 1024 > /proc/sys/vm/nr_hugepages
```

## 7. Troubleshooting

### Service Won't Start
```bash
# Check logs
journalctl -u simpledb -n 50

# Check if port is in use
netstat -tuln | grep 9999

# Test manually
cd /opt/accelerated-data-engineering/examples/database/build
./simple_db
```

### Can't Connect
```bash
# Check firewall
ufw status

# Test from local machine
telnet SERVER_IP 9999
nc -zv SERVER_IP 9999
```

### High Memory Usage
```bash
# Check process memory
ps aux | grep simple_db | awk '{print $4, $11}'

# Restart service
sudo systemctl restart simpledb
```

## 8. Next Steps

- [ ] Set up automated backups
- [ ] Configure SSL/TLS
- [ ] Set up high availability (multiple nodes)
- [ ] Implement monitoring alerts
- [ ] Configure log rotation
- [ ] Set up CI/CD pipeline
- [ ] Performance benchmarking
- [ ] Security hardening

## 9. Documentation Links

- **[Full Cloud Development Guide](docs/cloud_development.md)**
- **[Terraform Documentation](terraform/README.md)**
- **[SimpleDB Documentation](examples/database/README.md)**
- **[W&B ML Example](examples/wandb/README.md)**

## 10. Getting Help

- **Issues**: [GitHub Issues](https://github.com/EdwardPlata/accelerated-data-engineering/issues)
- **Documentation**: Check `/docs` directory
- **Examples**: Review `/examples` for code samples

## Cost Estimates

### Monthly Costs by Configuration

**Minimal (Development):**
- Linode: $10-20/month (Shared CPU)
- DigitalOcean: $12-24/month (Basic Droplet)
- Perfect for: Testing, development

**Standard (Production):**
- Linode: $240/month (Dedicated 8 cores)
- DigitalOcean: $336/month (Dedicated CPU)
- Perfect for: Production databases

**High-Performance (ML/GPU):**
- AWS: $390/month (GPU instance)
- Perfect for: ML training, data analytics

### Saving Money

1. **Use reserved instances**: Save 30-50%
2. **Auto-scale**: Scale down during off-hours
3. **Monitor usage**: Track and optimize resources
4. **Right-size**: Start small, scale as needed

## Security Checklist

- [ ] Change default passwords
- [ ] Configure firewall (only allow your IP)
- [ ] Disable root SSH login
- [ ] Enable automatic security updates
- [ ] Set up SSL certificates
- [ ] Configure fail2ban
- [ ] Regular backups
- [ ] Monitor security logs

## Support

For commercial support, consulting, or questions:
- Email: support@example.com
- GitHub: [Create an issue](https://github.com/EdwardPlata/accelerated-data-engineering/issues/new)
- Documentation: [Full docs](docs/)
