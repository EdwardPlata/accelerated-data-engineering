variable "do_token" {
  description = "DigitalOcean API Token"
  type        = string
  sensitive   = true
}

variable "region" {
  description = "DigitalOcean region"
  type        = string
  default     = "nyc3"
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
