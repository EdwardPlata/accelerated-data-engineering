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

output "ssh_command" {
  description = "SSH command to connect"
  value       = "ssh root@${digitalocean_droplet.simpledb_server.ipv4_address}"
}
