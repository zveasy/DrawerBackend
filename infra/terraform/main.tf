module "vpc" {
  source          = "./modules/vpc"
  name            = "register-${var.environment}"
  cidr_block      = var.vpc_cidr
  private_subnets = var.private_subnets
  public_subnets  = var.public_subnets
  azs             = data.aws_availability_zones.available.names
  tags            = { Environment = var.environment }
}

data "aws_availability_zones" "available" {}

module "ecr" {
  source = "./modules/ecr"
  name   = "register-ops"
}

module "eks" {
  source             = "./modules/eks"
  cluster_name       = var.cluster_name
  private_subnet_ids = module.vpc.private_subnet_ids
  public_subnet_ids  = module.vpc.public_subnet_ids
  tags               = { Environment = var.environment }
}

module "s3_ota" {
  source      = "./modules/s3_bucket"
  bucket_name = "register-ota-${var.environment}"
}

module "github_oidc" {
  source = "./modules/iam_github_oidc"
  repo   = var.github_repo
}
