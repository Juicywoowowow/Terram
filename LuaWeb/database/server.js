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
        // Health check
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
