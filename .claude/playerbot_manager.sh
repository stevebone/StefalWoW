#!/bin/bash
# PlayerBot Management Launcher

echo "==============================================="
echo "PlayerBot Management System"
echo "==============================================="
echo

while true; do
    echo "Select an option:"
    echo "1. Run Security Scan"
    echo "2. Start Monitoring Service"
    echo "3. Create Rollback Point"
    echo "4. Emergency Rollback"
    echo "5. System Health Check"
    echo "6. Open Monitoring Dashboard"
    echo "7. Exit"
    echo
    read -p "Enter your choice (1-7): " choice

    case $choice in
        1)
            echo "Running security scan..."
            python3 "C:\TrinityBots\TrinityCore\.claude/scripts/dependency_scanner.py"
            ;;
        2)
            echo "Starting monitoring service..."
            python3 "C:\TrinityBots\TrinityCore\.claude/monitoring/monitor_service.py"
            ;;
        3)
            echo "Creating rollback point..."
            python3 "C:\TrinityBots\TrinityCore\.claude/scripts/rollback.py" create
            ;;
        4)
            echo "Performing emergency rollback..."
            python3 "C:\TrinityBots\TrinityCore\.claude/scripts/rollback.py" emergency
            ;;
        5)
            echo "Running health check..."
            python3 "C:\TrinityBots\TrinityCore\.claude/scripts/health_check.py"
            ;;
        6)
            echo "Opening monitoring dashboard..."
            if command -v xdg-open > /dev/null; then
                xdg-open "C:\TrinityBots\TrinityCore\.claude/monitoring/dashboard.html"
            elif command -v open > /dev/null; then
                open "C:\TrinityBots\TrinityCore\.claude/monitoring/dashboard.html"
            else
                echo "Please open C:\TrinityBots\TrinityCore\.claude/monitoring/dashboard.html manually"
            fi
            ;;
        7)
            echo "Goodbye!"
            exit 0
            ;;
        *)
            echo "Invalid choice!"
            ;;
    esac
    
    echo
    read -p "Press Enter to continue..."
    echo
done
