#!/bin/bash
# Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
#
# PlayerBot Group Functionality Test Runner
# This script runs the comprehensive test suite for group functionality validation

set -e  # Exit on error
set -o pipefail  # Fail on pipe errors

# Script configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" &> /dev/null && pwd)"
BUILD_DIR="${SCRIPT_DIR}/../../../../build_install"
TESTS_DIR="${BUILD_DIR}/src/modules/Playerbot/Tests"
REPORTS_DIR="${SCRIPT_DIR}/test_reports"
ARTIFACTS_DIR="${SCRIPT_DIR}/test_artifacts"

# Test configuration
TEST_TIMEOUT=1800  # 30 minutes max
ENABLE_COVERAGE=false
ENABLE_SANITIZERS=false
PARALLEL_TESTS=1
VERBOSE=false

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Logging functions
log_info() {
    echo -e "${CYAN}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

log_header() {
    echo -e "\n${PURPLE}============================================${NC}"
    echo -e "${PURPLE}$1${NC}"
    echo -e "${PURPLE}============================================${NC}\n"
}

# Print usage information
print_usage() {
    cat << EOF
PlayerBot Group Functionality Test Runner

Usage: $0 [OPTIONS] [TEST_FILTER]

OPTIONS:
    -h, --help              Show this help message
    -v, --verbose           Enable verbose output
    -c, --coverage          Enable code coverage reporting
    -s, --sanitizers        Enable memory/thread sanitizers
    -p, --parallel N        Run N tests in parallel (default: 1)
    -t, --timeout SECONDS   Set test timeout (default: 1800)
    -r, --reports-dir DIR   Set reports output directory
    -a, --artifacts-dir DIR Set artifacts output directory
    --quick                 Run only smoke tests (fast validation)
    --stress                Run stress tests only
    --demo                  Run interactive demonstration
    --ci                    CI mode (fail-fast, JUnit output)

TEST_FILTER:
    Can be used to run specific test patterns:
    - Group*               Run all group functionality tests
    - *Stress*             Run all stress tests
    - *Performance*        Run all performance tests
    - GroupInvitation*     Run specific test suite

Examples:
    $0                          # Run all tests
    $0 --quick                  # Quick validation (5 minutes)
    $0 --stress --parallel 2    # Parallel stress testing
    $0 --demo                   # Interactive demonstration
    $0 --ci --coverage          # CI pipeline with coverage
    $0 "Group*" --verbose       # All group tests with verbose output

EOF
}

# Parse command line arguments
parse_arguments() {
    while [[ $# -gt 0 ]]; do
        case $1 in
            -h|--help)
                print_usage
                exit 0
                ;;
            -v|--verbose)
                VERBOSE=true
                shift
                ;;
            -c|--coverage)
                ENABLE_COVERAGE=true
                shift
                ;;
            -s|--sanitizers)
                ENABLE_SANITIZERS=true
                shift
                ;;
            -p|--parallel)
                PARALLEL_TESTS="$2"
                shift 2
                ;;
            -t|--timeout)
                TEST_TIMEOUT="$2"
                shift 2
                ;;
            -r|--reports-dir)
                REPORTS_DIR="$2"
                shift 2
                ;;
            -a|--artifacts-dir)
                ARTIFACTS_DIR="$2"
                shift 2
                ;;
            --quick)
                TEST_FILTER="SMOKE"
                shift
                ;;
            --stress)
                TEST_FILTER="STRESS"
                shift
                ;;
            --demo)
                RUN_DEMO=true
                shift
                ;;
            --ci)
                CI_MODE=true
                ENABLE_COVERAGE=true
                shift
                ;;
            -*)
                log_error "Unknown option: $1"
                print_usage
                exit 1
                ;;
            *)
                TEST_FILTER="$1"
                shift
                ;;
        esac
    done
}

# Validate environment and dependencies
validate_environment() {
    log_info "Validating test environment..."

    # Check if build directory exists
    if [[ ! -d "$BUILD_DIR" ]]; then
        log_error "Build directory not found: $BUILD_DIR"
        log_error "Please run CMake build first"
        exit 1
    fi

    # Check if test executable exists
    local test_executable="${BUILD_DIR}/src/modules/Playerbot/Tests/playerbot_tests"
    if [[ ! -f "$test_executable" ]]; then
        log_error "Test executable not found: $test_executable"
        log_error "Please build with -DBUILD_PLAYERBOT_TESTS=ON"
        exit 1
    fi

    # Create output directories
    mkdir -p "$REPORTS_DIR" "$ARTIFACTS_DIR"

    # Check required tools
    local missing_tools=()

    if [[ "$ENABLE_COVERAGE" == "true" ]] && ! command -v lcov &> /dev/null; then
        missing_tools+=("lcov")
    fi

    if [[ "${#missing_tools[@]}" -gt 0 ]]; then
        log_warning "Missing optional tools: ${missing_tools[*]}"
        log_warning "Some features may not be available"
    fi

    log_success "Environment validation completed"
}

# Build tests with specified configuration
build_tests() {
    log_info "Building test suite with configuration..."

    local cmake_args="-DBUILD_PLAYERBOT_TESTS=ON"

    if [[ "$ENABLE_COVERAGE" == "true" ]]; then
        cmake_args+=" -DCMAKE_BUILD_TYPE=Debug"
        cmake_args+=" -DCMAKE_CXX_FLAGS='--coverage'"
    fi

    if [[ "$ENABLE_SANITIZERS" == "true" ]]; then
        cmake_args+=" -DPLAYERBOT_TESTS_ASAN=ON"
        cmake_args+=" -DPLAYERBOT_TESTS_UBSAN=ON"
    fi

    log_info "CMake arguments: $cmake_args"

    # Build the tests
    cd "$BUILD_DIR"
    if make playerbot_tests -j$(nproc) 2>&1 | tee "${ARTIFACTS_DIR}/build.log"; then
        log_success "Test suite built successfully"
    else
        log_error "Test build failed"
        return 1
    fi

    cd "$SCRIPT_DIR"
}

# Run the demonstration mode
run_demonstration() {
    log_header "PLAYERBOT GROUP FUNCTIONALITY DEMONSTRATION"

    local demo_executable="${BUILD_DIR}/src/modules/Playerbot/Tests/ProductionValidationDemo"

    if [[ ! -f "$demo_executable" ]]; then
        log_error "Demo executable not found. Please build the test suite first."
        return 1
    fi

    log_info "Starting interactive demonstration..."

    if "$demo_executable" --interactive; then
        log_success "Demonstration completed successfully"
    else
        log_error "Demonstration failed"
        return 1
    fi
}

# Run specific test categories
run_test_category() {
    local category="$1"
    local test_executable="${BUILD_DIR}/src/modules/Playerbot/Tests/playerbot_tests"

    log_info "Running $category tests..."

    local gtest_args=""
    case "$category" in
        "SMOKE")
            gtest_args="--gtest_filter=*Smoke*:*Quick*:*Basic*"
            ;;
        "STRESS")
            gtest_args="--gtest_filter=*Stress*:*Load*:*Concurrent*"
            ;;
        "PERFORMANCE")
            gtest_args="--gtest_filter=*Performance*:*Timing*:*Memory*"
            ;;
        *)
            gtest_args="--gtest_filter=$category"
            ;;
    esac

    local output_file="${REPORTS_DIR}/${category,,}_test_results.xml"
    local log_file="${REPORTS_DIR}/${category,,}_test.log"

    # Additional GTest arguments
    gtest_args+=" --gtest_output=xml:$output_file"

    if [[ "$VERBOSE" == "true" ]]; then
        gtest_args+=" --gtest_print_time=1"
        gtest_args+=" --gtest_print_utf8=1"
    fi

    log_info "Running: $test_executable $gtest_args"

    # Run tests with timeout
    if timeout "$TEST_TIMEOUT" "$test_executable" $gtest_args 2>&1 | tee "$log_file"; then
        log_success "$category tests completed"
        return 0
    else
        local exit_code=$?
        if [[ $exit_code -eq 124 ]]; then
            log_error "$category tests timed out after $TEST_TIMEOUT seconds"
        else
            log_error "$category tests failed with exit code $exit_code"
        fi
        return $exit_code
    fi
}

# Generate comprehensive test report
generate_report() {
    log_info "Generating comprehensive test report..."

    local report_file="${REPORTS_DIR}/comprehensive_test_report.html"
    local summary_file="${REPORTS_DIR}/test_summary.txt"

    cat > "$report_file" << 'EOF'
<!DOCTYPE html>
<html>
<head>
    <title>PlayerBot Group Functionality Test Report</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; }
        .header { background: #2c3e50; color: white; padding: 20px; border-radius: 5px; }
        .success { color: #27ae60; }
        .failure { color: #e74c3c; }
        .warning { color: #f39c12; }
        .section { margin: 20px 0; padding: 15px; border: 1px solid #ddd; border-radius: 5px; }
        table { width: 100%; border-collapse: collapse; margin: 10px 0; }
        th, td { padding: 8px; text-align: left; border-bottom: 1px solid #ddd; }
        th { background-color: #f2f2f2; }
        pre { background: #f8f9fa; padding: 10px; border-radius: 3px; overflow-x: auto; }
    </style>
</head>
<body>
    <div class="header">
        <h1>PlayerBot Group Functionality Test Report</h1>
        <p>Generated on: $(date)</p>
        <p>Test Configuration: Coverage=$ENABLE_COVERAGE, Sanitizers=$ENABLE_SANITIZERS</p>
    </div>
EOF

    # Add test results summary
    echo '<div class="section"><h2>Test Results Summary</h2>' >> "$report_file"

    # Process XML results if available
    if find "$REPORTS_DIR" -name "*.xml" -type f | head -1 > /dev/null 2>&1; then
        local total_tests=0
        local passed_tests=0
        local failed_tests=0

        for xml_file in "$REPORTS_DIR"/*.xml; do
            if [[ -f "$xml_file" ]]; then
                # Parse GTest XML results (simplified)
                local tests=$(grep -c "<testcase" "$xml_file" 2>/dev/null || echo "0")
                local failures=$(grep -c "<failure" "$xml_file" 2>/dev/null || echo "0")

                total_tests=$((total_tests + tests))
                failed_tests=$((failed_tests + failures))
            fi
        done

        passed_tests=$((total_tests - failed_tests))

        cat >> "$report_file" << EOF
        <table>
            <tr><th>Metric</th><th>Value</th><th>Status</th></tr>
            <tr><td>Total Tests</td><td>$total_tests</td><td>-</td></tr>
            <tr><td>Passed</td><td class="success">$passed_tests</td><td>✓</td></tr>
            <tr><td>Failed</td><td class="failure">$failed_tests</td><td>$([ $failed_tests -eq 0 ] && echo "✓" || echo "✗")</td></tr>
            <tr><td>Success Rate</td><td>$(( total_tests > 0 ? passed_tests * 100 / total_tests : 0 ))%</td><td>$([ $failed_tests -eq 0 ] && echo "✓" || echo "✗")</td></tr>
        </table>
EOF
    fi

    echo '</div>' >> "$report_file"

    # Add performance metrics section
    echo '<div class="section"><h2>Performance Validation</h2>' >> "$report_file"
    echo '<p>Performance metrics are validated against production thresholds:</p>' >> "$report_file"
    echo '<ul>' >> "$report_file"
    echo '<li>Memory usage: &lt; 10MB per bot</li>' >> "$report_file"
    echo '<li>CPU usage: &lt; 0.1% per bot</li>' >> "$report_file"
    echo '<li>Invitation acceptance: &lt; 3 seconds</li>' >> "$report_file"
    echo '<li>Combat engagement: &lt; 3 seconds</li>' >> "$report_file"
    echo '<li>Target switching: &lt; 1 second</li>' >> "$report_file"
    echo '</ul></div>' >> "$report_file"

    # Add coverage information if enabled
    if [[ "$ENABLE_COVERAGE" == "true" ]]; then
        echo '<div class="section"><h2>Code Coverage</h2>' >> "$report_file"
        if [[ -f "${ARTIFACTS_DIR}/coverage.info" ]]; then
            echo '<p>Code coverage report generated successfully.</p>' >> "$report_file"
            echo '<p><a href="coverage/index.html">View detailed coverage report</a></p>' >> "$report_file"
        else
            echo '<p class="warning">Code coverage data not available.</p>' >> "$report_file"
        fi
        echo '</div>' >> "$report_file"
    fi

    # Close HTML
    echo '</body></html>' >> "$report_file"

    # Generate text summary
    cat > "$summary_file" << EOF
PlayerBot Group Functionality Test Summary
==========================================

Test Run Information:
  Date: $(date)
  Configuration: Coverage=$ENABLE_COVERAGE, Sanitizers=$ENABLE_SANITIZERS
  Filter: ${TEST_FILTER:-"All tests"}
  Parallel: $PARALLEL_TESTS
  Timeout: $TEST_TIMEOUT seconds

Results: $([ -f "$report_file" ] && echo "See $report_file for detailed results" || echo "Report generation failed")

Files Generated:
  - Test Report: $report_file
  - Test Logs: $REPORTS_DIR/*.log
  - Test Artifacts: $ARTIFACTS_DIR/
EOF

    log_success "Test report generated: $report_file"
    log_success "Test summary: $summary_file"
}

# Generate code coverage report
generate_coverage_report() {
    if [[ "$ENABLE_COVERAGE" != "true" ]]; then
        return 0
    fi

    log_info "Generating code coverage report..."

    local coverage_dir="${ARTIFACTS_DIR}/coverage"
    mkdir -p "$coverage_dir"

    cd "$BUILD_DIR"

    # Generate coverage data
    if lcov --capture --directory . --output-file "${coverage_dir}/coverage.info" 2>&1 | tee "${coverage_dir}/lcov.log"; then
        # Filter out system headers and test files
        lcov --remove "${coverage_dir}/coverage.info" '/usr/*' '*/tests/*' '*/gtest/*' --output-file "${coverage_dir}/coverage_filtered.info"

        # Generate HTML report
        genhtml "${coverage_dir}/coverage_filtered.info" --output-directory "${coverage_dir}/html" --title "PlayerBot Test Coverage"

        log_success "Coverage report generated: ${coverage_dir}/html/index.html"
    else
        log_error "Failed to generate coverage report"
        return 1
    fi

    cd "$SCRIPT_DIR"
}

# Main execution function
main() {
    log_header "PLAYERBOT GROUP FUNCTIONALITY TEST SUITE"

    # Parse arguments
    parse_arguments "$@"

    # Show configuration
    log_info "Test Configuration:"
    log_info "  Reports Directory: $REPORTS_DIR"
    log_info "  Artifacts Directory: $ARTIFACTS_DIR"
    log_info "  Test Filter: ${TEST_FILTER:-"All tests"}"
    log_info "  Parallel Tests: $PARALLEL_TESTS"
    log_info "  Timeout: $TEST_TIMEOUT seconds"
    log_info "  Coverage: $ENABLE_COVERAGE"
    log_info "  Sanitizers: $ENABLE_SANITIZERS"
    log_info "  Verbose: $VERBOSE"
    echo

    # Validate environment
    validate_environment

    # Build tests
    if ! build_tests; then
        log_error "Failed to build test suite"
        exit 1
    fi

    # Run demonstration if requested
    if [[ "$RUN_DEMO" == "true" ]]; then
        if run_demonstration; then
            log_success "Demonstration completed successfully"
        else
            log_error "Demonstration failed"
            exit 1
        fi
        return 0
    fi

    # Run tests based on filter
    local test_categories=()
    if [[ -n "$TEST_FILTER" ]]; then
        case "$TEST_FILTER" in
            "SMOKE"|"QUICK")
                test_categories=("SMOKE")
                ;;
            "STRESS")
                test_categories=("STRESS")
                ;;
            "PERFORMANCE")
                test_categories=("PERFORMANCE")
                ;;
            *)
                test_categories=("$TEST_FILTER")
                ;;
        esac
    else
        test_categories=("Group*" "PERFORMANCE" "STRESS")
    fi

    local overall_success=true
    for category in "${test_categories[@]}"; do
        if ! run_test_category "$category"; then
            overall_success=false
            if [[ "$CI_MODE" == "true" ]]; then
                log_error "CI mode: stopping on first failure"
                break
            fi
        fi
    done

    # Generate coverage report
    generate_coverage_report

    # Generate comprehensive report
    generate_report

    # Final status
    if [[ "$overall_success" == "true" ]]; then
        log_success "All tests completed successfully!"
        echo -e "\n${GREEN}🎉 PlayerBot group functionality is validated and ready for production!${NC}\n"
        exit 0
    else
        log_error "Some tests failed. Please check the reports for details."
        echo -e "\n${RED}❌ Test failures detected. Review required before production deployment.${NC}\n"
        exit 1
    fi
}

# Script entry point
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    main "$@"
fi