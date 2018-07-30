#!/usr/bin/env python
# coding: utf-8

from .plugin import Plugin


class SubtitleEntry:
    def __init__(self, name, url):
        self.name = name
        self.url = url

    def __str__(self):
        return 'SubtitleEntry <{}, {}>'.format(self.name, self.url)


class SubtitleProvider(Plugin):
    def __init__(self):
        super().__init__()

    def search(self, word, count, **kwargs):
        ''' yield a list of SubtitleEntry instance '''
        yield from ()
