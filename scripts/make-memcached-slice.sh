#!/usr/bin/env bash
set -euo pipefail

# Build a function slice from the memcached frozen stage-B IR: the listed
# functions keep their definitions and everything else becomes a declaration.
# This mirrors the 2026-08-04 slice experiments that localized the memory
# peak to the ASCII item + slab allocation core.

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &>/dev/null && pwd)
PROJECT_ROOT=$(dirname "${SCRIPT_DIR}")

DEFAULT_INPUT="/tmp/notdec-memcached-stage-b-20260801.bc"
DEFAULT_LLVM_BIN="${PROJECT_ROOT}/llvm-22.1.0.obj/bin"

input_path=${DEFAULT_INPUT}
llvm_bin=${DEFAULT_LLVM_BIN}
funcs_file=
output_path=

usage() {
  cat <<EOF
Usage: $0 --funcs-file PATH --output PATH [options]

Options:
  --input PATH      Full stage-B bitcode (default: ${DEFAULT_INPUT})
  --llvm-bin PATH   LLVM 22 bin directory (default: ${DEFAULT_LLVM_BIN})
  --funcs-file PATH One function name per line; these keep their definitions
  --output PATH     Output slice .bc path
EOF
}

die() {
  echo "error: $*" >&2
  exit 2
}

require_value() {
  local option=$1
  if [[ $# -lt 2 || -z ${2:-} ]]; then
    die "${option} requires a value"
  fi
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --input)
      require_value "$@"
      input_path=$2
      shift 2
      ;;
    --llvm-bin)
      require_value "$@"
      llvm_bin=$2
      shift 2
      ;;
    --funcs-file)
      require_value "$@"
      funcs_file=$2
      shift 2
      ;;
    --output)
      require_value "$@"
      output_path=$2
      shift 2
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      die "unknown argument: $1"
      ;;
  esac
done

[[ -r "${input_path}" ]] || die "input does not exist or is not readable: ${input_path}"
[[ -r "${funcs_file}" ]] || die "--funcs-file does not exist or is not readable: ${funcs_file}"
[[ -n "${output_path}" ]] || die "--output is required"
[[ -x "${llvm_bin}/llvm-extract" ]] || die "llvm-extract not found in ${llvm_bin}"
[[ -x "${llvm_bin}/opt" ]] || die "opt not found in ${llvm_bin}"

mapfile -t funcs < <(grep -vE '^[[:space:]]*($|#)' "${funcs_file}")
[[ ${#funcs[@]} -gt 0 ]] || die "no functions listed in ${funcs_file}"

extract_args=()
for func in "${funcs[@]}"; do
  extract_args+=(--func="${func}")
done

tmp_output="${output_path}.tmp.bc"
"${llvm_bin}/llvm-extract" "${extract_args[@]}" "${input_path}" -o "${tmp_output}"
"${llvm_bin}/opt" -passes=verify "${tmp_output}" -o "${output_path}"
rm -f "${tmp_output}"

defined=$("${llvm_bin}/llvm-dis" "${output_path}" -o - | grep -c '^define')
echo "wrote ${output_path} with ${defined} defined functions"
