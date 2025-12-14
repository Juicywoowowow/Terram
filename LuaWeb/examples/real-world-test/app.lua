-- Real World Example: "LuaWeb Feed"
-- Now with Auth, Replies, and persistent storage

local luaweb = require("luaweb")
local app = luaweb.server(8080, "feed_app")

-- ============================================================
-- HELPER FUNCTIONS
-- ============================================================

local function url_decode(str)
    str = string.gsub(str, "+", " ")
    str = string.gsub(str, "%%(%x%x)", function(h)
        return string.char(tonumber(h, 16))
    end)
    return str
end

local function parse_form_body(body)
    local params = {}
    if not body then return params end
    for pair in string.gmatch(body, "[^&]+") do
        local key, val = string.match(pair, "([^=]+)=?(.*)")
        if key then
            key = url_decode(key)
            val = val and url_decode(val) or ""
            params[key] = val
        end
    end
    return params
end

-- Generate a random token for sessions
local function generate_token()
    local chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"
    local token = ""
    for i = 1, 32 do
        local r = math.random(1, #chars)
        token = token .. string.sub(chars, r, r)
    end
    return token
end

-- ============================================================
-- DATABASE SETUP
-- ============================================================

local DB_NAME = "luafeed.db"

local function init_db()
    local db = app:db(DB_NAME)
    
    -- Users table
    db:exec([[
        CREATE TABLE IF NOT EXISTS users (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            username TEXT UNIQUE NOT NULL,
            password TEXT NOT NULL,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP
        )
    ]])
    
    -- Sessions table
    db:exec([[
        CREATE TABLE IF NOT EXISTS sessions (
            token TEXT PRIMARY KEY,
            user_id INTEGER NOT NULL,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP
        )
    ]])
    
    -- Posts table
    db:exec([[
        CREATE TABLE IF NOT EXISTS posts (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id INTEGER,
            author TEXT NOT NULL,
            content TEXT NOT NULL,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP
        )
    ]])
    
    -- Replies table
    db:exec([[
        CREATE TABLE IF NOT EXISTS replies (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            post_id INTEGER NOT NULL,
            user_id INTEGER NOT NULL,
            author_name TEXT NOT NULL,
            content TEXT NOT NULL,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP
        )
    ]])
    
    db:close()
end

init_db()

-- ============================================================
-- MIDDLEWARE
-- ============================================================

app:use(function(req, res, next)
    print(string.format("[%s] %s %s", os.date("%X"), req.method, req.path))
    next()
end)

-- Auth Middleware: Check for session cookie
app:use(function(req, res, next)
    -- Initialize user as nil
    req.user = nil
    
    -- Dump all cookies to debug
    for k,v in pairs(req.cookies) do
        print("DEBUG COOKIE: [" .. k .. "] = " .. v)
    end
    
    local token = req.cookies["session_token"]
    if token then
        local db = app:db(DB_NAME)
        -- Find session
        local sessions = db:query("SELECT user_id FROM sessions WHERE token = ?", {token})
        if #sessions > 0 then
            local user_id = sessions[1].user_id
            -- Fetch user details
            local users = db:query("SELECT id, username FROM users WHERE id = ?", {user_id})
            if #users > 0 then
                req.user = users[1]
            end
        end
        db:close()
    end
    next()
end)

-- ============================================================
-- ROUTES
-- ============================================================

app:static("/static", "examples/real-world-test/public")

-- Home Page
app:get("/", function(req, res)
    local db = app:db(DB_NAME)
    
    local sql = [[
        SELECT 
            p.*, 
            (SELECT COUNT(*) FROM replies WHERE post_id = p.id) as reply_count 
        FROM posts p 
        ORDER BY p.id DESC LIMIT 50
    ]]
    local posts = db:query(sql)
    
    -- Hack: Handle potential nil user in template carefully
    -- We pass req.user explicitly
    
    db:close()
    
    res:render("examples/real-world-test/templates/index.lwt", {
        title = "LuaWeb Feed",
        user = req.user,
        posts = posts
    })
end)

-- Post Detail Page
app:get("/post/:id", function(req, res)
    local post_id = tonumber(req.params.id)
    if not post_id then 
        res:status(404):text("Post not found")
        return 
    end
    
    local db = app:db(DB_NAME)
    
    -- Fetch post
    local posts = db:query("SELECT * FROM posts WHERE id = ?", {post_id})
    if #posts == 0 then
        db:close()
        res:status(404):text("Post not found")
        return
    end
    
    -- Fetch replies
    local replies = db:query("SELECT * FROM replies WHERE post_id = ? ORDER BY id ASC", {post_id})
    
    db:close()
    
    res:render("examples/real-world-test/templates/post.lwt", {
        post = posts[1],
        replies = replies,
        user = req.user
    })
end)

-- Submit Reply
app:post("/post/:id/reply", function(req, res)
    if not req.user then
        res:status(403):text("You must be logged in to reply")
        return
    end
    
    local post_id = tonumber(req.params.id)
    local form = parse_form_body(req.body)
    local content = form.content
    
    if content and content ~= "" then
        local db = app:db(DB_NAME)
        db:exec(
            "INSERT INTO replies (post_id, user_id, author_name, content) VALUES (?, ?, ?, ?)", 
            {post_id, req.user.id, req.user.username, content}
        )
        db:close()
    end
    
    res:status(302):header("Location", "/post/" .. post_id):send("Redirecting...")
end)

-- AUTH ROUTES

-- Show Login
app:get("/login", function(req, res)
    res:render("examples/real-world-test/templates/login.lwt", {})
end)

-- Handle Login
app:post("/login", function(req, res)
    local form = parse_form_body(req.body)
    local username = form.username
    local password = form.password
    
    local db = app:db(DB_NAME)
    -- Simple plain text password check for demo
    local users = db:query("SELECT id, username FROM users WHERE username = ? AND password = ?", {username, password})
    
    if #users > 0 then
        -- Create session
        local token = generate_token()
        db:exec("INSERT INTO sessions (token, user_id) VALUES (?, ?)", {token, users[1].id})
        db:close()
        
        -- Set cookie
        res:cookie("session_token", token, { path = "/", httpOnly = true, maxAge = 86400 * 30 })
        res:status(302):header("Location", "/"):send("Redirecting...")
    else
        db:close()
        res:render("examples/real-world-test/templates/login.lwt", {
            error = "Invalid username or password"
        })
    end
end)

-- Show Register
app:get("/register", function(req, res)
    res:render("examples/real-world-test/templates/register.lwt", {})
end)

-- Handle Register
app:post("/register", function(req, res)
    local form = parse_form_body(req.body)
    local username = form.username
    local password = form.password
    
    if not username or username == "" or not password or password == "" then
        res:render("examples/real-world-test/templates/register.lwt", {error = "All fields required"})
        return
    end
    
    local db = app:db(DB_NAME)
    -- Check if exists
    local check = db:query("SELECT id FROM users WHERE username = ?", {username})
    if #check > 0 then
        db:close()
        res:render("examples/real-world-test/templates/register.lwt", {error = "Username taken"})
        return
    end
    
    -- Create user
    -- db:exec returns the last_insert_id as a number, not a table
    local user_id = db:exec("INSERT INTO users (username, password) VALUES (?, ?)", {username, password})
    
    -- For now, fetch ID manually if needed or just redirect to login
    db:close()
    
    res:status(302):header("Location", "/login"):send("Registered! Please login.")
end)

-- Logout
app:get("/logout", function(req, res)
    local token = req.cookies["session_token"]
    if token then
        local db = app:db(DB_NAME)
        db:exec("DELETE FROM sessions WHERE token = ?", {token})
        db:close()
    end
    
    res:clearCookie("session_token")
    res:status(302):header("Location", "/"):send("Logged out")
end)

-- Create Post (Authenticated)
app:post("/posts/create", function(req, res)
    if not req.user then
        res:status(403):text("Access denied")
        return
    end
    
    local form = parse_form_body(req.body)
    local content = form.content
    
    if content and content ~= "" then
        local db = app:db(DB_NAME)
        db:exec("INSERT INTO posts (user_id, author, content) VALUES (?, ?, ?)", 
               {req.user.id, req.user.username, content})
        db:close()
    end
    
    res:status(302):header("Location", "/"):send("Redirecting...")
end)

-- Delete Post (Authenticated + Owner check)
app:post("/posts/delete", function(req, res)
    if not req.user then
        res:status(403):text("Access denied")
        return
    end

    local form = parse_form_body(req.body)
    local id = tonumber(form.id)
    
    if id then
        local db = app:db(DB_NAME)
        -- Check ownership
        local posts = db:query("SELECT user_id FROM posts WHERE id = ?", {id})
        
        if #posts > 0 then
            -- Allow if user owns it OR if it's a legacy post (user_id is null) but we just delete it for demo
            if posts[1].user_id == req.user.id or not posts[1].user_id then
                db:exec("DELETE FROM posts WHERE id = ?", {id})
                db:exec("DELETE FROM replies WHERE post_id = ?", {id})
            end
        end
        db:close()
    end
    
    res:status(302):header("Location", "/"):send("Redirecting...")
end)

print("Starting LuaWeb Feed v2 (Auth + Replies)...")
print("Open http://localhost:8080")
app:run()
