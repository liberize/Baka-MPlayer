#!/usr/bin/env python
# coding: utf-8

import logging
from .plugin import ConfigItem, Plugin
from .subtitleprovider import SubtitleEntry, SubtitleProvider
from .mediaprovider import MediaEntry, MediaProvider
from .pluginmanager import plugin_manager

logging.basicConfig(level=logging.DEBUG, format='%(message)s')
