# Greengrass SDK Lite Developer setup

This guide is intended for developers working on the Greengrass SDK Lite
codebase itself.

## Using Nix

Using Nix will allow you to use a reproducible development environment matching
CI as well as run the CI checks locally.

If you don't already have Nix, see the `Install Nix` section in
`../.github/workflows/ci.yml` to get the same version of Nix as used in CI.

To run all the formatters used by this project, run `nix fmt` in the project
root directory. `nix fmt <filepath>` can format a single file.

Note that untracked git files will be formatted as well, so if using build
directories or other files not tracked by git or in gitignore, add them to your
`./.git/info/exclude`.

To reproduce the CI checks locally, run `nix flake check -L`. Ensure this passes
for each commit in your PRs.

If making a PR to main, you can check all of your branches commits with
`git rebase main -x "nix flake check -L"`.

## Running Coverity

After installing Coverity and adding its bin dir to your path, run the following
in the project root dir:

```sh
cmake -B build
coverity scan
```

The html output will be in `build/cov-out`.
