#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 Sigmastone OÜ
# SPDX-License-Identifier: Apache-2.0
#
# What a run of the build workflow is, decided once and read by every step after it. Run under
# `shell: bash` from the repository root, with GITHUB_REF, GITHUB_SHA and GITHUB_OUTPUT set as
# Actions sets them:
#
#   - A `v*` tag. The MSI is built, and if the tagged commit is contained in `dev` - this
#     repository's main branch - it is published as a GitHub Release. A tag with a pre-release
#     suffix (v0.1.0-rc.1) makes a pre-release. The tag's version must start with the project
#     version in CMakeLists.txt; CMake checks the same thing, this only fails sooner.
#   - Anything else: a push, a pull request or a manual run. The version is the project version
#     with a `-dev.<commit>` suffix and nothing is published.
#
# Outputs: version, release (true|false), prerelease (true|false), and tag for a tag run.

set -euo pipefail

project_version=$(grep -m1 -oE 'VERSION [0-9]+\.[0-9]+\.[0-9]+' CMakeLists.txt | cut -d' ' -f2)
if [[ -z "$project_version" ]]; then
  echo "::error::could not read the project version from CMakeLists.txt"
  exit 1
fi

if [[ "${GITHUB_REF:-}" == refs/tags/v* ]]; then
  tag="${GITHUB_REF#refs/tags/}"
  version="${tag#v}"

  if [[ "$version" != "$project_version" && "$version" != "$project_version"-* ]]; then
    echo "::error::tag $tag does not match the version in CMakeLists.txt ($project_version)"
    exit 1
  fi

  # `--is-ancestor` is true for the tip itself, so tagging the head of `dev` releases.
  git fetch --no-tags origin +refs/heads/dev:refs/remotes/origin/dev
  if git merge-base --is-ancestor "$GITHUB_SHA" origin/dev; then
    release=true
  else
    release=false
    echo "::notice::$tag is not contained in dev - building the MSI, publishing nothing"
  fi

  if [[ "$version" == *-* ]]; then prerelease=true; else prerelease=false; fi
  echo "tag=$tag" >> "$GITHUB_OUTPUT"
else
  version="$project_version-dev.$(git rev-parse --short HEAD)"
  release=false
  prerelease=true
fi

echo "version=$version" >> "$GITHUB_OUTPUT"
echo "release=$release" >> "$GITHUB_OUTPUT"
echo "prerelease=$prerelease" >> "$GITHUB_OUTPUT"
echo "Building $version (release=$release)"
