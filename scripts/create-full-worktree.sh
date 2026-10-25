#!/usr/bin/env bash
#
# Create a complete worktree tree for microsub2 development: one git worktree
# per repository, all on the same new branch and all at the commit currently
# checked out in the corresponding main worktree.
#
#   <new-tree>/notdec                  worktree of $SOURCE_ROOT
#   <new-tree>/notdec/external/<name>  worktree of each submodule repository
#   <new-tree>/tool                    worktree of $TOOL_SOURCE
#   <new-tree>/build.sh                full build: NotDec + microsub2
#   <new-tree>/tree-info.txt           branch and commits used by this tree
#
# The worktrees share the object database and refs of the main repositories, so
# no git object is copied.  The new tree therefore depends on the main tree's
# .git directories: keep $SOURCE_ROOT and $TOOL_SOURCE around, and remove the
# tree through `git worktree remove` in each repository instead of `rm -rf`.
#
# Uncommitted changes of the main worktrees are *not* carried over; only the
# commits are.  That matches the usual experiment setup: commit first, then
# create the tree.
#
# Usage:
#   create-full-worktree.sh <new-tree-path> <new-branch>
#
# <new-branch> must not exist in any of the repositories (the script refuses to
# reuse or check out an existing branch) and <new-tree-path> must not exist or
# must be an empty directory.
#
# Environment overrides: SOURCE_ROOT, TOOL_SOURCE, NOTDEC_LLVM_TARGET,
# MICROSUB2_LLVM_DIR, JOBS.

set -euo pipefail

SOURCE_ROOT=${SOURCE_ROOT:-/sn640/NotDec}
TOOL_SOURCE=${TOOL_SOURCE:-${SOURCE_ROOT}/binarysub-private}
NOTDEC_LLVM_LINK=llvm-22.1.0.nothreads.obj
NOTDEC_LLVM_TARGET=${NOTDEC_LLVM_TARGET:-${SOURCE_ROOT}/llvm-build-nothreads}
MICROSUB2_LLVM_DIR=${MICROSUB2_LLVM_DIR:-${SOURCE_ROOT}/llvm-22.1.0.obj}
JOBS=${JOBS:-$(nproc 2>/dev/null || echo 8)}

usage() {
  cat <<EOF
Usage: $0 <new-tree-path> <new-branch>

Create one worktree per repository under <new-tree-path>, all on the new
branch <new-branch> at the commit currently checked out in the main worktrees:

  <new-tree>/notdec                  worktree of ${SOURCE_ROOT}
  <new-tree>/notdec/external/<name>  worktree of each submodule repository
  <new-tree>/tool                    worktree of ${TOOL_SOURCE}
  <new-tree>/build.sh                full build (NotDec then microsub2)

<new-branch> must not exist in any repository.  <new-tree-path> must not exist
or must be an empty directory.  Uncommitted changes from the main worktrees are
not copied; only their current commits are used.

Environment overrides: SOURCE_ROOT, TOOL_SOURCE, NOTDEC_LLVM_TARGET,
MICROSUB2_LLVM_DIR, JOBS.
EOF
}

die() {
  printf 'error: %s\n' "$*" >&2
  exit 1
}

while (($#)); do
  case "$1" in
  -h | --help)
    usage
    exit 0
    ;;
  --)
    shift
    break
    ;;
  -*)
    die "unknown option: $1 (try --help)"
    ;;
  *)
    break
    ;;
  esac
done
(($# == 2)) || {
  usage >&2
  exit 2
}
NEW_TREE=$1
BRANCH=$2

# ---------------------------------------------------------------- validation

[[ ${NEW_TREE} == /* ]] || die "new tree path must be absolute: ${NEW_TREE}"
NEW_TREE=$(realpath -m -- "${NEW_TREE}")
SOURCE_ROOT=$(realpath -m -- "${SOURCE_ROOT}")
TOOL_SOURCE=$(realpath -m -- "${TOOL_SOURCE}")

git check-ref-format --branch "${BRANCH}" >/dev/null 2>&1 ||
  die "invalid branch name: ${BRANCH}"

[[ -e ${SOURCE_ROOT}/.git ]] || die "not a git worktree: ${SOURCE_ROOT}"
[[ -e ${TOOL_SOURCE}/.git ]] || die "not a git worktree: ${TOOL_SOURCE}"
[[ -d ${NOTDEC_LLVM_TARGET} ]] ||
  die "NotDec LLVM install not found: ${NOTDEC_LLVM_TARGET} (set NOTDEC_LLVM_TARGET)"
[[ -d ${MICROSUB2_LLVM_DIR} ]] ||
  die "microsub2 LLVM install not found: ${MICROSUB2_LLVM_DIR} (set MICROSUB2_LLVM_DIR)"

if [[ -e ${NEW_TREE} ]]; then
  [[ -d ${NEW_TREE} ]] || die "path exists and is not a directory: ${NEW_TREE}"
  [[ -z $(ls -A -- "${NEW_TREE}") ]] || die "directory is not empty: ${NEW_TREE}"
fi

# Keep the tree out of the source repositories: an untracked worktree inside
# NotDec or binarysub-private would pollute their `git status`.
case "${NEW_TREE}/" in
"${SOURCE_ROOT}"/*) die "refusing to create the tree inside ${SOURCE_ROOT}" ;;
"${TOOL_SOURCE}"/*) die "refusing to create the tree inside ${TOOL_SOURCE}" ;;
esac

# Top-level submodules, in .gitmodules order.  NotDec has no nested submodules;
# refuse to run if that ever changes, because the path mapping below is flat.
mapfile -t SUB_PATHS < <(
  git -C "${SOURCE_ROOT}" config --file .gitmodules --get-regexp '^submodule\..*\.path$' \
    | awk '{print $2}'
)
((${#SUB_PATHS[@]} > 0)) || die "no submodules found in ${SOURCE_ROOT}/.gitmodules"
mapfile -t RECURSIVE_PATHS < <(
  git -C "${SOURCE_ROOT}" submodule status --recursive | awk '{print $2}'
)
((${#RECURSIVE_PATHS[@]} == ${#SUB_PATHS[@]})) ||
  die "nested submodules are not supported by this script"

for path in "${SUB_PATHS[@]}"; do
  [[ -e ${SOURCE_ROOT}/${path}/.git ]] ||
    die "submodule is not initialized: ${path} (run: git -C ${SOURCE_ROOT} submodule update --init)"
done

REPOS=("${SOURCE_ROOT}")
for path in "${SUB_PATHS[@]}"; do
  REPOS+=("${SOURCE_ROOT}/${path}")
done
REPOS+=("${TOOL_SOURCE}")

conflicts=()
for repo in "${REPOS[@]}"; do
  if git -C "${repo}" show-ref --verify --quiet "refs/heads/${BRANCH}"; then
    conflicts+=("${repo}")
  fi
done
if ((${#conflicts[@]})); then
  printf 'error: branch %s already exists in:\n' "${BRANCH}" >&2
  printf '  %s\n' "${conflicts[@]}" >&2
  exit 1
fi

# ----------------------------------------------------------------- creation

CREATED_OK=0

cleanup() {
  local rc=$?
  if ((rc == 0)) || ((CREATED_OK == 1)); then
    return 0
  fi
  printf 'error: removing partially created worktrees\n' >&2
  local i repo wt
  # Reverse order: tool and submodules live inside the NotDec worktree.
  for ((i = ${#REPOS[@]} - 1; i >= 0; i--)); do
    repo=${REPOS[i]}
    while IFS= read -r wt; do
      [[ -n ${wt} && ${wt} == "${NEW_TREE}"/* ]] || continue
      git -C "${repo}" worktree remove --force -- "${wt}" 2>/dev/null || true
    done < <(git -C "${repo}" worktree list --porcelain 2>/dev/null | sed -n 's/^worktree //p')
    if git -C "${repo}" show-ref --verify --quiet "refs/heads/${BRANCH}"; then
      git -C "${repo}" branch -D -- "${BRANCH}" >/dev/null 2>&1 || true
    fi
  done
  rm -rf -- "${NEW_TREE}"
}
trap cleanup EXIT

mkdir -p -- "${NEW_TREE}"

printf 'Creating worktrees under %s on new branch %s\n' "${NEW_TREE}" "${BRANCH}"

base=$(git -C "${SOURCE_ROOT}" rev-parse --verify 'HEAD^{commit}')
git -C "${SOURCE_ROOT}" worktree add --no-track -b "${BRANCH}" \
  "${NEW_TREE}/notdec" "${base}"

mkdir -p -- "${NEW_TREE}/notdec/external"
for path in "${SUB_PATHS[@]}"; do
  dst="${NEW_TREE}/notdec/${path}"
  mkdir -p -- "$(dirname -- "${dst}")"
  if [[ -d ${dst} ]]; then
    # The superproject checkout may have left an empty placeholder directory.
    [[ -z $(ls -A -- "${dst}") ]] || die "refusing to replace non-empty directory: ${dst}"
    rmdir -- "${dst}"
  fi
  base=$(git -C "${SOURCE_ROOT}/${path}" rev-parse --verify 'HEAD^{commit}')
  git -C "${SOURCE_ROOT}/${path}" worktree add --no-track -b "${BRANCH}" "${dst}" "${base}"
done

base=$(git -C "${TOOL_SOURCE}" rev-parse --verify 'HEAD^{commit}')
git -C "${TOOL_SOURCE}" worktree add --no-track -b "${BRANCH}" "${NEW_TREE}/tool" "${base}"

# The no-RTTI LLVM install used by NotDec is gitignored and therefore absent
# from the new checkout; link it back to the main tree's build.
ln -s -- "${NOTDEC_LLVM_TARGET}" "${NEW_TREE}/notdec/${NOTDEC_LLVM_LINK}"

cat >"${NEW_TREE}/build.sh" <<EOF
#!/usr/bin/env bash
# Generated by ${BASH_SOURCE[0]} for branch ${BRANCH}.
set -euo pipefail

TREE='${NEW_TREE}'
SOURCE_ROOT='${SOURCE_ROOT}'
MICROSUB2_LLVM_DIR='${MICROSUB2_LLVM_DIR}'
JOBS=\${JOBS:-${JOBS}}

printf '==> configure NotDec\n'
cmake -S "\$TREE/notdec" -B "\$TREE/build-notdec" -G Ninja \\
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \\
  -DCMAKE_EXPORT_COMPILE_COMMANDS=TRUE \\
  -DLT_LLVM_INSTALL_DIR="\$TREE/notdec/${NOTDEC_LLVM_LINK}" \\
  -DBINARYSUB_ENABLE_BOUND_ORIGINS=ON \\
  -DNOTDEC_ENABLE_BINARYSUB_FIELD_ORIGINS=ON

printf '==> build NotDec (full, -j%s)\n' "\$JOBS"
cmake --build "\$TREE/build-notdec" -j"\$JOBS"

printf '==> build microsub2\n'
cd "\$TREE/tool/microsub2"
rm -f build/linux/x86_64/release/microsub2.exe
BINARYSUB_ENABLE_BOUND_ORIGINS=1 BINARYSUB_ENABLE_FIELD_ORIGINS=1 \\
NOTDEC_SOURCE_DIR="\$TREE/notdec" \\
NOTDEC_TYPERECOVERY_DIR="\$TREE/build-notdec" \\
MICROSUB2_NOTDEC_TR=1 \\
MICROSUB2_LLVM_DIR="\$MICROSUB2_LLVM_DIR" \\
xmake -P "\$TREE/tool/microsub2" -y microsub2.exe

printf '==> done: %s\n' "\$TREE/tool/microsub2/build/linux/x86_64/release/microsub2.exe"
EOF
chmod +x "${NEW_TREE}/build.sh"

# --------------------------------------------------------------- verification

TREE_INFO=${NEW_TREE}/tree-info.txt
{
  printf 'branch %s\n' "${BRANCH}"
  printf 'created %s\n' "$(date -Iseconds)"
} >"${TREE_INFO}"

verify_repo() {
  local repo=$1 wt=$2 label=$3
  local main_head new_head new_branch
  main_head=$(git -C "${repo}" rev-parse --verify 'HEAD^{commit}')
  new_head=$(git -C "${wt}" rev-parse --verify 'HEAD^{commit}')
  new_branch=$(git -C "${wt}" branch --show-current)
  [[ ${new_head} == "${main_head}" ]] ||
    die "${label}: HEAD ${new_head:0:12} does not match main worktree ${main_head:0:12}"
  [[ ${new_branch} == "${BRANCH}" ]] ||
    die "${label}: checked out '${new_branch}', expected '${BRANCH}'"
  printf '  %-28s %-12s %s\n' "${label}" "${new_branch}" "${new_head:0:12}"
  printf '%s %s\n' "${label}" "${new_head}" >>"${TREE_INFO}"
}

printf '\nChecking worktrees (new HEAD must equal the main worktree HEAD):\n'
printf '  %-28s %-12s %s\n' "repository" "branch" "commit"
verify_repo "${SOURCE_ROOT}" "${NEW_TREE}/notdec" "notdec"
for path in "${SUB_PATHS[@]}"; do
  verify_repo "${SOURCE_ROOT}/${path}" "${NEW_TREE}/notdec/${path}" "${path}"
done
verify_repo "${TOOL_SOURCE}" "${NEW_TREE}/tool" "tool"

CREATED_OK=1

cat <<EOF

Tree ready: ${NEW_TREE}
Build it:   ${NEW_TREE}/build.sh
Binary:     ${NEW_TREE}/tool/microsub2/build/linux/x86_64/release/microsub2.exe

Notes:
  - \`git -C ${NEW_TREE}/notdec status\` may show " M external/binarysub" when the
    library worktree is ahead of the superproject pointer; the main tree shows
    the same line for the same reason.
  - The tree shares .git with ${SOURCE_ROOT} and ${TOOL_SOURCE}; do not delete or
    move those, and remove the tree with \`git worktree remove\` when done.
EOF
