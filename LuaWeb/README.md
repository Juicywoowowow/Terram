# LuaWeb

A stupidly easy-to-use HTTP web server library for Lua, written in C++ with a Rust-powered template engine.

## ✨ Features

| Feature | Description |
|---------|-------------|
| 🚀 **Simple Lua API** | Create servers in just a few lines |
| ⚡ **Pure C++ Core** | Fast, native socket implementation |
| 🔄 **URL Parameters** | Express.js style `:param` routing |
| 📦 **JSON Support** | Auto-parse JSON bodies, send JSON responses |
| 🍪 **Cookie Support** | Read and set cookies with full options |
| 🔒 **Web Lua Sandbox** | Run Lua code from the browser (securely!) |
| 📁 **Static Files** | Serve HTML, CSS, JS, images |
| 🔗 **Middleware** | Logging, CORS, auth, and custom handlers |
| 🦀 **Template Engine** | Rust-powered `.lwt` templates |
| 🛠️ **CMake Build** | Modern, cross-platform build system |

## 🚀 Quick Start

```lua
local luaweb = require("luaweb")

local app = luaweb.server(8080)

-- Middleware: log all requests
app:use(function(req, res, next)
    print(req.method .. " " .. req.path)
    next()
end)

-- Static files
app:static("/public", "./public")

-- Routes
app:get("/", function(req, res)
    res:send("<h1>Hello, World!</h1>")
end)

app:get("/greet/:name", function(req, res)
    res:send("Hello, " .. req.params.name .. "!")
end)

app:post("/api/data", function(req, res)
    -- JSON body is auto-parsed!
    if req.json then
        res:json('{"received": true}')
    end
end)

app:run()
```

## 📖 Documentation

- **[SETUP.md](SETUP.md)** - How to build and install LuaWeb
- **[API Reference](#api-reference)** - Full API documentation below

---

## API Reference

### Creating a Server

```lua
local luaweb = require("luaweb")
local app = luaweb.server(8080)  -- port defaults to 8080
```

### Defining Routes

```lua
app:get("/path", handler)       -- GET request
app:post("/path", handler)      -- POST request
app:put("/path", handler)       -- PUT request
app:delete("/path", handler)    -- DELETE request
app:route("METHOD", "/path", handler)  -- Any method
```

### Request Object

```lua
req.method    -- "GET", "POST", etc.
req.path      -- "/api/users"
req.body      -- Raw request body (string)
req.json      -- Parsed JSON body (table, or nil if not JSON)
req.ip        -- Client IP address
req.headers   -- Table: req.headers["Content-Type"]
req.params    -- URL params: /users/:id → req.params.id
req.query     -- Query string: ?foo=bar → req.query.foo
req.cookies   -- Cookies: req.cookies.session_id
```

### Response Object

```lua
res:status(200)                    -- Set status code (chainable)
res:header("Key", "Val")           -- Set header (chainable)
res:send("<html>...")              -- Send HTML
res:text("plain text")             -- Send plain text
res:json('{"key":"val"}')          -- Send JSON
res:render("template.lwt", data)   -- Render template (see Templates)
res:cookie("name", "value", opts)  -- Set cookie
res:clearCookie("name")            -- Delete cookie
```

### Cookie Options

```lua
res:cookie("session", "abc123", {
    maxAge = 86400,       -- Seconds until expiry (default: session)
    path = "/",           -- Cookie path (default: "/")
    domain = "",          -- Cookie domain
    httpOnly = true,      -- No JavaScript access
    secure = false,       -- HTTPS only
    sameSite = "Lax"      -- "Strict", "Lax", or "None"
})
```

### Middleware

Middleware runs before every request. Call `next()` to continue.

```lua
-- Logging
app:use(function(req, res, next)
    print(os.date() .. " " .. req.method .. " " .. req.path)
    next()
end)

-- CORS
app:use(function(req, res, next)
    res:header("Access-Control-Allow-Origin", "*")
    next()
end)

-- Auth (blocks if no Authorization header)
app:use(function(req, res, next)
    if not req.headers["Authorization"] then
        res:status(401):send("Unauthorized")
        return  -- Don't call next()
    end
    next()
end)
```

### Static File Serving

```lua
app:static("/public", "./public")

-- /public/style.css → ./public/style.css
-- /public/ → ./public/index.html
```

Supports: HTML, CSS, JS, JSON, PNG, JPG, GIF, SVG, WebP, WOFF2, PDF, ZIP, and more.

### Template Engine

LuaWeb includes a **Rust-powered template engine** with custom `.lwt` syntax.

```lua
app:get("/page", function(req, res)
    res:render("templates/page.lwt", {
        title = "My Page",
        user = { name = "Alex", admin = true },
        items = { "Apple", "Banana", "Cherry" }
    })
end)
```

**Template Syntax (`.lwt` files):**

```html
<!DOCTYPE html>
<html>
<head><title>@{title}</title></head>
<body>
    <h1>Welcome, @{user.name}!</h1>
    
    @if user.admin
        <p>🔑 Admin access granted</p>
    @end
    
    <ul>
    @for item in items
        <li>@{item}</li>
    @end
    </ul>
</body>
</html>
```

| Syntax | Description |
|--------|-------------|
| `@{variable}` | Variable (HTML escaped) |
| `@{obj.field}` | Nested access |
| `@{var \| "default"}` | Default value |
| `@raw{html}` | Unescaped output |
| `@if cond` ... `@end` | Conditional |
| `@if !cond` | Negated condition |
| `@else` | Else branch |
| `@for item in list` ... `@end` | Loop |
| `@include "file.lwt"` | Include partial |
| `@-- comment` | Template comment |

### Web Lua Execution (Sandboxed)

Enable running Lua code from the browser:

```lua
app:enable_web_lua(true)
app:run()
```

POST code to `/lua/run`:

```bash
curl -X POST -d 'print("Hello!"); print(2 + 2)' http://localhost:8080/lua/run
```

**Sandbox blocks:** File I/O, OS commands, require, debug library  
**Sandbox allows:** Math, strings, tables, safe os.* functions, print()

---

## 📁 Project Structure

```
LuaWeb/
├── CMakeLists.txt
├── README.md
├── SETUP.md                # Build instructions
├── src/
│   ├── core/               # C++ HTTP server
│   │   ├── server.hpp/cpp
│   │   ├── request.hpp/cpp
│   │   ├── response.hpp/cpp
│   │   └── template.hpp/cpp
│   ├── bindings/           # Lua C++ bindings
│   │   └── lua_server.hpp/cpp
│   ├── vendor/             # Third-party libs
│   │   └── json.hpp        # nlohmann/json
│   └── main.cpp            # Runner executable
├── template/               # 🦀 Rust template engine
│   ├── Cargo.toml
│   └── src/
│       ├── lib.rs
│       ├── parser.rs
│       └── renderer.rs
├── lua/
│   └── sandbox.lua         # Security sandbox
├── scripts/
│   └── run_lua.py          # Python helper
└── examples/
    ├── hello_world.lua
    ├── public/             # Static files
    └── templates/          # Example .lwt files
```

## ✅ Feature Status

- [x] HTTP Server (C++)
- [x] Lua API & Bindings
- [x] URL & Query Parameters
- [x] JSON Body Parsing
- [x] Cookie Support
- [x] Middleware System
- [x] Static File Serving
- [x] Template Engine (Rust)
- [x] Web Lua Sandbox
- [ ] WebSocket Support
- [ ] HTTPS/TLS

## 🛠️ Tech Stack

| Component | Language | Why |
|-----------|----------|-----|
| HTTP Server | **C++** | Performance, sockets |
| User API | **Lua** | Simplicity, flexibility |
| Templates | **Rust** | Memory-safe parsing |
| Sandbox | **Python** | Process isolation |
| JSON | **nlohmann/json** | Battle-tested library |

## 📜 License

MIT License

---

Made with ❤️ and multiple programming languages
