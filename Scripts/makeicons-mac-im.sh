#!/bin/bash

ICONPATH=../CalGUI/icons/TARSISETC.png

mkdir -p CalGUI.iconset
convert -resize 16x16     "$ICONPATH" CalGUI.iconset/icon_16x16.png
convert -resize 32x32     "$ICONPATH" CalGUI.iconset/icon_32x32.png
convert -resize 128x128   "$ICONPATH" CalGUI.iconset/icon_128x128.png
convert -resize 256x256   "$ICONPATH" CalGUI.iconset/icon_256x256.png
convert -resize 512x512   "$ICONPATH" CalGUI.iconset/icon_512x512.png
convert -resize 1024x1024 "$ICONPATH" CalGUI.iconset/icon_512x512@2x.png

png2icns CalGUI.icns CalGUI.iconset/*
