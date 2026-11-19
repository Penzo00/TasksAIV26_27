#!/usr/bin/env bash
# Safely commit local changes, migrate oversized objects in unpushed commits
# to Git LFS, estimate the outgoing Git pack, and push the current branch.
# Intended for Git Bash on Windows, macOS, and Linux.

set -Eeuo pipefail
export LC_ALL=C

# GitHub documents a 2 GiB maximum for one Git push. Stay below it with margin.
SAFE_PUSH_LIMIT_BYTES="${SAFE_PUSH_LIMIT_BYTES:-1932735283}" # 1.8 GiB
COMMIT_MESSAGE="${COMMIT_MESSAGE:-Update Unreal project}"

fail() {
  printf '\nERROR: %s\n' "$*" >&2
  exit 1
}

for cmd in git wc awk grep date; do
  command -v "$cmd" >/dev/null 2>&1 || fail "Required command not found: $cmd"
done
git lfs version >/dev/null 2>&1 || fail "Git LFS is not installed. Install Git LFS, then run: git lfs install"

git rev-parse --is-inside-work-tree >/dev/null 2>&1 || fail "Run this script from inside your Git repository."
REPO_ROOT="$(git rev-parse --show-toplevel)"
cd "$REPO_ROOT"

BRANCH="$(git branch --show-current)"
[[ -n "$BRANCH" ]] || fail "You are in a detached HEAD state. Check out the branch you want to push first."

UPSTREAM="$(git rev-parse --abbrev-ref --symbolic-full-name '@{upstream}' 2>/dev/null)" \
  || fail "The current branch has no upstream. Set one first, e.g. git push -u origin $BRANCH."
REMOTE="${UPSTREAM%%/*}"
REMOTE_BRANCH="${UPSTREAM#*/}"
REMOTE_REF="refs/remotes/$UPSTREAM"

if ! git remote get-url "$REMOTE" >/dev/null 2>&1; then
  fail "The configured upstream remote '$REMOTE' does not exist."
fi

if git rev-parse -q --verify MERGE_HEAD >/dev/null 2>&1 \
  || [[ -d "$(git rev-parse --git-path rebase-merge)" ]] \
  || [[ -d "$(git rev-parse --git-path rebase-apply)" ]]; then
  fail "A merge or rebase is in progress. Finish or abort it before running this script."
fi

UNMERGED_FILES="$(git diff --name-only --diff-filter=U)"
if [[ -n "$UNMERGED_FILES" ]]; then
  fail "The index contains merge conflicts. Resolve them before running this script."
fi

printf '\nRepository: %s\nBranch: %s\nRemote: %s\n' "$REPO_ROOT" "$BRANCH" "$REMOTE"

# Check that Git LFS is installed and its hooks are configured.
git lfs version >/dev/null 2>&1 || fail "Git LFS is not installed. Install Git LFS, then run: git lfs install"
git lfs install

printf '\nFetching latest remote references...\n'
git fetch --prune "$REMOTE"

if ! git show-ref --verify --quiet "$REMOTE_REF"; then
  fail "Cannot find remote-tracking branch $UPSTREAM. Check the branch/upstream configuration."
fi

# Rewriting is safe here only when the remote branch is already an ancestor
# of the current local branch. Stop if the remote has new commits or diverged.
if ! git merge-base --is-ancestor "$REMOTE_REF" HEAD; then
  fail "Local '$BRANCH' is behind or has diverged from '$UPSTREAM'. Integrate the remote changes first; this script will not rewrite remote history."
fi

printf '\nCurrent working-tree status:\n'
git status --short

if [[ -n "$(git status --porcelain)" ]]; then
  printf '\nThis script can stage and commit ALL listed, non-ignored changes.\n'
  read -r -p "Continue and commit all these changes? [y/N] " ANSWER
  case "$ANSWER" in
    y|Y|yes|YES)
      git add -A
      if ! git diff --cached --quiet; then
        read -r -p "Commit message [${COMMIT_MESSAGE}]: " ENTERED_MESSAGE
        if [[ -n "$ENTERED_MESSAGE" ]]; then
          COMMIT_MESSAGE="$ENTERED_MESSAGE"
        fi
        git commit -m "$COMMIT_MESSAGE"
      else
        printf 'No staged changes to commit.\n'
      fi
      ;;
    *)
      fail "Cancelled. No files were staged or committed."
      ;;
  esac
fi

# Re-check the ancestry after creating a potential new local commit.
if ! git merge-base --is-ancestor "$REMOTE_REF" HEAD; then
  fail "The remote branch is no longer an ancestor of the local branch. No push was attempted."
fi

AHEAD_COUNT="$(git rev-list --count "$REMOTE_REF..HEAD")"
if [[ "$AHEAD_COUNT" -eq 0 ]]; then
  printf '\nThere are no local commits to push. The branch is already up to date.\n'
  exit 0
fi

# Keep a local recovery pointer before rewriting any unpublished commits.
STAMP="$(date +%Y%m%d-%H%M%S)"
BACKUP_BRANCH="backup/${BRANCH//\//-}-before-lfs-${STAMP}"
git branch "$BACKUP_BRANCH" HEAD
printf '\nCreated local recovery branch: %s\n' "$BACKUP_BRANCH"
printf 'This backup branch is local only and will not be pushed.\n'

# Estimate the outgoing Git pack. LFS file contents are not part of this pack;
# Git stores small LFS pointer files instead.
estimate_pack_bytes() {
  local value
  value="$(printf '%s\n^%s\n' HEAD "$REMOTE_REF" | git pack-objects --stdout --revs --thin | wc -c)"
  value="${value//[[:space:]]/}"
  [[ "$value" =~ ^[0-9]+$ ]] || fail "Could not estimate the outgoing Git pack size."
  printf '%s' "$value"
}

format_gib() {
  awk -v bytes="$1" 'BEGIN { printf "%.2f GiB", bytes / (1024 * 1024 * 1024) }'
}

printf '\nLarge-file summary before migration (unpushed commits only):\n'
git lfs migrate info --above=1MB --top=20

# Start by migrating files >1 MB. If the outgoing pack is still too large,
# lower the threshold cautiously. The process stops if it still cannot get
# below the safety limit; it will never force-push.
MIGRATION_THRESHOLDS=("1MB" "100KB" "10KB")
PACK_BYTES=""

for THRESHOLD in "${MIGRATION_THRESHOLDS[@]}"; do
  printf '\nMigrating files larger than %s in unpublished commits...\n' "$THRESHOLD"
  git lfs migrate import --above="$THRESHOLD" --verbose

  PACK_BYTES="$(estimate_pack_bytes)"
  printf 'Estimated outgoing Git pack: %s\n' "$(format_gib "$PACK_BYTES")"

  if (( PACK_BYTES <= SAFE_PUSH_LIMIT_BYTES )); then
    break
  fi
  printf 'Still above the safety target; the next pass will migrate smaller files.\n'
done

if (( PACK_BYTES > SAFE_PUSH_LIMIT_BYTES )); then
  fail "The estimated Git pack is still $(format_gib "$PACK_BYTES"), above the 1.8 GiB safety target. No push was attempted. Keep the backup branch and split the unpublished commit(s) or lower the LFS threshold after reviewing LFS storage usage."
fi

printf '\nMigration summary after conversion:\n'
git lfs migrate info --above=1MB --top=20
printf '\nLFS files referenced by this repository:\n'
git lfs ls-files | tail -n 30

printf '\nReady to upload. Estimated Git pack: %s (safety target: 1.8 GiB).\n' "$(format_gib "$PACK_BYTES")"
read -r -p "Upload LFS objects and push '$BRANCH' to '$REMOTE'? [y/N] " PUSH_ANSWER
case "$PUSH_ANSWER" in
  y|Y|yes|YES)
    ;;
  *)
    fail "Cancelled before upload. Your migrated commits and backup branch remain local."
    ;;
esac

printf '\nUploading Git LFS objects...\n'
git lfs push "$REMOTE" "$BRANCH"

printf '\nPushing Git branch...\n'
git push "$REMOTE" "HEAD:refs/heads/$REMOTE_BRANCH"

printf '\nPush completed. Final status:\n'
git status -sb
printf '\nLocal recovery branch retained: %s\n' "$BACKUP_BRANCH"
printf 'You can delete it later after verifying the GitHub repository and a fresh clone.\n'
