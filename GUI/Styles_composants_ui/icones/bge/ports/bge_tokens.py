#!/usr/bin/env python3
"""BGE – port Python : jetons + rendu SVG du symbole et du bureau « squelette » (pour tests visuels / outillage)."""
LIGHT = dict(fg="#17133d", mut="#5f5a88", glass="rgba(255,255,255,.34)", glass_strong="rgba(255,255,255,.55)", glass_line="rgba(255,255,255,.7)",
             hair="rgba(79,70,229,.14)", hover="rgba(79,70,229,.10)", press="rgba(79,70,229,.18)", win="rgba(255,255,255,.74)", accent="#4F46E5", err="#d6303c", ok="#1f8a57")
DARK = dict(fg="#f4f2ff", mut="#b4b0da", glass="rgba(40,32,104,.36)", glass_strong="rgba(28,22,74,.62)", glass_line="rgba(255,255,255,.2)",
            hair="rgba(255,255,255,.12)", hover="rgba(255,255,255,.10)", press="rgba(255,255,255,.17)", win="rgba(26,21,68,.8)", accent="#B79AF5", err="#ff7a84", ok="#5fd49b")
BRAND = dict(indigo="#4F46E5", violet="#8B5CF6", lavande="#C4B5FD", or_="#AB8E18", nuit="#0F0B2E")

def logo(c1=BRAND["indigo"], c2=BRAND["violet"], c3=BRAND["lavande"], c4=BRAND["or_"], size=64):
    return (f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="150 29 199 420" height="{size}">'
            f'<rect x="150" y="29" width="48" height="420" rx="24" fill="{c1}"/><path d="M248 29A101 102.5 0 0 1 248 234Z" fill="{c2}"/>'
            f'<path d="M248 245A101 102 0 0 1 248 449Z" fill="{c3}"/><rect x="316" y="221" width="33" height="36" rx="15" fill="{c4}"/></svg>')
if __name__ == "__main__": print(logo())
