import os
import sys
sys.path.append('/Users/liberize/Code/GitHub/Baka-MPlayer/src/scripts/packages')

from upv import *
plugin_manager.load_plugins(['/Users/liberize/Code/GitHub/Baka-MPlayer/src/scripts/plugins'])
media_plugins = plugin_manager.get_plugins_by_type('MediaProvider')

os.environ['TMPDIR'] = os.environ['TMPDIR'] + '/test-upv'
baidupcs = media_plugins[0]
print(baidupcs.fetch(0, 10))
