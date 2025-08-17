environment     = "prod"
aws_region      = "us-east-1"
vpc_cidr        = "10.1.0.0/16"
private_subnets = ["10.1.1.0/24", "10.1.2.0/24", "10.1.3.0/24"]
public_subnets  = ["10.1.101.0/24", "10.1.102.0/24", "10.1.103.0/24"]
cluster_name    = "register-prod"
github_repo     = "example/register_mvp"
