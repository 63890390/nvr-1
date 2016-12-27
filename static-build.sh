#!/bin/sh

set -e

SCRIPT=$(readlink -f ${0})
PROJECT_DIR=$(dirname ${SCRIPT})
BUILD_DIR="$(pwd)/static"
mkdir -p ${BUILD_DIR}

DEPENDENCIES_MET=1
for DEPENDENCY in "gcc" "make" "pkg-config" "yasm"; do
    if ! type ${DEPENDENCY} > /dev/null; then
        DEPENDENCIES_MET=0
        echo "Please install $DEPENDENCY"
    fi
done

if type "curl" > /dev/null; then
    DL="curl"
elif type "wget" > /dev/null; then
    DL="wget -O -"
else
    echo "Please install curl or wget"
    exit 1;
fi

if [ ${DEPENDENCIES_MET} -eq 0 ]; then
    exit 1;
fi

if [ ! -f "$BUILD_DIR/x264/.built" ]; then
    rm -rf "$BUILD_DIR/x264"
    mkdir -p "$BUILD_DIR/x264"
    cd "$BUILD_DIR/x264"
    ${DL} ftp://ftp.videolan.org/pub/x264/snapshots/last_x264.tar.bz2 | tar -xj --wildcards --strip-components=1 "x264*"
    ./configure --enable-static --disable-cli
    make -j$(nproc --all)
    touch .built
fi

if [ ! -f "$BUILD_DIR/ffmpeg/.built" ]; then
    rm -rf "$BUILD_DIR/ffmpeg"
    mkdir -p "$BUILD_DIR/ffmpeg"
    cd "$BUILD_DIR/ffmpeg"
    ${DL} https://ffmpeg.org/releases/ffmpeg-snapshot-git.tar.bz2 | tar -xj --strip-components=1 ffmpeg
    PKG_CONFIG_PATH="$BUILD_DIR/x264" \
        ./configure --pkg-config-flags="--static" --extra-cflags="-I$BUILD_DIR/x264" --extra-ldflags="-L$BUILD_DIR/x264" \
        --enable-static --disable-shared --enable-gpl \
        --disable-all --enable-libx264 \
        --enable-avcodec --enable-avformat \
        --enable-decoder=h264 \
        --enable-muxer=h264 --enable-muxer=rtsp --enable-muxer=mp4 \
        --enable-demuxer=h264 --enable-demuxer=rtsp \
        --enable-parser=h264 \
        --enable-protocol=file --enable-protocol=rtp --enable-protocol=tcp
    make -j$(nproc --all)
    touch .built
fi

cd ${PROJECT_DIR}

gcc -std=c99 -Istatic/ffmpeg main.c jsmn.c utils.c nvr.c config.c ini.c log.c \
    "$BUILD_DIR/ffmpeg/libavformat/libavformat.a" \
    "$BUILD_DIR/ffmpeg/libavcodec/libavcodec.a" \
    "$BUILD_DIR/ffmpeg/libavutil/libavutil.a" \
    "$BUILD_DIR/x264/libx264.a" \
    -lpthread -lm -ldl -o "$BUILD_DIR/nvr"
