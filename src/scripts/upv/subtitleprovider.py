#!/usr/bin/env python
# coding: utf-8

from .plugin import Plugin


class SubtitleEntry:
    def __init__(self, name, url, downloader="self"):
        '''
        url can be local file path or empty
        downloader can be 'self', 'default'
        '''
        self.name = name
        self.url = url
        self.downloader = downloader

    def __str__(self):
        return 'SubtitleEntry <{}, {}>'.format(self.name, self.url)


class SubtitleProvider(Plugin):
    def __init__(self):
        super().__init__()

    def search(self, word, count, **kwargs):
        '''
        return a list of SubtitleEntry instance
        entry path can be empty
        '''
        return []

    def download(self, entry):
        '''
        self downloader implementation
        fill entry url, and return the modified entry
        '''
        return entry
