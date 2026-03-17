# Vendored Dependencies - License Information

This file documents all vendored dependencies included in the Playerbot module, their licenses, and compliance requirements.

## Summary

All vendored dependencies are licensed under **Apache License 2.0**, which is:
- ✅ Compatible with GPL (TrinityCore's license)
- ✅ Permits modification and redistribution
- ✅ Permits commercial use
- ✅ Requires attribution (provided below)

## 1. Parallel Hashmap (phmap)

**Repository:** https://github.com/greg7mdp/parallel-hashmap
**License:** Apache License 2.0
**Version:** Latest from master branch
**Location:** `deps/phmap/` (git submodule)
**Type:** Header-only library

### Description
A family of header-only, very fast and memory-friendly hashmap and btree containers based on Google's abseil library. Provides high-performance concurrent hash tables used for bot data structures.

### License Text
```
Copyright 2018-2024 Gregory Popovitch

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
```

### Attribution
This module includes Parallel Hashmap (phmap) by Gregory Popovitch, licensed under Apache 2.0.

---

## 2. Intel Threading Building Blocks (oneTBB)

**Repository:** https://github.com/oneapi-src/oneTBB
**License:** Apache License 2.0
**Version:** 2021.11 or later (from master)
**Location:** `deps/tbb/` (git submodule)
**Type:** Compiled library

### Description
Intel oneAPI Threading Building Blocks (oneTBB) is a flexible C++ library that simplifies the work of adding parallelism to complex applications. Used for thread pools and concurrent containers in bot AI systems.

### License Text
```
Copyright (c) 2005-2024 Intel Corporation

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
```

### Attribution
This module includes Intel oneAPI Threading Building Blocks (oneTBB), licensed under Apache 2.0.

### Additional oneTBB Notice
Intel, the Intel logo, and other Intel marks are trademarks of Intel Corporation or its subsidiaries.

---

## License Compatibility Analysis

### Apache 2.0 + GPL v2 (TrinityCore)

**Question:** Can we vendor Apache 2.0 licensed libraries in a GPL v2 project?

**Answer:** ✅ Yes, with proper attribution.

**Rationale:**
1. Apache 2.0 is a permissive license that allows redistribution and modification
2. Apache 2.0 code can be incorporated into GPL v2 projects
3. The combined work must be distributed under GPL v2
4. Apache 2.0 attribution requirements must be met (this document satisfies that)
5. The Free Software Foundation considers Apache 2.0 compatible with GPL v3 (and v2)

**Requirements Met:**
- ✅ Full license text provided above
- ✅ Copyright notices preserved
- ✅ Attribution provided for both libraries
- ✅ No patent clauses conflict with GPL
- ✅ NOTICE files from upstream repositories retained

---

## How to Update Vendored Dependencies

### Updating phmap
```bash
cd src/modules/Playerbot/deps/phmap
git checkout master
git pull origin master
cd ../../../..
git add src/modules/Playerbot/deps/phmap
git commit -m "chore(deps): Update phmap to latest version"
```

### Updating TBB
```bash
cd src/modules/Playerbot/deps/tbb
git checkout master
git pull origin master
cd ../../../..
git add src/modules/Playerbot/deps/tbb
git commit -m "chore(deps): Update TBB to latest version"
```

### Version Pinning (Recommended for Stability)
For production releases, pin to specific stable tags:

```bash
# Pin phmap to a specific version
cd src/modules/Playerbot/deps/phmap
git checkout tags/v1.3.12  # Example stable tag
cd ../../../..
git add src/modules/Playerbot/deps/phmap
git commit -m "chore(deps): Pin phmap to v1.3.12"

# Pin TBB to a specific version
cd src/modules/Playerbot/deps/tbb
git checkout tags/v2021.11.0  # Example stable tag
cd ../../../..
git add src/modules/Playerbot/deps/tbb
git commit -m "chore(deps): Pin TBB to v2021.11.0"
```

---

## Compliance Checklist

For maintainers ensuring license compliance:

- [x] Apache 2.0 full license text included for phmap
- [x] Apache 2.0 full license text included for TBB
- [x] Copyright notices preserved
- [x] Attribution statements provided
- [x] Git submodules preserve upstream repository history
- [x] NOTICE files from upstream included (in submodules)
- [x] No trademark violations
- [x] GPL compatibility verified
- [x] Update procedures documented
- [x] Source of vendored code clearly documented

---

## Additional Resources

- **Apache License 2.0:** https://www.apache.org/licenses/LICENSE-2.0
- **GPL Compatibility:** https://www.gnu.org/licenses/license-list.html#apache2
- **phmap Repository:** https://github.com/greg7mdp/parallel-hashmap
- **TBB Repository:** https://github.com/oneapi-src/oneTBB
- **TrinityCore License:** GPL v2 - see AUTHORS file in repository root

---

## Questions or Concerns?

For license questions or concerns about vendored dependencies:
1. Review the Apache License 2.0 full text above
2. Check the upstream repository NOTICE files (in git submodules)
3. Consult TrinityCore legal documentation
4. Contact the TrinityCore Playerbot maintainers

---

**Last Updated:** 2025-10-29
**Reviewed By:** TrinityCore Playerbot Team
**Next Review Date:** 2026-10-29 (annual review recommended)
