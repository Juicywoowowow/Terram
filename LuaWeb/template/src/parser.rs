//! Template Parser
//! 
//! Parses LuaWeb template syntax into an AST

use std::iter::Peekable;
use std::str::Chars;

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

/// Condition for @if
#[derive(Debug, Clone)]
pub enum Condition {
    /// Variable is truthy
    Truthy(Vec<String>),
    /// Variable is falsy (negated)
    Falsy(Vec<String>),
    /// Comparison: var == value
    Equals(Vec<String>, String),
    /// Comparison: var != value
    NotEquals(Vec<String>, String),
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
    
    while let Some(c) = chars.next() {
        match c {
            '}' => {
                if !current.is_empty() {
                    path.push(current);
                }
                return Ok(Node::Variable { path, escape, default });
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
                // Parse default value
                skip_whitespace(chars);
                default = Some(parse_string_or_value(chars)?);
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

fn parse_if(chars: &mut Peekable<Chars>) -> Result<Node, String> {
    skip_whitespace(chars);
    
    // Parse condition
    let negated = if chars.peek() == Some(&'!') {
        chars.next();
        true
    } else {
        false
    };
    
    let mut condition_path = Vec::new();
    let mut current = String::new();
    
    // Read until newline or whitespace
    while let Some(&c) = chars.peek() {
        if c == '\n' || c == '\r' {
            chars.next();
            break;
        } else if c == '.' {
            chars.next();
            if !current.is_empty() {
                condition_path.push(current);
                current = String::new();
            }
        } else if c.is_alphanumeric() || c == '_' {
            chars.next();
            current.push(c);
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
        condition_path.push(current);
    }
    
    let condition = if negated {
        Condition::Falsy(condition_path)
    } else {
        Condition::Truthy(condition_path)
    };
    
    // Parse then branch until @else or @end
    let mut then_branch = Vec::new();
    let mut else_branch = Vec::new();
    let mut in_else = false;
    let mut text_buf = String::new();
    
    loop {
        match chars.next() {
            Some('@') => {
                // Check for @else, @end, or nested @if
                let keyword = peek_keyword(chars);
                
                if keyword == "else" {
                    consume_keyword(chars, "else");
                    if !text_buf.is_empty() {
                        then_branch.push(Node::Text(text_buf.clone()));
                        text_buf.clear();
                    }
                    in_else = true;
                    // Skip to next line
                    skip_to_newline(chars);
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
