# LuaWeb

A stupidly easy-to-use HTTP web server library for Lua, written in C++ with zero external dependencies (no luasocket!).

## Features

- 🚀 **Simple Lua API** - Create servers in just a few lines
- ⚡ **Pure C++ Core** - Fast, native socket implementation
- 🔄 **URL Parameters** - Express.js style `:param` routing
- 📦 **JSON Support** - Built-in JSON response helpers
- 🔒 **Web Lua Execution** - Run Lua code from the browser (sandboxed!)
- 🛠️ **CMake Build** - Modern, cross-platform build system

## Quick Start

```lua
local luaweb = require("luaweb")

local app = luaweb.server(8080)

app:get("/", function(req, res)
    res:send("<h1>Hello, World!</h1>")
end)

app:get("/greet/:name", function(req, res)
    res:send("Hello, " .. req.params.name .. "!")
end)

app:run()
```

## Building

### Requirements

- CMake 3.16+
- C++17 compiler (clang, gcc)
- Lua 5.4 (or 5.3)

### Build Commands

```bash
# Configure
cmake -B build

# Build
cmake --build build

# Set up local testing environment
cmake --build build --target setup_local
```

### Output

- `build/luaweb.so` - Lua module (use with `require("luaweb")`)
- `build/luaweb_runner` - Standalone Lua runner with LuaWeb pre-loaded

## Usage

### Running Scripts

```bash
# Run a Lua script
./build/luaweb_runner examples/hello_world.lua

# Execute inline Lua
./build/luaweb_runner -e 'print("Hello!")'

# Interactive REPL
./build/luaweb_runner -i
```

### API Reference

#### Creating a Server

```lua
local luaweb = require("luaweb")
local app = luaweb.server(8080)  -- port is optional, defaults to 8080
```

#### Defining Routes

```lua
-- GET request
app:get("/path", function(req, res)
    res:send("Hello!")
end)

-- POST request
app:post("/api/data", function(req, res)
    res:json('{"status":"ok"}')
end)

-- PUT request
app:put("/items/:id", function(req, res)
    -- req.params.id contains the URL parameter
end)

-- DELETE request
app:delete("/items/:id", function(req, res)
    res:status(204):send("")
end)
```

#### Request Object

```lua
req.method    -- "GET", "POST", etc.
req.path      -- "/api/users"
req.body      -- Request body (string)
req.ip        -- Client IP address
req.headers   -- Table of headers (e.g., req.headers["Content-Type"])
req.params    -- URL parameters (e.g., /users/:id → req.params.id)
req.query     -- Query parameters (e.g., ?foo=bar → req.query.foo)
```

#### Response Object

```lua
res:status(200)           -- Set status code (chainable)
res:header("Key", "Val")  -- Set header (chainable)
res:send("<html>...")     -- Send HTML response
res:text("plain text")    -- Send plain text
res:json('{"key":"val"}') -- Send JSON (sets Content-Type)
```

#### Starting the Server

```lua
app:run()  -- Blocks and runs the server
app:stop() -- Stop the server (call from signal handler)
```

### Web Lua Execution (Sandboxed)

Enable running Lua code from the browser:

```lua
app:enable_web_lua(true)
app:run()
```

Then POST Lua code to `/lua/run`:

```bash
curl -X POST -d 'print("Hello from browser!")' http://localhost:8080/lua/run
```

#### Sandbox Security

The sandbox **blocks**:
- ❌ File system access (`io.*`)
- ❌ OS commands (`os.execute`, `os.remove`, etc.)
- ❌ Module loading (`require`, `dofile`, `loadfile`)
- ❌ Debug library (`debug.*`)
- ❌ Raw table access (`rawget`, `rawset`)

The sandbox **allows**:
- ✅ Math operations (`math.*`)
- ✅ String manipulation (`string.*`)
- ✅ Table operations (`table.*`)
- ✅ Safe OS functions (`os.time`, `os.date`, `os.clock`)
- ✅ `print()` (captured and returned)

## Project Structure

```
LuaWeb/
├── CMakeLists.txt
├── README.md
├── src/
│   ├── core/           # C++ HTTP server
│   │   ├── server.hpp/cpp
│   │   ├── request.hpp/cpp
│   │   └── response.hpp/cpp
│   ├── bindings/       # Lua C++ bindings
│   │   └── lua_server.hpp/cpp
│   └── main.cpp        # Runner executable
├── lua/
│   └── sandbox.lua     # Security sandbox
├── scripts/
│   └── run_lua.py      # Python helper for web Lua
└── examples/
    └── hello_world.lua
```

## Future Plans

- [ ] Static file serving
- [ ] WebSocket support
- [ ] Middleware system
- [ ] HTTPS/TLS support
- [ ] Template engine

## License

MIT License
