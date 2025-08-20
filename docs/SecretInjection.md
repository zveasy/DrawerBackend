# Secret Injection

Sensitive configuration values such as AWS certificates and keys are no longer
stored directly in the config file. Instead, `cfg::Aws` expects the names of
environment variables that point to the actual secret material.

## Usage

Set the following variables before starting the service:

```
export AWS_ROOT_CA=/etc/register-mvp/certs/AmazonRootCA1.pem
export AWS_CERT=/etc/register-mvp/certs/device.pem.crt
export AWS_KEY=/etc/register-mvp/certs/private.pem.key
```

The config file then references the variables:

```
"aws": {
  "root_ca_env": "AWS_ROOT_CA",
  "cert_env": "AWS_CERT",
  "key_env": "AWS_KEY"
}
```

Secrets are fetched at runtime via the environment and are not persisted by the
configuration loader. The loader also schedules a periodic refresh based on
`aws.rotation_check_minutes`, allowing external secret stores to rotate
credentials without restarting the service.
