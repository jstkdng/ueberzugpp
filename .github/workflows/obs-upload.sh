#!/bin/bash
set -e

DATETIME=$(date -R)
VERSION=$(grep "set(UEBERZUGPP_VERSION" CMakeLists.txt | grep -oP '\(\K[^\)]+' | cut -f 2 -d ' ')
TARFILE="ueberzugpp_$VERSION.tar.xz"
DSCFILE="ueberzugpp.dsc"
SPECFILE="ueberzugpp.spec"
declare -a FILES=("$TARFILE" "$DSCFILE" "$SPECFILE")

sed -e "s;@@VERSION@@;${VERSION};g" -e "s;@@DATETIME@@;${DATETIME};g" < debian/changelog.in > debian/changelog

tar cfJ "$TARFILE" ./*

cat > "$DSCFILE" << EOF
Format: 3.0 (native)
Source: ueberzugpp
Binary: ueberzugpp
Architecture: any
Version: $VERSION
Maintainer: JustKidding <jk@vin.ovh>
Homepage: https://github.com/jstkdng/ueberzugpp
Standards-Version: 4.5.1
Build-Depends: $(sed -n '/^Build-Depends:/,/^$/p' < debian/control | tr -d '\n' | cut -f 2 -d : | sed -r 's;^ *| *$;;g')
Package-List:
 ueberzugpp deb misc optional arch=any
Checksums-Sha1:
 $(sha1sum < "$TARFILE" | cut -f 1 -d ' ') $(stat -c '%s' "$TARFILE") $TARFILE
Checksums-Sha256:
 $(sha256sum < "$TARFILE" | cut -f 1 -d ' ') $(stat -c '%s' "$TARFILE") $TARFILE
Files:
 $(md5sum < "$TARFILE" | cut -f 1 -d ' ') $(stat -c '%s' "$TARFILE") $TARFILE
EOF

cat > "$SPECFILE" << EOF
#
# spec file for package ueberzugpp
#
# Copyright (c) 2023 SUSE LLC
#
# All modifications and additions to the file contributed by third parties
# remain the property of their copyright owners, unless otherwise agreed
# upon. The license for this file, and modifications and additions to the
# file, is the same license as for the pristine package itself (unless the
# license for the pristine package is not an Open Source License, in which
# case the license is the MIT License). An "Open Source License" is a
# license that conforms to the Open Source Definition (Version 1.9)
# published by the Open Source Initiative.

# Please submit bugfixes or comments via https://bugs.opensuse.org/
#


%define short_name ueberzug
Name:           ueberzugpp
Version:        $VERSION
Release:        0
Summary:        Utility to render images in terminals
License:        GPL-3.0-or-later
URL:            https://github.com/jstkdng/%{name}
Source:         https://github.com/jstkdng/%{name}/archive/refs/tags/v%{version}.tar.gz#/%{name}_%{version}.tar.xz
BuildRequires:  automake
BuildRequires:  cmake
BuildRequires:  cmake(spdlog)
BuildRequires:  extra-cmake-modules
BuildRequires:  gcc-c++
BuildRequires:  make
BuildRequires:  range-v3-devel
BuildRequires:	pkgconfig(openssl)
BuildRequires:  pkgconfig(CLI11)
BuildRequires:  pkgconfig(chafa)
BuildRequires:  pkgconfig(libsixel)
BuildRequires:  pkgconfig(nlohmann_json)
BuildRequires:  pkgconfig(opencv4)
BuildRequires:  pkgconfig(tbb)
BuildRequires:  pkgconfig(vips)
BuildRequires:  pkgconfig(wayland-client)
BuildRequires:  pkgconfig(wayland-protocols)
BuildRequires:  pkgconfig(xcb-image)

%description
Überzug++ is a C++ command line utility which allows to draw images
on terminals by using child windows or using sixel on supported
terminals. (This is a drop-in replacement for the now defunct
ueberzug project.)

Advantages over w3mimgdisplay and ueberzug:

- support for wayland (sway only)
- no race conditions as a new window is created to display images
- "expose" events will be processed, so that images will be
  redrawn when switching workspaces
- tmux support on X11
- terminals without the WINDOWID environment variable are supported
- chars are used as position and size unit
- A lot of image formats are supported (through opencv and libvips)
- GIF and animated WEBP support on X11 and Sixel
- Resized images are cached for faster viewing

%prep
%autosetup -c

%build
%cmake -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_SKIP_RPATH=YES -DCMAKE_BUILD_TYPE=Release -DENABLE_WAYLAND=ON
%cmake_build

%install
%cmake_install

%files
%{_bindir}/%{short_name}
%{_bindir}/%{short_name}pp
%license LICENSE
%doc README.md
%{_mandir}/man1/%{short_name}.1.gz
%{_mandir}/man1/%{short_name}pp.1.gz

%changelog

EOF

BASE_URL="https://api.opensuse.org/source/$OBS_PROJECT/ueberzugpp"
for FILE in "${FILES[@]}"
do
    URL="$BASE_URL/$FILE?rev=upload"
    curl -XPUT -H 'Content-Type: application/octet-stream' -u "$OBS_AUTH" --data-binary "@$FILE" "$URL"
done

curl -XPOST -u "$OBS_AUTH" "$BASE_URL" -F "cmd=commit"

