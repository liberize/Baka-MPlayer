#!/usr/bin/env python
# coding: utf-8

import os
import json
import logging
import inspect
from .pluginmanager import plugin_manager

_logger = logging.getLogger(__name__)


class ConfigItem:
    def __init__(self, name, title="", type=str, validator=None):
        self.name = name
        self.title = title or name
        self.type = type or str
        self.validator = validator
        self.value = ''

    def __str__(self):
        return 'ConfigItem <{}, {}, {}>'.format(self.name, self.title, self.type.__name__)


class Plugin:
    def __init__(self):
        # public
        self.name = ""
        self.icon = ""
        self.description = ""
        self.config_items = []
        # private
        self.__path = ""
        self.__enabled = True

    def __init_subclass__(cls, **kwargs):
        super().__init_subclass__(**kwargs)
        plugin = cls()
        plugin.__path = os.path.dirname(inspect.getfile(cls))
        plugin.__load_config()
        if plugin.name:
            plugin_manager.register_plugin(plugin)

    def __str__(self):
        return 'Plugin <{}, {}>'.format(self.name or 'no name',
                                        self.description or 'no description')

    def __load_config(self):
        conf_file = os.path.join(self.__path, 'config.json')
        if not os.path.isfile(conf_file):
            return
        try:
            with open(conf_file, 'r') as f:
                conf = json.load(f)
            for i in self.config_items:
                if i.name in conf:
                    setattr(self, i.name, conf[i.name])
        except:
            _logger.error("plugin %s load config failed", self.name)

    def __save_config(self):
        conf_file = os.path.join(self.__path, 'config.json')
        try:
            conf = {i.name: getattr(self, i.name) for i in self.config_items}
            with open(conf_file, 'w') as f:
                json.dump(conf, f)
        except:
            _logger.error("plugin %s save config failed", self.name)

    @property
    def config(self):
        try:
            for i in self.config_items:
                value = getattr(self, i.name)
                i.value = '' if not value else str(value)
        except:
            _logger.error("plugin %s get config failed", self.name)
        return self.config_items

    @config.setter
    def config(self, conf):
        try:
            for i in self.config_items:
                if i.name in conf:
                    value = i.type() if conf[i.name] is None else i.type(conf[i.name])
                    setattr(self, i.name, value)
            self.__save_config()
        except:
            _logger.error("plugin %s set config failed", self.name)

    @property
    def path(self):
        return self.__path

    @property
    def enabled(self):
        return self.__enabled

    def enable(self):
        _logger.info("enable plugin %s", self.name)
        self.__enabled = True

    def disable(self):
        _logger.info("disable plugin %s", self.name)
        self.__enabled = False

    def load(self):
        _logger.info("load plugin %s", self.name)

    def unload(self):
        _logger.info("unload plugin %s", self.name)

    def is_type(self, PluginSubClass):
        return isinstance(self, PluginSubClass)
