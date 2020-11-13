if command -v apt-get &> /dev/null
then
    echo "installing dependencies using apt-get"
    sudo apt-get install libturbojpeg0-dev
elif command -v brew &> /dev/null
then
    echo "installing dependencies using brew"
    brew install jpeg-turbo 
    export LDFLAGS="-L$(brew --prefix)/opt/jpeg-turbo/lib"
    export CFLAGS="-I$(brew --prefix)/opt/jpeg-turbo/include"
fi

