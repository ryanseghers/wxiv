#!/usr/bin/env bash
# I installed git by trying to use it and my mac popped a dialog to install developer tools and I did that

# install brew using the command from their home page
# /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

brew install pkg-config cmake ninja clang-format

# THIS DOES NOT WORK WITHOUT ENV CHANGE!
# macos already has bison 2.3 in /usr/bin and so brew does not mess with that, so have to modify path to use the new bison
# otherwise the vcpkg build fails
# (could modify path temporarily in the vcpkg build script)
# echo 'export PATH="/opt/homebrew/opt/bison/bin:$PATH"' >> ~/.profile
brew install bison

