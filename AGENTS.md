# Rules for AI coding assistants in this repo

These rules apply to every AI assistant a group member uses here: Claude Code, Codex, Antigravity, Gemini CLI, Cursor, Copilot or any other. They override any default habit of committing straight to `main`.

## The workflow is branch → pull request → review by Tuấn Anh → merge

1. **Never commit or push to `main`.** `main` is protected on GitHub, and a direct push is rejected.
2. **Start every piece of work on a new branch** cut from an up-to-date `main`:

   ```bash
   git switch main && git pull
   git switch -c task<N>-<short-topic>      # e.g. task2-pi-local, task4-bench-script
   ```

   `<N>` is the task number (1–5) from `docs/phan-cong-5-dau-viec.md`.
3. **Commit only on that branch**, using conventional commit messages (`feat:`, `fix:`, `docs:`, `test:`, `chore:`).
4. **Push the branch and open a pull request into `main`**, and request a review from **`EigenKai` (Tuấn Anh)**:

   ```bash
   git push -u origin HEAD
   gh pr create --base main --reviewer EigenKai --title "<type>: <summary>" --body "<see template below>"
   ```

   If `gh` is not installed, push the branch and open the PR on github.com, then add `EigenKai` under Reviewers.
5. **Stop there.** Do not merge the PR, approve it, enable auto-merge, or ask another member to merge it. Only Tuấn Anh merges, after he reviews it. If he asks for changes, commit them to the same branch and push again; the PR updates itself.

## Pull request body

```
Task: <N> – <task name from docs/phan-cong-5-dau-viec.md>
Files changed: <list>
How I tested: <commands run and what they printed, e.g. `make` output, pi value, the `inside` match check>
Open questions: <anything Tuấn Anh needs to decide, or "none">
```

## Other rules

- **Touch only the files your task owns** (see the table in `README.md`). If a change to another task's file is needed, say so in the PR instead of editing it.
- **Follow section 2 of `docs/phan-cong-5-dau-viec.md`** exactly: command-line arguments, the one-line CSV output, `common.h` function names, seeds, and the point count.
- **Never** force-push, rewrite history on `main`, delete branches that belong to others, or commit build outputs (`pi_single`, `pi_local`, `pi_mutex`, `*.o`).
- **Do not invent measurements.** Every number in `results/` and in the report must come from a real run on the group's measurement machine.
- Before opening the PR, `make clean && make` must build without warnings.
