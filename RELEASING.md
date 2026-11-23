# Releasing Luminance HDR

This document describes the manual, tag-first release process for maintainers.
It uses a short-lived release branch, immutable annotated tags, and GitHub
Releases. The 3.0.0 examples can be adapted to later versions.

## Principles

- Cut the release from `convert-to-qt6` until the Qt 6 branch has been merged
  into `master`.
- Use a short-lived `release/<version>` branch, for example `release/3.0.0`.
- Use immutable annotated tags:
  - RCs: `v3.0.0-rc1`, `v3.0.0-rc2`, ...
  - Final: `v3.0.0`
- Never move, delete, or reuse a published tag. If an RC is wrong, create the
  next RC.
- Publish every RC as a GitHub **pre-release**. Only the final version is
  marked as the latest release.
- Merge release fixes back into `convert-to-qt6` so the release branch does
  not become a permanently divergent maintenance branch.

## One-time setup

### 1. Create the release branch

```bash
git switch convert-to-qt6
git pull origin convert-to-qt6
git switch -c release/3.0.0
git push -u origin release/3.0.0
```

### 2. Update the version source of truth

The build version is defined in `build_files/Modules/Version.cmake`.

For a 3.0.0 release line:

- Set `LHDR_VERSION_MAJOR` to `3`.
- Set `LHDR_VERSION_MINOR` to `0`.
- Set `LHDR_VERSION_PATCH` to `0`.
- Add or maintain `LHDR_VERSION_PRERELEASE`:
  - `rc1`, `rc2`, ... for release candidates.
  - empty for the final release.
- Use a clean version for exact tag builds and append `_GIT-<hash>` only for
  development builds.

The generated files under `build/` and `src/Common/config.h.in` are derived
from these values. Do not hand-edit generated files.

### 3. Update release metadata

Update stale release text in the same commit that changes the version:

- `Changelog`: add a new section for the release.
- `README.md`: update the release badge target.
- `INSTALL.md`: update macOS application and DMG names.
- `net.sourceforge.qtpfsgui.LuminanceHDR.appdata.xml`: add a stable release
  entry. AppStream release entries are normally added for final versions, not
  for RCs.

### 4. Configure CI for release tags

`.github/workflows/windows-msys2.yml` should accept:

```yaml
on:
  workflow_dispatch:
  push:
    tags: ['v3.0.0*']
  release:
    types: [published]
```

Artifact names should include `${{ github.ref_name }}`, for example:

```yaml
name: luminance-hdr-${{ github.ref_name }}-windows-ucrt64
```

In `build_files/platforms/msys2/UpdateInfo.cmake`, make sure a tag build does
not use `GIT_DESCRIBE` as both the branch name and the version, otherwise the
installer filename can contain the version twice.

## Preflight checklist

Before tagging an RC or the final release:

- [ ] The working tree is clean and the release branch is up to date.
- [ ] `Version.cmake` has the intended version and prerelease value.
- [ ] The local default build succeeds with Qt WebEngine enabled.
- [ ] The local build succeeds with `-DUSE_WEBENGINE=OFF`.
- [ ] The Windows MSYS2/UCRT64 workflow succeeds.
- [ ] The CLI starts, for example with `--help`.
- [ ] The About dialog and Windows installer show the expected version.
- [ ] `Changelog`, README badge, and installation docs are current.
- [ ] No generated file contains a stale hardcoded version.

## Create a release candidate

Use `rc1` for the first RC and increment the number for each following RC.

```bash
git switch release/3.0.0
git pull origin release/3.0.0

# Edit build_files/Modules/Version.cmake:
# set LHDR_VERSION_PRERELEASE to "rc1" (or "rc2", etc.)
# Update Changelog if the RC needs its own notes.

git commit -am "Release 3.0.0-rc1"
git push origin release/3.0.0
```

After CI passes, create an annotated tag on that exact commit:

```bash
git tag -a v3.0.0-rc1 -m "Release 3.0.0-rc1"
git push origin v3.0.0-rc1
```

Create the GitHub pre-release:

```bash
gh release create v3.0.0-rc1 \
  --prerelease \
  --title "3.0.0-rc1" \
  --generate-notes
```

Download the build artifacts from the tag-triggered workflow and attach them
to the GitHub pre-release:

```bash
gh release upload v3.0.0-rc1 /path/to/artifact.zip
```

Verify that:

- The GitHub release is marked as a pre-release.
- The tag points to the intended commit.
- The artifact filenames contain the correct version.
- The Windows installer reports the correct version.

For the next RC, change only the prerelease value, create a new commit, and
tag it `v3.0.0-rc2`. Never move `v3.0.0-rc1`.

## Create the final release

When an RC has been validated:

1. Set `LHDR_VERSION_PRERELEASE` to an empty value.
2. Finalize `Changelog`, `README.md`, `INSTALL.md`, and AppStream metadata.
3. Run the full preflight checklist.
4. Commit and push to `release/3.0.0`.
5. After CI passes, create and push the final annotated tag:

```bash
git tag -a v3.0.0 -m "Release 3.0.0"
git push origin v3.0.0
```

6. Publish the GitHub release without `--prerelease`:

```bash
gh release create v3.0.0 \
  --title "3.0.0" \
  --generate-notes
```

7. Attach the final Windows, macOS, Linux, and source artifacts.
8. Confirm that GitHub marks `3.0.0` as the latest release.

## Merge-back and cleanup

After the final release has been published:

```bash
git switch convert-to-qt6
git pull origin convert-to-qt6
git merge --no-ff release/3.0.0
git push origin convert-to-qt6
```

Then merge `convert-to-qt6` into `master` through the normal pull request
workflow, so the final release commit becomes reachable from the mainline.

Only after merge-back and verification are complete:

```bash
git push origin --delete release/3.0.0
git branch -d release/3.0.0
```

## Gotchas

- The tags `v.2.6.1` and `v.2.6.1.1` are not reachable from `master` or
  `convert-to-qt6`, so `git describe` may report older versions. Do not use
  those tags as the basis for new releases.
- Builds from source archives or some worktrees can produce `_GIT-GITDIR`.
  Prefer normal clones for release builds.
- Windows packaging uses `git describe`, while CMake uses `Version.cmake`.
  Keep both aligned by tagging the exact release commit.
- The legacy updater compares version strings strictly. A tagged RC may be
  reported as out of date; this is expected unless the updater is replaced.
- Do not cherry-pick release fixes casually. Commit them on the release branch
  and merge them back into `convert-to-qt6`.
