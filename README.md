# Terram

A LÖVE-like 2D game engine. Write games in Lua, powered by C++/SDL2/OpenGL.

## Build

```bash
mkdir build && cd build
cmake ..
make -j4
```

## Usage

```bash
./terram <game_directory>
./terram ../examples/hello
```

## Lua API

### Callbacks

```lua
function terram.load()      -- Called once at startup
function terram.update(dt)  -- Called every frame (dt = delta time)
function terram.draw()      -- Called every frame for rendering
```

### terram.graphics

```lua
terram.graphics.clear(r, g, b)
terram.graphics.setColor(r, g, b, a)
terram.graphics.rectangle("fill"/"line", x, y, w, h)
terram.graphics.circle("fill"/"line", x, y, radius)
terram.graphics.line(x1, y1, x2, y2)
terram.graphics.newImage(path)  -- Returns image table
terram.graphics.draw(image, x, y, rotation, scaleX, scaleY)
```

### terram.window

```lua
terram.window.setTitle(title)
terram.window.getWidth()
terram.window.getHeight()
```

### terram.input

```lua
terram.input.isKeyDown(key)   -- "a"-"z", "0"-"9", "space", "up", etc.
terram.input.isKeyPressed(key)
terram.input.isMouseDown(button)  -- 1=left, 2=middle, 3=right
terram.input.getMouseX()
terram.input.getMouseY()
```

## Example

```lua
local x, y = 400, 300

function terram.load()
    terram.window.setTitle("My Game")
end

function terram.update(dt)
    if terram.input.isKeyDown("w") then y = y - 200 * dt end
    if terram.input.isKeyDown("s") then y = y + 200 * dt end
end

function terram.draw()
    terram.graphics.clear(0.1, 0.1, 0.2)
    terram.graphics.setColor(1, 0.5, 0.2)
    terram.graphics.rectangle("fill", x, y, 50, 50)
end
```

## License

MIT
