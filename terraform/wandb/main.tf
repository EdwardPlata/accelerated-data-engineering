terraform {
  required_version = ">= 1.0"
  
  required_providers {
    aws = {
      source  = "hashicorp/aws"
      version = "~> 5.0"
    }
  }
}

provider "aws" {
  region = var.region
}

# EC2 instance with GPU for ML workloads
resource "aws_instance" "wandb_ml_server" {
  ami           = var.ami_id
  instance_type = "g4dn.xlarge"  # GPU instance for ML
  
  key_name = aws_key_pair.deployer.key_name
  
  vpc_security_group_ids = [aws_security_group.wandb_sg.id]
  
  root_block_device {
    volume_size = 100
    volume_type = "gp3"
  }
  
  user_data = templatefile("${path.module}/scripts/setup-wandb.sh", {
    wandb_api_key = var.wandb_api_key
  })
  
  tags = {
    Name        = "wandb-ml-server"
    Environment = "production"
    Purpose     = "ml-training"
  }
}

resource "aws_key_pair" "deployer" {
  key_name   = "wandb-deployer-key"
  public_key = var.ssh_public_key
}

resource "aws_security_group" "wandb_sg" {
  name        = "wandb-security-group"
  description = "Security group for W&B ML server"
  
  ingress {
    from_port   = 22
    to_port     = 22
    protocol    = "tcp"
    cidr_blocks = var.allowed_ips
  }
  
  ingress {
    from_port   = 8080
    to_port     = 8080
    protocol    = "tcp"
    cidr_blocks = var.allowed_ips
  }
  
  egress {
    from_port   = 0
    to_port     = 0
    protocol    = "-1"
    cidr_blocks = ["0.0.0.0/0"]
  }
  
  tags = {
    Name = "wandb-security-group"
  }
}
