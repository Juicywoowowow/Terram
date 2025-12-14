-- hello_world.lua - Basic LuaWeb example
-- Run with: ./luaweb_runner hello_world.lua

local luaweb = require("luaweb")

-- Create a server on port 8080
local app = luaweb.server(8080)

-- ============================================================
-- MIDDLEWARE: Runs before every request
-- ============================================================

-- Logging middleware
app:use(function(req, res, next)
    local timestamp = os.date("%Y-%m-%d %H:%M:%S")
    print(string.format("[%s] %s %s", timestamp, req.method, req.path))
    next()  -- Continue to next middleware/handler
end)

-- CORS middleware (allow cross-origin requests)
app:use(function(req, res, next)
    res:header("Access-Control-Allow-Origin", "*")
    res:header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE")
    next()
end)

-- ============================================================
-- STATIC FILE SERVING
-- ============================================================

-- Serve files from ./public at /public URL path
app:static("/public", "../examples/public")

-- ============================================================
-- WEB LUA EXECUTION
-- ============================================================

-- Enable web Lua execution (sandboxed!)
app:enable_web_lua(true)

-- Root endpoint
app:get("/", function(req, res)
    res:send([[
<!DOCTYPE html>
<html>
<head>
    <title>LuaWeb Hello World</title>
    <style>
        body { font-family: system-ui, sans-serif; max-width: 800px; margin: 50px auto; padding: 20px; background: #1a1a2e; color: #eee; }
        h1 { color: #e94560; }
        h2 { color: #0f3460; background: #e94560; padding: 10px; border-radius: 5px; }
        .endpoint { background: #16213e; padding: 10px 15px; margin: 10px 0; border-radius: 5px; border-left: 4px solid #e94560; }
        code { background: #0f3460; padding: 2px 6px; border-radius: 3px; }
        a { color: #00d9ff; }
        textarea { width: 100%; height: 100px; font-family: monospace; background: #0f3460; color: #eee; border: 1px solid #e94560; padding: 10px; border-radius: 5px; }
        button { background: #e94560; color: white; border: none; padding: 10px 20px; border-radius: 5px; cursor: pointer; margin-top: 10px; }
        button:hover { background: #ff6b6b; }
        #output { background: #0f3460; padding: 15px; border-radius: 5px; margin-top: 10px; white-space: pre-wrap; font-family: monospace; min-height: 50px; }
    </style>
</head>
<body>
    <h1>🚀 Welcome to LuaWeb!</h1>
    <p>Your Lua-powered web server is running.</p>
    
    <h2>Available Endpoints</h2>
    <div class="endpoint">
        <strong>GET /</strong> - This page
    </div>
    <div class="endpoint">
        <strong>GET /greet/:name</strong> - Personalized greeting<br>
        Example: <a href="/greet/World">/greet/World</a>
    </div>
    <div class="endpoint">
        <strong>GET /api/time</strong> - Current server time as JSON<br>
        Example: <a href="/api/time">/api/time</a>
    </div>
    <div class="endpoint">
        <strong>POST /api/echo</strong> - Echo back posted data as JSON
    </div>
    <div class="endpoint">
        <strong>POST /lua/run</strong> - Execute Lua code (sandboxed!)
    </div>
    
    <h2>🔒 Try Web Lua Execution</h2>
    <p>Enter Lua code below. It runs in a secure sandbox (no file/OS access).</p>
    <textarea id="code">-- Try some Lua code!
print("Hello from the browser!")
print("2 + 2 = " .. (2 + 2))

for i = 1, 5 do
    print("Count: " .. i)
end</textarea>
    <br>
    <button onclick="runLua()">▶ Run Code</button>
    <div id="output">Output will appear here...</div>
    
    <script>
    async function runLua() {
        const code = document.getElementById('code').value;
        const output = document.getElementById('output');
        output.textContent = 'Running...';
        
        try {
            const response = await fetch('/lua/run', {
                method: 'POST',
                body: code
            });
            const result = await response.text();
            output.textContent = result || '(no output)';
        } catch (err) {
            output.textContent = 'Error: ' + err.message;
        }
    }
    </script>
</body>
</html>
    ]])
end)

-- Greeting with URL parameter
app:get("/greet/:name", function(req, res)
    local name = req.params.name or "World"
    res:send("<h1>Hello, " .. name .. "!</h1><p><a href='/'>Back to home</a></p>")
end)

-- JSON API endpoint
app:get("/api/time", function(req, res)
    local time_info = os.date("*t")
    res:json(string.format(
        '{"hour":%d,"minute":%d,"second":%d,"date":"%04d-%02d-%02d"}',
        time_info.hour, time_info.min, time_info.sec,
        time_info.year, time_info.month, time_info.day
    ))
end)

-- Echo endpoint
app:post("/api/echo", function(req, res)
    res:json('{"received":' .. (req.body and ('"' .. req.body .. '"') or 'null') .. '}')
end)

-- ============================================================
-- TEMPLATE ENGINE DEMO
-- ============================================================

-- Template rendering using .lwt files (Rust-powered!)
app:get("/demo", function(req, res)
    res:render("../examples/templates/welcome.lwt", {
        title = "LuaWeb Template Demo",
        user = {
            name = "LuaWeb User",
            email = "user@luaweb.dev",
            admin = true
        },
        items = {
            { name = "Widget", price = "9.99" },
            { name = "Gadget", price = "19.99" },
            { name = "Gizmo", price = "29.99" },
            { name = "Doohickey", price = "39.99" }
        }
    })
end)

print("Server starting on http://localhost:8080")
print("Web Lua execution enabled - POST to /lua/run")
print("Template demo at http://localhost:8080/demo")
print("Press Ctrl+C to stop")
app:run()
