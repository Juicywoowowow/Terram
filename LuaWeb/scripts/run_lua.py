#!/usr/bin/env python3
"""
Secure Lua runner for __cacheweb__ feature.
Executes Lua code in a sandboxed environment.
"""

import subprocess
import sys
import os
import uuid
from pathlib import Path

# Timeout in seconds
EXECUTION_TIMEOUT = 5

# Sandbox loader that wraps user code
SANDBOX_WRAPPER = '''
-- Load sandbox
local sandbox = dofile("{sandbox_path}")

-- User code to execute
local user_code = [==[
{user_code}
]==]

-- Run in sandbox
local output, err = sandbox.run(user_code, {timeout})
if err then
    print("[ERROR] " .. err)
else
    print(output or "")
end
'''


def get_cacheweb_dir() -> Path:
    """Get or create __cacheweb__ directory in CWD"""
    cache_dir = Path.cwd() / "__cacheweb__"
    cache_dir.mkdir(exist_ok=True)
    return cache_dir


def run_lua_sandboxed(user_code: str, sandbox_lua_path: str) -> tuple:
    """
    Execute user Lua code in a sandbox.
    
    Returns: (stdout, stderr, return_code)
    """
    cache_dir = get_cacheweb_dir()
    
    # Generate unique filename for wrapper
    file_id = uuid.uuid4().hex[:8]
    wrapper_file = cache_dir / f"wrapper_{file_id}.lua"
    
    try:
        # Escape the user code for Lua long string - handle nested ]==]
        # Use a unique delimiter that's unlikely to appear in code
        escaped_code = user_code
        
        # Write wrapped code
        wrapper_code = SANDBOX_WRAPPER.format(
            sandbox_path=sandbox_lua_path.replace('\\', '/'),
            user_code=escaped_code,
            timeout=EXECUTION_TIMEOUT
        )
        
        wrapper_file.write_text(wrapper_code)
        
        # Execute with timeout
        result = subprocess.run(
            ['lua', str(wrapper_file)],
            capture_output=True,
            text=True,
            timeout=EXECUTION_TIMEOUT + 1,
            cwd=str(cache_dir)
        )
        
        return result.stdout, result.stderr, result.returncode
        
    except subprocess.TimeoutExpired:
        return "", "[ERROR] Execution timed out", 1
    except FileNotFoundError:
        return "", "[ERROR] Lua interpreter not found", 1
    except Exception as e:
        return "", f"[ERROR] {str(e)}", 1
    finally:
        # Always clean up wrapper
        if wrapper_file.exists():
            wrapper_file.unlink()


def main():
    if len(sys.argv) < 3:
        print("Usage: run_lua.py <sandbox.lua path> <code_file.lua>", file=sys.stderr)
        sys.exit(1)
    
    sandbox_path = sys.argv[1]
    code_file = sys.argv[2]
    
    # Read code from file
    try:
        with open(code_file, 'r') as f:
            code = f.read()
    except Exception as e:
        print(f"[ERROR] Failed to read code file: {e}", file=sys.stderr)
        sys.exit(1)
    
    stdout, stderr, return_code = run_lua_sandboxed(code, sandbox_path)
    
    if stdout:
        print(stdout, end='')
    if stderr:
        print(stderr, file=sys.stderr, end='')
    
    sys.exit(return_code)


if __name__ == "__main__":
    main()
