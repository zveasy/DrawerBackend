variable "cluster_name" { type = string }
variable "private_subnet_ids" { type = list(string) }
variable "public_subnet_ids" {
  type    = list(string)
  default = []
}
variable "node_desired" {
  type    = number
  default = 2
}
variable "node_max" {
  type    = number
  default = 3
}
variable "node_min" {
  type    = number
  default = 1
}
variable "instance_type" {
  type    = string
  default = "t3.medium"
}
variable "tags" {
  type    = map(string)
  default = {}
}
