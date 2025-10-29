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
