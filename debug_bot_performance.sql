-- Debug Bot Performance Issues
-- Run this while server is running to check for blocking queries

-- Check for long-running queries
SELECT
    TIME,
    STATE,
    INFO,
    COMMAND
FROM INFORMATION_SCHEMA.PROCESSLIST
WHERE COMMAND != 'Sleep'
  AND TIME > 1
ORDER BY TIME DESC
LIMIT 20;

-- Check connection pool usage
SELECT
    PROCESSLIST_STATE as State,
    COUNT(*) as Count
FROM performance_schema.threads
WHERE PROCESSLIST_DB IN ('auth', 'characters', 'world')
GROUP BY PROCESSLIST_STATE;

-- Check for table locks
SELECT
    r.trx_id waiting_trx_id,
    r.trx_mysql_thread_id waiting_thread,
    r.trx_query waiting_query,
    b.trx_id blocking_trx_id,
    b.trx_mysql_thread_id blocking_thread,
    b.trx_query blocking_query
FROM information_schema.innodb_lock_waits w
INNER JOIN information_schema.innodb_trx b ON b.trx_id = w.blocking_trx_id
INNER JOIN information_schema.innodb_trx r ON r.trx_id = w.requesting_trx_id;
