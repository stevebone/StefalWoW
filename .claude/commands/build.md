# Build and Test Command

Configure, build, and test the TrinityCore Playerbot project.

## Usage

```bash
/build                         # Full build
/build --target worldserver    # Build specific target
/build --clean                 # Clean build
/build --test                  # Build and run tests
/build --profile               # Build with profiling enabled
/build --config Debug          # Build in Debug configuration
/build --config Release        # Build in Release configuration
```

## Examples

```bash
# Standard build
/build

# Clean build with tests
/build --clean --test

# Build specific target
/build --target worldserver

# Debug build with profiling
/build --config Debug --profile
```

## Execution Steps

1. **Pre-build checks**:
   - Verify CMake is installed
   - Check dependencies (Boost, OpenSSL, MySQL)
   - Validate build environment

2. **Configuration** (if needed):
   - Run CMake configuration
   - Apply build options from `.env`
   - Generate build files

3. **Compilation**:
   - Build specified targets
   - Use parallel compilation (-j flag)
   - Track compilation times per file

4. **Post-build**:
   - Copy dependencies
   - Verify binaries
   - Run tests if requested

5. **Analysis**:
   - Build time analysis
   - Identify slow-compiling files
   - Suggest optimizations

## Build Targets

Available targets:
- `worldserver` - Game world server
- `bnetserver` - Battle.net authentication server
- `all` - All targets (default)
- `clean` - Clean build artifacts

## Configuration Options

From `.env` file:
- `CMAKE_BUILD_TYPE`: Debug, Release, RelWithDebInfo (default)
- `BUILD_PARALLEL_JOBS`: Number of parallel compilation jobs (default: 8)
- `BOOST_ROOT`: Boost library location
- `OPENSSL_ROOT_DIR`: OpenSSL location
- `MYSQL_ROOT_DIR`: MySQL location

## Performance Analysis

The build command tracks:
- Total build time
- Time per compilation unit
- Slowest files to compile
- Template instantiation overhead
- Include file dependencies

## Output

Generates:
- Build log in `.claude/logs/build_<timestamp>.log`
- Performance report in `.claude/reports/build_performance.json`
- Recommendations for build optimization
- Error/warning summary

## Troubleshooting

Common issues:
- **Missing dependencies**: Check `.env` configuration
- **Compilation errors**: Run `/review` first for code quality check
- **Slow builds**: See build performance recommendations
- **Linker errors**: Ensure all dependencies are found

## Notes

- First build takes 30-60 minutes
- Incremental builds take 2-5 minutes
- Use caching to speed up repeated builds
- Build artifacts stored in `build/` directory
