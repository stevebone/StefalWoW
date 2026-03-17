# Deploy Changes Command

Prepare and deploy TrinityCore Playerbot changes with comprehensive validation.

## Usage

```bash
/deploy --check              # Pre-deployment checks only
/deploy --stage              # Deploy to staging environment
/deploy --prod               # Deploy to production environment
/deploy --rollback           # Rollback last deployment
/deploy --dry-run            # Simulate deployment without applying
```

## Examples

```bash
# Check if ready for deployment
/deploy --check

# Deploy to staging for testing
/deploy --stage

# Deploy to production (requires approval)
/deploy --prod

# Rollback if issues detected
/deploy --rollback
```

## Pre-Deployment Checks

Before deployment, the following checks are performed:

### 1. Code Quality ✅
- Full code review passes
- No critical issues detected
- Security vulnerabilities resolved
- Code style compliance verified

### 2. Build Verification ✅
- Project builds successfully
- All targets compile without errors
- Dependencies resolved
- Binaries verified

### 3. Testing ✅
- Unit tests pass
- Integration tests pass
- Performance tests pass
- Regression tests pass

### 4. Trinity Integration ✅
- TrinityCore API compatibility verified
- No core modifications detected
- Database schema validated
- Hooks properly integrated

### 5. Documentation ✅
- Changelog updated
- API documentation current
- Configuration documented
- Migration guide provided (if needed)

## Deployment Steps

### Staging Deployment

1. **Pre-flight checks**
   - Run all validation tests
   - Generate deployment package
   - Create backup point

2. **Deploy to staging**
   - Stop staging worldserver
   - Deploy new binaries
   - Apply database migrations
   - Start worldserver
   - Verify server starts successfully

3. **Validation**
   - Run smoke tests
   - Check bot spawning
   - Verify AI functionality
   - Monitor for errors

4. **Report**
   - Deployment summary
   - Test results
   - Performance metrics
   - Issue log

### Production Deployment

1. **Final validation**
   - Staging tests passed
   - Change approval obtained
   - Backup created
   - Rollback plan ready

2. **Maintenance mode**
   - Announce maintenance window
   - Gracefully disconnect players
   - Stop worldserver
   - Create final backup

3. **Deploy**
   - Deploy binaries
   - Apply database migrations
   - Update configuration
   - Verify file integrity

4. **Startup**
   - Start bnetserver
   - Start worldserver
   - Monitor startup logs
   - Verify bot systems

5. **Post-deployment**
   - Run validation tests
   - Monitor performance
   - Check error logs
   - Announce completion

## Rollback Procedure

If issues are detected after deployment:

1. **Trigger rollback**
   ```bash
   /deploy --rollback
   ```

2. **Restore previous version**
   - Stop servers
   - Restore previous binaries
   - Rollback database changes
   - Restore configuration

3. **Verify rollback**
   - Start servers
   - Run validation tests
   - Check functionality
   - Monitor stability

4. **Investigation**
   - Collect error logs
   - Analyze failure cause
   - Create issue report
   - Plan remediation

## Deployment Package

The deployment package includes:
- Compiled binaries (worldserver, bnetserver)
- Dependencies (MySQL client, OpenSSL)
- Database migration scripts
- Configuration files
- Documentation updates
- Rollback scripts

Package structure:
```
playerbot-deploy-<version>/
├── bin/
│   ├── worldserver.exe
│   ├── bnetserver.exe
│   └── *.dll
├── sql/
│   ├── updates/
│   │   └── playerbot_<version>.sql
│   └── rollback/
│       └── playerbot_<version>_rollback.sql
├── config/
│   └── worldserver.conf.dist
├── docs/
│   ├── CHANGELOG.md
│   └── MIGRATION_GUIDE.md
└── scripts/
    ├── deploy.sh
    └── rollback.sh
```

## Environment Configuration

### Staging Environment
- **Purpose**: Testing and validation
- **Data**: Copy of production (anonymized)
- **Players**: Development team only
- **Monitoring**: Detailed logging enabled

### Production Environment
- **Purpose**: Live service
- **Data**: Real player data
- **Players**: All players
- **Monitoring**: Critical alerts only

## Validation Tests

### Smoke Tests
- Server starts successfully
- Bot spawner initializes
- Database connection works
- Basic commands functional

### Functional Tests
- Bot spawning works
- AI decision making works
- Combat system works
- Group formation works
- Quest completion works

### Performance Tests
- Bot spawn time < 100ms
- AI update time < 50ms
- Memory per bot < 10MB
- CPU per bot < 0.5%

## Monitoring

Post-deployment monitoring includes:
- Server uptime
- Error rate
- Bot spawn rate
- Performance metrics
- Database query times

## Checklist

Before production deployment:
- [ ] Staging deployment successful
- [ ] All tests passing
- [ ] Documentation updated
- [ ] Change log prepared
- [ ] Team notified
- [ ] Backup created
- [ ] Rollback plan ready
- [ ] Monitoring configured
- [ ] Maintenance window scheduled
- [ ] Players notified

## Notes

- Production deployments require explicit approval
- Always test in staging first
- Maintain backup before deployment
- Monitor for 24 hours post-deployment
- Document all changes
- Communicate with team

## Emergency Procedures

### Critical Issue During Deployment
1. Stop deployment immediately
2. Assess impact
3. Decide: continue, rollback, or hold
4. Communicate status
5. Document incident

### Post-Deployment Critical Issue
1. Trigger rollback immediately
2. Restore service
3. Investigate root cause
4. Create hotfix
5. Re-deploy with fix

## Success Criteria

Deployment is successful when:
- ✅ Server runs stable for 1 hour
- ✅ No critical errors in logs
- ✅ Bots spawning correctly
- ✅ Performance metrics normal
- ✅ No player complaints
- ✅ Monitoring shows green status
