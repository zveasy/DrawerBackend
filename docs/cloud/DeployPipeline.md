# Deploy Pipeline

GitHub Actions builds the ops API image and deploys it via Helm.

1. Push to `main` triggers workflow.
2. Workflow assumes AWS role via OIDC.
3. Docker image built and pushed to ECR.
4. Helm upgrade installs chart to EKS cluster.
