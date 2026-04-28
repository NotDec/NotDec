#!/usr/bin/env bash

set -euo pipefail

archive_old_logs() {
  local log_dir archive_dir cutoff_date

  # 日志文件名用 `YYYYMMDD-XX-...` 这种前缀，所以直接按前 8 位日期筛。
  log_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
  archive_dir="$log_dir/archive"
  cutoff_date="$(date -d '10 days ago' +%Y%m%d)"

  mkdir -p "$archive_dir"

  while IFS= read -r -d '' path; do
    local name prefix target

    name="${path##*/}"
    prefix="${name%%-*}"

    [[ "$prefix" =~ ^[0-9]{8}$ ]] || continue

    # `10天前` 这里按“今天往前数 10 天，含当天”处理。
    if (( 10#$prefix <= 10#$cutoff_date )); then
      target="$archive_dir/$name"

      if [[ -e "$target" ]]; then
        printf 'skip %s: %s already exists\n' "$name" "$target" >&2
        continue
      fi

      mv -- "$path" "$target"
      printf 'moved %s\n' "$name"
    fi
  done < <(find "$log_dir" -maxdepth 1 -type f -name '*.md' -print0)
}

archive_old_logs
