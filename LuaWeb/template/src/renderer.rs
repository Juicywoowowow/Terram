//! Template Renderer
//!
//! Renders parsed template nodes with JSON data
//! Supports: filters, loop variables (_loop.index, etc.), and comparison operators

use crate::parser::{Node, Condition, Filter, CompareValue};
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
        
        Node::Variable { path, escape, default, filters } => {
            let value = resolve_path(data, path);
            let mut text = value_to_string(&value);
            
            // Apply default if empty
            if text.is_empty() {
                text = default.clone().unwrap_or_default();
            }
            
            // Apply filters
            text = apply_filters(text, filters, &value)?;
            
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
                let total = items.len();
                
                for (index, item) in items.iter().enumerate() {
                    // Create a new data context with loop variable
                    let mut loop_data = data.clone();
                    
                    if let Value::Object(ref mut map) = loop_data {
                        map.insert(var_name.clone(), item.clone());
                        
                        if let Some(idx_name) = index_name {
                            map.insert(idx_name.clone(), Value::Number((index as i64).into()));
                        }
                        
                        // Add _loop object with helpful properties
                        let mut loop_obj = serde_json::Map::new();
                        loop_obj.insert("index".to_string(), Value::Number((index as i64).into()));
                        loop_obj.insert("index1".to_string(), Value::Number(((index + 1) as i64).into()));
                        loop_obj.insert("first".to_string(), Value::Bool(index == 0));
                        loop_obj.insert("last".to_string(), Value::Bool(index == total - 1));
                        loop_obj.insert("length".to_string(), Value::Number((total as i64).into()));
                        loop_obj.insert("even".to_string(), Value::Bool(index % 2 == 0));
                        loop_obj.insert("odd".to_string(), Value::Bool(index % 2 == 1));
                        
                        map.insert("_loop".to_string(), Value::Object(loop_obj));
                    }
                    
                    for node in body {
                        render_node(node, &loop_data, template_path, output)?;
                    }
                }
            } else if let Value::Object(obj) = array {
                // Iterate over object keys
                let total = obj.len();
                
                for (index, (key, value)) in obj.iter().enumerate() {
                    let mut loop_data = data.clone();
                    
                    if let Value::Object(ref mut map) = loop_data {
                        // For objects, var_name gets the value, we can add key as well
                        map.insert(var_name.clone(), value.clone());
                        map.insert("_key".to_string(), Value::String(key.clone()));
                        
                        if let Some(idx_name) = index_name {
                            map.insert(idx_name.clone(), Value::Number((index as i64).into()));
                        }
                        
                        // Add _loop object
                        let mut loop_obj = serde_json::Map::new();
                        loop_obj.insert("index".to_string(), Value::Number((index as i64).into()));
                        loop_obj.insert("index1".to_string(), Value::Number(((index + 1) as i64).into()));
                        loop_obj.insert("first".to_string(), Value::Bool(index == 0));
                        loop_obj.insert("last".to_string(), Value::Bool(index == total - 1));
                        loop_obj.insert("length".to_string(), Value::Number((total as i64).into()));
                        loop_obj.insert("key".to_string(), Value::String(key.clone()));
                        
                        map.insert("_loop".to_string(), Value::Object(loop_obj));
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

/// Apply a chain of filters to a value
fn apply_filters(mut text: String, filters: &[Filter], original_value: &Value) -> Result<String, String> {
    for filter in filters {
        text = apply_single_filter(text, filter, original_value)?;
    }
    Ok(text)
}

/// Apply a single filter
fn apply_single_filter(text: String, filter: &Filter, original_value: &Value) -> Result<String, String> {
    match filter {
        Filter::Upper => Ok(text.to_uppercase()),
        Filter::Lower => Ok(text.to_lowercase()),
        Filter::Capitalize => {
            let mut chars = text.chars();
            match chars.next() {
                Some(first) => Ok(first.to_uppercase().chain(chars).collect()),
                None => Ok(String::new()),
            }
        }
        Filter::Title => {
            Ok(text.split_whitespace()
                .map(|word| {
                    let mut chars = word.chars();
                    match chars.next() {
                        Some(first) => first.to_uppercase().chain(chars).collect(),
                        None => String::new(),
                    }
                })
                .collect::<Vec<_>>()
                .join(" "))
        }
        Filter::Trim => Ok(text.trim().to_string()),
        Filter::Length => {
            match original_value {
                Value::Array(arr) => Ok(arr.len().to_string()),
                Value::Object(obj) => Ok(obj.len().to_string()),
                Value::String(s) => Ok(s.chars().count().to_string()),
                _ => Ok(text.chars().count().to_string()),
            }
        }
        Filter::Reverse => Ok(text.chars().rev().collect()),
        Filter::First => {
            match original_value {
                Value::Array(arr) => {
                    if let Some(first) = arr.first() {
                        Ok(value_to_string(first))
                    } else {
                        Ok(String::new())
                    }
                }
                _ => Ok(text.chars().next().map(|c| c.to_string()).unwrap_or_default()),
            }
        }
        Filter::Last => {
            match original_value {
                Value::Array(arr) => {
                    if let Some(last) = arr.last() {
                        Ok(value_to_string(last))
                    } else {
                        Ok(String::new())
                    }
                }
                _ => Ok(text.chars().last().map(|c| c.to_string()).unwrap_or_default()),
            }
        }
        Filter::Default(default_val) => {
            if text.is_empty() {
                Ok(default_val.clone())
            } else {
                Ok(text)
            }
        }
        Filter::Truncate(len) => {
            if text.chars().count() > *len {
                let truncated: String = text.chars().take(*len).collect();
                Ok(format!("{}...", truncated))
            } else {
                Ok(text)
            }
        }
        Filter::Join(sep) => {
            match original_value {
                Value::Array(arr) => {
                    let items: Vec<String> = arr.iter().map(value_to_string).collect();
                    Ok(items.join(sep))
                }
                _ => Ok(text),
            }
        }
        Filter::Replace(old, new) => {
            Ok(text.replace(old, new))
        }
        Filter::Slice(start, end) => {
            let chars: Vec<char> = text.chars().collect();
            let len = chars.len() as i64;
            
            let start_idx = if *start < 0 { (len + start).max(0) as usize } else { (*start as usize).min(chars.len()) };
            let end_idx = match end {
                Some(e) if *e < 0 => (len + e).max(0) as usize,
                Some(e) => (*e as usize).min(chars.len()),
                None => chars.len(),
            };
            
            if start_idx < end_idx {
                Ok(chars[start_idx..end_idx].iter().collect())
            } else {
                Ok(String::new())
            }
        }
        Filter::Abs => {
            if let Ok(num) = text.parse::<f64>() {
                Ok(num.abs().to_string())
            } else {
                Ok(text)
            }
        }
        Filter::Round => {
            if let Ok(num) = text.parse::<f64>() {
                Ok(num.round().to_string())
            } else {
                Ok(text)
            }
        }
        Filter::Floor => {
            if let Ok(num) = text.parse::<f64>() {
                Ok(num.floor().to_string())
            } else {
                Ok(text)
            }
        }
        Filter::Ceil => {
            if let Ok(num) = text.parse::<f64>() {
                Ok(num.ceil().to_string())
            } else {
                Ok(text)
            }
        }
    }
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
        Value::Number(n) => {
            if let Some(i) = n.as_i64() {
                i.to_string()
            } else {
                n.to_string()
            }
        },
        Value::String(s) => s.clone(),
        Value::Array(_) => "[Array]".to_string(),
        Value::Object(_) => "[Object]".to_string(),
    }
}

/// Convert JSON value to f64 for numeric comparisons
fn value_to_number(value: &Value) -> Option<f64> {
    match value {
        Value::Number(n) => n.as_f64(),
        Value::String(s) => s.parse::<f64>().ok(),
        Value::Bool(b) => Some(if *b { 1.0 } else { 0.0 }),
        _ => None,
    }
}

/// Resolve a CompareValue to an actual value
fn resolve_compare_value(cv: &CompareValue, data: &Value) -> Value {
    match cv {
        CompareValue::String(s) => Value::String(s.clone()),
        CompareValue::Number(n) => serde_json::json!(*n),
        CompareValue::Bool(b) => Value::Bool(*b),
        CompareValue::Path(path) => resolve_path(data, path),
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
            let left = resolve_path(data, path);
            let right = resolve_compare_value(expected, data);
            values_equal(&left, &right)
        }
        Condition::NotEquals(path, expected) => {
            let left = resolve_path(data, path);
            let right = resolve_compare_value(expected, data);
            !values_equal(&left, &right)
        }
        Condition::GreaterThan(path, expected) => {
            let left = resolve_path(data, path);
            let right = resolve_compare_value(expected, data);
            compare_values(&left, &right) == Some(std::cmp::Ordering::Greater)
        }
        Condition::LessThan(path, expected) => {
            let left = resolve_path(data, path);
            let right = resolve_compare_value(expected, data);
            compare_values(&left, &right) == Some(std::cmp::Ordering::Less)
        }
        Condition::GreaterOrEqual(path, expected) => {
            let left = resolve_path(data, path);
            let right = resolve_compare_value(expected, data);
            matches!(compare_values(&left, &right), Some(std::cmp::Ordering::Greater) | Some(std::cmp::Ordering::Equal))
        }
        Condition::LessOrEqual(path, expected) => {
            let left = resolve_path(data, path);
            let right = resolve_compare_value(expected, data);
            matches!(compare_values(&left, &right), Some(std::cmp::Ordering::Less) | Some(std::cmp::Ordering::Equal))
        }
        Condition::And(left, right) => {
            evaluate_condition(left, data) && evaluate_condition(right, data)
        }
        Condition::Or(left, right) => {
            evaluate_condition(left, data) || evaluate_condition(right, data)
        }
    }
}

/// Check if two values are equal
fn values_equal(left: &Value, right: &Value) -> bool {
    match (left, right) {
        (Value::Null, Value::Null) => true,
        (Value::Bool(a), Value::Bool(b)) => a == b,
        (Value::Number(a), Value::Number(b)) => {
            a.as_f64().zip(b.as_f64()).map(|(a, b)| (a - b).abs() < f64::EPSILON).unwrap_or(false)
        }
        (Value::String(a), Value::String(b)) => a == b,
        // Cross-type comparisons
        (Value::Number(n), Value::String(s)) | (Value::String(s), Value::Number(n)) => {
            n.as_f64().map(|n| n.to_string() == *s || s.parse::<f64>().ok() == Some(n)).unwrap_or(false)
        }
        _ => false,
    }
}

/// Compare two values, returning ordering if comparable
fn compare_values(left: &Value, right: &Value) -> Option<std::cmp::Ordering> {
    let left_num = value_to_number(left);
    let right_num = value_to_number(right);
    
    if let (Some(l), Some(r)) = (left_num, right_num) {
        Some(l.partial_cmp(&r).unwrap_or(std::cmp::Ordering::Equal))
    } else {
        // String comparison
        let left_str = value_to_string(left);
        let right_str = value_to_string(right);
        Some(left_str.cmp(&right_str))
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
    
    #[test]
    fn test_upper_filter() {
        let nodes = crate::parser::parse_template("@{name | upper}").unwrap();
        let data = json!({"name": "hello"});
        let result = render(&nodes, &data, "test.lwt").unwrap();
        assert_eq!(result, "HELLO");
    }
    
    #[test]
    fn test_comparison() {
        let nodes = crate::parser::parse_template("@if age >= 18\nAdult\n@end").unwrap();
        let data = json!({"age": 21});
        let result = render(&nodes, &data, "test.lwt").unwrap();
        assert!(result.contains("Adult"));
    }
}
