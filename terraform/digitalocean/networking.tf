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
