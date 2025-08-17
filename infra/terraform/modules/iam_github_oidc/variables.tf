variable "repo" {
  description = "GitHub repo in org/name"
  type        = string
}

variable "role_name" {
  type    = string
  default = "register-github-oidc"
}
