# Infrastructure as Code

This directory contains Terraform modules and environment overlays for the Drawer register.

## Layout
- `modules/` reusable components (VPC, ECR, EKS, S3, IAM OIDC)
- `envs/` environment specific configurations (`staging`, `prod`)
- `tests/` lint and validation helpers

Run Terraform from an environment directory, e.g.

```bash
cd infra/terraform/envs/staging
terraform init
terraform apply
```
