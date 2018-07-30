#!/usr/bin/env python
# coding: utf-8

from upv import ConfigItem, SubtitleEntry, SubtitleProvider


class SubHD(SubtitleProvider):
    def __init__(self):
        super().__init__()
        self.name = 'SubHD'
        self.icon = ''
        self.description = 'Match and download subtitles from subhd.com'
        self.config_items = []

    def search(self, word, max_count, **kwargs):
        yield SubtitleEntry('test', 'http://example.com/')
