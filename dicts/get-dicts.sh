#/bin/sh

URL="https://github.com/Yomin/rtk-dicts"
FILE="master.tar.gz"
FILEURL="$URL/archive/$FILE"

if which git >/dev/null 2>&1; then
    git clone "$URL.git"
    ln -s rtk-dicts/primitives
else
    if which wget >/dev/null 2>&1; then
        wget $FILEURL
    elif which curl >/dev/null 2>&1; then
        curl -L -O $FILEURL
    else
        exit 1
    fi
    tar -xzf $FILE
    rm $FILE
    ln -s rtk-dicts-master/primitives
fi

exit 0
