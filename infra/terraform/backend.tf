terraform {
  backend "s3" {
    bucket         = "CHANGE_ME_STATE_BUCKET"
    key            = "register/terraform.tfstate"
    region         = "us-east-1"
    dynamodb_table = "CHANGE_ME_LOCK_TABLE"
    encrypt        = true
  }
}
