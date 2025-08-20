group "default" {
  targets = ["api", "pos", "api_tui"]
}

variable "TAG" {
  default = "local"
}

# Common settings reused by all targets
target "_common" {
  context    = "."
  dockerfile = "Dockerfile"
  platforms  = ["linux/amd64", "linux/arm64"]
  labels = {
    "org.opencontainers.image.source" = "${BAKE_ORG}/${BAKE_REPO}"
  }
}

target "api" {
  inherits = ["_common"]
  tags     = [
    "register_mvp-api:${TAG}",
  ]
  args = {
    USE_MOCK_GPIO = "ON"
  }
}

target "pos" {
  inherits = ["_common"]
  tags     = [
    "register_mvp-pos:${TAG}",
  ]
  args = {
    USE_MOCK_GPIO = "ON"
  }
}

target "api_tui" {
  inherits = ["_common"]
  tags     = [
    "register_mvp-api_tui:${TAG}",
  ]
  args = {
    USE_MOCK_GPIO = "ON"
  }
}
