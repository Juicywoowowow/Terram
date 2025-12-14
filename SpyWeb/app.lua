-- SpyWeb - A hobby website built with LuaWeb
-- Showcases the new LWT template features

local luaweb = require("luaweb")

-- Create server on port 3000
local app = luaweb.server(3000)

-- ============================================================
-- SAMPLE DATA
-- ============================================================

local site_data = {
    title = "SpyWeb",
    tagline = "Your Cozy Corner of the Internet",
    version = "1.0.0",
    author = {
        name = "Agent X",
        email = "agent@spyweb.local",
        bio = "A mysterious developer who loves Lua, Rust, and building cool stuff on the web."
    }
}

local blog_posts = {
    {
        id = 1,
        title = "Getting Started with LuaWeb",
        slug = "getting-started-luaweb",
        excerpt = "Learn how to build web applications with LuaWeb's simple yet powerful API. We'll cover routing, templates, and more!",
        content = "LuaWeb is a fantastic framework that combines the simplicity of Lua with the power of a modern web server. In this post, we'll explore the basics...",
        category = "tutorials",
        tags = {"lua", "web", "beginner"},
        views = 1542,
        date = "2025-12-10",
        featured = true
    },
    {
        id = 2,
        title = "Mastering LWT Templates",
        slug = "mastering-lwt-templates",
        excerpt = "Deep dive into LWT's template syntax including the new filters, loop variables, and comparison operators.",
        content = "The LWT template engine has recently received major upgrades. Let's explore filters like |upper, |truncate, and the powerful _loop variables...",
        category = "tutorials",
        tags = {"templates", "lwt", "advanced"},
        views = 892,
        date = "2025-12-12",
        featured = true
    },
    {
        id = 3,
        title = "Building a REST API",
        slug = "building-rest-api",
        excerpt = "Create a complete RESTful API with LuaWeb including authentication and database integration.",
        content = "APIs are the backbone of modern applications. In this tutorial, we'll build a complete REST API...",
        category = "tutorials",
        tags = {"api", "rest", "database"},
        views = 2341,
        date = "2025-12-08",
        featured = false
    },
    {
        id = 4,
        title = "What I Learned This Week",
        slug = "weekly-learnings",
        excerpt = "Random thoughts and discoveries from my coding adventures this week.",
        content = "This week was full of interesting discoveries. I learned about Rust's ownership model, explored new Lua patterns...",
        category = "personal",
        tags = {"journal", "learning"},
        views = 256,
        date = "2025-12-13",
        featured = false
    }
}

local projects = {
    {
        name = "LuaWeb",
        description = "A stupidly easy-to-use HTTP web server library for Lua",
        status = "active",
        stars = 142,
        language = "Multi-language",
        url = "https://github.com/example/luaweb"
    },
    {
        name = "TermEditor",
        description = "A minimalist terminal text editor with mouse support",
        status = "active",
        stars = 89,
        language = "C/Lua",
        url = "https://github.com/example/termeditor"
    },
    {
        name = "PatternGen",
        description = "Neural network-based pattern generator using TensorFlow",
        status = "completed",
        stars = 45,
        language = "Python",
        url = "https://github.com/example/patterngen"
    },
    {
        name = "Erode Lang",
        description = "A compiled language with ARM64 native code generation",
        status = "wip",
        stars = 23,
        language = "C",
        url = "https://github.com/example/erode"
    }
}

local stats = {
    total_visitors = 15420,
    posts_written = #blog_posts,
    projects_count = #projects,
    cups_of_coffee = 9001,
    lines_of_code = 142857
}

-- ============================================================
-- MIDDLEWARE
-- ============================================================

-- Logging
app:use(function(req, res, next)
    local timestamp = os.date("%Y-%m-%d %H:%M:%S")
    print(string.format("[%s] %s %s", timestamp, req.method, req.path))
    next()
end)

-- Visit counter via cookies
app:use(function(req, res, next)
    local visits = tonumber(req.cookies.visits) or 0
    visits = visits + 1
    res:cookie("visits", tostring(visits), {
        maxAge = 86400 * 30, -- 30 days
        path = "/"
    })
    -- Store for templates
    req.visit_count = visits
    next()
end)

-- ============================================================
-- STATIC FILES
-- ============================================================

app:static("/public", "./public")

-- Helper to get visit count
local function get_visits(req)
    return tonumber(req.cookies.visits) or 0
end

-- ============================================================
-- ROUTES
-- ============================================================

-- Home Page
app:get("/", function(req, res)
    res:render("templates/index.lwt", {
        site = site_data,
        posts = blog_posts,
        projects = projects,
        stats = stats,
        visits = get_visits(req),
        current_page = "home"
    })
end)

-- Blog listing
app:get("/blog", function(req, res)
    local category = req.query.category
    local filtered = {}
    
    if category then
        for _, post in ipairs(blog_posts) do
            if post.category == category then
                table.insert(filtered, post)
            end
        end
    else
        filtered = blog_posts
    end
    
    res:render("templates/blog.lwt", {
        site = site_data,
        posts = filtered,
        category = category,
        visits = get_visits(req),
        current_page = "blog"
    })
end)

-- Single blog post
app:get("/blog/:slug", function(req, res)
    local slug = req.params.slug
    local found = nil
    
    for _, post in ipairs(blog_posts) do
        if post.slug == slug then
            found = post
            break
        end
    end
    
    if found then
        res:render("templates/post.lwt", {
            site = site_data,
            post = found,
            visits = get_visits(req),
            current_page = "blog"
        })
    else
        res:status(404):send("<h1>Post Not Found</h1><a href='/blog'>Back to Blog</a>")
    end
end)

-- Projects page
app:get("/projects", function(req, res)
    res:render("templates/projects.lwt", {
        site = site_data,
        projects = projects,
        visits = get_visits(req),
        current_page = "projects"
    })
end)

-- Stats dashboard
app:get("/stats", function(req, res)
    -- Calculate some dynamic stats
    local total_views = 0
    local popular_posts = {}
    
    for _, post in ipairs(blog_posts) do
        total_views = total_views + post.views
        if post.views > 500 then
            table.insert(popular_posts, post)
        end
    end
    
    res:render("templates/stats.lwt", {
        site = site_data,
        stats = stats,
        posts = blog_posts,
        popular_posts = popular_posts,
        total_views = total_views,
        visits = get_visits(req),
        current_page = "stats"
    })
end)

-- About page
app:get("/about", function(req, res)
    res:render("templates/about.lwt", {
        site = site_data,
        author = site_data.author,
        visits = get_visits(req),
        current_page = "about"
    })
end)

-- API endpoint for JSON data
app:get("/api/posts", function(req, res)
    local json_str = '['
    for i, post in ipairs(blog_posts) do
        json_str = json_str .. string.format(
            '{"id":%d,"title":"%s","views":%d,"category":"%s"}',
            post.id, post.title, post.views, post.category
        )
        if i < #blog_posts then json_str = json_str .. ',' end
    end
    json_str = json_str .. ']'
    res:json(json_str)
end)

-- ============================================================
-- START SERVER
-- ============================================================

print("🕵️  SpyWeb starting on http://localhost:3000")
print("   Features: Filters | Loop Variables | Comparisons")
print("   Press Ctrl+C to stop")
app:run()
