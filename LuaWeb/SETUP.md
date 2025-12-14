# LuaWeb Setup Guide

This guide covers how to build LuaWeb from source, including the Rust template engine.

## 📋 Requirements

### Required
- **CMake** 3.16+
- **C++17 compiler** (clang or gcc)
- **Lua 5.4** (or 5.3) with development headers
- **Python 3** (for web Lua sandbox)

### Optional (for template engine)
- **Rust** (cargo) - for building the template engine

### Installing Dependencies

**macOS (Homebrew):**
```bash
brew install cmake lua python3 rust
```

**Ubuntu/Debian:**
```bash
sudo apt update
sudo apt install cmake build-essential liblua5.4-dev python3 cargo
```

**Fedora:**
```bash
sudo dnf install cmake gcc-c++ lua-devel python3 cargo
```

**Arch Linux:**
```bash
sudo pacman -S cmake lua python rust
```

---

## 🔨 Building LuaWeb

### Step 1: Clone the Repository

```bash
git clone https://github.com/Juicywoowowow/Terram.git
cd Terram/LuaWeb
```

### Step 2: Build the C++ Core

```bash
# Configure the build
cmake -B build

# Build everything
cmake --build build -j$(nproc)

# Set up local test environment
cmake --build build --target setup_local
```

### Step 3: Build the Rust Template Engine (Optional)

The template engine is optional. If you don't build it, `res:render()` will return an error.

```bash
# Build the Rust library
cd template
cargo build --release
cd ..

# Copy the library to build directory
# On macOS:
cp template/target/release/liblwtemplate.dylib build/

# On Linux:
cp template/target/release/liblwtemplate.so build/
```

Or use the CMake target:

```bash
cmake --build build --target build_template
```

---

## 📦 Build Output

After building, you'll have:

| File | Description |
|------|-------------|
| `build/luaweb.so` | Lua module - use with `require("luaweb")` |
| `build/luaweb_runner` | Standalone executable with LuaWeb pre-loaded |
| `build/liblwtemplate.dylib` | Rust template engine (macOS) |
| `build/liblwtemplate.so` | Rust template engine (Linux) |

---

## 🚀 Running LuaWeb

### Option 1: Using the Runner (Recommended)

The `luaweb_runner` executable has LuaWeb pre-loaded:

```bash
cd build
./luaweb_runner ../examples/hello_world.lua
```

### Option 2: Using System Lua

Copy `luaweb.so` to your Lua path:

```bash
# Option A: Copy to Lua's cpath
sudo cp build/luaweb.so /usr/local/lib/lua/5.4/

# Option B: Set LUA_CPATH environment variable
export LUA_CPATH="$PWD/build/?.so;;"

# Then run with regular lua
lua examples/hello_world.lua
```

### Option 3: Inline Code

```bash
./build/luaweb_runner -e 'print("Hello from LuaWeb!")'
```

### Option 4: Interactive REPL

```bash
./build/luaweb_runner -i
```

---

## 📁 Directory Structure for Your Project

When using LuaWeb in your own project, your directory should look like:

```
my_project/
├── luaweb.so              # Copy from build/
├── liblwtemplate.dylib    # Copy from template/target/release/ (optional)
├── scripts/
│   └── run_lua.py         # Copy from LuaWeb/scripts/ (for web sandbox)
├── lua/
│   └── sandbox.lua        # Copy from LuaWeb/lua/ (for web sandbox)
├── templates/             # Your .lwt template files
│   └── index.lwt
├── public/                # Static files
│   ├── style.css
│   └── script.js
└── app.lua                # Your main Lua script
```

Or use the runner:

```
my_project/
├── luaweb_runner          # Copy from build/
├── liblwtemplate.dylib    # Template engine
├── scripts/run_lua.py     # Sandbox helper
├── lua/sandbox.lua        # Sandbox config
└── app.lua
```

---

## ⚙️ CMake Options

```bash
# Debug build (with debug symbols)
cmake -B build -DCMAKE_BUILD_TYPE=Debug

# Release build (optimized)
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Specify Lua version
cmake -B build -DLUA_VERSION=5.3
```

---

## 🔧 Troubleshooting

### "Cannot find Lua"

Make sure Lua development headers are installed:

```bash
# macOS
brew install lua

# Ubuntu/Debian
sudo apt install liblua5.4-dev
```

### "Template engine not available"

Make sure you've built and copied the Rust library:

```bash
cd template && cargo build --release && cd ..
cp template/target/release/liblwtemplate.* build/
```

### "Permission denied" on sandbox

The Python sandbox script needs execute permissions:

```bash
chmod +x scripts/run_lua.py
```

### Port already in use

Change the port in your Lua script:

```lua
local app = luaweb.server(3000)  -- Use a different port
```

---

## 📝 Verifying Installation

Run the hello world example:

```bash
cd build
./luaweb_runner ../examples/hello_world.lua
```

Then visit:
- http://localhost:8080/ - Main page
- http://localhost:8080/demo - Template demo
- http://localhost:8080/cookie-demo - Cookie demo
- http://localhost:8080/public/ - Static files

Test JSON parsing:

```bash
curl -X POST -H "Content-Type: application/json" \
     -d '{"message":"Hello"}' \
     http://localhost:8080/api/echo
```

---

## 🔄 Updating LuaWeb

```bash
cd Terram
git pull origin main
cd LuaWeb
cmake --build build -j$(nproc)
cmake --build build --target setup_local

# Rebuild template engine if changed
cd template && cargo build --release && cd ..
```

---

## 📜 License

MIT License
