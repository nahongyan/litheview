# Third-party notices

LitheView Runtime incorporates Chromium and other third-party components.
Every binary SDK package contains two files generated from the exact
`//wr:litheview` GN dependency graph used for that build:

- `THIRD_PARTY_NOTICES.txt`: attribution notices and complete license texts.
- `SBOM.spdx.json`: an SPDX 2.2 JSON software bill of materials for automated compliance
  and inventory workflows.

Those generated files are authoritative for a binary release and must remain
with any permitted redistribution of the Runtime. Third-party components are
licensed by their respective copyright holders; neither the Apache-2.0 license
for this repository nor `RUNTIME-LICENSE.md` replaces those terms.

The Qt example contains source code that integrates with Qt. Qt itself is not
included in this repository or the SDK package. Obtain Qt separately and
review the license that applies to your chosen Qt edition and deployment.

Report a missing or incorrect notice privately to `1061517027@qq.com` before
redistributing the affected release.
