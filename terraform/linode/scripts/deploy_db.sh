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
cat > /etc/systemd/system/simpledb.service <<'EOF'
[Unit]
Description=SimpleDB High-Performance C++ Database
After=network.target

[Service]
Type=simple
User=root
WorkingDirectory=/opt/accelerated-data-engineering/examples/database/build
ExecStart=/opt/accelerated-data-engineering/examples/database/build/simple_db --daemon --port=9999
Restart=always
RestartSec=10
StandardOutput=append:/data/logs/simpledb.log
StandardError=append:/data/logs/simpledb-error.log

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
