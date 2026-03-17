#!C:\Program Files\Python313\python.exe
"""
Dependency Security Scanner for TrinityCore PlayerBot
"""
import json
import re
import os
import sys
from pathlib import Path
from datetime import datetime

def scan_dependencies():
    """Scan project dependencies for vulnerabilities"""
    results = {
        "scan_date": datetime.now().isoformat(),
        "vulnerable_packages": [],
        "outdated_packages": [],
        "security_issues": []
    }
    
    print("\n=== Dependency Security Scan ===")
    
    # Check CMakeLists.txt for dependencies
    cmake_file = Path("CMakeLists.txt")
    if cmake_file.exists():
        print("Analyzing CMakeLists.txt...")
        try:
            with open(cmake_file, 'r', encoding='utf-8') as f:
                content = f.read()
                
                # Check Boost version
                boost_match = re.search(r'find_package\(Boost\s+(\d+\.\d+)', content, re.IGNORECASE)
                if boost_match:
                    boost_version = boost_match.group(1)
                    print(f"Found Boost version: {boost_version}")
                    
                    if float(boost_version) < 1.70:
                        results["vulnerable_packages"].append({
                            "package": "Boost",
                            "version": boost_version,
                            "severity": "MEDIUM",
                            "recommendation": "Update to Boost 1.70+ for security fixes"
                        })
                
                # Check MySQL version
                mysql_match = re.search(r'find_package\(MySQL\s+(\d+\.\d+)', content, re.IGNORECASE)
                if mysql_match:
                    mysql_version = mysql_match.group(1)
                    print(f"Found MySQL version: {mysql_version}")
        except Exception as e:
            print(f"Warning: Could not read CMakeLists.txt: {e}")
    
    # Check for hardcoded credentials in source files
    src_path = Path("src")
    if src_path.exists():
        print("Scanning source files for security issues...")
        suspicious_patterns = [
            (r'password\s*=\s*["\'\'][^"\'\']+ ["\'\']', "Hardcoded password"),
            (r'api[_-]?key\s*=\s*["\'\'][^"\'\']+["\'\']', "Hardcoded API key"),
            (r'secret\s*=\s*["\'\'][^"\'\']+["\'\']', "Hardcoded secret"),
            (r'token\s*=\s*["\'\'][^"\'\']+["\'\']', "Hardcoded token")
        ]
        
        scanned_files = 0
        for ext in ['*.cpp', '*.h', '*.hpp']:
            for file_path in src_path.rglob(ext):
                scanned_files += 1
                if scanned_files % 100 == 0:
                    print(f"  Scanned {scanned_files} files...")
                
                try:
                    with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                        content = f.read()
                        for pattern, issue_type in suspicious_patterns:
                            if re.search(pattern, content, re.IGNORECASE):
                                results["security_issues"].append({
                                    "file": str(file_path.relative_to(Path.cwd())),
                                    "issue": issue_type,
                                    "severity": "HIGH",
                                    "line": "Unknown"
                                })
                except Exception:
                    continue
        
        print(f"  Scanned {scanned_files} source files")
    
    # Generate summary
    print("\n=== Scan Results ===")
    print(f"Vulnerable packages: {len(results['vulnerable_packages'])}")
    print(f"Security issues: {len(results['security_issues'])}")
    
    if results['vulnerable_packages']:
        print("\n[WARNING] Vulnerable Packages:")
        for pkg in results['vulnerable_packages']:
            print(f"  - {pkg['package']} {pkg['version']}: {pkg['recommendation']}")
    
    if results['security_issues']:
        print("\n🔍 Security Issues Found:")
        for issue in results['security_issues'][:5]:  # Show first 5
            print(f"  - {issue['file']}: {issue['issue']}")
        if len(results['security_issues']) > 5:
            print(f"  ... and {len(results['security_issues']) - 5} more")
    
    # Save results
    os.makedirs(".claude", exist_ok=True)
    with open(".claude/security_scan_results.json", "w", encoding="utf-8") as f:
        json.dump(results, f, indent=2)

    print(f"\n[OK] Results saved to .claude/security_scan_results.json")

    # Return exit code based on critical issues
    critical_count = len([p for p in results['vulnerable_packages'] if p['severity'] == 'CRITICAL'])
    critical_count += len([i for i in results['security_issues'] if i['severity'] == 'CRITICAL'])

    if critical_count > 0:
        print(f"\n[ERROR] {critical_count} critical issues found!")
        return 1
    else:
        print("\n[OK] No critical issues found")
        return 0

if __name__ == "__main__":
    sys.exit(scan_dependencies())
