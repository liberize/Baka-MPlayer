#!/usr/bin/env python
# coding: utf-8

import os
import logging
from upv import *

_logger = logging.getLogger(__name__)

# accessible from c++:
# ConfigItem, Plugin, SubtitleEntry, MediaEntry


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
    if not plugin or not plugin.is_type(SubtitleProvider):
        return None

    result = []
    try:
        for entries in plugin.search(word, count, **kwargs):
            if isinstance(entries, SubtitleEntry):
                result.append(entries)
            else:
                for entry in entries:
                    if isinstance(entry, SubtitleEntry):
                        result.append(entry)
            if len(result) >= count:
                break
    except Exception as e:
        _logger.error('search_subtitle: error: %s', e)
    return result


def fetch_media(plugin_name, count, **kwargs):
    plugin = plugin_manager.get_plugin_by_name(plugin_name)
    if not plugin or not plugin.is_type(MediaProvider):
        return None

    result = []
    try:
        for entries in plugin.fetch(count, **kwargs):
            if isinstance(entries, MediaEntry):
                result.append(entries)
            else:
                for entry in entries:
                    if isinstance(entry, MediaEntry):
                        result.append(entry)
            if len(result) >= count:
                break
    except Exception as e:
        _logger.error('fetch_media: error: %s', e)
    return result


def search_media(plugin_name, word, count, **kwargs):
    plugin = plugin_manager.get_plugin_by_name(plugin_name)
    if not plugin or not plugin.is_type(MediaProvider):
        return None

    result = []
    try:
        for entries in plugin.search(word, count, **kwargs):
            if isinstance(entries, MediaEntry):
                result.append(entries)
            else:
                for entry in entries:
                    if isinstance(entry, MediaEntry):
                        result.append(entry)
            if len(result) >= count:
                break
    except Exception as e:
        _logger.error('search_media: error: %s', e)
    return result


if __name__ == '__main__':
    plugin_dir = os.path.abspath('plugins')
    for plugin in load_plugins([plugin_dir]):
        print('found', plugin, 'in', plugin.path, 'config', plugin.config)
