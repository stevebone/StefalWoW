# TrinityCore Build System - Quick Reference Card

## 🚀 Most Common Commands

### Daily Development Workflow (Playerbot Module)
```batch
# Edit your code, then:
build_playerbot_and_worldserver_debug.bat

# Deploy and test:
cd M:\Wplayerbot\bin
worldserver.exe -c worldserver.conf
```

### Performance Testing
```batch
# Switch to Release build:
configure_release.bat
build_playerbot_and_worldserver_release.bat
```

---

## 📋 Build Commands Overview

| Command | Purpose | Build Time | When to Use |
|---------|---------|------------|-------------|
| `configure_debug.bat` | Setup Debug build | 2-3 min | First time or after CMake changes |
| `configure_release.bat` | Setup Release build | 2-3 min | For production deployment |
| `build_playerbot_and_worldserver_debug.bat` | **RECOMMENDED: Build Playerbot + worldserver (Debug)** | 5-10 min | **Daily development** |
| `build_playerbot_and_worldserver_release.bat` | Build Playerbot + worldserver (Release) | 5-8 min | Performance testing |
| `build_debug.bat` | Full solution build (Debug) | 20-40 min | Complete rebuild needed |
| `build_release.bat` | Full solution build (Release) | 15-30 min | Production build |

---

## ⚡ Quick Decision Tree

```
Need to build? Start here:
│
├─ Changed Playerbot code?
│  ├─ Development/Testing? → build_playerbot_and_worldserver_debug.bat ✅
│  └─ Performance Test? → build_playerbot_and_worldserver_release.bat
│
├─ First time building?
│  ├─ configure_debug.bat
│  └─ build_debug.bat
│
├─ Switching Debug ↔ Release?
│  ├─ configure_[debug/release].bat
│  └─ build_[debug/release].bat
│
└─ Build errors?
   ├─ Try: Clean build (rmdir /s /q build)
   └─ Then: configure_debug.bat && build_debug.bat
```

---

## 📊 Configuration Comparison

| Aspect | Debug | Release | Notes |
|--------|-------|---------|-------|
| **Binary Size** | 76 MB | 25 MB | 3x smaller in Release |
| **Build Speed** | Slower | Faster | ~25% faster in Release |
| **Runtime Speed** | Baseline | +20-50% | Significant improvement |
| **Debugging** | Full | Limited | Choose based on need |
| **Memory Usage** | Higher | Lower | 20-40% less in Release |

---

## 🔧 Essential Troubleshooting

### Build Failed?

1. **First Try:**
   ```batch
   # Clean and rebuild
   rmdir /s /q build
   configure_debug.bat
   build_debug.bat
   ```

2. **Linking Errors (LNK2038)?**
   - Check: `type build\CMakeCache.txt | findstr "Boost"`
   - Debug libs should have `-gd-` suffix
   - Release libs should NOT have `-gd-` suffix

3. **Missing Libraries?**
   ```batch
   # Install vcpkg dependencies
   C:\libs\vcpkg\vcpkg.exe install tbb:x64-windows parallel-hashmap:x64-windows
   ```

---

## 📁 Key File Locations

### Build Outputs
- **Debug worldserver:** `build\bin\Debug\worldserver.exe`
- **Release worldserver:** `build\bin\Release\worldserver.exe`
- **Playerbot library:** `build\src\server\modules\Playerbot\[Debug/Release]\playerbot.lib`

### Configuration Files
- **CMake cache:** `build\CMakeCache.txt`
- **VS Solution:** `build\TrinityCore.sln`
- **Build logs:** `build_*_output.txt`

### Dependencies
- **vcpkg:** `C:\libs\vcpkg`
- **Boost:** `C:\libs\boost_1_78_0`
- **MySQL:** `C:\Program Files\MySQL\MySQL Server 9.4`

---

## 🎯 Best Practices

### DO:
✅ Use `build_playerbot_and_worldserver_debug.bat` for daily development
✅ Clean build directory when switching configurations
✅ Check CMakeCache.txt if library errors occur
✅ Use Release build for performance testing

### DON'T:
❌ Mix Debug and Release builds in same directory
❌ Skip configure step after pulling CMake changes
❌ Use Debug build for production
❌ Ignore linking errors - they won't fix themselves

---

## 💡 Pro Tips

1. **Faster Builds:** Reduce `/maxcpucount:2` to `/maxcpucount:1` if running out of memory
2. **Debug Symbols:** `.pdb` files are huge but essential for debugging
3. **Incremental Builds:** Only rebuild what changed - much faster!
4. **Release Testing:** Always test Release build before deployment

---

## 🆘 Need Help?

1. Check `BUILD_DOCUMENTATION.md` for detailed explanations
2. Review `build_*_output.txt` for error details
3. Verify libraries with `C:\libs\vcpkg\vcpkg.exe list`
4. Ensure Visual Studio 2022 Enterprise is installed

---

## 📈 Performance Metrics

| Build Type | Full Build | Incremental | Module Only |
|------------|------------|-------------|-------------|
| **Debug** | 20-40 min | 5-10 min | 2-5 min |
| **Release** | 15-30 min | 5-8 min | 2-5 min |

---

## 🔄 Version Info

- **Visual Studio:** 2022 Enterprise (v17+)
- **CMake:** 3.24+
- **Boost:** 1.78
- **MySQL:** 9.4
- **C++ Standard:** C++20

---

**Last Updated:** 2025-10-16
**Quick Tip:** Bookmark this file for instant reference!