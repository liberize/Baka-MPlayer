#!/usr/bin/env python
# coding: utf-8

from __future__ import absolute_import
import pkgutil
import logging

_logger = logging.getLogger(__name__)


class PluginManager:
    def __init__(self):
        self.__plugins = {}

    def load_plugins(self, locations):
        self.__plugins.clear()
        for finder, name, ispkg in pkgutil.walk_packages(locations):
            try:
                loader = finder.find_module(name)
                loader.load_module(name)
            except Exception:
                _logger.warning("skipped plugin '%s' due to an error.", name, exc_info=True)
                continue

    def register_plugin(self, plugin):
        if plugin.name in self.__plugins:
            _logger.warning("plugin '%s' already exists, ignored.", plugin.name)
            return
        self.__plugins[plugin.name] = plugin
        plugin.load()

    def unregister_plugin(self, plugin):
        if plugin.name not in self.__plugins:
            _logger.warning("plugin '%s' not registered.", plugin.name)
            return
        plugin.unload()
        del self.__plugins[plugin.name]

    def get_plugin_by_name(self, name):
        return self.__plugins.get(name, None)

    def get_all_plugins(self):
        return list(self.__plugins.values())

    def get_enabled_plugins(self):
        return [p for p in self.get_all_plugins() if p.enabled]

    def get_disabled_plugins(self):
        return [p for p in self.get_all_plugins() if not p.enabled]

    def get_plugins_by_type(self, PluginSubClass):
        return [p for p in self.get_enabled_plugins() if p.is_type(PluginSubClass)]

    def enable_plugins(self, names):
        for name in names:
            if name not in self.__plugins:
                _logger.warning("enable_plugins: plugin '%s' not loaded, ignored.", name)
                continue
            self.__plugins[name].enabled = True

    def disable_plugins(self, names):
        for name in names:
            if name not in self.__plugins:
                _logger.warning("disable_plugins: plugin '%s' not loaded, ignored.", name)
                continue
            self.__plugins[name].enabled = False

plugin_manager = PluginManager()
