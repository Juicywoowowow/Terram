-- Terram Hello World Example
-- Move the square with WASD keys

local x, y = 400, 300
local speed = 200

function terram.load()
    terram.window.setTitle("Hello Terram!")
end

function terram.update(dt)
    -- Movement
    if terram.input.isKeyDown("w") then y = y - speed * dt end
    if terram.input.isKeyDown("s") then y = y + speed * dt end
    if terram.input.isKeyDown("a") then x = x - speed * dt end
    if terram.input.isKeyDown("d") then x = x + speed * dt end

    -- Keep in bounds
    local w, h = terram.window.getWidth(), terram.window.getHeight()
    if x < 25 then x = 25 end
    if x > w - 25 then x = w - 25 end
    if y < 25 then y = 25 end
    if y > h - 25 then y = h - 25 end
end

function terram.draw()
    -- Dark blue background
    terram.graphics.clear(0.1, 0.1, 0.2)

    -- Orange player square
    terram.graphics.setColor(1, 0.5, 0.2)
    terram.graphics.rectangle("fill", x - 25, y - 25, 50, 50)

    -- White border
    terram.graphics.setColor(1, 1, 1)
    terram.graphics.rectangle("line", x - 25, y - 25, 50, 50)

    -- Draw a circle
    terram.graphics.setColor(0.2, 0.8, 0.4)
    terram.graphics.circle("fill", 100, 100, 40)
end
