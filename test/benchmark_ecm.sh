#!/bin/sh
set -eu

if [ "$#" -lt 1 ] || [ "$#" -gt 2 ]; then
    echo "Usage: $0 <path-to-textual_flow_benchmark> [workers]" >&2
    exit 2
fi

examples_dir="$(CDPATH= cd "$(dirname "$0")/../examples" && pwd)"
benchmark="$1"
workers="${2:-1}"

echo "ECM Mark (Phase 3.5), $workers worker(s)" >&2
"$benchmark" "$examples_dir/ecm_mark_ph35.xml" "$workers" > /dev/null

echo "ECM Acts (Phase 4), $workers worker(s)" >&2
"$benchmark" "$examples_dir/ecm_acts_ph4.xml" "$workers" > /dev/null
