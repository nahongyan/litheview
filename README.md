# LitheView SDK

LitheView is an embeddable web UI runtime for native Windows applications.
This repository contains the public C/C++ headers, CMake package files, and
standalone examples used to integrate the binary runtime.

## Downloads

Download SDK packages from the
[LitheView download page](https://litheview.com/download) or this repository's
GitHub Releases page. Do not download release binaries from pull requests,
issues, or third-party mirrors.

Every release publishes:

- `litheview-sdk-<version>-win-x64.zip`
- `litheview-sdk-<version>-win-x64.zip.sha256`
- release notes and compatibility information

Verify a downloaded archive before extracting it:

```powershell
Get-FileHash .\litheview-sdk-0.1.0-win-x64.zip -Algorithm SHA256
```

## Requirements

- Windows 10 or later, x64
- Visual Studio 2022 with the Desktop development with C++ workload
- CMake 3.21 or later

## Build the Win32 example

After extracting an SDK package, run:

```powershell
cmake -S examples/litheview_demo -B build -DLitheView_DIR=lib/cmake/LitheView
cmake --build build --config Release --parallel 28
```

The Qt Widgets example is under `examples/litheview_qt_demo`. Qt is not
included in the SDK and remains subject to its own license terms.

## Repository layout

```text
include/litheview/          Public C and C++ headers
cmake/                     CMake package source files
examples/litheview_demo/   Native Win32 integration example
examples/litheview_qt_demo Qt Widgets integration example
```

Binary SDK packages additionally contain `bin/`, `lib/`, compliance notices,
an SPDX software bill of materials, and checksums for all packaged files.

## Licensing

The public headers, examples, and CMake files in this repository are licensed
under the [Apache License 2.0](LICENSE). The compiled LitheView runtime,
including `litheview.dll`, is not licensed under Apache-2.0; its use is governed
by [RUNTIME-LICENSE.md](RUNTIME-LICENSE.md) or a separately signed commercial
agreement.

Third-party components retain their original licenses. Release packages carry
the generated `THIRD_PARTY_NOTICES.txt` and `SBOM.spdx.json` files. See
[THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) for details.

## Support and security

- Integration help: [SUPPORT.md](SUPPORT.md)
- Security reports: [SECURITY.md](SECURITY.md)
- Commercial licensing: `1061517027@qq.com`
- Contribution guidelines: [CONTRIBUTING.md](CONTRIBUTING.md)

## Supporting LitheView

Sponsorship options are described in [SPONSORS.md](SPONSORS.md). Sponsorship
does not grant a commercial runtime license or priority support unless a
separate written agreement says otherwise.
