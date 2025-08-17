module "infra" {
  source          = "../.."
  environment     = var.environment
  aws_region      = var.aws_region
  vpc_cidr        = var.vpc_cidr
  private_subnets = var.private_subnets
  public_subnets  = var.public_subnets
  cluster_name    = var.cluster_name
  github_repo     = var.github_repo
}

variable "environment" { type = string }
variable "aws_region" { type = string }
variable "vpc_cidr" { type = string }
variable "private_subnets" { type = list(string) }
variable "public_subnets" { type = list(string) }
variable "cluster_name" { type = string }
variable "github_repo" { type = string }
