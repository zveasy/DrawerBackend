# Cluster Runbook

This document describes common operations on the EKS cluster.

## Kubeconfig
```bash
aws eks update-kubeconfig --name <cluster>
```

## Upgrades
Use `terraform apply` to update the cluster or node group.

## Rollback
If a deployment fails, use Helm to rollback:
```bash
helm -n ops rollback register-ops <rev>
```
