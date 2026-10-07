#!/usr/bin/env bash
# Install the host packages a minimal distro container needs to build and test
# DDS with Bazelisk. Used by .github/workflows/ci_linux_distros.yml, where it runs
# as root right after actions/checkout (a REST tarball, since the base images
# have no git); also usable for local reproduction (docs/BUILD_SYSTEM.md).
#
# Everything else (Bazel, clang/libc++, Python 3.14, JDK) is downloaded by
# Bazel. The host supplies only:
#   - glibc headers, crt files and libraries (no sysroot is configured);
#   - binutils, for /usr/bin/ld (MODULE.bazel links Linux with GNU ld);
#   - git, python3 and the small utilities Bazel's repository rules shell out to;
#   - tar/gzip/zstd for the GitHub Actions cache.
set -euo pipefail

if [[ ! -r /etc/os-release ]]; then
    echo "install_distro_deps: /etc/os-release not found" >&2
    exit 1
fi
# shellcheck source=/dev/null
. /etc/os-release
echo "install_distro_deps: ${PRETTY_NAME:-${ID:-unknown}}"

common=(git binutils findutils diffutils unzip tar gzip zstd)

case " ${ID:-} ${ID_LIKE:-} " in
    *" fedora "* | *" rhel "* | *" centos "*)
        dnf -y install "${common[@]}" python3 glibc-devel which
        dnf clean all
        ;;
    *" debian "* | *" ubuntu "*)
        export DEBIAN_FRONTEND=noninteractive
        apt-get update
        apt-get install -y --no-install-recommends "${common[@]}" python3 libc6-dev ca-certificates
        rm -rf /var/lib/apt/lists/*
        ;;
    *" arch "*)
        # pacman 7 sandboxes its downloader with a seccomp filter, which fails
        # (error 22) where seccomp is unavailable, e.g. amd64 emulation on
        # Apple silicon. The container is throwaway, so skip that part.
        pacman -Syu --noconfirm --needed --disable-sandbox-syscalls "${common[@]}" python glibc which
        ;;
    *" suse "* | *" opensuse "*)
        zypper --non-interactive install "${common[@]}" python3 glibc-devel which
        ;;
    *)
        echo "install_distro_deps: unsupported distribution '${ID:-}' (ID_LIKE='${ID_LIKE:-}')" >&2
        exit 1
        ;;
esac

# sed reads all input; head could exit first and SIGPIPE ldd under pipefail.
ldd --version | sed -n 1p
