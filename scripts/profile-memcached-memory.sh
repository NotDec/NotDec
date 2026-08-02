#!/usr/bin/env bash
set -euo pipefail

# Run the memcached source-IR recovery with a reproducible allocator profile and
# a lightweight RSS/PSS time series. The jemalloc profile explains live-object
# ownership; smaps_rollup remains the reference for the process memory limit.

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &>/dev/null && pwd)
PROJECT_ROOT=$(dirname "${SCRIPT_DIR}")

DEFAULT_INPUT="/tmp/notdec-memcached-stage-b-20260801.bc"
DEFAULT_NOTDEC="${PROJECT_ROOT}/build-relwithdebinfo-20260731/bin/notdec"
DEFAULT_THRESHOLD_GIB=16
DEFAULT_THREADS=1
JEMALLOC_PATH="/usr/lib/x86_64-linux-gnu/libjemalloc.so.2"
RUN_DATE=$(date +%Y%m%d)

input_path=${DEFAULT_INPUT}
notdec_path=${DEFAULT_NOTDEC}
output_dir=
threshold_gib=${DEFAULT_THRESHOLD_GIB}
threads=${DEFAULT_THREADS}
native_allocator=0

usage() {
  cat <<EOF
Usage: $0 [options]

Run memcached's frozen stage-B IR through notdec while recording RSS/PSS and,
by default, jemalloc live-allocation samples.

Options:
  --input PATH          Input bitcode (default: ${DEFAULT_INPUT})
  --notdec PATH         notdec executable (default: ${DEFAULT_NOTDEC})
  --output-dir PATH     Run directory (default: a new /tmp directory)
  --threshold-gib N     Stop at N GiB RSS (default: ${DEFAULT_THRESHOLD_GIB})
  --threads N           NOTDEC_BINARYSUB_THREADS value (default: ${DEFAULT_THREADS})
  --native              Use the native allocator instead of jemalloc
  -h, --help            Show this help

The run directory contains command.txt, rss-pss.csv, events.log, pid,
target-pid, exit-code, notdec.stdout.log, notdec.stderr.log, work/, eval/,
and jemalloc heap dumps when jemalloc profiling is enabled.
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
    --notdec)
      require_value "$@"
      notdec_path=$2
      shift 2
      ;;
    --output-dir)
      require_value "$@"
      output_dir=$2
      shift 2
      ;;
    --threshold-gib)
      require_value "$@"
      threshold_gib=$2
      shift 2
      ;;
    --threads)
      require_value "$@"
      threads=$2
      shift 2
      ;;
    --native)
      native_allocator=1
      shift
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
[[ -x "${notdec_path}" ]] || die "notdec executable does not exist or is not executable: ${notdec_path}"
[[ "${threshold_gib}" =~ ^[1-9][0-9]*$ ]] || die "--threshold-gib must be a positive integer"
[[ "${threads}" =~ ^[1-9][0-9]*$ ]] || die "--threads must be a positive integer"

notdec_realpath=$(readlink -f "${notdec_path}") || die "cannot resolve notdec path: ${notdec_path}"
threshold_kib=$((threshold_gib * 1024 * 1024))

if [[ -z "${output_dir}" ]]; then
  output_dir=$(mktemp -d "/tmp/notdec-memcached-profile-${RUN_DATE}-XXXXXX")
else
  if [[ -e "${output_dir}" && ! -d "${output_dir}" ]]; then
    die "output path is not a directory: ${output_dir}"
  fi
  mkdir -p "${output_dir}"
  if [[ -n "$(find "${output_dir}" -mindepth 1 -maxdepth 1 -print -quit)" ]]; then
    die "output directory is not empty: ${output_dir}"
  fi
fi

mkdir -p "${output_dir}/work" "${output_dir}/eval"
printf '%s\n' "${output_dir}" >"${output_dir}/run-dir"
printf 'elapsed_ms,rss_kib,pss_kib,private_kib\n' >"${output_dir}/rss-pss.csv"
: >"${output_dir}/events.log"

run_args=(
  "${notdec_path}"
  "${input_path}"
  --tr-level=2
  --merge-struct-ptr-load-store
  --gen-work-dir
  --fast-work-dir
  --work-dir="${output_dir}/work"
  --merge-eval-dir="${output_dir}/eval"
  -o "${output_dir}/out.ll"
)

env_args=()
if (( native_allocator )); then
  # Do not let an inherited profiling environment silently change the native run.
  env_args+=("-u" "MALLOC_CONF" "-u" "LD_PRELOAD" "NOTDEC_BINARYSUB_THREADS=${threads}")
else
  [[ -r "${JEMALLOC_PATH}" ]] || die "jemalloc is unavailable: ${JEMALLOC_PATH} (use --native)"
  env_args+=(
    "NOTDEC_BINARYSUB_THREADS=${threads}"
    "MALLOC_CONF=prof:true,prof_active:true,lg_prof_sample:19,lg_prof_interval:30,prof_prefix:${output_dir}/jeprof"
    "LD_PRELOAD=${JEMALLOC_PATH}"
  )
fi

{
  printf 'setsid env'
  for arg in "${env_args[@]}"; do
    printf ' %q' "${arg}"
  done
  printf ' /usr/bin/time -v'
  for arg in "${run_args[@]}"; do
    printf ' %q' "${arg}"
  done
  printf '\n'
} >"${output_dir}/command.txt"

printf 'START threshold_gib=%s threads=%s allocator=%s\n' \
  "${threshold_gib}" "${threads}" "$([[ ${native_allocator} -eq 1 ]] && echo native || echo jemalloc)" \
  >>"${output_dir}/events.log"

# setsid gives the time wrapper and notdec one process group. We can therefore
# stop both when the RSS threshold is reached, including the real notdec child.
setsid env "${env_args[@]}" /usr/bin/time -v "${run_args[@]}" \
  >"${output_dir}/notdec.stdout.log" \
  2>"${output_dir}/notdec.stderr.log" &
launcher_pid=$!
printf '%s\n' "${launcher_pid}" >"${output_dir}/pid"

stop_group() {
  if kill -0 "${launcher_pid}" 2>/dev/null; then
    kill -TERM -- "-${launcher_pid}" 2>/dev/null || true
  fi
}

interrupted=0
on_interrupt() {
  interrupted=1
  printf 'STOP reason=signal\n' >>"${output_dir}/events.log"
  stop_group
}
trap on_interrupt INT TERM HUP

# /usr/bin/time is a wrapper, so locate the actual notdec process below it.
# Monitoring the wrapper alone was the source of a previous run that ignored
# the 16 GiB limit while notdec continued to grow.
find_notdec_pid() {
  local -a queue=("${launcher_pid}")
  local pid child exe children
  local i=0

  while (( i < ${#queue[@]} )); do
    pid=${queue[$i]}
    ((i += 1))
    [[ -d "/proc/${pid}" ]] || continue

    exe=$(readlink -f "/proc/${pid}/exe" 2>/dev/null || true)
    if [[ "${exe}" == "${notdec_realpath}" ]]; then
      printf '%s\n' "${pid}"
      return 0
    fi

    children=$(cat "/proc/${pid}/task/${pid}/children" 2>/dev/null || true)
    for child in ${children}; do
      queue+=("${child}")
    done
  done
  return 1
}

target_pid=
start_ms=$(date +%s%3N)
printf 'target-pid pending\n' >"${output_dir}/target-pid"
threshold_reached=0

while kill -0 "${launcher_pid}" 2>/dev/null; do
  if [[ -z "${target_pid}" ]]; then
    target_pid=$(find_notdec_pid || true)
    if [[ -n "${target_pid}" ]]; then
      printf '%s\n' "${target_pid}" >"${output_dir}/target-pid"
      printf 'TARGET_PID=%s\n' "${target_pid}" >>"${output_dir}/events.log"
    fi
  fi

  if [[ -n "${target_pid}" && -r "/proc/${target_pid}/smaps_rollup" ]]; then
    now_ms=$(date +%s%3N)
    elapsed_ms=$((now_ms - start_ms))
    stats=$(awk '
      /^Rss:/ { rss = $2 }
      /^Pss:/ { pss = $2 }
      /^Private_(Clean|Dirty):/ { private += $2 }
      END { printf "%s %s %s", rss + 0, pss + 0, private + 0 }
    ' "/proc/${target_pid}/smaps_rollup" 2>/dev/null || true)
    [[ -n "${stats}" ]] || continue
    read -r rss_kib pss_kib private_kib <<<"${stats}" || continue
    printf '%s,%s,%s,%s\n' \
      "${elapsed_ms}" "${rss_kib}" "${pss_kib}" "${private_kib}" \
      >>"${output_dir}/rss-pss.csv"

    if (( rss_kib >= threshold_kib )); then
      printf 'STOP reason=rss-%sg elapsed_ms=%s rss_kib=%s pss_kib=%s private_kib=%s\n' \
        "${threshold_gib}" "${elapsed_ms}" "${rss_kib}" "${pss_kib}" "${private_kib}" \
        | tee -a "${output_dir}/events.log"
      threshold_reached=1
      stop_group
      break
    fi
  fi

  sleep 0.1
done

set +e
wait "${launcher_pid}"
status=$?
set -e
printf '%s\n' "${status}" >"${output_dir}/exit-code"

if (( interrupted )); then
  echo "Profile interrupted; run directory: ${output_dir}"
  exit 130
fi

if (( threshold_reached )); then
  echo "Profile stopped at RSS threshold; run directory: ${output_dir}"
  echo "notdec exit code after SIGTERM: ${status}"
  exit 0
fi

echo "Profile completed; run directory: ${output_dir}"
exit "${status}"
