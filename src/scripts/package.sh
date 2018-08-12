#!/bin/bash

[ -d packages ] || exit 1

cd packages
find . -name "*.pyc" -delete
find . -name ".DS_Store" -delete
find . -name "*.egg-info" | xargs rm -rf
find . -name "*.dist-info" | xargs rm -rf
find . -name "__pycache__" | xargs rm -rf
zip -9rv packages.zip .
mv packages.zip ..
cd ..
