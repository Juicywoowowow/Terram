//! Template Renderer
//!
//! Renders parsed template nodes with JSON data

use crate::parser::{Node, Condition};
use serde_json::Value;
use std::path::Path;
use std::fs;

/// Render nodes with data, returning HTML string
pub fn render(nodes: &[Node], data: &Value, template_path: &str) -> Result<String, String> {
    let mut output = String::new();
    
    for node in nodes {
        render_node(node, data, template_path, &mut output)?;
    }
    
    Ok(output)
}

fn render_node(node: &Node, data: &Value, template_path: &str, output: &mut String) -> Result<(), String> {
    match node {
        Node::Text(text) => {
            output.push_str(text);
        }
        
        Node::Variable { path, escape, default } => {
            let value = resolve_path(data, path);
            let text = value_to_string(&value);
            
            let text = if text.is_empty() {
                default.clone().unwrap_or_default()
            } else {
                text
            };
            
            if *escape {
                output.push_str(&html_escape(&text));
            } else {
                output.push_str(&text);
            }
        }
        
        Node::If { condition, then_branch, else_branch } => {
            let result = evaluate_condition(condition, data);
            
            if result {
                for node in then_branch {
                    render_node(node, data, template_path, output)?;
                }
            } else {
                for node in else_branch {
                    render_node(node, data, template_path, output)?;
                }
            }
        }
        
        Node::For { var_name, index_name, iterable, body } => {
            let array = resolve_path(data, iterable);
            
            if let Value::Array(items) = array {
                for (index, item) in items.iter().enumerate() {
                    // Create a new data context with loop variable
                    let mut loop_data = data.clone();
                    
                    if let Value::Object(ref mut map) = loop_data {
                        map.insert(var_name.clone(), item.clone());
                        
                        if let Some(idx_name) = index_name {
                            map.insert(idx_name.clone(), Value::Number((index as i64).into()));
                        }
                    }
                    
                    for node in body {
                        render_node(node, &loop_data, template_path, output)?;
                    }
                }
            } else if let Value::Object(obj) = array {
                // Iterate over object keys
                for (index, (key, value)) in obj.iter().enumerate() {
                    let mut loop_data = data.clone();
                    
                    if let Value::Object(ref mut map) = loop_data {
                        // For objects, var_name gets the value, we can add key as well
                        map.insert(var_name.clone(), value.clone());
                        map.insert("_key".to_string(), Value::String(key.clone()));
                        
                        if let Some(idx_name) = index_name {
                            map.insert(idx_name.clone(), Value::Number((index as i64).into()));
                        }
                    }
                    
                    for node in body {
                        render_node(node, &loop_data, template_path, output)?;
                    }
                }
            }
        }
        
        Node::Include(path) => {
            // Resolve path relative to current template
            let base_dir = Path::new(template_path).parent().unwrap_or(Path::new("."));
            let include_path = base_dir.join(path);
            
            let content = fs::read_to_string(&include_path)
                .map_err(|e| format!("Cannot include '{}': {}", include_path.display(), e))?;
            
            let nodes = crate::parser::parse_template(&content)
                .map_err(|e| format!("Error parsing included '{}': {}", include_path.display(), e))?;
            
            for node in &nodes {
                render_node(node, data, include_path.to_str().unwrap_or(template_path), output)?;
            }
        }
    }
    
    Ok(())
}

/// Resolve a dotted path in JSON data
fn resolve_path(data: &Value, path: &[String]) -> Value {
    let mut current = data;
    
    for key in path {
        current = match current {
            Value::Object(map) => map.get(key).unwrap_or(&Value::Null),
            Value::Array(arr) => {
                if let Ok(index) = key.parse::<usize>() {
                    arr.get(index).unwrap_or(&Value::Null)
                } else {
                    &Value::Null
                }
            }
            _ => &Value::Null,
        };
    }
    
    current.clone()
}

/// Convert JSON value to string
fn value_to_string(value: &Value) -> String {
    match value {
        Value::Null => String::new(),
        Value::Bool(b) => b.to_string(),
        Value::Number(n) => n.to_string(),
        Value::String(s) => s.clone(),
        Value::Array(_) => "[Array]".to_string(),
        Value::Object(_) => "[Object]".to_string(),
    }
}

/// Evaluate a condition against data
fn evaluate_condition(condition: &Condition, data: &Value) -> bool {
    match condition {
        Condition::Truthy(path) => {
            let value = resolve_path(data, path);
            is_truthy(&value)
        }
        Condition::Falsy(path) => {
            let value = resolve_path(data, path);
            !is_truthy(&value)
        }
        Condition::Equals(path, expected) => {
            let value = resolve_path(data, path);
            value_to_string(&value) == *expected
        }
        Condition::NotEquals(path, expected) => {
            let value = resolve_path(data, path);
            value_to_string(&value) != *expected
        }
    }
}

/// Check if a JSON value is truthy
fn is_truthy(value: &Value) -> bool {
    match value {
        Value::Null => false,
        Value::Bool(b) => *b,
        Value::Number(n) => n.as_f64().map(|f| f != 0.0).unwrap_or(false),
        Value::String(s) => !s.is_empty(),
        Value::Array(a) => !a.is_empty(),
        Value::Object(o) => !o.is_empty(),
    }
}

/// HTML escape a string
fn html_escape(s: &str) -> String {
    let mut result = String::with_capacity(s.len());
    
    for c in s.chars() {
        match c {
            '&' => result.push_str("&amp;"),
            '<' => result.push_str("&lt;"),
            '>' => result.push_str("&gt;"),
            '"' => result.push_str("&quot;"),
            '\'' => result.push_str("&#x27;"),
            _ => result.push(c),
        }
    }
    
    result
}

#[cfg(test)]
mod tests {
    use super::*;
    use serde_json::json;
    
    #[test]
    fn test_variable_interpolation() {
        let nodes = crate::parser::parse_template("Hello @{name}!").unwrap();
        let data = json!({"name": "World"});
        let result = render(&nodes, &data, "test.lwt").unwrap();
        assert_eq!(result, "Hello World!");
    }
    
    #[test]
    fn test_html_escaping() {
        let nodes = crate::parser::parse_template("@{content}").unwrap();
        let data = json!({"content": "<script>alert('xss')</script>"});
        let result = render(&nodes, &data, "test.lwt").unwrap();
        assert!(result.contains("&lt;script&gt;"));
    }
    
    #[test]
    fn test_condition() {
        let nodes = crate::parser::parse_template("@if show\nYES\n@end").unwrap();
        let data = json!({"show": true});
        let result = render(&nodes, &data, "test.lwt").unwrap();
        assert!(result.contains("YES"));
    }
}
