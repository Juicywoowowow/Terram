-- sandbox.lua - Strict Lua sandbox for web execution
-- This file sets up a safe environment before running untrusted code

local sandbox = {}

-- Dummy function that does nothing
local function dummy_noop()
    return nil
end

-- Dummy function that returns an error message
local function dummy_blocked(name)
    return function(...)
        return nil, "[SANDBOX] " .. name .. " is blocked"
    end
end

-- Dummy io object - all methods blocked
local dummy_io = {
    open = dummy_blocked("io.open"),
    close = dummy_noop,
    read = dummy_blocked("io.read"),
    write = dummy_blocked("io.write"),
    lines = dummy_blocked("io.lines"),
    input = dummy_blocked("io.input"),
    output = dummy_blocked("io.output"),
    popen = dummy_blocked("io.popen"),
    tmpfile = dummy_blocked("io.tmpfile"),
    type = function() return nil end,
    flush = dummy_noop,
    stdin = nil,
    stdout = nil,
    stderr = nil,
}

-- Dummy os object - most methods blocked, safe ones allowed
local dummy_os = {
    -- BLOCKED (dangerous)
    execute = dummy_blocked("os.execute"),
    exit = dummy_blocked("os.exit"),
    getenv = dummy_blocked("os.getenv"),
    remove = dummy_blocked("os.remove"),
    rename = dummy_blocked("os.rename"),
    setlocale = dummy_blocked("os.setlocale"),
    tmpname = dummy_blocked("os.tmpname"),
    
    -- ALLOWED (safe)
    clock = os.clock,
    date = os.date,
    difftime = os.difftime,
    time = os.time,
}

-- Safe globals whitelist
local safe_globals = {
    -- Basic
    "_VERSION",
    "assert",
    "error",
    "ipairs",
    "next",
    "pairs",
    "pcall",
    "xpcall",
    "select",
    "tonumber",
    "tostring",
    "type",
    "unpack",
    
    -- Tables
    "table",
    
    -- Strings
    "string",
    
    -- Math
    "math",
    
    -- Coroutines (safe, no I/O)
    "coroutine",
    
    -- UTF-8 (Lua 5.3+)
    "utf8",
}

-- Build the sandbox environment
function sandbox.create_env()
    local env = {}
    
    -- Copy safe globals
    for _, name in ipairs(safe_globals) do
        env[name] = _G[name]
    end
    
    -- Replace dangerous modules with dummies
    env.io = dummy_io
    env.os = dummy_os
    env.debug = nil  -- Completely blocked
    env.package = nil
    env.require = dummy_blocked("require")
    env.dofile = dummy_blocked("dofile")
    env.loadfile = dummy_blocked("loadfile")
    env.load = dummy_blocked("load")
    env.loadstring = dummy_blocked("loadstring")
    env.rawget = dummy_blocked("rawget")
    env.rawset = dummy_blocked("rawset")
    env.rawequal = dummy_blocked("rawequal")
    env.rawlen = dummy_blocked("rawlen")
    env.collectgarbage = dummy_blocked("collectgarbage")
    env.getfenv = dummy_blocked("getfenv")
    env.setfenv = dummy_blocked("setfenv")
    env.module = dummy_blocked("module")
    env.newproxy = dummy_blocked("newproxy")
    env.getmetatable = dummy_blocked("getmetatable")
    env.setmetatable = dummy_blocked("setmetatable")
    
    -- Capture print output instead of stdout
    local output_buffer = {}
    env.print = function(...)
        local args = {...}
        local parts = {}
        for i = 1, select("#", ...) do
            parts[i] = tostring(args[i])
        end
        table.insert(output_buffer, table.concat(parts, "\t"))
    end
    
    -- Function to get captured output
    env.__get_output = function()
        return table.concat(output_buffer, "\n")
    end
    
    -- Self-reference
    env._G = env
    
    return env
end

-- Run code in sandbox with timeout
function sandbox.run(code, timeout_seconds)
    timeout_seconds = timeout_seconds or 5
    
    local env = sandbox.create_env()
    
    -- Compile the code in text mode only (no bytecode)
    local fn, err = load(code, "=sandbox", "t", env)
    if not fn then
        return nil, "Compile error: " .. tostring(err)
    end
    
    -- Set instruction limit using debug hooks (before we enter sandbox)
    local instruction_count = 0
    local max_instructions = 1000000  -- 1 million instructions
    
    local function instruction_hook()
        instruction_count = instruction_count + 1
        if instruction_count > max_instructions then
            error("[SANDBOX] Instruction limit exceeded", 2)
        end
    end
    
    -- Install the hook
    debug.sethook(instruction_hook, "", 10000)
    
    -- Execute with pcall for error handling
    local ok, result = pcall(fn)
    
    -- Remove the hook
    debug.sethook()
    
    if not ok then
        return nil, "Runtime error: " .. tostring(result)
    end
    
    -- Return captured output
    return env.__get_output(), nil
end

return sandbox
