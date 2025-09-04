# WARP.md

This file provides guidance to WARP (warp.dev) when working with code in this repository.

## Project Overview

Content Assimilation Engine (CAE) is a high-performance I/O system built on top of Hermes that provides multiple storage adapters and HDF5 integration. The system supports various adapters (POSIX, stdio, MPI-IO, NVIDIA GDS) and includes specialized tools for HDF5 dataset processing and job orchestration.

## Core Architecture

### Main Components
- **Adapters Layer** (`adapters/`): Pluggable I/O adapters for different storage systems
  - POSIX adapter for standard file operations
  - stdio adapter for C standard library I/O
  - MPI-IO adapter for parallel I/O
  - NVIDIA GDS adapter for GPU Direct Storage
  - VFD (Virtual File Driver) for HDF5 integration
- **OMNI System** (`omni/`): Job orchestration and HDF5 dataset processing tools
- **Configuration System** (`config/`): YAML-based configuration management
- **Documentation System** (`astro/`): Astro-based documentation generation

### Key Abstractions
- **Adapter Factory Pattern**: All adapters implement common interfaces defined in `adapter_types.h`
- **Configuration Management**: YAML configuration parsing with size unit support (KB, MB, GB, TB)
- **Format Factory**: Extensible system for supporting different data formats (binary, HDF5)

## Build System & Development

### Primary Build Commands
```bash
# Standard CMake build
mkdir build && cd build
cmake ..
make -j$(nproc)

# Using CMake presets (debug configuration available)
cmake --preset=debug
cmake --build --preset=debug

# Spack installation (recommended)
spack install iowarp +mpiio +vfd +compress +encrypt
spack load iowarp
```

### Build Options
Key CMake options that control the build:
- `CAE_ENABLE_HERMES=ON/OFF` - Main Hermes integration (required for most features)
- `CAE_ENABLE_POSIX_ADAPTER=ON/OFF` - POSIX I/O adapter
- `CAE_ENABLE_STDIO_ADAPTER=ON/OFF` - stdio I/O adapter  
- `CAE_ENABLE_MPIIO_ADAPTER=ON/OFF` - MPI-IO parallel adapter
- `CAE_ENABLE_VFD=ON/OFF` - HDF5 Virtual File Driver
- `CAE_ENABLE_NVIDIA_GDS_ADAPTER=ON/OFF` - NVIDIA GPU Direct Storage
- `BUILD_MPI_TESTS=ON/OFF` - MPI-dependent tests

### Testing
```bash
# Run unit tests
cd build
ctest

# Run OMNI-specific tests
cd omni
./test_wrp_h5.bat      # Test HDF5 OMNI reader
./run_test.bat         # Test legacy dataset reader

# Run specific test types using pytest function (when Hermes enabled)
# Tests are organized by type in test/jarvis_wrp_cae/pipelines/
```

### Linting
```bash
# Run linting (defined as CMake target)
make lint
```

## Key Executables

The build produces several important binaries in the `bin/` directory:

### OMNI System Binaries
- `wrp` - Main OMNI job orchestrator
- `wrp_h5` - HDF5 dataset reader with YAML configuration support
- `wrp_binary_format_mpi` - MPI binary format processor
- `dataset_reader` - Legacy HDF5 dataset reader

### OMNI Usage Patterns
```bash
# HDF5 dataset processing with YAML config
./bin/wrp_h5 config/dataset_config.yaml

# Legacy dataset processing
./bin/dataset_reader config/dataset_config.yaml
```

## Configuration System

### YAML Configuration Structure
The system uses YAML for configuration with specific patterns:

**Client Configuration** (`config/cae_client_default.yaml`):
- Path inclusions/exclusions with glob patterns
- Page size specifications with unit support (KB, MB, GB, TB)
- Adapter mode settings (kDefault, kBypass, kScratch, kWorkflow)
- Flushing modes (kSync, kAsync)

**Dataset Configuration** (OMNI system):
```yaml
name: point
tags:
  - "description tags"
uri: "hdf5://path/to/file.h5/dataset_name"
start: [0, 0, 0]      # Hyperslab start coordinates
count: [1, 1, 1]      # Hyperslab count
stride: [1, 1, 1]     # Hyperslab stride
run: script.sh        # External processing script
dest: s3://bucket/output.parquet  # Destination
```

## Documentation System

The project includes an Astro-based documentation system:

```bash
cd astro

# Development server
npm run dev

# Generate and build docs
npm run build

# Generate docs only
npm run docs
```

## Platform-Specific Notes

### Windows Development
- PowerShell scripts provided (`.ps1` and `.bat` files)
- vcpkg integration available (`omni/build_with_vcpkg.bat`)
- Windows-specific executables use `.exe` extension

### Spack Integration
The project is designed for Spack package management:
- Multiple Spack-based CI workflows (ARM64, Ubuntu variants)
- Package variants: `+mpiio +vfd +compress +encrypt +posix`
- Cross-platform Spack support (Linux, macOS, ARM64)

## CI/CD & Quality

### GitHub Actions Workflows
- Platform-specific builds: Windows, macOS, Ubuntu (multiple variants)
- Spack-based builds with different configurations
- Docker and Synology NAS deployment
- Code quality: Codespell, commitlint
- Documentation builds (Astro)

### Code Quality Tools
- Codespell for spell checking
- Commitlint for commit message validation
- AddressSanitizer support for debugging builds

## Development Patterns

### Adding New Adapters
1. Implement adapter interface following patterns in `adapters/`
2. Add CMake configuration options
3. Register with adapter factory system
4. Add appropriate tests in `test/unit/`

### Adding New Data Format Support
1. Create format client in `omni/format/`
2. Register with format factory (`format_factory.cc`)
3. Add configuration parsing if needed
4. Create corresponding OMNI binary if required

### Configuration Changes
- Client configurations go in `config/` directory
- Use YAML format with size unit support
- Follow existing patterns for adapter modes and flushing
- Test with both legacy and new OMNI tools
