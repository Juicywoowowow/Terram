//! Template Parser
//! 
//! Parses LuaWeb template syntax into an AST
//!
//! Supports:
//! - @{variable} - Variable interpolation (HTML escaped)
//! - @{variable | filter} - With filters (upper, lower, capitalize, length, trim, etc.)
//! - @raw{variable} - Raw variable (no escaping)
//! - @if condition ... @else ... @end (with comparison operators)
//! - @for item in items ... @end (with loop variables)
//! - @include "partial.lwt"
//! - @-- comment

use std::iter::Peekable;
use std::str::Chars;

/// Filter to apply to a variable
#[derive(Debug, Clone)]
pub enum Filter {
    Upper,
    Lower,
    Capitalize,
    Title,
    Trim,
    Length,
    Reverse,
    First,
    Last,
    Default(String),
    Truncate(usize),
    Join(String),
    Replace(String, String),
    Slice(i64, Option<i64>),
    Abs,
    Round,
    Floor,
    Ceil,
}

/// AST Node types
#[derive(Debug, Clone)]
pub enum Node {
    /// Raw text to output as-is
    Text(String),
    
    /// Variable interpolation: @{var} or @{obj.field}
    Variable {
        path: Vec<String>,
        escape: bool,  // true = HTML escape, false = raw
        default: Option<String>,
        filters: Vec<Filter>,  // NEW: chain of filters
    },
    
    /// Conditional: @if condition ... @else ... @end
    If {
        condition: Condition,
        then_branch: Vec<Node>,
        else_branch: Vec<Node>,
    },
    
    /// Loop: @for item in items ... @end
    For {
        var_name: String,
        index_name: Option<String>,
        iterable: Vec<String>,
        body: Vec<Node>,
    },
    
    /// Include: @include "path.lwt"
    Include(String),
}

/// Condition for @if - now with full comparison operators
#[derive(Debug, Clone)]
pub enum Condition {
    /// Variable is truthy
    Truthy(Vec<String>),
    /// Variable is falsy (negated)
    Falsy(Vec<String>),
    /// Comparison: var == value
    Equals(Vec<String>, CompareValue),
    /// Comparison: var != value
    NotEquals(Vec<String>, CompareValue),
    /// Comparison: var > value
    GreaterThan(Vec<String>, CompareValue),
    /// Comparison: var < value
    LessThan(Vec<String>, CompareValue),
    /// Comparison: var >= value
    GreaterOrEqual(Vec<String>, CompareValue),
    /// Comparison: var <= value
    LessOrEqual(Vec<String>, CompareValue),
    /// Logical AND
    And(Box<Condition>, Box<Condition>),
    /// Logical OR
    Or(Box<Condition>, Box<Condition>),
}

/// Value to compare against
#[derive(Debug, Clone)]
pub enum CompareValue {
    String(String),
    Number(f64),
    Bool(bool),
    Path(Vec<String>),  // Compare to another variable
}

/// Parse a template string into AST nodes
pub fn parse_template(input: &str) -> Result<Vec<Node>, String> {
    let mut nodes = Vec::new();
    let mut chars = input.chars().peekable();
    let mut text_buf = String::new();
    
    while let Some(c) = chars.next() {
        if c == '@' {
            // Check what follows @
            match chars.peek() {
                Some('{') => {
                    // Flush text buffer
                    if !text_buf.is_empty() {
                        nodes.push(Node::Text(text_buf.clone()));
                        text_buf.clear();
                    }
                    chars.next(); // consume '{'
                    nodes.push(parse_variable(&mut chars, true)?);
                }
                Some('-') => {
                    // Check for comment: @--
                    chars.next();
                    if chars.peek() == Some(&'-') {
                        chars.next();
                        // Skip until end of line
                        while let Some(c) = chars.next() {
                            if c == '\n' {
                                break;
                            }
                        }
                    } else {
                        text_buf.push('@');
                        text_buf.push('-');
                    }
                }
                Some('r') => {
                    // Check for @raw{
                    let mut peek = String::new();
                    let mut temp_chars: Vec<char> = Vec::new();
                    
                    // Peek ahead for "raw{"
                    for _ in 0..4 {
                        if let Some(&c) = chars.peek() {
                            peek.push(c);
                            temp_chars.push(chars.next().unwrap());
                        }
                    }
                    
                    if peek == "raw{" {
                        if !text_buf.is_empty() {
                            nodes.push(Node::Text(text_buf.clone()));
                            text_buf.clear();
                        }
                        nodes.push(parse_variable(&mut chars, false)?);
                    } else {
                        text_buf.push('@');
                        for c in temp_chars {
                            text_buf.push(c);
                        }
                    }
                }
                Some('i') => {
                    // @if or @include
                    let keyword = read_keyword(&mut chars);
                    if keyword == "if" {
                        if !text_buf.is_empty() {
                            nodes.push(Node::Text(text_buf.clone()));
                            text_buf.clear();
                        }
                        nodes.push(parse_if(&mut chars)?);
                    } else if keyword == "include" {
                        if !text_buf.is_empty() {
                            nodes.push(Node::Text(text_buf.clone()));
                            text_buf.clear();
                        }
                        nodes.push(parse_include(&mut chars)?);
                    } else {
                        text_buf.push('@');
                        text_buf.push_str(&keyword);
                    }
                }
                Some('f') => {
                    // @for
                    let keyword = read_keyword(&mut chars);
                    if keyword == "for" {
                        if !text_buf.is_empty() {
                            nodes.push(Node::Text(text_buf.clone()));
                            text_buf.clear();
                        }
                        nodes.push(parse_for(&mut chars)?);
                    } else {
                        text_buf.push('@');
                        text_buf.push_str(&keyword);
                    }
                }
                Some('@') => {
                    // Escaped @: @@
                    chars.next();
                    text_buf.push('@');
                }
                _ => {
                    text_buf.push('@');
                }
            }
        } else {
            text_buf.push(c);
        }
    }
    
    // Flush remaining text
    if !text_buf.is_empty() {
        nodes.push(Node::Text(text_buf));
    }
    
    Ok(nodes)
}

fn read_keyword(chars: &mut Peekable<Chars>) -> String {
    let mut keyword = String::new();
    while let Some(&c) = chars.peek() {
        if c.is_alphabetic() {
            keyword.push(chars.next().unwrap());
        } else {
            break;
        }
    }
    keyword
}

fn parse_variable(chars: &mut Peekable<Chars>, escape: bool) -> Result<Node, String> {
    let mut path = Vec::new();
    let mut current = String::new();
    let mut default = None;
    let mut filters = Vec::new();
    
    while let Some(c) = chars.next() {
        match c {
            '}' => {
                if !current.is_empty() {
                    path.push(current);
                }
                return Ok(Node::Variable { path, escape, default, filters });
            }
            '.' => {
                if !current.is_empty() {
                    path.push(current);
                    current = String::new();
                }
            }
            '|' => {
                if !current.is_empty() {
                    path.push(current);
                    current = String::new();
                }
                // Parse filter or default value
                skip_whitespace(chars);
                
                // Check if it's a quoted default value or a filter name
                if chars.peek() == Some(&'"') || chars.peek() == Some(&'\'') {
                    default = Some(parse_string_or_value(chars)?);
                } else {
                    // Parse filter name
                    if let Some(filter) = parse_filter(chars)? {
                        filters.push(filter);
                    }
                }
            }
            c if c.is_alphanumeric() || c == '_' => {
                current.push(c);
            }
            c if c.is_whitespace() => {
                // Skip whitespace
            }
            _ => {
                return Err(format!("Unexpected character '{}' in variable", c));
            }
        }
    }
    
    Err("Unclosed variable: expected '}'".to_string())
}

/// Parse a filter like `upper`, `truncate:50`, `replace:"old":"new"`
fn parse_filter(chars: &mut Peekable<Chars>) -> Result<Option<Filter>, String> {
    let mut name = String::new();
    
    // Read filter name
    while let Some(&c) = chars.peek() {
        if c.is_alphabetic() || c == '_' {
            name.push(chars.next().unwrap());
        } else {
            break;
        }
    }
    
    if name.is_empty() {
        return Ok(None);
    }
    
    skip_whitespace(chars);
    
    // Check for filter arguments after ':'
    let has_args = chars.peek() == Some(&':');
    if has_args {
        chars.next(); // consume ':'
        skip_whitespace(chars);
    }
    
    let filter = match name.as_str() {
        "upper" => Filter::Upper,
        "lower" => Filter::Lower,
        "capitalize" => Filter::Capitalize,
        "title" => Filter::Title,
        "trim" => Filter::Trim,
        "length" | "len" => Filter::Length,
        "reverse" => Filter::Reverse,
        "first" => Filter::First,
        "last" => Filter::Last,
        "abs" => Filter::Abs,
        "round" => Filter::Round,
        "floor" => Filter::Floor,
        "ceil" => Filter::Ceil,
        "default" => {
            if has_args {
                let val = parse_filter_string_arg(chars)?;
                Filter::Default(val)
            } else {
                return Err("'default' filter requires an argument: default:\"value\"".to_string());
            }
        }
        "truncate" => {
            if has_args {
                let num = parse_filter_number_arg(chars)?;
                Filter::Truncate(num as usize)
            } else {
                Filter::Truncate(50) // default truncate length
            }
        }
        "join" => {
            if has_args {
                let sep = parse_filter_string_arg(chars)?;
                Filter::Join(sep)
            } else {
                Filter::Join(", ".to_string())
            }
        }
        "replace" => {
            if has_args {
                let old = parse_filter_string_arg(chars)?;
                skip_whitespace(chars);
                if chars.peek() == Some(&':') {
                    chars.next();
                    skip_whitespace(chars);
                    let new = parse_filter_string_arg(chars)?;
                    Filter::Replace(old, new)
                } else {
                    Filter::Replace(old, String::new())
                }
            } else {
                return Err("'replace' filter requires arguments: replace:\"old\":\"new\"".to_string());
            }
        }
        "slice" => {
            if has_args {
                let start = parse_filter_number_arg(chars)? as i64;
                skip_whitespace(chars);
                let end = if chars.peek() == Some(&':') {
                    chars.next();
                    skip_whitespace(chars);
                    Some(parse_filter_number_arg(chars)? as i64)
                } else {
                    None
                };
                Filter::Slice(start, end)
            } else {
                return Err("'slice' filter requires arguments: slice:start or slice:start:end".to_string());
            }
        }
        _ => {
            return Err(format!("Unknown filter: '{}'", name));
        }
    };
    
    Ok(Some(filter))
}

fn parse_filter_string_arg(chars: &mut Peekable<Chars>) -> Result<String, String> {
    let quote = match chars.peek() {
        Some(&'"') => { chars.next(); '"' }
        Some(&'\'') => { chars.next(); '\'' }
        _ => {
            // Unquoted - read until non-alphanumeric
            let mut val = String::new();
            while let Some(&c) = chars.peek() {
                if c.is_alphanumeric() || c == '_' {
                    val.push(chars.next().unwrap());
                } else {
                    break;
                }
            }
            return Ok(val);
        }
    };
    
    let mut val = String::new();
    while let Some(c) = chars.next() {
        if c == quote {
            return Ok(val);
        }
        if c == '\\' {
            if let Some(escaped) = chars.next() {
                val.push(escaped);
            }
        } else {
            val.push(c);
        }
    }
    Err("Unclosed string in filter argument".to_string())
}

fn parse_filter_number_arg(chars: &mut Peekable<Chars>) -> Result<f64, String> {
    let mut num_str = String::new();
    let mut has_dot = false;
    
    if chars.peek() == Some(&'-') {
        num_str.push(chars.next().unwrap());
    }
    
    while let Some(&c) = chars.peek() {
        if c.is_ascii_digit() {
            num_str.push(chars.next().unwrap());
        } else if c == '.' && !has_dot {
            has_dot = true;
            num_str.push(chars.next().unwrap());
        } else {
            break;
        }
    }
    
    num_str.parse::<f64>().map_err(|_| format!("Invalid number: '{}'", num_str))
}

fn skip_whitespace(chars: &mut Peekable<Chars>) {
    while let Some(&c) = chars.peek() {
        if c.is_whitespace() && c != '\n' {
            chars.next();
        } else {
            break;
        }
    }
}

fn parse_string_or_value(chars: &mut Peekable<Chars>) -> Result<String, String> {
    skip_whitespace(chars);
    
    let mut value = String::new();
    let quote_char = match chars.peek() {
        Some(&'"') => {
            chars.next();
            Some('"')
        }
        Some(&'\'') => {
            chars.next();
            Some('\'')
        }
        _ => None,
    };
    
    while let Some(c) = chars.next() {
        if let Some(q) = quote_char {
            if c == q {
                return Ok(value);
            }
        } else if c == '}' {
            // Put back the closing brace by returning early
            // We need to handle this differently - just collect until }
            return Ok(value);
        }
        
        if c == '}' && quote_char.is_none() {
            break;
        }
        
        value.push(c);
    }
    
    if quote_char.is_some() {
        Err("Unclosed string in default value".to_string())
    } else {
        Ok(value)
    }
}

/// Parse a full condition expression with support for:
/// - Comparison: var == "value", var > 10, var != other.var
/// - Logical: cond1 and cond2, cond1 or cond2
/// - Negation: !var, !condition
fn parse_condition_expr(chars: &mut Peekable<Chars>) -> Result<Condition, String> {
    skip_whitespace(chars);
    
    // Parse left side (either a simple condition or a negated one)
    let left = parse_simple_condition(chars)?;
    
    skip_whitespace(chars);
    
    // Check for logical operators (and, or)
    let mut keyword = String::new();
    while let Some(&c) = chars.peek() {
        if c.is_alphabetic() {
            // Don't consume - just peek ahead
            break;
        } else if c == '\n' || c == '\r' {
            return Ok(left);
        } else if c.is_whitespace() {
            chars.next();
        } else {
            break;
        }
    }
    
    // Try to read "and" or "or"
    let checkpoint: Vec<char> = Vec::new();
    while let Some(&c) = chars.peek() {
        if c.is_alphabetic() {
            keyword.push(chars.next().unwrap());
            if keyword == "and" || keyword == "or" {
                break;
            }
        } else {
            break;
        }
    }
    
    if keyword == "and" {
        skip_whitespace(chars);
        let right = parse_condition_expr(chars)?;
        return Ok(Condition::And(Box::new(left), Box::new(right)));
    } else if keyword == "or" {
        skip_whitespace(chars);
        let right = parse_condition_expr(chars)?;
        return Ok(Condition::Or(Box::new(left), Box::new(right)));
    }
    
    // No logical operator, return left condition
    Ok(left)
}

/// Parse a simple condition (variable, comparison, or negated condition)
fn parse_simple_condition(chars: &mut Peekable<Chars>) -> Result<Condition, String> {
    skip_whitespace(chars);
    
    // Check for negation
    let negated = if chars.peek() == Some(&'!') {
        chars.next();
        skip_whitespace(chars);
        true
    } else {
        false
    };
    
    // Parse the variable path (left side of potential comparison)
    let path = parse_condition_path(chars)?;
    
    skip_whitespace(chars);
    
    // Check for comparison operator
    let op = parse_comparison_operator(chars);
    
    if let Some(operator) = op {
        skip_whitespace(chars);
        let compare_value = parse_compare_value(chars)?;
        
        let condition = match operator.as_str() {
            "==" => Condition::Equals(path, compare_value),
            "!=" => Condition::NotEquals(path, compare_value),
            ">" => Condition::GreaterThan(path, compare_value),
            "<" => Condition::LessThan(path, compare_value),
            ">=" => Condition::GreaterOrEqual(path, compare_value),
            "<=" => Condition::LessOrEqual(path, compare_value),
            _ => return Err(format!("Unknown operator: {}", operator)),
        };
        
        Ok(condition)
    } else {
        // No comparison, just truthy/falsy check
        if negated {
            Ok(Condition::Falsy(path))
        } else {
            Ok(Condition::Truthy(path))
        }
    }
}

/// Parse a dotted path for condition (e.g., "user.age", "_loop.first")
fn parse_condition_path(chars: &mut Peekable<Chars>) -> Result<Vec<String>, String> {
    let mut path = Vec::new();
    let mut current = String::new();
    
    while let Some(&c) = chars.peek() {
        if c == '.' {
            chars.next();
            if !current.is_empty() {
                path.push(current);
                current = String::new();
            }
        } else if c.is_alphanumeric() || c == '_' {
            current.push(chars.next().unwrap());
        } else {
            break;
        }
    }
    
    if !current.is_empty() {
        path.push(current);
    }
    
    Ok(path)
}

/// Parse comparison operator (==, !=, >, <, >=, <=)
fn parse_comparison_operator(chars: &mut Peekable<Chars>) -> Option<String> {
    let mut op = String::new();
    
    match chars.peek() {
        Some(&'=') => {
            chars.next();
            if chars.peek() == Some(&'=') {
                chars.next();
                return Some("==".to_string());
            }
        }
        Some(&'!') => {
            chars.next();
            if chars.peek() == Some(&'=') {
                chars.next();
                return Some("!=".to_string());
            }
        }
        Some(&'>') => {
            chars.next();
            if chars.peek() == Some(&'=') {
                chars.next();
                return Some(">=".to_string());
            }
            return Some(">".to_string());
        }
        Some(&'<') => {
            chars.next();
            if chars.peek() == Some(&'=') {
                chars.next();
                return Some("<=".to_string());
            }
            return Some("<".to_string());
        }
        _ => {}
    }
    
    None
}

/// Parse a value to compare against (string, number, bool, or variable path)
fn parse_compare_value(chars: &mut Peekable<Chars>) -> Result<CompareValue, String> {
    skip_whitespace(chars);
    
    match chars.peek() {
        // Quoted string
        Some(&'"') | Some(&'\'') => {
            let quote = chars.next().unwrap();
            let mut val = String::new();
            while let Some(c) = chars.next() {
                if c == quote {
                    return Ok(CompareValue::String(val));
                }
                if c == '\\' {
                    if let Some(escaped) = chars.next() {
                        val.push(escaped);
                    }
                } else {
                    val.push(c);
                }
            }
            Err("Unclosed string in comparison".to_string())
        }
        // Number (including negative)
        Some(&c) if c.is_ascii_digit() || c == '-' => {
            let mut num_str = String::new();
            if chars.peek() == Some(&'-') {
                num_str.push(chars.next().unwrap());
            }
            let mut has_dot = false;
            while let Some(&c) = chars.peek() {
                if c.is_ascii_digit() {
                    num_str.push(chars.next().unwrap());
                } else if c == '.' && !has_dot {
                    has_dot = true;
                    num_str.push(chars.next().unwrap());
                } else {
                    break;
                }
            }
            let num = num_str.parse::<f64>()
                .map_err(|_| format!("Invalid number: {}", num_str))?;
            Ok(CompareValue::Number(num))
        }
        // Boolean or variable path
        Some(&c) if c.is_alphabetic() || c == '_' => {
            let path = parse_condition_path(chars)?;
            
            // Check for boolean keywords
            if path.len() == 1 {
                match path[0].as_str() {
                    "true" => return Ok(CompareValue::Bool(true)),
                    "false" => return Ok(CompareValue::Bool(false)),
                    "nil" | "null" => return Ok(CompareValue::Bool(false)),
                    _ => {}
                }
            }
            
            Ok(CompareValue::Path(path))
        }
        _ => Err("Expected a value to compare against".to_string()),
    }
}

fn parse_if(chars: &mut Peekable<Chars>) -> Result<Node, String> {
    skip_whitespace(chars);
    
    // Parse the full condition expression
    let condition = parse_condition_expr(chars)?;
    
    // Skip optional newline (for block style), but allow inline content
    skip_whitespace(chars);
    if chars.peek() == Some(&'\n') {
        chars.next();
    } else if chars.peek() == Some(&'\r') {
        chars.next();
        if chars.peek() == Some(&'\n') {
            chars.next();
        }
    }
    
    // Parse then branch until @else or @end
    let mut then_branch = Vec::new();
    let mut else_branch = Vec::new();
    let mut in_else = false;
    let mut text_buf = String::new();
    
    loop {
        match chars.next() {
            Some('@') => {
                if chars.peek() == Some(&'-') {
                    chars.next(); // consume first -
                    if chars.peek() == Some(&'-') {
                        chars.next(); // consume second -
                        // Skip until end of line
                        while let Some(c) = chars.next() {
                            if c == '\n' {
                                break;
                            }
                        }
                        continue;
                    } else {
                        text_buf.push('@');
                        text_buf.push('-');
                        continue;
                    }
                }

                // Check for @else, @end, or nested @if
                let keyword = peek_keyword(chars);
                
                if keyword == "else" {
                    consume_keyword(chars, "else");
                    if !text_buf.is_empty() {
                        then_branch.push(Node::Text(text_buf.clone()));
                        text_buf.clear();
                    }
                    in_else = true;
                    // Skip optional newline (for block style), but allow inline content
                    skip_whitespace(chars);
                    if chars.peek() == Some(&'\n') {
                        chars.next();
                    } else if chars.peek() == Some(&'\r') {
                        chars.next();
                        if chars.peek() == Some(&'\n') {
                            chars.next();
                        }
                    }
                } else if keyword == "end" {
                    consume_keyword(chars, "end");
                    if !text_buf.is_empty() {
                        if in_else {
                            else_branch.push(Node::Text(text_buf));
                        } else {
                            then_branch.push(Node::Text(text_buf));
                        }
                    }
                    break;
                } else if keyword == "if" {
                    consume_keyword(chars, "if");
                    if !text_buf.is_empty() {
                        if in_else {
                            else_branch.push(Node::Text(text_buf.clone()));
                        } else {
                            then_branch.push(Node::Text(text_buf.clone()));
                        }
                        text_buf.clear();
                    }
                    let nested_if = parse_if(chars)?;
                    if in_else {
                        else_branch.push(nested_if);
                    } else {
                        then_branch.push(nested_if);
                    }
                } else if keyword == "for" {
                    consume_keyword(chars, "for");
                    if !text_buf.is_empty() {
                        if in_else {
                            else_branch.push(Node::Text(text_buf.clone()));
                        } else {
                            then_branch.push(Node::Text(text_buf.clone()));
                        }
                        text_buf.clear();
                    }
                    let nested_for = parse_for(chars)?;
                    if in_else {
                        else_branch.push(nested_for);
                    } else {
                        then_branch.push(nested_for);
                    }
                } else if chars.peek() == Some(&'{') {
                    chars.next();
                    if !text_buf.is_empty() {
                        if in_else {
                            else_branch.push(Node::Text(text_buf.clone()));
                        } else {
                            then_branch.push(Node::Text(text_buf.clone()));
                        }
                        text_buf.clear();
                    }
                    let var = parse_variable(chars, true)?;
                    if in_else {
                        else_branch.push(var);
                    } else {
                        then_branch.push(var);
                    }
                } else {
                    text_buf.push('@');
                }
            }
            Some(c) => {
                text_buf.push(c);
            }
            None => {
                return Err("Unclosed @if: expected @end".to_string());
            }
        }
    }
    
    Ok(Node::If {
        condition,
        then_branch,
        else_branch,
    })
}

fn peek_keyword(chars: &mut Peekable<Chars>) -> String {
    let mut keyword = String::new();
    let mut temp: Vec<char> = Vec::new();
    
    while let Some(&c) = chars.peek() {
        if c.is_alphabetic() {
            temp.push(chars.next().unwrap());
            keyword.push(temp.last().copied().unwrap());
        } else {
            break;
        }
    }
    
    // Put characters back (we can't actually do this with Peekable, so we need a different approach)
    // For now, return keyword and have caller consume it properly
    // This is a limitation - we'll use consume_keyword after peek_keyword
    
    keyword
}

fn consume_keyword(chars: &mut Peekable<Chars>, expected: &str) {
    // The keyword was already consumed by peek_keyword
    // Just skip any trailing whitespace on the same line
}

fn skip_to_newline(chars: &mut Peekable<Chars>) {
    while let Some(&c) = chars.peek() {
        if c == '\n' {
            chars.next();
            break;
        } else if c == '\r' {
            chars.next();
        } else {
            chars.next();
        }
    }
}

fn parse_for(chars: &mut Peekable<Chars>) -> Result<Node, String> {
    skip_whitespace(chars);
    
    // Parse: var in iterable  OR  i, var in iterable
    let mut var_name = String::new();
    let mut index_name = None;
    let mut iterable = Vec::new();
    
    // Read first identifier
    while let Some(&c) = chars.peek() {
        if c.is_alphanumeric() || c == '_' {
            var_name.push(chars.next().unwrap());
        } else {
            break;
        }
    }
    
    skip_whitespace(chars);
    
    // Check for comma (index variable)
    if chars.peek() == Some(&',') {
        chars.next();
        skip_whitespace(chars);
        index_name = Some(var_name);
        var_name = String::new();
        
        while let Some(&c) = chars.peek() {
            if c.is_alphanumeric() || c == '_' {
                var_name.push(chars.next().unwrap());
            } else {
                break;
            }
        }
        skip_whitespace(chars);
    }
    
    // Expect "in"
    let in_keyword = read_keyword(chars);
    if in_keyword != "in" {
        return Err(format!("Expected 'in' in @for, got '{}'", in_keyword));
    }
    
    skip_whitespace(chars);
    
    // Read iterable path
    let mut current = String::new();
    while let Some(&c) = chars.peek() {
        if c == '\n' || c == '\r' {
            chars.next();
            break;
        } else if c == '.' {
            chars.next();
            if !current.is_empty() {
                iterable.push(current);
                current = String::new();
            }
        } else if c.is_alphanumeric() || c == '_' {
            current.push(chars.next().unwrap());
        } else if c.is_whitespace() {
            chars.next();
            if !current.is_empty() {
                break;
            }
        } else {
            break;
        }
    }
    
    if !current.is_empty() {
        iterable.push(current);
    }
    
    // Parse body until @end
    let mut body = Vec::new();
    let mut text_buf = String::new();
    
    loop {
        match chars.next() {
            Some('@') => {
                if chars.peek() == Some(&'-') {
                    chars.next(); // consume first -
                    if chars.peek() == Some(&'-') {
                        chars.next(); // consume second -
                        // Skip until end of line
                        while let Some(c) = chars.next() {
                            if c == '\n' {
                                break;
                            }
                        }
                        continue;
                    } else {
                        text_buf.push('@');
                        text_buf.push('-');
                        continue;
                    }
                }

                let keyword = peek_keyword(chars);
                
                if keyword == "end" {
                    if !text_buf.is_empty() {
                        body.push(Node::Text(text_buf));
                    }
                    break;
                } else if keyword == "if" {
                    if !text_buf.is_empty() {
                        body.push(Node::Text(text_buf.clone()));
                        text_buf.clear();
                    }
                    body.push(parse_if(chars)?);
                } else if keyword == "for" {
                    if !text_buf.is_empty() {
                        body.push(Node::Text(text_buf.clone()));
                        text_buf.clear();
                    }
                    body.push(parse_for(chars)?);
                } else if chars.peek() == Some(&'{') {
                    chars.next();
                    if !text_buf.is_empty() {
                        body.push(Node::Text(text_buf.clone()));
                        text_buf.clear();
                    }
                    body.push(parse_variable(chars, true)?);
                } else {
                    text_buf.push('@');
                }
            }
            Some(c) => {
                text_buf.push(c);
            }
            None => {
                return Err("Unclosed @for: expected @end".to_string());
            }
        }
    }
    
    Ok(Node::For {
        var_name,
        index_name,
        iterable,
        body,
    })
}

fn parse_include(chars: &mut Peekable<Chars>) -> Result<Node, String> {
    skip_whitespace(chars);
    
    // Expect quoted string
    let quote = match chars.next() {
        Some('"') => '"',
        Some('\'') => '\'',
        _ => return Err("Expected quoted path after @include".to_string()),
    };
    
    let mut path = String::new();
    while let Some(c) = chars.next() {
        if c == quote {
            return Ok(Node::Include(path));
        }
        path.push(c);
    }
    
    Err("Unclosed string in @include".to_string())
}

#[cfg(test)]
mod tests {
    use super::*;
    
    #[test]
    fn test_simple_variable() {
        let nodes = parse_template("Hello @{name}!").unwrap();
        assert_eq!(nodes.len(), 3);
    }
    
    #[test]
    fn test_if_else() {
        let nodes = parse_template("@if logged_in\nHello\n@else\nGuest\n@end").unwrap();
        assert_eq!(nodes.len(), 1);
    }
}
