<!--
  ~ Licensed to the Apache Software Foundation (ASF) under one
  ~ or more contributor license agreements.  See the NOTICE file
  ~ distributed with this work for additional information
  ~ regarding copyright ownership.  The ASF licenses this file
  ~ to you under the Apache License, Version 2.0 (the
  ~ "License"); you may not use this file except in compliance
  ~ with the License.  You may obtain a copy of the License at
  ~
  ~   http://www.apache.org/licenses/LICENSE-2.0
  ~
  ~ Unless required by applicable law or agreed to in writing,
  ~ software distributed under the License is distributed on an
  ~ "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
  ~ KIND, either express or implied.  See the License for the
  ~ specific language governing permissions and limitations
  ~ under the License.
-->

# Building Apache Iceberg™ C++

This guide covers how to build iceberg-cpp from source, how its third-party
dependencies are provisioned, and the available build options. For a short
walkthrough see the [Quick Start](README.md#quick-start) in the README.

## Prerequisites

**Required:**

- A C++23-compliant compiler. Only the C++23 standard is enforced
  (`CMAKE_CXX_STANDARD 23`), not a compiler version; per the README the known-good
  toolchains are GCC 14+, Clang 18+, MSVC 2022+ (CI currently exercises GCC 14)
- [CMake](https://cmake.org/) 3.25+ (the floor declared in `CMakeLists.txt`)
- [Ninja](https://ninja-build.org/) is the recommended build backend

**Conditional / optional:**

- CMake 3.28+ is required when enabling the built-in SQL catalog database
  connectors (`ICEBERG_SQL_SQLITE`, `ICEBERG_SQL_POSTGRESQL`,
  `ICEBERG_SQL_MYSQL`); configuration fails otherwise.
- [sccache](https://github.com/mozilla/sccache) for faster (re)builds; enable it
  by setting the compiler launchers (see [sccache](#sccache)).
- [Meson](https://mesonbuild.com/) (>= 1.3, per `meson.build`) if you prefer the
  alternative build system (see [Building with Meson](#building-with-meson)).
- Python 3 and [pre-commit](https://pre-commit.com/) for linting.

If `CMAKE_BUILD_TYPE` is not set, the build defaults to `Debug`.

## Quick start

```bash
cmake -S . -B build -G Ninja
cmake --build build
ctest --test-dir build --output-on-failure
```

By default this builds a static library with the battery-included
("bundle") dependencies and the REST catalog client, and compiles the
vendored third-party libraries (Apache Arrow, Avro, etc.) from source via
CMake's `FetchContent`. Each vendored dependency is pinned to a specific
release URL (Arrow and nanoarrow additionally record a SHA256 checksum; Avro
is pinned to a git commit). See
[Dependency provisioning](#dependency-provisioning) to use prebuilt copies
instead.

## Build options

All options are declared in `CMakeLists.txt` and set with `-D<OPTION>=ON|OFF`.

| Option | Default | Enables |
|--------|---------|---------|
| `ICEBERG_BUILD_STATIC` | `ON` | Build the static library |
| `ICEBERG_BUILD_SHARED` | `OFF` | Build the shared library |
| `ICEBERG_BUILD_TESTS` | `ON` | Build tests (and call `enable_testing()`) |
| `ICEBERG_BUILD_BUNDLE` | `ON` | Build the battery-included library (vendors Arrow and Avro) |
| `ICEBERG_BUILD_REST` | `ON` | Build the REST catalog client (pulls in cpr) |
| `ICEBERG_BUILD_REST_INTEGRATION_TESTS` | `OFF` | Build REST catalog integration tests (forced `OFF` on Windows) |
| `ICEBERG_BUILD_HIVE` | `OFF` | Build the Hive (HMS) catalog client (needs Thrift) |
| `ICEBERG_BUILD_SQL_CATALOG` | `OFF` | Build the SQL catalog client |
| `ICEBERG_SQL_SQLITE` | `OFF` | Build the SQLite connector for the SQL catalog |
| `ICEBERG_SQL_POSTGRESQL` | `OFF` | Build the PostgreSQL connector for the SQL catalog |
| `ICEBERG_SQL_MYSQL` | `OFF` | Build the MySQL connector for the SQL catalog |
| `ICEBERG_S3` | `OFF` | Build with S3 support (enables Arrow S3 + AWS SDK) |
| `ICEBERG_SIGV4` | `OFF` | Build with SigV4 support (requires `ICEBERG_BUILD_REST=ON`) |
| `ICEBERG_BUNDLE_AWSSDK` | `ON` | Bundle the AWS SDK; takes effect only when `ICEBERG_S3=ON` (then requires `ICEBERG_BUILD_BUNDLE=ON`). SigV4-only builds always use a system AWS SDK. |
| `ICEBERG_BUNDLE_THRIFT` | `ON` | Bundle Thrift (from Arrow) for the Hive catalog (requires `ICEBERG_BUILD_BUNDLE=ON`; only effective when `ICEBERG_BUILD_HIVE` is on) |
| `ICEBERG_ENABLE_ASAN` | `OFF` | Enable Address Sanitizer (GCC/Clang only) |
| `ICEBERG_ENABLE_UBSAN` | `OFF` | Enable Undefined Behavior Sanitizer (GCC/Clang only) |

Notes on dependent options:

- `ICEBERG_SIGV4=ON` with `ICEBERG_BUILD_REST=OFF` is a fatal configuration error.
- `ICEBERG_BUNDLE_AWSSDK=ON` takes effect only when `ICEBERG_S3=ON`, and then
  requires `ICEBERG_BUILD_BUNDLE=ON` (otherwise configuration fails). A SigV4-only
  build (`ICEBERG_S3=OFF`) ignores this option and uses a system AWS SDK.
  `ICEBERG_BUNDLE_THRIFT=ON` (when Hive is enabled) likewise requires `ICEBERG_BUILD_BUNDLE=ON`.
- The SQL connectors are opt-in. With `ICEBERG_BUILD_SQL_CATALOG=ON` and no
  connector enabled, you get a SQL catalog that only works with a user-supplied
  `CatalogStore`.

To install, configure with `-DCMAKE_INSTALL_PREFIX=/path/to/install` and run
`cmake --install build`.

## Dependency provisioning

iceberg-cpp uses a **vendored-by-default with system fallback** model.
`cmake_modules/IcebergThirdpartyToolchain.cmake` declares the vendorable
dependencies with `FetchContent` plus `FIND_PACKAGE_ARGS ... CONFIG`, so CMake
first tries `find_package(<dep> CONFIG)`; if a prebuilt copy is discoverable
(e.g. on `CMAKE_PREFIX_PATH`) it is used as-is, otherwise the pinned source is
downloaded and built. A few dependencies are system-only — a plain
`find_package` with no vendored fallback (see below).

### Vendored dependencies (downloaded + built, or found on `CMAKE_PREFIX_PATH`)

| Dependency | Pinned version | Resolved when |
|------------|----------------|----------------|
| Apache Arrow + Parquet | 24.0.0 | `ICEBERG_BUILD_BUNDLE=ON` |
| Apache Avro (C++) | git commit `997d50d3…` of `apache/avro` | `ICEBERG_BUILD_BUNDLE=ON` |
| nanoarrow | 0.8.0 | always |
| CRoaring | 4.4.3 | always |
| nlohmann_json | 3.11.3 | always |
| spdlog | 1.15.3 | always |
| cpr | 1.14.1 | `ICEBERG_BUILD_REST=ON` |
| sqlpp23 | 0.69 | `ICEBERG_BUILD_SQL_CATALOG=ON` with a connector enabled |

Arrow and nanoarrow are pinned to a release tarball with a recorded SHA256
checksum; CRoaring, nlohmann_json, spdlog, cpr, and sqlpp23 are pinned by
version tag in their download URL (no checksum); Avro is fetched from a pinned
git commit.

### Dependencies looked up with `find_package`

These are not part of the vendored-FetchContent table above. Some are always
present (e.g. ZLIB, and Threads pulled in via spdlog); the rest are looked up only
when the feature that needs them is enabled (noted per item). AWS SDK and Thrift
also have an Arrow-bundled path, described in their entries.

- **ZLIB** — `find_package(ZLIB REQUIRED)`; always required (the core library
  links it directly, and Arrow/Avro depend on it too).
- **zstd** — `find_package(zstd CONFIG)`, optional; linked if present (only
  looked up when the bundle is built).
- **CURL** — `find_package(CURL REQUIRED)` for cpr; cpr is configured to use the
  system curl.
- **AWS SDK** — resolved via `find_package(AWSSDK REQUIRED COMPONENTS ...)` for
  SigV4, or for S3 when `ICEBERG_BUNDLE_AWSSDK=OFF`. When `ICEBERG_S3=ON` and
  `ICEBERG_BUNDLE_AWSSDK=ON`, it is built as part of Arrow's bundled dependencies.
- **Thrift** — for the Hive catalog: bundled from Arrow's build when
  `ICEBERG_BUNDLE_THRIFT=ON`, otherwise `find_package(ThriftAlt MODULE REQUIRED)`
  (see `cmake_modules/FindThriftAlt.cmake`).
- **Snappy** — `find_package(Snappy CONFIG)`, optional; looked up only when Avro
  is vendored (`ICEBERG_BUILD_BUNDLE=ON`) and linked into that build if found. A
  system Avro skips it.
- **SQLite3 / PostgreSQL / MySQL** — native client libraries for the
  corresponding SQL connector, located via `find_package` once the connector is
  enabled.

### Using a prebuilt Arrow (or other system copy)

Point `CMAKE_PREFIX_PATH` at a prefix (a conda environment, a system install,
etc.) that contains a CMake config package for the dependency, and the
`find_package` fallback picks it up instead of compiling the vendored source:

```bash
cmake -S . -B build -G Ninja \
  -DCMAKE_PREFIX_PATH=/path/to/arrow \
  -DICEBERG_BUILD_BUNDLE=ON
cmake --build build
```

### Overriding download URLs (mirrors)

If you hit network issues, override the source URL for a vendored dependency via
environment variables (read in `IcebergThirdpartyToolchain.cmake`):

| Variable | Dependency |
|----------|------------|
| `ICEBERG_ARROW_URL` | Apache Arrow tarball |
| `ICEBERG_AVRO_URL` | Apache Avro tarball (alternative to git) |
| `ICEBERG_AVRO_GIT_URL` | Apache Avro git repository |
| `ICEBERG_NANOARROW_URL` | nanoarrow tarball |
| `ICEBERG_CROARING_URL` | CRoaring tarball |
| `ICEBERG_NLOHMANN_JSON_URL` | nlohmann_json tarball |
| `ICEBERG_SPDLOG_URL` | spdlog tarball |
| `ICEBERG_CPR_URL` | cpr tarball |
| `ICEBERG_SQLPP23_URL` | sqlpp23 tarball |

```bash
export ICEBERG_ARROW_URL="https://your-mirror.example/apache-arrow-24.0.0.tar.gz"
cmake -S . -B build
```

## Platform notes

- **Linux / macOS** vendor the dependencies by default. CI builds them on
  `ubuntu-26.04` (GCC 14) and `macos-26` in `Debug`. On Ubuntu, CI installs
  `libcurl4-openssl-dev` for the REST client.
- **Windows** uses [vcpkg](https://vcpkg.io/) for the native dependencies. CI
  installs `zlib`, `nlohmann-json`, `nanoarrow`, `roaring`, and `cpr`
  (`x64-windows`) and configures with the vcpkg toolchain file, building in
  `Release`:

  ```bash
  cmake -S . -B build -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake \
    -DCMAKE_BUILD_TYPE=Release
  ```

  REST catalog integration tests are not built on Windows (forced off).
- **SQL connectors** need the native client library. CI provisions SQLite via
  `apt-get install libsqlite3-dev` on Ubuntu and `vcpkg install sqlite3:x64-windows`
  on Windows.

The `ci/scripts/build_iceberg.sh <source_dir> [rest_integration_tests=OFF]
[sccache=OFF] [s3=OFF] [sigv4=OFF] [bundle_awssdk=ON]` and
`ci/scripts/build_example.sh <source_dir>` scripts drive the main test/AWS jobs
and are a useful reference for the exact flags per platform. Some jobs invoke the
build tool directly instead — the SQL catalog job calls `cmake` inline, and the
Meson job calls `meson` — so consult the specific workflow for those.

## Sanitizers and tests

Enable the sanitizers (GCC/Clang only; ignored with a warning on other
compilers) and run the test suite:

```bash
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DICEBERG_ENABLE_ASAN=ON \
  -DICEBERG_ENABLE_UBSAN=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

Run a single suite with a regex:

```bash
ctest --test-dir build -R schema_test --output-on-failure
```

CI runs the sanitizer job on Ubuntu with tuned `ASAN_OPTIONS` / `UBSAN_OPTIONS`
and a UBSan suppressions file (`.github/ubsan-suppressions.txt`).

### sccache

The build scripts opt in to sccache by setting the compiler launchers; you can
do the same directly:

```bash
cmake -S . -B build -G Ninja \
  -DCMAKE_C_COMPILER_LAUNCHER=sccache \
  -DCMAKE_CXX_COMPILER_LAUNCHER=sccache
```

## Building with Meson

Meson is supported as a secondary build system and is exercised in CI on Ubuntu,
macOS, and Windows:

```bash
meson setup builddir
meson compile -C builddir
meson test -C builddir --timeout-multiplier 0
```

Meson uses its built-ins instead of several CMake options:

- `--default-library=<shared|static|both>` instead of `ICEBERG_BUILD_STATIC` /
  `ICEBERG_BUILD_SHARED`
- `-Db_sanitize=address,undefined` instead of `ICEBERG_ENABLE_ASAN` /
  `ICEBERG_ENABLE_UBSAN`
- `--libdir`, `--bindir`, `--includedir`, `--datadir` for install directories

Meson-specific options (set with `-D<option>=<value>`), from `meson.options`:

| Option | Default | Enables |
|--------|---------|---------|
| `rest` | `enabled` | REST catalog client |
| `rest_integration_test` | `disabled` | REST catalog integration test |
| `sigv4` | `disabled` | AWS SigV4 authentication for the REST catalog |
| `tests` | `enabled` | Tests |

The Meson build does **not** currently expose the SQL catalog or Hive catalog
options; for those features, use the CMake build.

## Building the examples

The example links `iceberg` with its `bundle` and `rest` components, so install
a build that has both (`ICEBERG_BUILD_BUNDLE=ON` and `ICEBERG_BUILD_REST=ON`,
the defaults), then build the example against the install prefix:

```bash
cd example
cmake -S . -B build -G Ninja -DCMAKE_PREFIX_PATH=/path/to/install
cmake --build build
```

If you used a provided Arrow, include its prefix too:

```bash
cmake -S . -B build -G Ninja -DCMAKE_PREFIX_PATH="/path/to/install;/path/to/arrow"
```
