#!/usr/bin/env bash
# =============================================================================
# Precision-Calculator: Automated Breakpoint & Extremity Probing Runner
# File: breakpoints/runner.sh
# =============================================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

echo "========================================================================"
echo "         PRECISION-CALCULATOR: AUTOMATED BREAKPOINT HARNESS             "
echo "========================================================================"

cd "$PROJECT_ROOT"

# Ensure calculator is built
if [[ ! -x "calculator" ]]; then
    echo "[*] Building calculator binary..."
    make
fi

echo "[*] Executing full extremity probing suite..."
python3 "$SCRIPT_DIR/test_breakpoints.py"

echo "[*] Probing complete. Results logged to $SCRIPT_DIR/test_results.csv"

