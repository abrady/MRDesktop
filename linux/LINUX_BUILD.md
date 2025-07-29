# Linux Build Guide for MRDesktop

This guide explains how to build and test MRDesktop on Linux using containerized builds with Podman or Docker.

## Prerequisites

- **Podman** (recommended) or **Docker**
- **Git** (to clone the repository)

### Installing Podman

**Ubuntu/Debian:**
```bash
sudo apt update
sudo apt install podman
```

**Fedora:**
```bash
sudo dnf install podman
```

**CentOS/RHEL:**
```bash
sudo yum install podman
```

### Installing Docker (alternative)

If you prefer Docker over Podman, install Docker and set the environment variable:
```bash
export CONTAINER_ENGINE=docker
```

## Quick Start

1. **Build the container image:**
   ```bash
   python linux/build-linux.py image
   ```

2. **Build the project (debug):**
   ```bash
   python linux/build-linux.py build debug
   ```

3. **Build the project (release):**
   ```bash
   python linux/build-linux.py build release
   ```

4. **Run tests:**
   ```bash
   ./run-tests-linux.sh debug
   ```

5. **Interactive development shell:**
   ```bash
   python linux/build-linux.py shell
   ```

## Available Commands

### build-linux.py

| Command | Description |
|---------|-------------|
| `python linux/build-linux.py build [debug\|release]` | Build the project |
| `python linux/build-linux.py test [debug\|release]` | Run tests |
| `python linux/build-linux.py shell` | Start interactive shell |
| `python linux/build-linux.py image` | Build container image |
| `python linux/build-linux.py clean` | Clean build artifacts |
| `python linux/build-linux.py help` | Show help |

### run-tests-linux.sh

```bash
./run-tests-linux.sh [debug|release]
```

Runs the full test suite including:
- Unit tests (protocol, video encoder/decoder)
- Integration tests (network functionality)
- CTest with detailed output

## Build Outputs

After building, you'll find the binaries in:

- **Debug builds:** `build/debug/`
- **Release builds:** `build/release/`

### Generated Executables

- `MRDesktopServer` - The desktop capture server
- `MRDesktopConsoleClient` - Console client for testing
- `tests/basic_tests` - Unit tests
- `tests/integration_tests` - Integration tests

## Testing

The project includes comprehensive tests:

### Unit Tests
- Protocol message validation
- Video encoder/decoder functionality
- Component isolation testing

### Integration Tests
- Network connectivity
- Client-server communication
- Mock server testing

### Running Tests Manually

Inside the container shell:
```bash
cd build/debug
ctest --verbose --output-on-failure

# Or run individual test suites
./tests/basic_tests
./tests/integration_tests
```

## Development Workflow

### Setting up development environment

1. **Start interactive shell:**
   ```bash
   python linux/build-linux.py shell
   ```

2. **Inside the container, configure and build:**
   ```bash
   cmake --preset linux-debug
   cmake --build build/debug --parallel $(nproc)
   ```

3. **Run tests:**
   ```bash
   cd build/debug
   ctest
   ```

4. **Debug with GDB:**
   ```bash
   gdb ./MRDesktopServer
   ```

### Development Tips

- The container mounts the project directory at `/workspace`
- Changes to source files are immediately available in the container
- Use `python linux/build-linux.py shell` for iterative development
- Container includes development tools: GDB, Valgrind, strace

## Container Environment

The Linux build container includes:

### Base System
- Ubuntu 22.04 (Jammy)
- Build-essential (GCC, G++, Make)
- CMake and Ninja

### Libraries
- FFMPEG (via vcpkg)
- X11 development libraries
- Audio libraries (ALSA, PulseAudio)
- Video acceleration libraries (VA-API, VDPAU)

### Development Tools
- GDB debugger
- Valgrind memory checker
- strace system call tracer
- Google Test framework

## Environment Variables

| Variable | Default | Description |
|----------|---------|-------------|
| `CONTAINER_ENGINE` | `podman` | Container engine to use (`podman` or `docker`) |

Example:
```bash
export CONTAINER_ENGINE=docker
python linux/build-linux.py build
```

## Troubleshooting

### Container Build Fails

1. **Clear container cache:**
   ```bash
   podman system prune -a
   # or for Docker:
   docker system prune -a
   ```

2. **Rebuild image:**
   ```bash
   python linux/build-linux.py image
   ```

### Build Fails

1. **Clean build directory:**
   ```bash
   python linux/build-linux.py clean
   python linux/build-linux.py build
   ```

2. **Check container shell:**
   ```bash
   python linux/build-linux.py shell
   # Manually run build commands to see detailed errors
   ```

### Tests Fail

1. **Run tests individually:**
   ```bash
   python linux/build-linux.py shell
   cd build/debug
   ./tests/basic_tests --gtest_filter=ProtocolTest.*
   ```

2. **Check for running processes:**
   ```bash
   # Make sure no other instances are using port 8080
   netstat -tulpn | grep 8080
   ```

## CI/CD Integration

The containerized build system is designed for CI/CD integration:

```yaml
# Example GitHub Actions workflow
- name: Build Linux
  run: |
    python linux/build-linux.py image
    python linux/build-linux.py build release

- name: Test Linux
  run: ./run-tests-linux.sh release
```

## Performance Notes

- **First build** may take 10-15 minutes (downloading dependencies)
- **Subsequent builds** are much faster (cached layers)
- **Parallel builds** use all available CPU cores
- **Container overhead** is minimal for development

## Supported Platforms

This Linux build system works on:
- Ubuntu 20.04+
- Debian 11+
- Fedora 35+
- CentOS 8+
- Any Linux distribution with Podman/Docker support

The generated binaries are compatible with most modern Linux distributions.