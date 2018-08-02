#!/usr/bin/env python
# coding: utf-8

import os
import logging
from upv import *

_logger = logging.getLogger(__name__)

# accessible from c++:
# ConfigItem, Plugin, SubtitleEntry, MediaEntry


def init_env(temp_dir):
    os.environ["TMPDIR"] = temp_dir
    os.environ["TEMP"] = temp_dir
    os.chdir(temp_dir)


def load_plugins(plugin_dirs, disable_list=[]):
    plugin_manager.load_plugins(plugin_dirs)
    if disable_list:
        plugin_manager.disable_plugins(disable_list)


def get_all_plugins():
    return plugin_manager.get_all_plugins()


def get_subtitle_plugins():
    return plugin_manager.get_plugins_by_type(SubtitleProvider)


def get_media_plugins():
    return plugin_manager.get_plugins_by_type(MediaProvider)


def is_subtitle_plugin(plugin_name):
    plugin = plugin_manager.get_plugin_by_name(plugin_name)
    if not plugin:
        return False
    return plugin.is_type(SubtitleProvider)


def is_media_plugin(plugin_name):
    plugin = plugin_manager.get_plugin_by_name(plugin_name)
    if not plugin:
        return False
    return plugin.is_type(MediaProvider)


def enable_plugin(plugin_name):
    plugin = plugin_manager.get_plugin_by_name(plugin_name)
    if not plugin:
        return
    plugin.enable()


def disable_plugin(plugin_name):
    plugin = plugin_manager.get_plugin_by_name(plugin_name)
    if not plugin:
        return
    plugin.disable()


def get_plugin_config(plugin_name):
    plugin = plugin_manager.get_plugin_by_name(plugin_name)
    if not plugin:
        return
    return plugin.config


def update_plugin_config(plugin_name, conf):
    plugin = plugin_manager.get_plugin_by_name(plugin_name)
    if not plugin:
        return
    plugin.config = conf


def search_subtitle(plugin_name, word, count, **kwargs):
    plugin = plugin_manager.get_plugin_by_name(plugin_name)
    if not plugin or not plugin.enabled or not plugin.is_type(SubtitleProvider):
        return None
    return plugin.search(word, count, **kwargs):


def download_subtitle(plugin_name, entry):
    plugin = plugin_manager.get_plugin_by_name(plugin_name)
    if not plugin or not plugin.enabled or not plugin.is_type(SubtitleProvider):
        return None
    return plugin.download(entry):


def fetch_media(plugin_name, start, count, **kwargs):
    plugin = plugin_manager.get_plugin_by_name(plugin_name)
    if not plugin or not plugin.enabled or not plugin.is_type(MediaProvider):
        return None
    return plugin.fetch(start, count, **kwargs):


def search_media(plugin_name, word, count, **kwargs):
    plugin = plugin_manager.get_plugin_by_name(plugin_name)
    if not plugin or not plugin.enabled or not plugin.is_type(MediaProvider):
        return None
    return plugin.search(word, count, **kwargs):


def download_media(plugin_name, entry):
    plugin = plugin_manager.get_plugin_by_name(plugin_name)
    if not plugin or not plugin.enabled or not plugin.is_type(MediaProvider):
        return None
    return plugin.download(entry):


if __name__ == '__main__':
    plugin_dir = os.path.abspath('plugins')
    for plugin in load_plugins([plugin_dir]):
        print('found', plugin, 'in', plugin.path, 'config', plugin.config)
