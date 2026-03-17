# Database Operations Command

Interact with TrinityCore databases using the trinity-database MCP server.

## Prerequisites

Requires the `trinity-database` MCP server to be configured in `.claude/mcp-servers-config.json`.

## Usage

```bash
/db schema <table_pattern>                    # Show table schema
/db query "<SQL>"                             # Execute SQL query
/db optimize                                  # Run database optimization
/db check                                     # Verify data integrity
/db backup                                    # Create database backup
/db stats                                     # Show database statistics
```

## Examples

```bash
# Show playerbot tables schema
/db schema playerbot%

# Query character data
/db query "SELECT name, level, class FROM characters WHERE account IN (SELECT id FROM trinity_auth.account WHERE username LIKE 'bot_%') LIMIT 10"

# Check bot spawn data integrity
/db check

# Optimize all Playerbot tables
/db optimize

# Get database statistics
/db stats
```

## Available Operations

### Schema Operations

**Show table schema**:
```bash
/db schema playerbot_ai_state
```

**List all tables**:
```bash
/db schema %
```

**Show indexes**:
```bash
/db query "SHOW INDEX FROM playerbot_ai_state"
```

### Data Queries

**Safe read-only queries**:
```sql
-- Bot statistics
SELECT
    COUNT(*) as total_bots,
    AVG(level) as avg_level,
    MAX(level) as max_level
FROM trinity_characters.characters
WHERE account IN (
    SELECT id FROM trinity_auth.account WHERE username LIKE 'bot_%'
);

-- Bot class distribution
SELECT
    class,
    COUNT(*) as count
FROM trinity_characters.characters
WHERE account IN (
    SELECT id FROM trinity_auth.account WHERE username LIKE 'bot_%'
)
GROUP BY class
ORDER BY count DESC;
```

### Optimization

**Optimize tables**:
```bash
/db optimize
```

Optimizes:
- Playerbot custom tables
- Frequently updated character tables
- Rebuilds indexes
- Updates table statistics

### Integrity Checks

**Check data integrity**:
```bash
/db check
```

Verifies:
- Foreign key constraints
- Orphaned records
- Data consistency
- Index integrity

### Backup & Restore

**Create backup**:
```bash
/db backup
```

Creates timestamped backup of:
- Playerbot custom tables
- Bot character data
- Bot account data

**Restore from backup**:
```bash
/db restore <timestamp>
```

## Database Access

The MCP server connects to:
- **trinity_auth**: Authentication and account data
- **trinity_characters**: Character and bot data
- **trinity_world**: World data, quests, items

## Security

- Read-only mode enabled by default
- Write operations require explicit permission
- Dangerous operations (DROP, TRUNCATE) are blocked
- Query timeout: 30 seconds
- Max result rows: 1000

## Performance Tips

1. **Use EXPLAIN** to analyze query performance:
```sql
EXPLAIN SELECT * FROM characters WHERE account = 123;
```

2. **Add LIMIT** to large queries:
```sql
SELECT * FROM characters LIMIT 100;
```

3. **Use indexes** effectively:
```sql
-- Check if index exists
SHOW INDEX FROM characters WHERE Key_name = 'idx_account';
```

4. **Monitor slow queries**:
```bash
/db query "SHOW VARIABLES LIKE 'slow_query%'"
```

## Common Queries

### Bot Management

```sql
-- List all bots
SELECT a.username, c.name, c.level, c.class
FROM trinity_auth.account a
JOIN trinity_characters.characters c ON c.account = a.id
WHERE a.username LIKE 'bot_%'
ORDER BY c.level DESC;

-- Bot spawn counts by zone
SELECT zone, COUNT(*) as bot_count
FROM trinity_characters.characters
WHERE account IN (SELECT id FROM trinity_auth.account WHERE username LIKE 'bot_%')
GROUP BY zone;

-- Bot online status
SELECT COUNT(*) as online_bots
FROM trinity_characters.characters
WHERE online = 1
AND account IN (SELECT id FROM trinity_auth.account WHERE username LIKE 'bot_%');
```

### Performance Monitoring

```sql
-- Table sizes
SELECT
    table_name,
    ROUND(data_length / 1024 / 1024, 2) as size_mb
FROM information_schema.tables
WHERE table_schema = 'trinity_characters'
AND table_name LIKE 'playerbot%'
ORDER BY data_length DESC;

-- Index usage
SELECT
    table_name,
    index_name,
    cardinality
FROM information_schema.statistics
WHERE table_schema = 'trinity_characters'
AND table_name LIKE 'playerbot%';
```

## Output

Query results are formatted as:
- Console table for small results
- JSON export for large datasets
- CSV export option available

## Notes

- Always use parameterized queries for safety
- Test queries on small datasets first
- Use transactions for multiple updates
- Backup before bulk operations
- Monitor query performance with EXPLAIN
