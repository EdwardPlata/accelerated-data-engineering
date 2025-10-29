variable "linode_token" {
  description = "Linode API Token"
  type        = string
  sensitive   = true
}

variable "region" {
  description = "Linode region"
  type        = string
  default     = "us-east"
}

variable "root_password" {
  description = "Root password for Linode instance"
  type        = string
  sensitive   = true
}

variable "ssh_public_key" {
  description = "SSH public key for authentication"
  type        = string
}

variable "allowed_ips" {
  description = "IP addresses allowed to connect"
  type        = list(string)
  default     = []
}

variable "db_port" {
  description = "SimpleDB port"
  type        = number
  default     = 9999
}
