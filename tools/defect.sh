#!/usr/bin/env bash
# ============================================================================
# File: tools/defect.sh
# Purpose: Manages the Defect Life Cycle for Precision-Calculator in docs/DEFECT_LOG.md.
# States:
#   NEW -> ASSIGNED -> OPEN -> (DUPLICATE | REJECTED | DEFERRED) -> FIXED
#       -> RETEST -> (REOPENED -> OPEN) | (VERIFIED -> CLOSED)
# ============================================================================

set -euo pipefail

DEFECT_LOG="docs/DEFECT_LOG.md"

usage() {
    echo "Usage:"
    echo "  $0 add <ID> <Title> <Module> <Severity> <Priority> <FoundBy> <Expected> <Actual> <Steps>"
    echo "  $0 status <ID> <NEW_STATE> [Note]"
    echo "  $0 list"
    exit 1
}

if [[ $# -lt 1 ]]; then
    usage
fi

CMD="$1"
shift

case "$CMD" in
    add)
        if [[ $# -lt 9 ]]; then
            echo "Error: 'add' requires: ID Title Module Severity Priority FoundBy Expected Actual Steps"
            exit 1
        fi
        ID="$1"; TITLE="$2"; MODULE="$3"; SEV="$4"; PRIO="$5"; FOUND_BY="$6"
        EXP="$7"; ACT="$8"; STEPS="$9"
        DATE=$(date +"%Y-%m-%d %H:%M")

        # Format markdown entry
        cat <<EOF >> "$DEFECT_LOG"

### Defect: $ID - $TITLE
- **Module:** $MODULE
- **Severity:** $SEV | **Priority:** $PRIO
- **Found By:** $FOUND_BY | **Date:** $DATE
- **Current Status:** \`NEW\`
- **Expected Result:** $EXP
- **Actual Result:** $ACT
- **Reproduction Steps:** \`$STEPS\`
- **History:**
  - [$DATE] Logged as \`NEW\` by $FOUND_BY.
EOF
        echo "Added defect $ID to $DEFECT_LOG in state NEW."
        ;;

    status)
        if [[ $# -lt 2 ]]; then
            echo "Error: 'status' requires: ID NEW_STATE [Note]"
            exit 1
        fi
        ID="$1"; NEW_STATE="$2"; NOTE="${3:-State update}"
        DATE=$(date +"%Y-%m-%d %H:%M")

        case "$NEW_STATE" in
            NEW|ASSIGNED|OPEN|DUPLICATE|REJECTED|DEFERRED|FIXED|RETEST|REOPENED|VERIFIED|CLOSED)
                ;;
            *)
                echo "Error: Invalid state '$NEW_STATE'. Valid states:"
                echo "  NEW, ASSIGNED, OPEN, DUPLICATE, REJECTED, DEFERRED, FIXED, RETEST, REOPENED, VERIFIED, CLOSED"
                exit 1
                ;;
        esac

        if grep -q "### Defect: $ID" "$DEFECT_LOG"; then
            # Append history note
            sed -i '' -e "/### Defect: $ID/,/### Defect:/ s/- \*\*Current Status:\*\* .*/- \*\*Current Status:\*\* \`$NEW_STATE\`/" "$DEFECT_LOG" 2>/dev/null || \
            sed -i -e "/### Defect: $ID/,/### Defect:/ s/- \*\*Current Status:\*\* .*/- \*\*Current Status:\*\* \`$NEW_STATE\`/" "$DEFECT_LOG"
            
            echo "  - [$DATE] Transitioned to \`$NEW_STATE\`: $NOTE" >> "$DEFECT_LOG"
            echo "Updated defect $ID to $NEW_STATE."
        else
            echo "Error: Defect $ID not found in $DEFECT_LOG."
            exit 1
        fi
        ;;

    list)
        echo "=== Current Defects in $DEFECT_LOG ==="
        grep -E "^### Defect:|- \*\*Current Status:\*\*" "$DEFECT_LOG" || echo "No defects logged."
        ;;

    *)
        usage
        ;;
esac

