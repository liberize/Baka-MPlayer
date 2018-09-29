#!/usr/bin/env python
# coding: utf-8

import os
import base64
import json
import tempfile
import re
import requests
from urllib.parse import urlencode
from upv import input, ConfigItem, ConfigError, PluginError, MediaEntry, MediaProvider
from .api import PCS
import logging

_logger = logging.getLogger(__name__)


class BaiduPCS(MediaProvider):
    def __init__(self):
        super().__init__()
        self.name = 'BaiduPCS'
        self.icon = 'icon.jpg'
        self.description = 'Watch media hosted on pan.baidu.com'
        self.config_items = [
            ConfigItem('username', 'User Name', str, lambda s: len(s) > 0),
            ConfigItem('password', 'Password', str, lambda s: len(s) >= 6),
            ConfigItem('file_dir', 'Download Directory', str, lambda s: os.path.isdir(s))
        ]
        self.pcs = None

    def login(self):
        if not self.pcs:
            if not self.username or not self.password:
                raise ConfigError('Please configure User Name and Password!')
            os.chdir(self.path)

            def captcha_handler(image_url):
                data = requests.get(image_url).content
                with tempfile.NamedTemporaryFile(suffix=".png") as captcha_file:
                    captcha_file.write(data)
                    captcha_file.flush()
                    label = '<span>Input verify code:</span><img style="vertical-align:middle" align="right" src="{}">'
                    code = input(label.format(captcha_file.name))
                return code

            self.pcs = PCS(self.username, self.password, captcha_callback=captcha_handler)

    def fetch(self, start, count, **kwargs):
        assert start % count == 0
        return self.do_search('.mp4', start // count + 1, count)

    def humanized_size(self, num, suffix='B'):
        for unit in ['', 'Ki', 'Mi', 'Gi', 'Ti', 'Pi', 'Ei', 'Zi']:
            if abs(num) < 1024.0:
                return "%3.1f%s%s" % (num, unit, suffix)
            num /= 1024.0
        return "%.1f%s%s" % (num, 'Yi', suffix)

    def search(self, word, max_count, **kwargs):
        return self.do_search(word, 1, max_count)

    def do_search(self, word, page, limit):
        self.login()
        data = self.pcs.search('/', word, page=page, limit=limit).content
        resp = json.loads(data.decode('utf-8'))
        if resp['errno'] != 0:
            raise PluginError('Search failed with errno: {}'.format(resp['errno']))

        os.chdir(os.environ['TMPDIR'])
        result = []
        for item in resp['list']:
            if item['isdir'] != 0:
                continue
            if item['category'] != 1 and not re.search(r'\.(mp4|mkv|rm|rmvb|avi|mov|mp3|m4a|wav|flac)', item['path'].lower()):
                continue
            entry = MediaEntry(item['server_filename'], item['path'], {}, '',
                               'path: {}, size: {}'.format(item['path'], self.humanized_size(item['size'])))
            result.append(entry)
        return result

    def download(self, entry, what):
        self.login()
        if what == '':
            url, headers = self.pcs.download_url2(entry.url)
            options = {
                'preset': 'pcs',
                'user_agent': headers['User-Agent'],
                'referer': headers['Referer'],
                'cookies': headers['Cookie'],
                'max_conn': '16'
            }
            if self.file_dir:
                options['file_dir'] = self.file_dir
            entry.url = 'mtsp://' + base64.b64encode(url.encode('utf-8')).decode('utf-8') + '?' + urlencode(options)
        elif what == 'cover':
            data = self.pcs.thumbnail(entry.url, 144, 256).content
            with tempfile.NamedTemporaryFile(dir=os.environ['TMPDIR'], suffix=".jpg", delete=False) as cover_file:
                cover_file.write(data)
                entry.cover = cover_file.name
        return entry
