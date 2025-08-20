# Release Process

This project uses [Semantic Versioning](https://semver.org/).
Follow these steps to create a new release:

1. Ensure all changes for the release are merged into `main`.
2. Update `CHANGELOG.md` with the new version and date.
3. Bump the version in `CMakeLists.txt` if needed.
4. Commit and push the changes.
5. Run the **Release** workflow from the Actions tab and provide the version (e.g. `v1.2.3`).
   The workflow will tag the commit, build the binaries, and publish a GitHub release.

## Release criteria

- All tests and linters pass.
- `CHANGELOG.md` accurately reflects user-facing changes.
- Version numbers follow `MAJOR.MINOR.PATCH` format.

