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
  value       = "/dev/disk/by-id/scsi-0Linode_Volume_${linode_volume.simpledb_data.label}"
}
