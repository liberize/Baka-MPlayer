#!/usr/bin/env python
# coding: utf-8

from .plugin import Plugin


class MediaEntry:
    def __init__(self, name, url, cover, description, downloader='default'):
        '''
        url can be real url, local file path, stdin (-), or file descriptor (fd://123)
        downloader can be 'self', 'ytdl', 'default'
        setting downloader to 'ytdl' is equivalent to prefixing url with 'ytdl://'
        '''
        self.name = name
        self.url = url
        self.cover = cover
        self.description = description
        self.downloader = downloader

    def __str__(self):
        return 'MediaEntry <{}, {}>'.format(self.name, self.url)


class MediaProvider(Plugin):
    def __init__(self):
        super().__init__()

    def fetch(self, start, count, **kwargs):
        '''
        return a list of MediaEntry instance
        '''
        return []

    def search(self, word, count, **kwargs):
        '''
        return a list of MediaEntry instance
        '''
        return []

    def download(self, entry):
        '''
        self downloader implementation
        fill entry url, and return the modified entry
        sample usages:
        1. parsing real url
        2. downloading whole file to local disk
        3. using stdin, named pipe or socket for streaming purpose
        '''
        return entry
