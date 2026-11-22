#!/usr/bin/env bash
# General-purpose Git/Git LFS publishing helper.
# Works in Git Bash on Windows, and in Bash on macOS/Linux.
#
# - Commits all non-ignored working-tree changes only after confirmation.
# - Migrates large files and common ML/scientific/Unreal binary formats
#   in unpublished commits to Git LFS.
# - Never force-pushes and never rewrites commits already present on a remote.
# - Keeps an optional local recovery branch before rewriting unpublished history.
# - Estimates Git pack size and splits a linear first-parent push into
#   multiple fast-forward pushes, each below GitHub's 2 GiB per-push limit.

set -Eeuo pipefail
export LC_ALL=C

# GitHub enforces a 2 GiB maximum for a single Git push. Keep margin.
SAFE_PUSH_LIMIT_BYTES="${SAFE_PUSH_LIMIT_BYTES:-1932735283}" # 1.8 GiB
LFS_THRESHOLD="${LFS_THRESHOLD:-50MB}"
DEFAULT_COMMIT_MESSAGE="${COMMIT_MESSAGE:-Update project}"
DEFAULT_REMOTE="${PUSH_REMOTE:-origin}"

# These file types are commonly large, binary, or expensive to diff/merge.
# They are migrated to LFS even when an individual file is smaller than
# LFS_THRESHOLD. Any other file larger than LFS_THRESHOLD is also migrated.
LFS_PATTERNS=(
  # Machine learning / AI models and checkpoints
  "*.onnx" "*.npz" "*.npy" "*.pt" "*.pth" "*.ckpt"
  "*.safetensors" "*.pb" "*.tflite" "*.h5" "*.hdf5"
  "*.keras" "*.gguf" "*.joblib" "*.pkl"
  # Scientific / tabular / numerical data
  "*.parquet" "*.feather" "*.arrow" "*.mat"
  # Unreal Engine assets
  "*.uasset" "*.umap" "*.ubulk" "*.uexp" "*.uptnl"
)
LFS_INCLUDE_PATTERNS="$(IFS=,; printf '%s' "${LFS_PATTERNS[*]}")"

fail() {
  printf '\nERROR: %s\n' "$*" >&2
  exit 1
}

require_command() {
  command -v "$1" >/dev/null 2>&1 || fail "Required command not found: $1"
}

for cmd in git awk wc date; do
  require_command "$cmd"
done

git lfs version >/dev/null 2>&1 || fail "Git LFS is not installed. Install Git LFS from https://git-lfs.com/ and run 'git lfs install'."
git rev-parse --is-inside-work-tree >/dev/null 2>&1 || fail "Run this script from inside a Git repository."

REPO_ROOT="$(git rev-parse --show-toplevel)"
cd "$REPO_ROOT"

BRANCH="$(git branch --show-current)"
[[ -n "$BRANCH" ]] || fail "Detached HEAD detected. Check out the branch you intend to publish."

# Resolve the push destination from the configured upstream, or use origin/<local branch>
# for a new branch / a repository without an upstream yet.
UPSTREAM="$(git rev-parse --abbrev-ref --symbolic-full-name '@{upstream}' 2>/dev/null || true)"
if [[ -n "$UPSTREAM" ]]; then
  REMOTE="$(git config --get "branch.${BRANCH}.remote" || true)"
  REMOTE_MERGE="$(git config --get "branch.${BRANCH}.merge" || true)"
  [[ -n "$REMOTE" && -n "$REMOTE_MERGE" ]] || fail "Could not resolve the configured upstream '$UPSTREAM'."
  REMOTE_BRANCH="${REMOTE_MERGE#refs/heads/}"
else
  REMOTE="$DEFAULT_REMOTE"
  REMOTE_BRANCH="${PUSH_BRANCH:-$BRANCH}"
fi

git remote get-url "$REMOTE" >/dev/null 2>&1 || fail "Remote '$REMOTE' does not exist. Check 'git remote -v' or set PUSH_REMOTE."
REMOTE_REF="refs/remotes/${REMOTE}/${REMOTE_BRANCH}"

# Refuse to operate while Git is in the middle of another history operation.
GIT_DIR="$(git rev-parse --git-dir)"
for state_file in MERGE_HEAD CHERRY_PICK_HEAD REVERT_HEAD; do
  [[ ! -f "${GIT_DIR}/${state_file}" ]] || fail "A Git operation is in progress (${state_file}). Finish or abort it first."
done
for state_dir in rebase-merge rebase-apply; do
  [[ ! -d "${GIT_DIR}/${state_dir}" ]] || fail "A rebase is in progress. Finish or abort it first."
done
[[ -z "$(git diff --name-only --diff-filter=U)" ]] || fail "Unresolved merge conflicts are present. Resolve them first."

printf '\nRepository: %s\nBranch: %s\nRemote: %s\nRemote branch: %s\n' \
  "$REPO_ROOT" "$BRANCH" "$REMOTE" "$REMOTE_BRANCH"
printf 'Large-file migration threshold: %s\n' "$LFS_THRESHOLD"
printf 'Per-push Git pack safety target: %.2f GiB\n' \
  "$(awk -v n="$SAFE_PUSH_LIMIT_BYTES" 'BEGIN { print n/(1024*1024*1024) }')"
printf '\nGitHub Pro notes:\n'
printf '  - Git push limit remains 2 GiB per individual push.\n'
printf '  - Git LFS maximum individual file size on Pro is 2 GB.\n'
printf '  - Pro currently includes 10 GiB LFS storage and 10 GiB monthly download bandwidth.\n'
printf '  - This script cannot read your account-level LFS quota or billing budget.\n'

printf '\nInitializing Git LFS hooks...\n'
git lfs install

printf '\nFetching latest remote references...\n'
git fetch --prune "$REMOTE"

HAS_REMOTE_BRANCH=0
REMOTE_BASE_SHA=""
if git show-ref --verify --quiet "$REMOTE_REF"; then
  HAS_REMOTE_BRANCH=1
  REMOTE_BASE_SHA="$(git rev-parse "$REMOTE_REF")"
else
  if [[ -n "$UPSTREAM" ]]; then
    fail "The configured upstream '$UPSTREAM' was not found after fetching. Do not create a replacement branch blindly; inspect the remote first."
  fi
  printf "Remote branch '%s/%s' does not exist yet; this will be its first push.\n" "$REMOTE" "$REMOTE_BRANCH"
fi

if (( HAS_REMOTE_BRANCH )); then
  if ! git merge-base --is-ancestor "$REMOTE_BASE_SHA" HEAD; then
    if git merge-base --is-ancestor HEAD "$REMOTE_BASE_SHA"; then
      fail "Your local branch is behind '$REMOTE/$REMOTE_BRANCH'. Integrate remote changes before publishing."
    fi
    fail "Your local branch and '$REMOTE/$REMOTE_BRANCH' have diverged. Resolve the divergence first; this script will not force-push."
  fi
fi

printf '\nCurrent working-tree status:\n'
git status --short
if [[ -n "$(git status --porcelain)" ]]; then
  printf '\nThe script can stage and commit ALL listed, non-ignored changes.\n'
  printf 'Review the list carefully: source, models, datasets, assets and configuration files are included; ignored files are not.\n'
  read -r -p "Stage and commit all these changes? [y/N] " COMMIT_ANSWER
  case "$COMMIT_ANSWER" in
    y|Y|yes|YES)
      git add -A
      if ! git diff --cached --quiet; then
        # GitHub Pro's documented Git LFS per-file limit is 2 GB. Catch new or
        # modified staged files that are already too large before committing.
        while IFS= read -r -d '' STAGED_FILE; do
          [[ -f "$STAGED_FILE" ]] || continue
          FILE_BYTES="$(stat -c '%s' -- "$STAGED_FILE" 2>/dev/null || stat -f '%z' "$STAGED_FILE" 2>/dev/null || wc -c < "$STAGED_FILE")"
          FILE_BYTES="${FILE_BYTES//[[:space:]]/}"
          if [[ "$FILE_BYTES" =~ ^[0-9]+$ ]] && (( FILE_BYTES >= 2000000000 )); then
            fail "'$STAGED_FILE' is at least 2,000,000,000 bytes. GitHub Pro's Git LFS per-file limit is 2 GB. Split/shard the file or store it outside GitHub before committing."
          fi
        done < <(git diff --cached --name-only --diff-filter=ACMRT -z)
        read -r -p "Commit message [${DEFAULT_COMMIT_MESSAGE}]: " ENTERED_MESSAGE
        COMMIT_MESSAGE="${ENTERED_MESSAGE:-$DEFAULT_COMMIT_MESSAGE}"
        git commit -m "$COMMIT_MESSAGE"
      else
        printf 'No staged changes to commit.\n'
      fi
      ;;
    *)
      fail "Cancelled. Nothing was committed or pushed. Review the working tree and rerun the script."
      ;;
  esac
fi

# Re-check the remote relationship after the commit operation.
git fetch "$REMOTE"
if (( HAS_REMOTE_BRANCH )); then
  REMOTE_BASE_SHA="$(git rev-parse "$REMOTE_REF")"
  git merge-base --is-ancestor "$REMOTE_BASE_SHA" HEAD \
    || fail "The remote branch changed or is no longer an ancestor of local HEAD. No history rewrite or push was attempted."
fi

# Determine whether there is anything to publish. For a brand-new remote branch,
# all current commits are candidates (some may already exist on another remote branch).
if (( HAS_REMOTE_BRANCH )); then
  AHEAD_COUNT="$(git rev-list --count "${REMOTE_REF}..HEAD")"
else
  AHEAD_COUNT="$(git rev-list --count HEAD)"
fi
if [[ "$AHEAD_COUNT" -eq 0 ]] && (( HAS_REMOTE_BRANCH )); then
  printf '\nThe branch is already up to date with %s/%s. Nothing to push.\n' "$REMOTE" "$REMOTE_BRANCH"
  exit 0
fi

# The migration rewrites unpublished commit IDs. Keep a local recovery branch first.
STAMP="$(date +%Y%m%d-%H%M%S)"
BACKUP_BRANCH="backup/${BRANCH//\//-}-before-lfs-${STAMP}"
while git show-ref --verify --quiet "refs/heads/${BACKUP_BRANCH}"; do
  STAMP="${STAMP}-1"
  BACKUP_BRANCH="backup/${BRANCH//\//-}-before-lfs-${STAMP}"
done
git branch "$BACKUP_BRANCH" HEAD
printf '\nCreated local recovery branch: %s\n' "$BACKUP_BRANCH"
printf 'This backup branch is local only and will not be pushed.\n'

printf '\nUnpublished large-file summary before migration:\n'
git lfs migrate info --above="$LFS_THRESHOLD" --top=25

printf '\nMigrating common ML/scientific/Unreal formats to Git LFS...\n'
git lfs migrate import --include="$LFS_INCLUDE_PATTERNS" --verbose

printf '\nMigrating all other files larger than %s to Git LFS...\n' "$LFS_THRESHOLD"
git lfs migrate import --above="$LFS_THRESHOLD" --verbose

printf '\nApplying existing .gitattributes rules to any older unpublished blobs...\n'
git lfs migrate import --fixup --verbose

# Migrations should leave a clean checkout. Stop if they did not.
[[ -z "$(git status --porcelain)" ]] || fail "The working tree is not clean after Git LFS migration. Inspect 'git status' before continuing. Recovery branch: $BACKUP_BRANCH"

format_gib() {
  awk -v bytes="$1" 'BEGIN { printf "%.2f GiB", bytes / (1024 * 1024 * 1024) }'
}

# Estimate the Git pack for a candidate commit, excluding objects already reachable
# from the selected GitHub remote's advertised remote-tracking refs and an optional
# earlier checkpoint in the same planned multi-push sequence.
estimate_pack_bytes() {
  local candidate="$1"
  local extra_base="${2:-}"
  local raw
  raw="$({
      printf '%s\n' "$candidate"
      git for-each-ref --format='^%(refname)' "refs/remotes/${REMOTE}/"
      if [[ -n "$extra_base" ]]; then
        printf '^%s\n' "$extra_base"
      fi
    } | git pack-objects --stdout --revs --thin | wc -c)" \
    || fail "Could not estimate the outgoing Git pack."
  raw="${raw//[[:space:]]/}"
  [[ "$raw" =~ ^[0-9]+$ ]] || fail "Could not parse the outgoing Git pack size: '$raw'."
  printf '%s' "$raw"
}

# Build a sequence of fast-forward checkpoints if the complete push is too large.
# A checkpoint is safe only when it lies on the branch's first-parent chain.
PLAN_COMMITS=()
PLAN_SIZES=()
WHOLE_PACK_BYTES="$(estimate_pack_bytes HEAD "")"
printf '\nEstimated Git pack for the complete push: %s\n' "$(format_gib "$WHOLE_PACK_BYTES")"

if (( WHOLE_PACK_BYTES <= SAFE_PUSH_LIMIT_BYTES )); then
  PLAN_COMMITS=("$(git rev-parse HEAD)")
  PLAN_SIZES=("$WHOLE_PACK_BYTES")
else
  printf '\nThe complete Git pack exceeds the safety target. Building smaller fast-forward pushes...\n'

  if (( HAS_REMOTE_BRANCH )); then
    # Safe incremental checkpoints require the remote tip to be on the local first-parent chain.
    if ! git rev-list --first-parent HEAD | grep -Fqx "$REMOTE_BASE_SHA"; then
      fail "The remote tip is not on the local first-parent chain. Automatic chunked pushing would not be guaranteed fast-forward. Reduce LFS_THRESHOLD or split/rebase the unpublished work manually."
    fi
    mapfile -t CANDIDATE_COMMITS < <(git rev-list --first-parent --reverse "${REMOTE_REF}..HEAD")
    PLAN_BASE="$REMOTE_BASE_SHA"
  else
    mapfile -t CANDIDATE_COMMITS < <(git rev-list --first-parent --reverse HEAD)
    PLAN_BASE=""
  fi

  LAST_FIT=""
  LAST_FIT_SIZE=""

  for CANDIDATE in "${CANDIDATE_COMMITS[@]}"; do
    [[ -n "$CANDIDATE" ]] || continue
    CANDIDATE_SIZE="$(estimate_pack_bytes "$CANDIDATE" "$PLAN_BASE")"

    if (( CANDIDATE_SIZE <= SAFE_PUSH_LIMIT_BYTES )); then
      LAST_FIT="$CANDIDATE"
      LAST_FIT_SIZE="$CANDIDATE_SIZE"
      continue
    fi

    # Push the last checkpoint that fits, then measure this candidate incrementally.
    if [[ -z "$LAST_FIT" ]]; then
      fail "A single first-parent commit requires $(format_gib "$CANDIDATE_SIZE") of Git objects, above the safety target. Lower LFS_THRESHOLD (for example, LFS_THRESHOLD=10MB bash ./push-to-github.sh), or split that commit manually. No push was attempted."
    fi

    PLAN_COMMITS+=("$LAST_FIT")
    PLAN_SIZES+=("$LAST_FIT_SIZE")
    PLAN_BASE="$LAST_FIT"
    LAST_FIT=""
    LAST_FIT_SIZE=""

    CANDIDATE_SIZE="$(estimate_pack_bytes "$CANDIDATE" "$PLAN_BASE")"
    if (( CANDIDATE_SIZE > SAFE_PUSH_LIMIT_BYTES )); then
      fail "The incremental objects introduced by commit ${CANDIDATE:0:12} require $(format_gib "$CANDIDATE_SIZE"), above the safety target. Lower LFS_THRESHOLD or split this commit manually. No further push was attempted."
    fi
    LAST_FIT="$CANDIDATE"
    LAST_FIT_SIZE="$CANDIDATE_SIZE"
  done

  if [[ -n "$LAST_FIT" ]]; then
    PLAN_COMMITS+=("$LAST_FIT")
    PLAN_SIZES+=("$LAST_FIT_SIZE")
  fi

  [[ "${#PLAN_COMMITS[@]}" -gt 0 ]] || fail "Could not construct a safe push plan. No push was attempted."
fi

printf '\nPlanned GitHub push sequence:\n'
for i in "${!PLAN_COMMITS[@]}"; do
  printf '  %d. %s  approximately %s\n' "$((i + 1))" "${PLAN_COMMITS[$i]:0:12}" "$(format_gib "${PLAN_SIZES[$i]}")"
done
printf '\nAll individual Git pushes are estimated below the 1.8 GiB safety target.\n'
printf 'LFS objects are uploaded separately from the ordinary Git pack.\n'

read -r -p "Upload LFS objects and push ${#PLAN_COMMITS[@]} fast-forward step(s) to ${REMOTE}/${REMOTE_BRANCH}? [y/N] " PUSH_ANSWER
case "$PUSH_ANSWER" in
  y|Y|yes|YES) ;;
  *) fail "Cancelled before upload. The migrated commits and recovery branch remain local." ;;
esac

printf '\nUploading Git LFS objects referenced by %s...\n' "$BRANCH"
git lfs push "$REMOTE" "$BRANCH"

printf '\nPushing Git branch checkpoint(s)...\n'
for i in "${!PLAN_COMMITS[@]}"; do
  CHECKPOINT="${PLAN_COMMITS[$i]}"
  if (( i == 0 )); then
    git push -u "$REMOTE" "${CHECKPOINT}:refs/heads/${REMOTE_BRANCH}"
  else
    git push "$REMOTE" "${CHECKPOINT}:refs/heads/${REMOTE_BRANCH}"
  fi
  # Keep the local remote-tracking ref current so the next checkpoint estimate
  # and any retry start from the last confirmed successful push.
  git update-ref "$REMOTE_REF" "$CHECKPOINT"
done

printf '\nRefreshing remote references...\n'
git fetch --prune "$REMOTE"
printf '\nPush completed. Final status:\n'
git status -sb
printf '\nLocal recovery branch retained: %s\n' "$BACKUP_BRANCH"
printf 'Keep it until you have verified the GitHub repository and a fresh clone.\n'
printf '\nReminder: review GitHub Billing > Metered usage for Git LFS storage and bandwidth.\n'
