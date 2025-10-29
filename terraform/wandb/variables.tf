variable "region" {
  description = "AWS region"
  type        = string
  default     = "us-east-1"
}

variable "ami_id" {
  description = "AMI ID for Ubuntu with GPU support"
  type        = string
  default     = "ami-0c55b159cbfafe1f0"  # Ubuntu 22.04
}

variable "ssh_public_key" {
  description = "SSH public key for authentication"
  type        = string
}

variable "allowed_ips" {
  description = "IP addresses allowed to connect"
  type        = list(string)
  default     = ["0.0.0.0/0"]
}

variable "wandb_api_key" {
  description = "Weights & Biases API key"
  type        = string
  sensitive   = true
}
