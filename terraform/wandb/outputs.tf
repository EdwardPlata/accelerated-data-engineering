output "instance_ip" {
  description = "Public IP address of ML server"
  value       = aws_instance.wandb_ml_server.public_ip
}

output "instance_id" {
  description = "EC2 instance ID"
  value       = aws_instance.wandb_ml_server.id
}

output "ssh_command" {
  description = "SSH command to connect"
  value       = "ssh ubuntu@${aws_instance.wandb_ml_server.public_ip}"
}
