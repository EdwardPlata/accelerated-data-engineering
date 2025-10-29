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
}
