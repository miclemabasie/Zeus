# Zeus – High-Performance Log Ingestion & Analytics Engine

## Status
- ✅ Module 1: ConfigManager (complete, 5/5 tests)
- ✅ Module 2: Logger (complete, 7/7 tests)
- 🔜 Module 3: LogMessage (next)

## Build

Requires: CMake 3.20+, GCC 11+ or Clang 14+, vcpkg.

```bash
mkdir -p build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake -G Ninja
cmake --build . --parallel 8
./src/zeus
./tests/zeus_tests