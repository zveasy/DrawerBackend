terraform {
  # Temporary local backend to allow planning/apply before S3/DynamoDB are created.
  backend "local" {
    path = "terraform.tfstate"
  }
  # Previous S3 backend (to restore when remote state is ready):
  # backend "s3" {
  #   bucket         = "CHANGE_ME_STATE_BUCKET"
  #   key            = "register/terraform.tfstate"
  #   region         = "us-east-1"
  #   dynamodb_table = "CHANGE_ME_LOCK_TABLE"
  #   encrypt        = true
  # }
}
