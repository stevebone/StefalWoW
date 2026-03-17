#!/usr/bin/env python3
"""
Real-time monitoring service for PlayerBot
"""
import json
import time
import psutil
import os
from pathlib import Path
from datetime import datetime

def get_system_metrics():
    """Get current system metrics"""
    try:
        cpu_percent = psutil.cpu_percent(interval=1)
        memory = psutil.virtual_memory()
        disk = psutil.disk_usage('/')
        
        # Check for TrinityCore processes
        trinity_running = False
        trinity_memory = 0
        
        for proc in psutil.process_iter(['pid', 'name', 'memory_info']):
            try:
                if 'worldserver' in proc.info['name'].lower():
                    trinity_running = True
                    trinity_memory = proc.info['memory_info'].rss / 1024 / 1024  # MB
                    break
            except (psutil.NoSuchProcess, psutil.AccessDenied):
                continue
        
        return {
            "timestamp": datetime.now().isoformat(),
            "cpu_usage": round(cpu_percent, 1),
            "memory_usage": round(memory.percent, 1),
            "memory_used_gb": round(memory.used / 1024**3, 2),
            "disk_usage": round(disk.percent, 1),
            "process_count": len(psutil.pids()),
            "trinity_running": trinity_running,
            "trinity_memory_mb": round(trinity_memory, 1) if trinity_running else 0
        }
    except Exception as e:
        return {
            "timestamp": datetime.now().isoformat(),
            "error": str(e),
            "cpu_usage": 0,
            "memory_usage": 0,
            "disk_usage": 0
        }

def main():
    print("Starting PlayerBot Monitoring Service...")
    
    # Ensure directories exist
    monitoring_dir = Path(".claude/monitoring")
    monitoring_dir.mkdir(parents=True, exist_ok=True)
    
    metrics_file = monitoring_dir / "metrics.json"
    log_file = monitoring_dir / "monitor.log"
    
    print(f"Metrics file: {metrics_file}")
    print(f"Log file: {log_file}")
    print("Press Ctrl+C to stop monitoring\n")
    
    try:
        iteration = 0
        while True:
            iteration += 1
            metrics = get_system_metrics()
            
            # Save metrics
            try:
                with open(metrics_file, 'w') as f:
                    json.dump(metrics, f, indent=2)
            except Exception as e:
                print(f"Error saving metrics: {e}")
            
            # Display current metrics
            if 'error' not in metrics:
                print(f"[{iteration:3d}] {datetime.now().strftime('%H:%M:%S')} - "
                      f"CPU: {metrics['cpu_usage']:5.1f}% | "
                      f"Memory: {metrics['memory_usage']:5.1f}% | "
                      f"Disk: {metrics['disk_usage']:5.1f}% | "
                      f"Trinity: {'✓' if metrics['trinity_running'] else '✗'}")
                
                # Log warnings
                warnings = []
                if metrics['cpu_usage'] > 80:
                    warnings.append(f"High CPU: {metrics['cpu_usage']}%")
                if metrics['memory_usage'] > 85:
                    warnings.append(f"High Memory: {metrics['memory_usage']}%")
                if metrics['disk_usage'] > 90:
                    warnings.append(f"High Disk: {metrics['disk_usage']}%")
                
                if warnings:
                    warning_msg = f"{datetime.now()}: WARNING - {', '.join(warnings)}"
                    print(f"⚠️  {warning_msg}")
                    try:
                        with open(log_file, 'a') as f:
                            f.write(warning_msg + "\n")
                    except Exception:
                        pass
            else:
                print(f"[{iteration:3d}] Error getting metrics: {metrics.get('error', 'Unknown')}")
            
            time.sleep(10)
    
    except KeyboardInterrupt:
        print("\n✓ Monitoring stopped by user")
    except Exception as e:
        print(f"\n❌ Monitoring error: {e}")

if __name__ == "__main__":
    main()
