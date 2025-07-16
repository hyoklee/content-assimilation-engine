#!/bin/bash

# Automated validation for balanced, concurrent MPI job launching
# Usage: bash validate_balanced_concurrent.sh [hostfile]

set -e

CONFIG=../omni/config/balanced_concurrent_test.yaml
HOSTFILE=${1:-my_hostfile}
WRP=./bin/wrp

if [ ! -f "$WRP" ]; then
    echo "❌ wrp executable not found. Run from build/ directory."
    exit 1
fi
if [ ! -f "$CONFIG" ]; then
    echo "❌ Test YAML config not found: $CONFIG"
    exit 1
fi
if [ ! -f "$HOSTFILE" ]; then
    echo "❌ Hostfile not found: $HOSTFILE"
    exit 1
fi

# Clean up any old temp hostfiles
rm -f hostfile_job_*.tmp

# Run the test
set +e
$WRP $CONFIG $HOSTFILE > balanced_concurrent_test.log 2>&1
RESULT=$?
set -e

if [ $RESULT -ne 0 ]; then
    echo "❌ wrp run FAILED. See balanced_concurrent_test.log for details."
    exit 1
fi

echo "✅ wrp run completed. Checking for temp hostfile cleanup..."
if ls hostfile_job_*.tmp 1> /dev/null 2>&1; then
    echo "❌ Temp hostfiles were not cleaned up!"
    ls hostfile_job_*.tmp
    exit 1
else
    echo "✅ Temp hostfiles cleaned up."
fi

echo -e "\n=== Node allocation summary ==="
# Parse node allocation from the new log format
declare -A node_counts
declare -A node_job_map
declare -A job_nodes

# Extract node allocations from log
while IFS= read -r line; do
    if [[ $line =~ Job[[:space:]]([0-9]+)[[:space:]]allocated[[:space:]]nodes:[[:space:]](.+) ]]; then
        job_id="${BASH_REMATCH[1]}"
        nodes_str="${BASH_REMATCH[2]}"
        # Remove the hostfile part from the end
        nodes_str="${nodes_str% (hostfile:*)}"
        # Split nodes by comma and space
        nodes=(${nodes_str//, / })
        
        echo "Job $job_id: ${nodes[*]}"
        job_nodes[$job_id]="${nodes[*]}"
        
        # Count node usage
        for node in "${nodes[@]}"; do
            node_counts[$node]=$(( ${node_counts[$node]:-0} + 1 ))
            node_job_map[$node]="${node_job_map[$node]} $job_id"
        done
    fi
done < balanced_concurrent_test.log

# Print node usage summary
if [ ${#node_counts[@]} -gt 0 ]; then
    echo -e "\nNode usage frequency:"
    for node in "${!node_counts[@]}"; do
        echo "  $node: ${node_counts[$node]} jobs (job IDs:${node_job_map[$node]})"
    done
    
    # Check for balance
    max=0; min=1000000
    for count in "${node_counts[@]}"; do
        [ $count -gt $max ] && max=$count
        [ $count -lt $min ] && min=$count
    done
    
    if [ $((max-min)) -gt 1 ]; then
        echo "⚠️  Warning: Node usage is imbalanced (max $max, min $min)"
    else
        echo "✅ Node usage is balanced."
    fi
    
    # Show job distribution
    echo -e "\nJob distribution:"
    for job_id in "${!job_nodes[@]}"; do
        echo "  Job $job_id: ${job_nodes[$job_id]}"
    done
else
    echo "❌ No node allocation information found in log."
    echo "Make sure wrp.cc is compiled with the latest changes."
fi

echo -e "\n=== Job completion summary ==="
grep --color=never 'Successfully completed processing' balanced_concurrent_test.log

if grep -q '✗ Failed to process' balanced_concurrent_test.log; then
    echo "❌ Some jobs FAILED. See balanced_concurrent_test.log for details."
    exit 1
fi

echo -e "\n====================================="
echo "🎉 BALANCED CONCURRENT TEST PASSED!"
echo "=====================================" 