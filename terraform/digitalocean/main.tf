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
