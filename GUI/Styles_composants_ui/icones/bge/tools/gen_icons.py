#!/usr/bin/env python3
"""Génère include/bge/icons_data.hpp à partir de ../baura-icons.js (source de vérité des icônes).
   Usage : python3 tools/gen_icons.py   (nécessite node)"""
import json, re, subprocess, pathlib
root = pathlib.Path(__file__).resolve().parents[2]
js = "global.window={};require('%s');console.log(JSON.stringify(window.BI.paths))" % (root/'baura-icons.js')
paths = json.loads(subprocess.check_output(['node','-e',js]))
def attrs(s): return dict(re.findall(r'([a-z-]+)="([^"]*)"', s))
def f(v): return ('%.3f' % float(v)).rstrip('0').rstrip('.')
def rect(a):
    x,y,w,h=[float(a[k]) for k in('x','y','width','height')]; r=min(float(a.get('rx',0)),w/2,h/2)
    if r==0: return 'M%s %sh%sv%sh-%sz'%(f(x),f(y),f(w),f(h),f(w))
    return ('M%s %sh%sa%s %s 0 0 1 %s %sv%sa%s %s 0 0 1 -%s %sh-%sa%s %s 0 0 1 -%s -%sv-%sa%s %s 0 0 1 %s -%sz'
      %(f(x+r),f(y),f(w-2*r),f(r),f(r),f(r),f(r),f(h-2*r),f(r),f(r),f(r),f(r),f(w-2*r),f(r),f(r),f(r),f(r),f(h-2*r),f(r),f(r),f(r),f(r)))
def circle(a):
    cx,cy,r=[float(a[k]) for k in('cx','cy','r')]
    return 'M%s %sa%s %s 0 1 0 %s 0a%s %s 0 1 0 -%s 0z'%(f(cx-r),f(cy),f(r),f(r),f(2*r),f(r),f(r),f(2*r))
out=['// GÉNÉRÉ par tools/gen_icons.py depuis baura-icons.js — ne pas éditer à la main.',
     '#pragma once','#include <cstddef>','namespace bge::icons {',
     'struct Elem { const char* d; bool fill; };','struct Icon { const char* name; const Elem* elems; std::size_t n; };']
tab=[]
for name,svg in paths.items():
    els=[]
    for m in re.finditer(r'<(path|rect|circle)\s([^>]*?)/?>',svg):
        a=attrs(m.group(2)); d={'path':lambda a:a['d'],'rect':rect,'circle':circle}[m.group(1)](a)
        els.append('{"%s",%s}'%(d,'true' if a.get('fill')=='currentColor' else 'false'))
    out.append('static const Elem e_%s[] = {%s};'%(name,','.join(els)))
    tab.append('{"%s",e_%s,%d}'%(name,name,len(els)))
out.append('static const Icon ALL[] = {\n  '+',\n  '.join(tab)+'\n};')
out.append('static const std::size_t COUNT = %d;'%len(tab)); out.append('}')
(root/'bge/include/bge/icons_data.hpp').write_text('\n'.join(out)+'\n')
print(len(tab),'icônes générées')
