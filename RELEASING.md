# Release process

This document describes the public GitHub Release process. Runtime binaries
are built and signed in the private Chromium workspace; they are never built
from or committed to this public repository.

## Prepare

1. Choose a Semantic Versioning release number and update `CHANGELOG.md`.
2. Build the SDK from a clean, trusted Windows build environment.
3. Run the full SDK verification suite.
4. Generate the package with `package_litheview_sdk.ps1`. Do not assemble or
   edit a release ZIP manually.
5. Sign distributable Windows binaries when a code-signing certificate is
   available. Until then, release notes must clearly state that binaries are
   unsigned.

The packager must produce exactly these upload assets:

```text
litheview-sdk-<version>-win-x64.zip
litheview-sdk-<version>-win-x64.zip.sha256
```

The ZIP must include `LICENSE`, `RUNTIME-LICENSE.md`, `NOTICE`,
`THIRD_PARTY_NOTICES.txt`, `SBOM.spdx.json`, `VERSION`, `manifest.json`, and
`SHA256SUMS.txt`.

## Verify

Run the public archive validator before upload:

```powershell
./scripts/verify-release-package.ps1 `
  -ArchivePath ./litheview-sdk-0.1.0-win-x64.zip
```

For releases expected to be signed, add `-RequireSignature` to the validation
command. The archive checksum remains mandatory for every release.

Create a draft GitHub Release tagged `v<version>`, upload both files, and copy
the relevant `CHANGELOG.md` section into the release notes. State supported
Windows versions, public API compatibility, known issues, and any migration
steps.

Do not mark a release as latest when it is a preview, release candidate, or a
security test build. Do not overwrite assets on an already published tag;
publish a new patch version so checksums and provenance remain stable.

## Publish

Publish the GitHub Release only after local validation succeeds. The
`validate-release.yml` workflow downloads the published assets and repeats the
public checks. If it fails, mark the release as a draft or prerelease while the
problem is corrected and publish a new version when asset integrity changed.

Finally update the website release list with the GitHub asset URL, archive
size, SHA-256 value, release date, and release-notes URL.
