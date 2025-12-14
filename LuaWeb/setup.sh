#!/bin/bash
#
# LuaWeb Setup Script
# Compiles all components: C++ core, Lua bindings, Rust template engine
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "================================================"
echo "LuaWeb Setup"
echo "================================================"
echo ""

# Detect OS
OS="unknown"
INSTALL_CMD=""

if [[ "$OSTYPE" == "darwin"* ]]; then
    OS="macos"
    INSTALL_CMD="brew install"
elif [[ -f /etc/debian_version ]]; then
    OS="debian"
    INSTALL_CMD="sudo apt install"
elif [[ -f /etc/fedora-release ]]; then
    OS="fedora"
    INSTALL_CMD="sudo dnf install"
elif [[ -f /etc/arch-release ]]; then
    OS="arch"
    INSTALL_CMD="sudo pacman -S"
else
    OS="linux"
    INSTALL_CMD="<package-manager> install"
fi

echo "Detected OS: $OS"
echo ""

# ============================================================
# Preflight Checks
# ============================================================

echo "Running preflight checks..."
echo ""

DEPS_NEEDED=""
DEPS_FOUND=""
DEPS_MISSING=""
INSTALL_HINTS=""
ALL_OK=true

# Check: CMake
if command -v cmake &> /dev/null; then
    CMAKE_VER=$(cmake --version | head -1 | awk '{print $3}')
    DEPS_FOUND="$DEPS_FOUND\n  [OK] cmake ($CMAKE_VER)"
else
    DEPS_MISSING="$DEPS_MISSING\n  [MISSING] cmake"
    case $OS in
        macos)  INSTALL_HINTS="$INSTALL_HINTS\n  brew install cmake" ;;
        debian) INSTALL_HINTS="$INSTALL_HINTS\n  sudo apt install cmake" ;;
        fedora) INSTALL_HINTS="$INSTALL_HINTS\n  sudo dnf install cmake" ;;
        arch)   INSTALL_HINTS="$INSTALL_HINTS\n  sudo pacman -S cmake" ;;
        *)      INSTALL_HINTS="$INSTALL_HINTS\n  Install cmake via your package manager" ;;
    esac
    ALL_OK=false
fi

# Check: C++ Compiler
CXX_FOUND=false
CXX_NAME=""
if command -v clang++ &> /dev/null; then
    CXX_VER=$(clang++ --version | head -1)
    DEPS_FOUND="$DEPS_FOUND\n  [OK] clang++ ($CXX_VER)"
    CXX_FOUND=true
    CXX_NAME="clang++"
elif command -v g++ &> /dev/null; then
    CXX_VER=$(g++ --version | head -1)
    DEPS_FOUND="$DEPS_FOUND\n  [OK] g++ ($CXX_VER)"
    CXX_FOUND=true
    CXX_NAME="g++"
fi

if [ "$CXX_FOUND" = false ]; then
    DEPS_MISSING="$DEPS_MISSING\n  [MISSING] C++ compiler (clang++ or g++)"
    case $OS in
        macos)  INSTALL_HINTS="$INSTALL_HINTS\n  xcode-select --install" ;;
        debian) INSTALL_HINTS="$INSTALL_HINTS\n  sudo apt install build-essential" ;;
        fedora) INSTALL_HINTS="$INSTALL_HINTS\n  sudo dnf install gcc-c++" ;;
        arch)   INSTALL_HINTS="$INSTALL_HINTS\n  sudo pacman -S gcc" ;;
        *)      INSTALL_HINTS="$INSTALL_HINTS\n  Install a C++ compiler" ;;
    esac
    ALL_OK=false
fi

# Check: Lua
LUA_FOUND=false
LUA_NAME=""
for lua_cmd in lua5.4 lua54 lua5.3 lua53 lua; do
    if command -v $lua_cmd &> /dev/null; then
        LUA_VER=$($lua_cmd -v 2>&1 | head -1)
        DEPS_FOUND="$DEPS_FOUND\n  [OK] lua ($LUA_VER)"
        LUA_FOUND=true
        LUA_NAME=$lua_cmd
        break
    fi
done

if [ "$LUA_FOUND" = false ]; then
    DEPS_MISSING="$DEPS_MISSING\n  [MISSING] Lua (5.3 or 5.4)"
    case $OS in
        macos)  INSTALL_HINTS="$INSTALL_HINTS\n  brew install lua" ;;
        debian) INSTALL_HINTS="$INSTALL_HINTS\n  sudo apt install liblua5.4-dev" ;;
        fedora) INSTALL_HINTS="$INSTALL_HINTS\n  sudo dnf install lua-devel" ;;
        arch)   INSTALL_HINTS="$INSTALL_HINTS\n  sudo pacman -S lua" ;;
        *)      INSTALL_HINTS="$INSTALL_HINTS\n  Install Lua 5.3 or 5.4 with dev headers" ;;
    esac
    ALL_OK=false
fi

# Check: Lua headers (for CMake)
LUA_HEADERS_FOUND=false
for header_path in /usr/include/lua5.4 /usr/include/lua5.3 /usr/include/lua /usr/local/include/lua /opt/homebrew/include/lua; do
    if [ -f "$header_path/lua.h" ] || [ -f "$header_path/lua.h" ]; then
        LUA_HEADERS_FOUND=true
        break
    fi
done

# On macOS with Homebrew, headers are in a different location
if [ "$OS" = "macos" ] && [ "$LUA_FOUND" = true ]; then
    LUA_HEADERS_FOUND=true  # Homebrew lua includes headers
fi

if [ "$LUA_HEADERS_FOUND" = false ] && [ "$LUA_FOUND" = true ]; then
    DEPS_MISSING="$DEPS_MISSING\n  [MISSING] Lua development headers"
    case $OS in
        debian) INSTALL_HINTS="$INSTALL_HINTS\n  sudo apt install liblua5.4-dev" ;;
        fedora) INSTALL_HINTS="$INSTALL_HINTS\n  sudo dnf install lua-devel" ;;
        *)      INSTALL_HINTS="$INSTALL_HINTS\n  Install Lua development headers" ;;
    esac
    ALL_OK=false
fi

# Check: Rust/Cargo
if command -v cargo &> /dev/null; then
    CARGO_VER=$(cargo --version)
    DEPS_FOUND="$DEPS_FOUND\n  [OK] cargo ($CARGO_VER)"
else
    DEPS_MISSING="$DEPS_MISSING\n  [MISSING] cargo (Rust)"
    case $OS in
        macos)  INSTALL_HINTS="$INSTALL_HINTS\n  brew install rust" ;;
        debian) INSTALL_HINTS="$INSTALL_HINTS\n  sudo apt install cargo" ;;
        fedora) INSTALL_HINTS="$INSTALL_HINTS\n  sudo dnf install cargo" ;;
        arch)   INSTALL_HINTS="$INSTALL_HINTS\n  sudo pacman -S rust" ;;
        *)      INSTALL_HINTS="$INSTALL_HINTS\n  curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh" ;;
    esac
    ALL_OK=false
fi

# Check: Python3
if command -v python3 &> /dev/null; then
    PYTHON_VER=$(python3 --version)
    DEPS_FOUND="$DEPS_FOUND\n  [OK] python3 ($PYTHON_VER)"
else
    DEPS_MISSING="$DEPS_MISSING\n  [MISSING] python3"
    case $OS in
        macos)  INSTALL_HINTS="$INSTALL_HINTS\n  brew install python3" ;;
        debian) INSTALL_HINTS="$INSTALL_HINTS\n  sudo apt install python3" ;;
        fedora) INSTALL_HINTS="$INSTALL_HINTS\n  sudo dnf install python3" ;;
        arch)   INSTALL_HINTS="$INSTALL_HINTS\n  sudo pacman -S python" ;;
        *)      INSTALL_HINTS="$INSTALL_HINTS\n  Install Python 3" ;;
    esac
    ALL_OK=false
fi

# Check: Node.js
if command -v node &> /dev/null; then
    NODE_VER=$(node --version)
    DEPS_FOUND="$DEPS_FOUND\n  [OK] node ($NODE_VER)"
else
    DEPS_MISSING="$DEPS_MISSING\n  [MISSING] node (Node.js)"
    case $OS in
        macos)  INSTALL_HINTS="$INSTALL_HINTS\n  brew install node" ;;
        debian) INSTALL_HINTS="$INSTALL_HINTS\n  sudo apt install nodejs npm" ;;
        fedora) INSTALL_HINTS="$INSTALL_HINTS\n  sudo dnf install nodejs npm" ;;
        arch)   INSTALL_HINTS="$INSTALL_HINTS\n  sudo pacman -S nodejs npm" ;;
        *)      INSTALL_HINTS="$INSTALL_HINTS\n  Install Node.js from https://nodejs.org" ;;
    esac
    ALL_OK=false
fi

# Check: npm
if command -v npm &> /dev/null; then
    NPM_VER=$(npm --version)
    DEPS_FOUND="$DEPS_FOUND\n  [OK] npm ($NPM_VER)"
else
    DEPS_MISSING="$DEPS_MISSING\n  [MISSING] npm"
    case $OS in
        macos)  INSTALL_HINTS="$INSTALL_HINTS\n  brew install node" ;;
        debian) INSTALL_HINTS="$INSTALL_HINTS\n  sudo apt install npm" ;;
        fedora) INSTALL_HINTS="$INSTALL_HINTS\n  sudo dnf install npm" ;;
        arch)   INSTALL_HINTS="$INSTALL_HINTS\n  sudo pacman -S npm" ;;
        *)      INSTALL_HINTS="$INSTALL_HINTS\n  Install npm" ;;
    esac
    ALL_OK=false
fi

# ============================================================
# Report Results
# ============================================================

echo "Dependencies found:"
echo -e "$DEPS_FOUND"
echo ""

if [ "$ALL_OK" = false ]; then
    echo "================================================"
    echo "SETUP ABORTED: Missing dependencies"
    echo "================================================"
    echo ""
    echo "Missing dependencies:"
    echo -e "$DEPS_MISSING"
    echo ""
    echo "Install commands:"
    echo -e "$INSTALL_HINTS"
    echo ""
    exit 1
fi

echo "All dependencies found."
echo ""

# ============================================================
# Build
# ============================================================

echo "================================================"
echo "Building LuaWeb"
echo "================================================"
echo ""

# Step 1: Build Rust template engine
echo "[1/4] Building Rust template engine..."
cd template
cargo build --release
cd ..
echo "      Done."
echo ""

# Step 2: Install Node.js dependencies for database bridge
echo "[2/4] Installing Node.js dependencies..."
cd database
npm install --silent
cd ..
echo "      Done."
echo ""

# Step 3: Build C++ core and Lua bindings
echo "[3/4] Building C++ core and Lua bindings..."
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
echo "      Done."
echo ""

# Step 4: Copy template engine to build directory
echo "[4/4] Copying template engine to build directory..."
if [ "$OS" = "macos" ]; then
    cp template/target/release/liblwtemplate.dylib build/
else
    cp template/target/release/liblwtemplate.so build/
fi
echo "      Done."
echo ""

# ============================================================
# Setup local environment
# ============================================================

echo "Setting up local test environment..."
cmake --build build --target setup_local
echo ""

# Create __DBWEB__ directory
mkdir -p __DBWEB__

# ============================================================
# Summary
# ============================================================

echo "================================================"
echo "BUILD COMPLETE"
echo "================================================"
echo ""
echo "Output files:"
echo "  build/luaweb_runner     - Standalone executable"
echo "  build/luaweb.so         - Lua module"
if [ "$OS" = "macos" ]; then
    echo "  build/liblwtemplate.dylib - Template engine"
else
    echo "  build/liblwtemplate.so   - Template engine"
fi
echo "  database/               - SQLite bridge (Node.js)"
echo "  __DBWEB__/              - Database storage directory"
echo ""
echo "To run the example:"
echo "  cd build && ./luaweb_runner ../examples/hello_world.lua"
echo ""
echo "Then visit: http://localhost:8080"
echo ""

