variable "name" { type = string }
variable "cidr_block" { type = string }
variable "private_subnets" { type = list(string) }
variable "public_subnets" { type = list(string) }
variable "azs" { type = list(string) }
variable "tags" {
  type    = map(string)
  default = {}
}
