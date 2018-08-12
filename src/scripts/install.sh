#!/bin/bash

echo -e "[install]\nprefix=" > ~/.pydistutils.cfg
pip3 install --target=./packages --ignore-installed "$1"
rm -f ~/.pydistutils.cfg
