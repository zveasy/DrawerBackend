# Terraform How-To

1. Create the remote state bucket and DynamoDB table manually.
2. Configure AWS credentials.
3. Choose an environment directory (`envs/staging` or `envs/prod`).
4. Run:
```bash
terraform init
terraform plan
terraform apply
```
