"""Applications existantes (identifiants usuels, minuscules) -> icône BAURA de leur RÔLE.  Aucun logo de marque n'est reproduit :
l'icône dit ce que fait l'application ; le nom reste affiché sous l'icône. L'utilisateur peut choisir « icônes d'origine » dans Paramètres > Apparence."""
import re
_GROUPS = {
 'browser': 'firefox chromium chrome google-chrome brave vivaldi opera edge microsoft-edge epiphany falkon librewolf tor-browser midori konqueror',
 'houb': 'houb', 'mail-client': 'thunderbird geary evolution outlook kmail mutt claws-mail', 'chat': 'telegram signal discord slack element whatsapp pidgin hexchat irssi matrix mattermost teams-chat',
 'video-call': 'zoom teams skype jitsi meet webex cheese', 'rss': 'newsboat akregator liferea feedreader', 'torrent': 'transmission qbittorrent deluge ktorrent', 'cloud': 'nextcloud dropbox syncthing owncloud onedrive megasync gdrive',
 'remote': 'anydesk teamviewer remmina vnc rdesktop xfreerdp rustdesk', 'vpn': 'openvpn wireguard protonvpn nordvpn mullvad', 'network-tools': 'wireshark nmap zenmap tcpdump',
 'writer': 'libreoffice-writer abiword word lowriter', 'spreadsheet': 'libreoffice-calc gnumeric excel localc', 'presentation': 'libreoffice-impress powerpoint loimpress',
 'office-suite': 'libreoffice onlyoffice wps-office office365', 'pdf-viewer': 'evince okular zathura mupdf xpdf acrobat foxit', 'ebook': 'calibre foliate fbreader koreader',
 'database': 'sqlite-browser pgadmin dbeaver mysql-workbench sqlitebrowser', 'translator': 'dictionary goldendict translate-shell', 'finance': 'gnucash homebank kmymoney skrooge',
 'tasks': 'todoist endeavour planner super-productivity', 'education': 'anki gcompris khan tuxtyping', 'science': 'octave scilab stellarium kalzium jupyter rstudio', 'graph': 'geogebra kmplot',
 'code-editor': 'vscode code vscodium sublime-text atom gedit-code kate geany notepadqq lite-xl', 'ide': 'eclipse qtcreator kdevelop intellij pycharm clion android-studio netbeans codeblocks',
 'terminal-editor': 'vim neovim nvim emacs nano helix micro', 'git': 'git gitk gitg github-desktop lazygit meld gitkraken sourcetree', 'containers': 'docker podman lxc portainer kubernetes lens',
 'virtual-machine': 'virtualbox qemu virt-manager vmware gnome-boxes', 'debugger': 'gdb lldb valgrind', 'api-tools': 'postman insomnia',
 'photo-editor': 'gimp krita photoshop darktable rawtherapee pinta lightroom digikam', 'paint': 'mypaint kolourpaint drawing tux-paint', 'vector': 'inkscape illustrator figma sodipodi',
 '3d': 'blender freecad openscad meshlab', 'video-editor': 'kdenlive openshot shotcut davinci-resolve pitivi olive', 'audio-editor': 'audacity ardour lmms reaper tenacity',
 'screen-recorder': 'obs obs-studio simplescreenrecorder kazam peek vokoscreen', 'font-manager': 'fontforge font-viewer gucharmap', 'color-picker': 'gcolor', 'scanner': 'simple-scan xsane skanlite',
 'video-player': 'vlc mpv celluloid smplayer mplayer parole', 'music-player': 'spotify clementine amarok lollypop deadbeef audacious strawberry rhythmbox', 'streaming': 'youtube netflix twitch primevideo disney',
 'radio': 'gpodder shortwave radiotray', 'game': 'steam lutris heroic supertuxkart minetest minecraft 0ad', 'game-launcher': 'epic-games gog-galaxy itch bottles',
 'passwords': 'keepassxc bitwarden seahorse lastpass 1password', 'antivirus': 'clamtk clamav', 'firewall': 'gufw ufw firewalld', 'cleaner': 'bleachbit stacer sweeper',
 'disk-usage': 'baobab filelight ncdu qdirstat', 'backup': 'deja-dup timeshift restic borg back-in-time', 'archive': 'file-roller ark xarchiver peazip 7zip engrampa',
 'process-manager': 'htop btop top', 'hardware-info': 'hardinfo cpu-z inxi lshw', 'system-tweaks': 'dconf-editor gnome-tweaks tweaks regedit', 'accessibility': 'orca',
 'ai-assistant': 'chatgpt claude gemini copilot ollama lm-studio', 'maps': 'gnome-maps marble organicmaps', 'contacts-card': 'thunderbird-contacts',
 'files': 'nautilus dolphin thunar nemo pcmanfm caja files', 'settings': 'gnome-control-center systemsettings', 'terminal': 'gnome-terminal konsole xterm alacritty kitty wezterm tilix terminator foot',
 'monitor': 'gnome-system-monitor ksysguard', 'store': 'gnome-software discover flathub snap-store synaptic', 'calculator': 'gnome-calculator kcalc qalculate', 'calendar': 'gnome-calendar korganizer',
 'clock': 'gnome-clocks', 'notes': 'gnome-notes xpad joplin obsidian notion', 'text-editor': 'gedit gnome-text-editor mousepad leafpad notepad', 'screenshot': 'gnome-screenshot spectacle ksnip flameshot',
 'photos': 'eog gwenview shotwell ristretto feh', 'disks': 'gnome-disks partitionmanager gparted', 'printer': 'system-config-printer cups', 'bluetooth': 'blueman', 'update': 'update-manager',
 'weather': 'gnome-weather', 'recorder': 'gnome-sound-recorder', 'sound': 'pavucontrol', 'network': 'nm-applet nm-connection-editor', 'help': 'yelp khelpcenter',
}
KNOWN = {}
for icon_id, names in _GROUPS.items():
    for nm in names.split(): KNOWN.setdefault(nm, icon_id)
# catégories freedesktop (.desktop Categories=) -> icône de repli
CATEGORY_FALLBACK = {'AudioVideo': 'video-player', 'Audio': 'music-player', 'Video': 'video-player', 'Development': 'code-editor', 'Education': 'education', 'Game': 'game', 'Graphics': 'photo-editor',
  'Network': 'browser', 'Office': 'writer', 'Science': 'science', 'Settings': 'settings', 'System': 'system-tweaks', 'Utility': 'app-generic', 'WebBrowser': 'browser', 'Email': 'mail-client',
  'FileManager': 'files', 'TerminalEmulator': 'terminal', 'TextEditor': 'text-editor', 'IDE': 'ide', 'Chat': 'chat', 'InstantMessaging': 'chat', 'VideoConference': 'video-call', 'Security': 'security',
  'Archiving': 'archive', 'Calculator': 'calculator', 'Calendar': 'calendar', 'Database': 'database', 'Spreadsheet': 'spreadsheet', 'WordProcessor': 'writer', 'Presentation': 'presentation',
  'Viewer': 'pdf-viewer', 'Player': 'video-player', 'Recorder': 'screen-recorder', 'Monitor': 'monitor', 'PackageManager': 'store', 'RemoteAccess': 'remote', 'P2P': 'torrent', 'Maps': 'maps', 'Finance': 'finance'}
