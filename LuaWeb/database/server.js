/**
 * LuaWeb Database Bridge
 * 
 * HTTP server that provides SQLite operations for the C++ core.
 * Uses better-sqlite3 for fast, synchronous database access.
 * 
 * Usage: node server.js <port> <server_id>
 */

const http = require('http');
const path = require('path');
const fs = require('fs');
const Database = require('better-sqlite3');

// Parse command line arguments
const PORT = parseInt(process.argv[2]) || 9876;
const SERVER_ID = process.argv[3] || 'default';

const HOST = '127.0.0.1';

// Store open database connections
const databases = new Map();

// This server's database directory
const BASE_DB_DIR = path.join(process.cwd(), '__DBWEB__');
const SERVER_DB_DIR = path.join(BASE_DB_DIR, `Dbserver_${SERVER_ID}`);

// Track absolute paths in use (for conflict detection)
const absolutePathsInUse = new Set();

// Ensure directories exist
function ensureDirectories() {
    if (!fs.existsSync(BASE_DB_DIR)) {
        fs.mkdirSync(BASE_DB_DIR, { recursive: true });
    }
    if (!fs.existsSync(SERVER_DB_DIR)) {
        fs.mkdirSync(SERVER_DB_DIR, { recursive: true });
    }
}

// Resolve database path
function resolveDbPath(dbPath) {
    if (!dbPath || dbPath === '') {
        dbPath = 'default.db';
    }

    // If absolute path, use as-is (but check for conflicts)
    if (path.isAbsolute(dbPath)) {
        return { path: dbPath, isAbsolute: true };
    }

    // Otherwise, put in server's directory
    ensureDirectories();
    return { path: path.join(SERVER_DB_DIR, dbPath), isAbsolute: false };
}

// Generate unique ID for database connections
let dbIdCounter = 0;
function generateDbId() {
    return `db_${++dbIdCounter}`;
}

// Parse JSON body from request
function parseBody(req) {
    return new Promise((resolve, reject) => {
        let body = '';
        req.on('data', chunk => body += chunk);
        req.on('end', () => {
            try {
                resolve(body ? JSON.parse(body) : {});
            } catch (e) {
                reject(new Error('Invalid JSON'));
            }
        });
        req.on('error', reject);
    });
}

// Send JSON response
function sendJson(res, data, status = 200) {
    const json = JSON.stringify(data);
    res.writeHead(status, {
        'Content-Type': 'application/json',
        'Content-Length': Buffer.byteLength(json)
    });
    res.end(json);
}

// Handle requests
async function handleRequest(req, res) {
    const url = new URL(req.url, `http://${HOST}:${PORT}`);
    const pathname = url.pathname;

    try {
        // Helper to get all DB files
        const getDbFiles = () => {
            try {
                return fs.readdirSync(SERVER_DB_DIR).filter(f => f.endsWith('.db'));
            } catch (e) { return []; }
        };

        // SQL Console UI
        if (pathname === '/console' && req.method === 'GET') {
            const dbName = url.searchParams.get('db');
            if (!dbName) return sendJson(res, { error: 'No db specified' }, 400);

            const html = `
            <!DOCTYPE html>
            <html>
            <head>
                <title>SQL Console: ${dbName}</title>
                <style>
                    body { font-family: monospace; background: #1a1a2e; color: #eee; padding: 20px; display: flex; flex-direction: column; height: 90vh; }
                    h1 { margin: 0 0 10px 0; color: #00d9ff; }
                    #editor { flex: 1; background: #16213e; color: #fff; border: 1px solid #30475e; padding: 10px; font-size: 16px; border-radius: 4px; resize: none; margin-bottom: 10px; }
                    #results { flex: 1; background: #0f1524; overflow: auto; padding: 10px; border: 1px solid #30475e; border-radius: 4px; white-space: pre-wrap; }
                    .controls { margin-bottom: 10px; display: flex; gap: 10px; }
                    button { background: #e94560; color: white; border: none; padding: 10px 20px; border-radius: 4px; cursor: pointer; font-weight: bold; }
                    button:hover { background: #ff6b6b; }
                    .back { background: #30475e; text-decoration: none; display: inline-flex; align-items: center; justify-content: center; color: white; padding: 0 20px; border-radius: 4px; }
                </style>
            </head>
            <body>
                <h1>> ${dbName}</h1>
                <div class="controls">
                    <a href="/" class="back">← Back</a>
                    <button onclick="runQuery()">▶ Run SQL</button>
                    <button onclick="clearOutput()" style="background: #444;">Clear</button>
                </div>
                <textarea id="editor" placeholder="SELECT * FROM ...">SELECT * FROM sqlite_master WHERE type='table';</textarea>
                <div id="results">// Results will appear here...</div>

                <script>
                    let dbId = null;

                    async function ensureOpen() {
                        if (dbId) return dbId;
                        try {
                            const res = await fetch('/open', {
                                method: 'POST', body: JSON.stringify({ path: '${dbName}' })
                            });
                            const data = await res.json();
                            if (data.ok) {
                                dbId = data.id;
                                return dbId;
                            }
                            throw new Error(data.error);
                        } catch (e) {
                            document.getElementById('results').textContent = 'Error opening DB: ' + e.message;
                            return null;
                        }
                    }

                    async function runQuery() {
                        const sql = document.getElementById('editor').value;
                        const output = document.getElementById('results');
                        output.textContent = 'Running...';
                        
                        const id = await ensureOpen();
                        if (!id) return;

                        const isSelect = sql.trim().toLowerCase().startsWith('select');
                        const endpoint = isSelect ? '/query' : '/exec';

                        try {
                            const res = await fetch(endpoint, {
                                method: 'POST', body: JSON.stringify({ id, sql })
                            });
                            const data = await res.json();
                            
                            if (data.ok) {
                                output.textContent = JSON.stringify(data.rows || data, null, 2);
                            } else {
                                output.style.color = '#ff6b6b';
                                output.textContent = 'Error: ' + data.error;
                            }
                        } catch (e) {
                            output.textContent = 'Network Error: ' + e.message;
                        }
                    }
                    
                    function clearOutput() {
                        document.getElementById('results').textContent = '';
                        document.getElementById('results').style.color = '#eee';
                    }
                </script>
            </body>
            </html>
            `;
            res.writeHead(200, { 'Content-Type': 'text/html' });
            res.end(html);
            return;
        }

        // Dashboard UI
        if (pathname === '/' && req.method === 'GET') {
            const files = getDbFiles();

            const dbList = files.map(filename => {
                // Check if currently open
                const openEntry = Array.from(databases.entries()).find(([, info]) => info.path.endsWith(filename));
                const isOpen = !!openEntry;
                const id = openEntry ? openEntry[0] : '-';

                return `
                <div class="card">
                    <div class="db-header">
                        <span class="db-id">${filename}</span>
                        <span class="db-status" style="background: ${isOpen ? '#2ecc71' : '#95a5a6'}">
                            ${isOpen ? 'Active' : 'On Disk'}
                        </span>
                    </div>
                    <div class="db-path">ID: ${id}</div>
                    <div class="controls" style="margin-top: 10px;">
                        <a href="/console?db=${filename}" style="background: #00d9ff; color: #000; padding: 5px 10px; text-decoration: none; border-radius: 4px; font-weight: bold; font-size: 0.9em;">
                            Open Console
                        </a>
                    </div>
                </div>`;
            }).join('');

            const html = `
            <!DOCTYPE html>
            <html>
            <head>
                <title>LuaWeb DB Bridge: ${SERVER_ID}</title>
                <style>
                    body { font-family: system-ui, sans-serif; background: #1a1a2e; color: #eee; padding: 20px; max-width: 1000px; margin: 0 auto; }
                    h1 { color: #e94560; border-bottom: 2px solid #16213e; padding-bottom: 10px; }
                    .stats { display: flex; gap: 20px; margin-bottom: 30px; }
                    .stat-box { background: #16213e; padding: 15px; border-radius: 8px; flex: 1; border-left: 4px solid #00d9ff; }
                    .stat-val { font-size: 24px; font-weight: bold; }
                    .stat-label { color: #8892b0; font-size: 14px; }
                    .card { background: #16213e; padding: 20px; border-radius: 8px; margin-bottom: 15px; }
                    .db-header { display: flex; justify-content: space-between; margin-bottom: 10px; align-items: center; }
                    .db-id { font-weight: bold; color: #00d9ff; font-size: 1.2em; }
                    .db-status { color: #000; padding: 2px 8px; border-radius: 4px; font-size: 0.8em; font-weight: bold; }
                    .db-path { font-family: monospace; color: #a8b2d1; font-size: 0.9em; }
                    .refresh { position: fixed; bottom: 20px; right: 20px; background: #e94560; color: white; border: none; padding: 10px 20px; border-radius: 20px; cursor: pointer; font-weight: bold; box-shadow: 0 4px 10px rgba(0,0,0,0.3); }
                    .refresh:hover { background: #ff6b6b; }
                </style>
            </head>
            <body>
                <h1>Database Bridge Monitor</h1>
                
                <div class="stats">
                    <div class="stat-box">
                        <div class="stat-val">${SERVER_ID}</div>
                        <div class="stat-label">Server ID</div>
                    </div>
                    <div class="stat-box">
                        <div class="stat-val">${PORT}</div>
                        <div class="stat-label">Bridge Port</div>
                    </div>
                    <div class="stat-box">
                        <div class="stat-val">${databases.size}</div>
                        <div class="stat-label">Memory Connections</div>
                    </div>
                </div>

                <h2>Database Files</h2>
                ${dbList || '<p style="color: #666; font-style: italic;">No database files found in storage directory.</p>'}

                <button class="refresh" onclick="window.location.reload()">Refresh Monitor</button>
            </body>
            </html>
            `;

            res.writeHead(200, { 'Content-Type': 'text/html' });
            res.end(html);
            return;
        }

        // Health check logic (legacy/internal)
        if (pathname === '/health' && req.method === 'GET') {
            return sendJson(res, {
                ok: true,
                serverId: SERVER_ID,
                port: PORT,
                connections: databases.size,
                dbDir: SERVER_DB_DIR
            });
        }

        // Open database
        if (pathname === '/open' && req.method === 'POST') {
            const body = await parseBody(req);
            const resolved = resolveDbPath(body.path);
            const dbPath = resolved.path;

            // Check for absolute path conflicts
            if (resolved.isAbsolute && absolutePathsInUse.has(dbPath)) {
                return sendJson(res, {
                    ok: false,
                    error: `Database already in use: ${dbPath}`
                }, 409);
            }

            // Check if already open by this server
            for (const [id, info] of databases.entries()) {
                if (info.path === dbPath) {
                    return sendJson(res, { ok: true, id: id, path: dbPath, reused: true });
                }
            }

            // Ensure parent directory exists for the database
            const dbDir = path.dirname(dbPath);
            if (!fs.existsSync(dbDir)) {
                fs.mkdirSync(dbDir, { recursive: true });
            }

            // Open new connection
            const db = new Database(dbPath);
            db.pragma('journal_mode = WAL');  // Better performance

            const id = generateDbId();
            databases.set(id, { db, path: dbPath, isAbsolute: resolved.isAbsolute });

            if (resolved.isAbsolute) {
                absolutePathsInUse.add(dbPath);
            }

            return sendJson(res, { ok: true, id: id, path: dbPath });
        }

        // Execute SQL (INSERT, UPDATE, DELETE, CREATE, etc.)
        if (pathname === '/exec' && req.method === 'POST') {
            const body = await parseBody(req);
            const { id, sql, params = [] } = body;

            const info = databases.get(id);
            if (!info) {
                return sendJson(res, { ok: false, error: 'Database not open' }, 400);
            }

            const stmt = info.db.prepare(sql);
            const result = stmt.run(...params);

            return sendJson(res, {
                ok: true,
                changes: result.changes,
                lastInsertRowid: Number(result.lastInsertRowid)
            });
        }

        // Query SQL (SELECT)
        if (pathname === '/query' && req.method === 'POST') {
            const body = await parseBody(req);
            const { id, sql, params = [] } = body;

            const info = databases.get(id);
            if (!info) {
                return sendJson(res, { ok: false, error: 'Database not open' }, 400);
            }

            const stmt = info.db.prepare(sql);
            const rows = stmt.all(...params);

            return sendJson(res, { ok: true, rows: rows });
        }

        // Close database
        if (pathname === '/close' && req.method === 'POST') {
            const body = await parseBody(req);
            const { id } = body;

            const info = databases.get(id);
            if (info) {
                if (info.isAbsolute) {
                    absolutePathsInUse.delete(info.path);
                }
                info.db.close();
                databases.delete(id);
            }

            return sendJson(res, { ok: true });
        }

        // Close all and shutdown
        if (pathname === '/shutdown' && req.method === 'POST') {
            console.log(`[${SERVER_ID}] Shutting down...`);

            for (const [id, info] of databases.entries()) {
                info.db.close();
            }
            databases.clear();
            absolutePathsInUse.clear();

            sendJson(res, { ok: true });

            // Exit after response
            setTimeout(() => process.exit(0), 100);
            return;
        }

        // Unknown endpoint
        return sendJson(res, { ok: false, error: 'Not found' }, 404);

    } catch (err) {
        console.error(`[${SERVER_ID}] Error:`, err.message);
        return sendJson(res, { ok: false, error: err.message }, 500);
    }
}

// Start server
ensureDirectories();
const server = http.createServer(handleRequest);

server.listen(PORT, HOST, () => {
    console.log(`[${SERVER_ID}] Database Bridge running on http://${HOST}:${PORT}`);
    console.log(`[${SERVER_ID}] Database directory: ${SERVER_DB_DIR}`);
});

// Graceful shutdown
function cleanup() {
    console.log(`[${SERVER_ID}] Cleaning up...`);
    for (const [id, info] of databases.entries()) {
        info.db.close();
    }
    server.close();
    process.exit(0);
}

process.on('SIGINT', cleanup);
process.on('SIGTERM', cleanup);
process.on('SIGHUP', cleanup);
