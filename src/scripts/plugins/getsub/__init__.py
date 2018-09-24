#!/usr/bin/env python
# coding: utf-8

from upv import ConfigItem, SubtitleEntry, SubtitleProvider
# import os
# import sys

# from main import GetSubtitles

# GetSubtitles(name="头号玩家.Ready.Player.One.2018.中英字幕.WEBrip.AAC.720P.x264-Lee",
#              query=False,
#              single=True,
#              more=False,
#              over=False,
#              debug=True,
#              sub_num=10,
#              downloader=None).start()


class GetSub(SubtitleProvider):
    def __init__(self):
        super().__init__()
        self.name = 'SubHD'
        self.icon = ''
        self.description = 'Match and download subtitles from subhd.com'
        self.config_items = []

    def search(self, word, max_count, **kwargs):
        return [SubtitleEntry('test', 'http://example.com/')]
