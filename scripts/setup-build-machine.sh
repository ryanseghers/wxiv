# notes from my WSL ubuntu 22.04.1 setup
#
# I just upgraded to gcc-12 and g++-12 to try to fix a problem, but that wasn't the problem, so may not
# be necessary, and yet that's how I'm building now.
#
sudo apt install -y gcc-12 g++-12 ninja-build gdb cmake curl zip unzip tar

# not sure if I need this
#sudo apt install -y build-essential

# get pip3 installed
sudo apt install -y python3-pip

# for wxwidgets
sudo apt install -y bison python3-pip pkg-config
sudo apt install -y libdbus-1-dev libxi-dev libxtst-dev
sudo apt install -y libx11-dev libxft-dev libxext-dev
# xrandr not found
sudo apt install -y libxkbcommon-x11-dev libx11-xcb-dev

sudo apt-get install -y autoconf libtool gperf libgles2-mesa-dev libxrandr-dev
sudo apt-get install -y libxi-dev libxcursor-dev libxdamage-dev libxinerama-dev

# for formatting
sudo apt install -y clang-format

# for opencv
sudo apt install -y flex

# without this I get this on any wxwidgets dialog ctor: no GSettings schemas are installed on the machine
sudo apt install -y gsettings-desktop-schemas

# without this I get this on file open dialog (at least): Settings schema 'org.gtk.Settings.FileChooser' is not installed
sudo apt-get install --reinstall libgtk-3-common
