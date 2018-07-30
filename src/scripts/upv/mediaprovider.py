#!/usr/bin/env python
# coding: utf-8

from .plugin import Plugin


class MediaEntry:
    def __init__(self, name, url, cover, description):
        self.name = name
        self.url = url
        self.cover = cover
        self.description = description

    def __str__(self):
        return 'MediaEntry <{}, {}>'.format(self.name, self.url)


class MediaProvider(Plugin):
    def __init__(self):
        super().__init__()

    def fetch(self, count):
        ''' yield a list of MediaEntry instance '''
        yield from ()

    def search(self, word, count, **kwargs):
        ''' yield a list of MediaEntry instance '''
        yield from ()
