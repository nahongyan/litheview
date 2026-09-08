# Contributing to LitheView SDK

Thank you for improving the public SDK integration surface. Contributions to
headers, examples, documentation, and build files are welcome. The proprietary
Runtime implementation is developed separately and is not part of this public
repository.

## Before opening a change

1. Open an issue for API changes, behavior changes, or substantial new
   examples so compatibility and scope can be agreed first.
2. Keep changes focused. Do not include generated build output, SDK binaries,
   credentials, private keys, customer data, or code you cannot license.
3. Follow the existing C++20 style and run formatting on changed C++ files.
4. Add or update a focused test when behavior changes.
5. Verify that each example uses only the public LitheView API.

## Developer Certificate of Origin

All commits must be signed off using the Developer Certificate of Origin 1.1.
By adding a `Signed-off-by` line, you certify that you have the right to submit
the contribution under the repository's Apache-2.0 license.

```text
Signed-off-by: Your Name <your.email@example.com>
```

Create it with `git commit -s`. A maintainer may ask you to correct missing
sign-offs before review.

## License of contributions

Unless explicitly agreed otherwise in writing, intentionally submitted
contributions are licensed under Apache License 2.0 as described in section 5
of that license. Contributions do not grant access to, ownership of, or a
license for the proprietary Runtime implementation.

## Pull request checklist

- The change has a clear reason and a focused scope.
- Public API compatibility has been considered.
- Tests or reproducible verification steps are included.
- Documentation and examples match the implemented behavior.
- No generated output or local IDE state is committed.
- Every commit contains a valid `Signed-off-by` line.

Security vulnerabilities must follow [SECURITY.md](SECURITY.md), not public
issues.
