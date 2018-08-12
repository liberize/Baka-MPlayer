#!/usr/bin/env python
# coding: utf-8

import sys
def _exit(*args):
    raise RuntimeError('plugin called exit with args: {}'.format(args))
sys.exit = _exit
sys.argv = [""]

import logging
from .plugin import ConfigItem, Plugin
from .subtitleprovider import SubtitleEntry, SubtitleProvider
from .mediaprovider import MediaEntry, MediaProvider
from .pluginmanager import plugin_manager

logging.basicConfig(level=logging.DEBUG, format='%(message)s')
