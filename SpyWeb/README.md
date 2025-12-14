# SpyWeb 🕵️

A simple hobby website built with **LuaWeb** showcasing the new LWT template features:
- ✨ **Filters** (`| upper`, `| truncate:50`, `| join`)
- 🔄 **Loop Variables** (`_loop.index`, `_loop.first`, `_loop.last`)
- ⚖️ **Comparison Operators** (`>=`, `<=`, `==`, `!=`, `>`, `<`)

## Quick Start

```bash
# Make sure LuaWeb is built first
cd ../LuaWeb
cmake -B build && cmake --build build
cd template && cargo build --release && cd ..
cp template/target/release/liblwtemplate.dylib build/

# Run SpyWeb
cd ../SpyWeb
../LuaWeb/build/luaweb_runner app.lua
```

Then visit: http://localhost:3000

## Features

- 🏠 Home page with dynamic greeting
- 📜 Blog posts with categories
- 🎨 Projects showcase
- 📊 Stats dashboard (demonstrates filters and comparisons)
- 🍪 Session tracking with cookies

## Template Features Demo

### Filters
```html
@{title | upper}                   <!-- HELLO WORLD -->
@{description | truncate:100}      <!-- First 100 chars... -->
@{tags | join:", "}                <!-- lua, web, rust -->
@{name | capitalize}               <!-- John -->
```

### Loop Variables
```html
@for item in items
    @if _loop.first
        <div class="first-item">@{item.name}</div>
    @end
    <span>Item #@{_loop.index1}</span>  <!-- 1-based index -->
    @if !_loop.last
        <hr>
    @end
@end
```

### Comparison Operators
```html
@if user.age >= 18
    <p>Welcome to the adult section</p>
@end

@if score > 90
    <span class="grade">A+</span>
@else
    @if score >= 80
        <span class="grade">B</span>
    @end
@end

@if status == "active" and role == "admin"
    <button>Admin Panel</button>
@end
```
