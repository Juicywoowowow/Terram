//! LuaWeb Template Engine
//! 
//! Custom template syntax:
//! - @{variable} - Variable interpolation (HTML escaped)
//! - @raw{variable} - Raw variable (no escaping)
//! - @if condition ... @else ... @end
//! - @for item in items ... @end
//! - @include "partial.lwt"
//! - @-- comment

mod parser;
mod renderer;

use std::ffi::{CStr, CString};
use std::os::raw::c_char;
use std::ptr;
use std::sync::Mutex;
use std::collections::HashMap;
use std::fs;
use std::time::SystemTime;

use parser::parse_template;
use renderer::render;

// Global error storage
static LAST_ERROR: Mutex<Option<String>> = Mutex::new(None);

// Template cache: path -> (mtime, compiled template)
static TEMPLATE_CACHE: Mutex<Option<HashMap<String, (SystemTime, Vec<parser::Node>)>>> = Mutex::new(None);

fn set_error(msg: String) {
    if let Ok(mut guard) = LAST_ERROR.lock() {
        *guard = Some(msg);
    }
}

fn get_cache() -> &'static Mutex<Option<HashMap<String, (SystemTime, Vec<parser::Node>)>>> {
    &TEMPLATE_CACHE
}

/// Render a template file with JSON data
/// Returns rendered HTML or NULL on error
#[no_mangle]
pub extern "C" fn lwtemplate_render(
    template_path: *const c_char,
    json_data: *const c_char,
    cache_dir: *const c_char,
) -> *mut c_char {
    // Safety check
    if template_path.is_null() || json_data.is_null() {
        set_error("NULL argument passed".to_string());
        return ptr::null_mut();
    }
    
    // Convert C strings
    let template_path = match unsafe { CStr::from_ptr(template_path) }.to_str() {
        Ok(s) => s,
        Err(_) => {
            set_error("Invalid UTF-8 in template path".to_string());
            return ptr::null_mut();
        }
    };
    
    let json_data = match unsafe { CStr::from_ptr(json_data) }.to_str() {
        Ok(s) => s,
        Err(_) => {
            set_error("Invalid UTF-8 in JSON data".to_string());
            return ptr::null_mut();
        }
    };
    
    let cache_dir = if cache_dir.is_null() {
        None
    } else {
        match unsafe { CStr::from_ptr(cache_dir) }.to_str() {
            Ok(s) => Some(s),
            Err(_) => None,
        }
    };
    
    // Parse JSON data
    let data: serde_json::Value = match serde_json::from_str(json_data) {
        Ok(v) => v,
        Err(e) => {
            set_error(format!("JSON parse error: {}", e));
            return ptr::null_mut();
        }
    };
    
    // Check cache
    let mut cache_guard = match get_cache().lock() {
        Ok(g) => g,
        Err(_) => {
            set_error("Failed to acquire cache lock".to_string());
            return ptr::null_mut();
        }
    };
    
    if cache_guard.is_none() {
        *cache_guard = Some(HashMap::new());
    }
    
    let cache = cache_guard.as_mut().unwrap();
    
    // Get file modification time
    let mtime = match fs::metadata(template_path) {
        Ok(meta) => meta.modified().unwrap_or(SystemTime::UNIX_EPOCH),
        Err(e) => {
            set_error(format!("Cannot read template file '{}': {}", template_path, e));
            return ptr::null_mut();
        }
    };
    
    // Check if cached and up-to-date
    let nodes = if let Some((cached_mtime, cached_nodes)) = cache.get(template_path) {
        if *cached_mtime == mtime {
            cached_nodes.clone()
        } else {
            // Reparse - file changed
            match parse_and_cache(template_path, mtime, cache) {
                Ok(n) => n,
                Err(e) => {
                    set_error(e);
                    return ptr::null_mut();
                }
            }
        }
    } else {
        // Not in cache - parse it
        match parse_and_cache(template_path, mtime, cache) {
            Ok(n) => n,
            Err(e) => {
                set_error(e);
                return ptr::null_mut();
            }
        }
    };
    
    drop(cache_guard); // Release lock before rendering
    
    // Render template
    let html = match render(&nodes, &data, template_path) {
        Ok(h) => h,
        Err(e) => {
            set_error(format!("Render error: {}", e));
            return ptr::null_mut();
        }
    };
    
    // Optionally write to cache directory
    if let Some(dir) = cache_dir {
        let _ = fs::create_dir_all(dir);
        // Could write cached output here if needed
    }
    
    // Return as C string
    match CString::new(html) {
        Ok(s) => s.into_raw(),
        Err(_) => {
            set_error("Output contains null byte".to_string());
            ptr::null_mut()
        }
    }
}

fn parse_and_cache(
    path: &str,
    mtime: SystemTime,
    cache: &mut HashMap<String, (SystemTime, Vec<parser::Node>)>,
) -> Result<Vec<parser::Node>, String> {
    let content = fs::read_to_string(path)
        .map_err(|e| format!("Cannot read '{}': {}", path, e))?;
    
    let nodes = parse_template(&content)
        .map_err(|e| format!("Parse error in '{}': {}", path, e))?;
    
    cache.insert(path.to_string(), (mtime, nodes.clone()));
    Ok(nodes)
}

/// Free a string returned by lwtemplate_render
#[no_mangle]
pub extern "C" fn lwtemplate_free(ptr: *mut c_char) {
    if !ptr.is_null() {
        unsafe {
            let _ = CString::from_raw(ptr);
        }
    }
}

/// Get the last error message
#[no_mangle]
pub extern "C" fn lwtemplate_get_error() -> *const c_char {
    static mut ERROR_CSTRING: Option<CString> = None;
    
    if let Ok(guard) = LAST_ERROR.lock() {
        if let Some(ref msg) = *guard {
            if let Ok(cstr) = CString::new(msg.clone()) {
                unsafe {
                    ERROR_CSTRING = Some(cstr);
                    return ERROR_CSTRING.as_ref().unwrap().as_ptr();
                }
            }
        }
    }
    
    ptr::null()
}

/// Clear the template cache
#[no_mangle]
pub extern "C" fn lwtemplate_clear_cache() {
    if let Ok(mut guard) = get_cache().lock() {
        *guard = None;
    }
}
