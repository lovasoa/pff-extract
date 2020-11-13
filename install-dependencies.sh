if command -v apt-get &> /dev/null
then
    echo "installing dependencies using apt-get"
    sudo apt-get install libturbojpeg0-dev
elif command -v dnf &> /dev/null
then
    echo "Installing dependencies with dnf"
    sudo dnf install turbojpeg-devel
elif command -v brew &> /dev/null
then
    echo "installing dependencies using brew"
    brew install jpeg-turbo 
    export LDFLAGS="-L$(brew --prefix)/opt/jpeg-turbo/lib"
    export CFLAGS="-I$(brew --prefix)/opt/jpeg-turbo/include"
else
    echo "No supported package manager found"
    exit 1
fi

