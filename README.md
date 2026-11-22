# TasksAIV26_27

Repository for the tasks given by AIV.

## General-purpose GitHub publishing (Git + Git LFS)

Use [`push-to-github.sh`](./push-to-github.sh) to publish this repository from Git Bash. It is designed for mixed-language projects, including Python, C#, C/C++, Unreal Engine, machine-learning models, and scientific datasets.

The script keeps source code and ordinary text files in Git. It migrates common large/binary formats and any other files above a configurable size into Git LFS. Supported examples include:

- Machine-learning models and checkpoints: `.onnx`, `.pt`, `.pth`, `.ckpt`, `.safetensors`, `.pb`, `.tflite`, `.h5`, `.hdf5`, `.keras`, `.gguf`.
- NumPy and scientific data: `.npz`, `.npy`, `.mat`.
- Tabular / columnar data: `.parquet`, `.feather`, `.arrow`.
- Unreal Engine assets: `.uasset`, `.umap`, `.ubulk`, `.uexp`, `.uptnl`.
- Any other file larger than the default `50MB` threshold, regardless of extension.

The extension-based formats above are migrated to LFS even when smaller than 50 MB. Other formats are selected by file size. When a matching file type is migrated, Git LFS records the corresponding tracking rule in `.gitattributes`; future additions of that type will then be filtered through LFS automatically.

### Important GitHub limits (GitHub Pro)

GitHub Pro does **not** remove the ordinary Git push-size limit:

- A single Git push is limited to **2 GiB**. The script targets **1.8 GiB** per push to leave a margin.
- A normal Git file is rejected above **100 MiB**. The script migrates files above 50 MB to LFS, before attempting the push.
- Git LFS individual-file limit on GitHub Pro is **2 GB**. A single ONNX, NPZ, or other file above that limit cannot be uploaded to LFS; split it into shards or store it in an artifact/object-storage service instead.
- GitHub Pro currently includes **10 GiB of LFS storage** and **10 GiB of monthly LFS download bandwidth**. This is the same included LFS allowance as GitHub Free. Revisions of an LFS file each consume storage; downloads consume bandwidth. The script cannot query the account's remaining quota or billing budget.

Check your current usage in GitHub **Settings → Billing and licensing → Metered usage** (menu wording may vary).

### 1. Prerequisites

Install:

1. Git for Windows (including Git Bash), or Git on macOS/Linux.
2. Git LFS: <https://git-lfs.com/>.
3. Authentication and write access to the GitHub repository.

Verify the tools from Git Bash:

```bash
git --version
git lfs version
```

Initialize Git LFS once per computer/user account:

```bash
git lfs install
```

### 2. Prepare the repository

1. Put `push-to-github.sh` in the repository root, next to this README.
2. Open Git Bash in the repository root.
3. Check the current branch and remote:

   ```bash
   git status -sb
   git branch -vv
   git remote -v
   git fetch --all --prune
   ```

4. Make sure the current branch tracks the branch you intend to update. If it already tracks (for example) `origin/main`, the script uses that upstream. If there is no upstream, it defaults to `origin/<current-branch>` and can create that remote branch on the first push.
5. Review `.gitignore`. It should exclude local caches, generated build folders, virtual environments, temporary files, and credentials. Do **not** ignore useful source, model, dataset, configuration, or project assets just because they are large. For example:

   ```gitignore
   # Python local environment/cache
   .venv/
   __pycache__/
   *.py[cod]
   .ipynb_checkpoints/

   # .NET / Visual Studio generated output
   bin/
   obj/
   .vs/

   # C/C++ generated build output (adapt to your project)
   build/
   cmake-build-*/

   # General local noise
   .DS_Store
   *.log
   ```

   Add or remove ignore rules according to what must be shared. For example, do not ignore `models/`, `data/`, `Content/`, `.onnx`, or `.npz` if those files are project deliverables you want in version control.

6. Inspect the changes before letting the script commit them:

   ```bash
   git status --short
   git diff
   git diff --cached
   ```

The script asks before staging and committing **all non-ignored changes**. This includes untracked files, modifications, and deletions. Review the list carefully; never include passwords, API keys, private datasets, or files you do not have rights to redistribute.

### 3. Run the publishing script

From the repository root in Git Bash:

```bash
bash ./push-to-github.sh
```

The script will:

1. Check the repository, branch, remote, Git LFS installation, and any in-progress merge/rebase.
2. Fetch remote references and stop if the remote branch has diverged or contains commits not present locally. It never force-pushes.
3. Ask whether to stage and commit all non-ignored working-tree changes.
4. Create a local recovery branch before rewriting any unpublished commit history.
5. Migrate the listed ML/scientific/Unreal file formats to LFS, regardless of size.
6. Migrate all other files larger than `50MB` to LFS.
7. Apply existing `.gitattributes` LFS rules to unpublished commits where a file was committed as a normal Git blob by mistake.
8. Estimate the outgoing ordinary Git pack. If the complete push is too large, it attempts to divide a linear first-parent history into multiple fast-forward pushes, each below 1.8 GiB.
9. Upload LFS objects, then push the Git branch. It asks for confirmation before uploading.

Git LFS migration rewrites commit IDs in the unpublished portion of the current branch. The script does not use `--everything`, does not rewrite commits already present on a remote, and keeps the pre-migration branch locally as a recovery point.

### 4. Configure the LFS size threshold

The default threshold for file types not listed above is 50 MB. You can use a lower threshold when a push still contains too much ordinary Git data:

```bash
LFS_THRESHOLD=10MB bash ./push-to-github.sh
```

For example, set `LFS_THRESHOLD=10MB` to migrate any other file larger than 10 MB. This can increase LFS storage usage, so do not lower it unnecessarily.

You can also set a custom commit message:

```bash
COMMIT_MESSAGE="Add inference model and dataset" bash ./push-to-github.sh
```

### 5. What the script cannot do

- It cannot increase GitHub's 2 GiB per-push limit or Git LFS quota.
- It cannot upload a single Git LFS file above the GitHub Pro 2 GB per-file limit.
- It does not automatically split one enormous commit if that single commit alone still exceeds the safe push size. It stops before pushing and tells you to lower `LFS_THRESHOLD` or split the commit manually.
- Automatic multi-push batching is only used when checkpoints can be pushed as a fast-forward along the local first-parent chain. If the branch has a more complex merge shape, the script stops rather than risk a non-fast-forward update.
- It cannot determine whether a file is semantically useful. `.gitignore` defines what is excluded; the script asks before adding all remaining changes.
- It cannot read your account's current Git LFS storage/bandwidth usage. Check GitHub billing settings regularly.

For very large training datasets or model archives that are frequently replaced, consider dedicated artifact/object storage rather than keeping every version in Git LFS.

### 6. After a successful push

Check the GitHub repository and keep the local `backup/...-before-lfs-...` branch until you have verified the latest commit and tested a fresh clone. A fresh clone should retrieve LFS content with:

```bash
git lfs install
git lfs pull
```

After confirming that the remote repository and a fresh checkout are correct, you may delete the local backup branch using its exact name:

```bash
git branch -d "backup/main-before-lfs-YYYYMMDD-HHMMSS"
```

Replace the example branch name with the one printed by the script.

### References

- [GitHub: GitHub repository limits](https://docs.github.com/en/repositories/creating-and-managing-repositories/repository-limits)
- [GitHub: Troubleshooting the 2 GiB push limit](https://docs.github.com/en/get-started/using-git/troubleshooting-the-2-gb-push-limit)
- [GitHub: About Git Large File Storage](https://docs.github.com/en/repositories/working-with-files/managing-large-files/about-git-large-file-storage)
- [GitHub: Git LFS billing and included usage](https://docs.github.com/en/billing/concepts/product-billing/git-lfs)
- [Git LFS: `git lfs migrate` documentation](https://github.com/git-lfs/git-lfs/blob/main/docs/man/git-lfs-migrate.adoc)
- [Git LFS official website](https://git-lfs.com/)
