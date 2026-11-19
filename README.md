# TasksAIV26_27

Repo of the tasks given by AIV.

## Pushing the Unreal Engine project to GitHub

This repository uses Git and Git LFS to version the Unreal Engine project. Unreal assets such as `.uasset` and `.umap` files can be large, so large files should be stored with Git LFS rather than as ordinary Git blobs.

GitHub limits a single Git push to 2 GiB. This repository includes a helper script, `push-unreal-to-github.sh`, which checks the outgoing Git pack and migrates sufficiently large files in unpublished commits to Git LFS before pushing. The script aims to keep the ordinary Git pack below **1.8 GiB**, leaving some headroom below GitHub's limit.

> **Important:** The script estimates the Git pack size; it cannot guarantee that every push will succeed. Git LFS storage and bandwidth limits are separate from Git's push-size limit.

### 1. Requirements

Before using the script, install and configure:

- **Git for Windows**, including **Git Bash**.
- **Git LFS**. Install it using the official instructions: <https://git-lfs.com/>.
- Access to this GitHub repository and permission to push to the target branch.

Check that Git and Git LFS are available from Git Bash:

```bash
git --version
git lfs version
```

Initialize Git LFS for your account (normally this is a one-time setup on each computer):

```bash
git lfs install
```

### 2. Prepare the repository before running the script

1. **Open Git Bash in the repository root** — the directory containing `AIV_UE5Intro.uproject` and `.git`.
2. **Make sure the correct branch is checked out.** For this repository, the branch is normally `main`:

   ```bash
   git switch main
   git status -sb
   git branch -vv
   ```

3. **Check the remote and upstream branch.** The script expects the current local branch to track a branch that already exists on the remote (for example, `main` tracking `origin/main`):

   ```bash
   git remote -v
   git fetch origin
   ```

   In `git branch -vv`, look for `[origin/main]` next to `main`. If the remote branch exists but the local branch has no upstream configured, set it with:

   ```bash
   git branch --set-upstream-to=origin/main main
   ```

   Do not run that command if `origin/main` does not exist. The helper script is intended for an existing remote-tracking branch, not the first-ever push to an empty remote repository.

4. **Review what Git sees before allowing the script to commit.**

   ```bash
   git status --short
   git diff
   git diff --cached
   ```

   The script offers to run `git add -A` and commit **all non-ignored changes**. That includes modified, deleted, and untracked files. Before answering `y`, make sure the listed changes are supposed to be part of the repository. Move private files out of the repository or add appropriate ignore rules for local-only files, credentials, generated data, and temporary files.

5. **Check `.gitignore`.** Keep generated Unreal folders out of Git, such as `Binaries/`, `Intermediate/`, `Saved/`, and `DerivedDataCache/`. Do not ignore the entire `Content/` directory or all `.uasset`/`.umap` files: those are often the actual project assets that need to be versioned. Use Git LFS for large assets instead.

6. **Make a backup if the work is important.** The script creates a local backup branch before rewriting unpublished commits, but keeping a separate copy of important work is still recommended.

### 3. Put the helper script in the repository

Copy `push-unreal-to-github.sh` into the root of the repository, next to the `.uproject` file:

```text
TasksAIV26_27/
├── .git/
├── .gitignore
├── .gitattributes
├── AIV_UE5Intro.uproject
├── Content/
├── Plugins/
├── Source/                 (if the project contains C++ code)
└── push-unreal-to-github.sh
```

If the script was downloaded to another folder, copy it into this directory first.

### 4. Run the script

In Git Bash, change to the repository directory. For example:

```bash
cd "/d/Unreal Projects/TasksAIV26_27"
```

Run:

```bash
bash ./push-unreal-to-github.sh
```

Using `bash` explicitly means you do not need to set the executable bit on Windows.

The script will:

1. Check that Git LFS is installed, that you are on a branch with an upstream, and that no merge/rebase or unresolved conflict is in progress.
2. Fetch the latest remote references and stop if the remote branch has commits that are not in your local branch, or if the branches have diverged. Resolve/synchronize the branch manually before trying again.
3. Show the working-tree changes and ask whether it may stage and commit all of them. It asks for a commit message; pressing Enter uses the default `Update Unreal project`.
4. Create a local recovery branch named similar to `backup/main-before-lfs-YYYYMMDD-HHMMSS`.
5. Inspect and migrate files in unpublished commits to Git LFS. It first migrates files larger than 1 MB. If the outgoing Git pack is still above the safety target, it progressively tries 100 KB and then 10 KB.
6. Estimate the outgoing ordinary Git pack. If it cannot get the estimate below 1.8 GiB, it stops without pushing.
7. Ask for confirmation, upload the required Git LFS objects, and then push the current branch to its configured remote branch.

The migration is based on **file size, not file type**. If the script has to lower the threshold, more files can be moved into Git LFS. Review the migration output and the updated `.gitattributes` file.

### 5. After the push

When the script reports that the push completed:

1. Check the repository page on GitHub and confirm that the latest commit is present.
2. Keep the printed `backup/...` branch until you have verified the project on GitHub and, ideally, tested a fresh clone with Git LFS enabled.
3. Once everything is verified, you may remove the local backup branch using the exact name printed by the script:

   ```bash
   git branch -d "backup/main-before-lfs-YYYYMMDD-HHMMSS"
   ```

   Replace the example name with the actual backup branch name. The backup branch is local; the script does not push it.

For a fresh clone, install Git LFS first and then download the LFS content:

```bash
git lfs install
git lfs pull
```

### 6. If the script stops or the push fails

- **No upstream branch:** Confirm that the corresponding branch exists on GitHub, fetch it, and configure the upstream as described in section 2.
- **Remote branch has new commits or has diverged:** Do not force-push. Fetch and integrate the remote changes (for example, using a reviewed `git pull --rebase`), resolve any conflicts, and run the script again.
- **The estimated Git pack remains above 1.8 GiB:** The script stops rather than trying a push that may exceed GitHub's limit. Keep the local backup branch. Review the largest files and consider splitting the unpublished work into smaller commits or deciding which additional file types should be stored in LFS.
- **Git LFS upload fails:** Check your network connection, GitHub authentication, and the repository's Git LFS storage/bandwidth quota. Re-running the script after fixing the issue is generally safe; the Git branch is only pushed after the LFS upload step succeeds.
- **The normal Git push fails after LFS upload:** The LFS objects may already have uploaded, but the branch may not have updated. Check `git status -sb` and the GitHub branch before retrying. Do not force-push unless the repository maintainers have explicitly agreed to a history rewrite.

The script does not delete project assets or remove the backup branch automatically. It also does not bypass GitHub's Git LFS quota or change the remote repository's history.

### References

- [GitHub: Troubleshooting the 2 GiB push limit](https://docs.github.com/en/get-started/using-git/troubleshooting-the-2-gb-push-limit)
- [GitHub: Configuring Git Large File Storage](https://docs.github.com/en/repositories/working-with-files/managing-large-files/configuring-git-large-file-storage)
- [Git LFS: `git lfs migrate` documentation](https://github.com/git-lfs/git-lfs/blob/main/docs/man/git-lfs-migrate.adoc)
- [Git LFS official website](https://git-lfs.com/)
