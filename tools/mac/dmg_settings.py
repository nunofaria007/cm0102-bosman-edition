# dmgbuild settings for the Mac download. tools/mac/make_app.sh runs:
#   dmgbuild -s tools/mac/dmg_settings.py -D app=<.app> -D background=<.tiff> "<volume name>" <out.dmg>
# The window is 640 x 400 points with no toolbar. The positions match tools/mac/dmg_background.py.
import os.path

app = defines["app"]  # noqa: F821 (dmgbuild provides defines)
app_name = os.path.basename(app)

format = "UDZO"
filesystem = "HFS+"
files = [app]
symlinks = {"Applications": "/Applications"}
background = defines.get("background", "builtin-arrow")  # noqa: F821
icon_locations = {app_name: (170, 255), "Applications": (470, 255)}
window_rect = ((200, 120), (640, 432))  # 400 points of content plus the title bar
show_status_bar = False
show_tab_view = False
show_toolbar = False
show_pathbar = False
show_sidebar = False
default_view = "icon-view"
icon_size = 112
text_size = 13
arrange_by = None
hide_extensions = [app_name]
