variable "aws_region" {
  description = "AWS region to deploy to"
  type        = string
}

variable "environment" {
  description = "Deployment environment (staging, prod)"
  type        = string
}

variable "vpc_cidr" {
  type = string
}

variable "private_subnets" {
  type = list(string)
}

variable "public_subnets" {
  type = list(string)
}

variable "cluster_name" {
  type = string
}

variable "github_repo" {
  description = "GitHub repository in org/repo form"
  type        = string
}
