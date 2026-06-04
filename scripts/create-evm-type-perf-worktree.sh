#!/usr/bin/env bash
set -euo pipefail

SOURCE_ROOT=${SOURCE_ROOT:-/sn640/NotDec}
WORKTREE_ROOT=${WORKTREE_ROOT:-/sn640/NotDec-worktree-evm-type-perf}
BASE_BRANCH=${BASE_BRANCH:-v2}
WORKTREE_BRANCH=${WORKTREE_BRANCH:-worktree/evm-type-perf}
LLVM_DIR_NAME=${LLVM_DIR_NAME:-llvm-22.1.0.obj}

usage() {
  cat <<EOF
Usage: $0 [--source PATH] [--worktree PATH] [--base BRANCH] [--branch BRANCH]

Defaults:
  --source   ${SOURCE_ROOT}
  --worktree ${WORKTREE_ROOT}
  --base     ${BASE_BRANCH}
  --branch   ${WORKTREE_BRANCH}

The script creates a NotDec worktree for EVM type-recovery performance work.
It also initializes submodules. If a recorded submodule commit is not available
from the remote, it clones from the source worktree's local submodule checkout.
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
  --source)
    SOURCE_ROOT=$2
    shift 2
    ;;
  --worktree)
    WORKTREE_ROOT=$2
    shift 2
    ;;
  --base)
    BASE_BRANCH=$2
    shift 2
    ;;
  --branch)
    WORKTREE_BRANCH=$2
    shift 2
    ;;
  -h | --help)
    usage
    exit 0
    ;;
  *)
    echo "Unknown argument: $1" >&2
    usage >&2
    exit 2
    ;;
  esac
done

if [[ ! -d "${SOURCE_ROOT}/.git" && ! -f "${SOURCE_ROOT}/.git" ]]; then
  echo "Source is not a git worktree: ${SOURCE_ROOT}" >&2
  exit 1
fi

if [[ -e "${WORKTREE_ROOT}" ]]; then
  echo "Worktree path already exists: ${WORKTREE_ROOT}" >&2
  echo "Remove it first or pass --worktree with a different path." >&2
  exit 1
fi

echo "Creating worktree ${WORKTREE_ROOT} from ${BASE_BRANCH}..."
if git -C "${SOURCE_ROOT}" show-ref --verify --quiet "refs/heads/${WORKTREE_BRANCH}"; then
  echo "Branch already exists: ${WORKTREE_BRANCH}" >&2
  echo "Pass --branch with a new name, or remove/reuse the branch manually." >&2
  exit 1
fi
git -C "${SOURCE_ROOT}" worktree add -b "${WORKTREE_BRANCH}" \
  "${WORKTREE_ROOT}" "${BASE_BRANCH}"

remove_failed_submodule_gitdir() {
  local path=$1
  local gitdir=""

  if [[ -e "${WORKTREE_ROOT}/${path}/.git" ]]; then
    gitdir=$(git -C "${WORKTREE_ROOT}/${path}" rev-parse --git-dir 2>/dev/null || true)
  fi

  if [[ -n "${gitdir}" && "${gitdir}" != /* ]]; then
    gitdir=$(cd "${WORKTREE_ROOT}/${path}" && cd "${gitdir}" && pwd -P)
  fi

  rm -rf "${WORKTREE_ROOT:?}/${path}"

  # Only remove gitdirs owned by this worktree's submodule state.
  if [[ -n "${gitdir}" && "${gitdir}" == *"/.git/worktrees/"*"/modules/"* ]]; then
    rm -rf "${gitdir}"
  fi
}

init_submodule() {
  local path=$1
  local target_rev
  target_rev=$(git -C "${WORKTREE_ROOT}" ls-files -s "${path}" | awk '{print $2}')

  echo "Initializing ${path}..."
  if git -C "${WORKTREE_ROOT}" submodule update --init "${path}"; then
    return
  fi

  echo "Remote init failed for ${path}; falling back to local clone."
  if [[ ! -d "${SOURCE_ROOT}/${path}/.git" && ! -f "${SOURCE_ROOT}/${path}/.git" ]]; then
    echo "Local submodule source is missing: ${SOURCE_ROOT}/${path}" >&2
    exit 1
  fi

  remove_failed_submodule_gitdir "${path}"
  git clone "${SOURCE_ROOT}/${path}" "${WORKTREE_ROOT}/${path}"
  git -C "${WORKTREE_ROOT}/${path}" checkout "${target_rev}"
}

mapfile -t SUBMODULES < <(git -C "${WORKTREE_ROOT}" config --file .gitmodules --get-regexp path |
  awk '{print $2}')

for path in "${SUBMODULES[@]}"; do
  init_submodule "${path}"
done

if [[ -d "${SOURCE_ROOT}/${LLVM_DIR_NAME}" && ! -e "${WORKTREE_ROOT}/${LLVM_DIR_NAME}" ]]; then
  echo "Linking ${LLVM_DIR_NAME} from source worktree..."
  ln -s "${SOURCE_ROOT}/${LLVM_DIR_NAME}" "${WORKTREE_ROOT}/${LLVM_DIR_NAME}"
fi

exclude_file=$(git -C "${WORKTREE_ROOT}" rev-parse --git-path info/exclude)
if ! grep -qxF "${LLVM_DIR_NAME}" "${exclude_file}"; then
  echo "${LLVM_DIR_NAME}" >>"${exclude_file}"
fi

echo "Configuring build..."
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=TRUE -S "${WORKTREE_ROOT}" \
  -B "${WORKTREE_ROOT}/build" -G Ninja

echo "Building validation targets..."
cmake --build "${WORKTREE_ROOT}/build" \
  --target notdec-decompile TypeBuilderTest binarysub -j4

echo "Done."
echo "Worktree: ${WORKTREE_ROOT}"
echo "Branch:   $(git -C "${WORKTREE_ROOT}" branch --show-current)"
