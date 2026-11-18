/* ===== BAURA OS – composants système : menu contextuel, boîtes de dialogue, recherche, vue des tâches, calendrier, réglages rapides ===== */
const esc=s=>String(s).replace(/[&<>"]/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;'}[c]));
const IC={
  open:'<path d="M5 12h14M13 6l6 6-6 6"/>', rename:'<path d="M4 20h4L19 9l-4-4L4 16z"/><path d="M13.5 6.5l4 4"/>',
  trash:'<path d="M4 7h16M9 7V4h6v3M6 7l1 13h10l1-13M10 11v6M14 11v6"/>', restore:'<path d="M3 12a9 9 0 1 0 3-6.7"/><path d="M3 4v5h5"/>',
  folder:'<path d="M3 7a2 2 0 0 1 2-2h4l2 2h8a2 2 0 0 1 2 2v8a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2z"/>', file:'<path d="M7 3h7l4 4v14H7z"/><path d="M14 3v4h4"/>',
  refresh:'<path d="M21 12a9 9 0 1 1-3-6.7"/><path d="M21 3v5h-5"/>', image:'<rect x="3" y="4" width="18" height="16" rx="3"/><circle cx="9" cy="10" r="1.6"/><path d="M3 17l5-5 4 4 3-3 6 6"/>',
  grid:'<rect x="4" y="4" width="7" height="7" rx="2"/><rect x="13" y="4" width="7" height="7" rx="2"/><rect x="4" y="13" width="7" height="7" rx="2"/><rect x="13" y="13" width="7" height="7" rx="2"/>',
  gear:'<circle cx="12" cy="12" r="3.4"/><path d="M12 3v3M12 18v3M3 12h3M18 12h3M5.6 5.6l2.1 2.1M16.3 16.3l2.1 2.1M18.4 5.6l-2.1 2.1M7.7 16.3l-2.1 2.1"/>',
  pin:'<path d="M9 4h6l-1 6 3 3H7l3-3z"/><path d="M12 13v7"/>', min:'<path d="M5 12h14"/>', max:'<rect x="5" y="5" width="14" height="14" rx="3"/>', x:'<path d="M6 6l12 12M18 6L6 18"/>',
  lock:'<rect x="5" y="11" width="14" height="9" rx="3"/><path d="M8.5 11V8a3.5 3.5 0 0 1 7 0v3"/>', moon:'<path d="M20 14.5A8 8 0 0 1 9.5 4a8 8 0 1 0 10.5 10.5z"/>',
  power:'<path d="M12 3v8"/><path d="M6.3 6.8a8 8 0 1 0 11.4 0"/>', search:'<circle cx="11" cy="11" r="7"/><path d="M20 20l-3.5-3.5"/>', calc:'<rect x="5" y="3" width="14" height="18" rx="3"/><path d="M8 8h8M8 12h2M14 12h2M8 16h2M14 16h2"/>',
  sun:'<circle cx="12" cy="12" r="4"/><path d="M12 3v2M12 19v2M3 12h2M19 12h2M5.6 5.6L7 7M17 17l1.4 1.4M18.4 5.6L17 7M7 17l-1.4 1.4"/>',
  wifi:'<path d="M2.5 9a14 14 0 0 1 19 0"/><path d="M5.7 12.4a9.5 9.5 0 0 1 12.6 0"/><path d="M9 15.7a5 5 0 0 1 6 0"/><circle cx="12" cy="19" r=".8" fill="currentColor"/>',
  bt:'<path d="M7 7l10 10-5 4V3l5 4L7 17"/>', plane:'<path d="M2 14l8-3V5a2 2 0 0 1 4 0v6l8 3v2l-8-1.5V19l2 1.5V22l-4-1-4 1v-1.5l2-1.5v-4.5L2 16z"/>',
  shield:'<path d="M12 3l7 3v5.5c0 4.3-2.9 7.6-7 9.5-4.1-1.9-7-5.2-7-9.5V6z"/><path d="M9 12l2.2 2.2L15.5 10"/>', download:'<path d="M12 4v11M7 11l5 5 5-5M5 20h14"/>', store:'<path d="M4 9l1.5-5h13L20 9M4 9v10h16V9M4 9h16M9 14h6"/>',
  leaf:'<path d="M5 19c0-8 5-14 15-14 0 10-6 15-14 15"/><path d="M5 19c2-5 5-8 9-10"/>', globe:'<circle cx="12" cy="12" r="9"/><path d="M3 12h18M12 3c3.2 3 3.2 15 0 18M12 3c-3.2 3-3.2 15 0 18"/>'
};
const svgi=(d,c)=>`<svg viewBox="0 0 24 24"${c?` class="${c}"`:''}>${d}</svg>`;

/* ---------- Types de fichiers → application + icône ---------- */
const FT_EXT={txt:'notes',md:'notes',log:'notes',skrib:'skrib',numbro:'numbro',wepoint:'wepoint',data:'data',crea:'crea',
  clip:'videos',mp4:'videos',webm:'videos',mov:'videos',mkv:'videos',mp3:'musique',wav:'musique',ogg:'musique',
  svg:'photos',png:'photos',jpg:'photos',jpeg:'photos',webp:'photos',gif:'photos'};
const FT_LABEL={notes:'Document texte',skrib:'Document Skrib',numbro:'Classeur Numbro',wepoint:'Présentation WePoint',data:'Base de données Data',crea:'Création Crea',videos:'Vidéo',musique:'Audio',photos:'Image'};
function fileKind(f){
  const e=FS.ext(f.name||'');
  if(f.type==='folder') return {app:'fichiers',g:G('#6D66F2','#4F46E5'),d:IC.folder,label:'Dossier'};
  let app=FT_EXT[e]; if(!app&&f.kind==='image') app='photos'; if(!app&&f.kind==='audio') app='musique'; if(!app&&f.kind==='video') app='videos';
  if(app&&byId[app]) return {app,g:byId[app].g,d:byId[app].d,label:FT_LABEL[app]||'Fichier'};
  if(e==='pdf') return {app:null,g:G('#F87171','#DC2626'),d:IC.file,label:'Document PDF'};
  return {app:null,g:G('#94A3C4','#5B6794'),d:IC.file,label:'Fichier'};
}
function fileIco(f,s){ const k=fileKind(f); return icoHTML(k.g,k.d,s) }
function openItem(id){
  const f=FS.get(id); if(!f) return;
  if(FS.inTrash(id)&&f.type==='file'){ Dlg.alert({title:f.name,text:'Ce fichier est dans la corbeille. Restaure-le pour l\'ouvrir.'}); return }
  if(f.type==='folder'){ openApp('fichiers',{folder:id}); return }
  const k=fileKind(f);
  if(k.app==='musique') openApp('musique',{track:f.track||0,play:true});
  else if(k.app) openApp(k.app,{file:id});
  else Dlg.alert({title:f.name,text:'Aucun aperçu disponible pour ce type de fichier.'});
}

/* ---------- Menu contextuel ---------- */
const Ctx=(function(){
  let el=null, items=[];
  function build(){
    el=document.createElement('div'); el.className='panel ctx'; el.setAttribute('role','menu'); document.body.appendChild(el);
    el.addEventListener('click',e=>{ const b=e.target.closest('.ci'); if(!b||b.disabled) return; const it=items[+b.dataset.i]; hide(); it&&it.run&&it.run() });
    el.addEventListener('contextmenu',e=>e.preventDefault());
    el.addEventListener('keydown',e=>{
      const bs=[...el.querySelectorAll('.ci:not([disabled])')], i=bs.indexOf(document.activeElement);
      if(e.key==='ArrowDown'){e.preventDefault();(bs[i+1]||bs[0]).focus()} else if(e.key==='ArrowUp'){e.preventDefault();(bs[i-1]||bs[bs.length-1]).focus()}
      else if(e.key==='Escape'){hide()}
    });
  }
  function show(list,x,y){
    if(!el) build(); if(typeof closePanels==='function') closePanels();
    items=list.filter(Boolean);
    el.innerHTML=items.map((it,i)=>it==='sep'?'<div class="csep"></div>':
      `<button class="ci${it.danger?' danger':''}" data-i="${i}" role="menuitem"${it.disabled?' disabled':''}>${it.icon?svgi(it.icon):'<i></i>'}<span>${esc(it.label)}</span>${it.kbd?`<kbd>${esc(it.kbd)}</kbd>`:''}</button>`).join('');
    el.style.left='0px'; el.style.top='0px'; el.classList.add('show');
    const r=el.getBoundingClientRect();
    el.style.left=Math.max(8,Math.min(x,innerWidth-r.width-8))+'px'; el.style.top=Math.max(8,Math.min(y,innerHeight-r.height-8))+'px';
  }
  function hide(){ if(el) el.classList.remove('show') }
  document.addEventListener('pointerdown',e=>{ if(el&&el.classList.contains('show')&&!el.contains(e.target)) hide() },true);
  addEventListener('blur',hide); addEventListener('resize',hide);
  addEventListener('keydown',e=>{ if(e.key==='Escape'&&el&&el.classList.contains('show')){ hide(); e.stopPropagation() } },true);
  return {show,hide,get isOpen(){return !!el&&el.classList.contains('show')}};
})();
document.addEventListener('contextmenu',e=>{ // pas de menu du navigateur sur l'interface (sauf champs de saisie)
  if(!e.target.closest('input,textarea,[contenteditable]')) e.preventDefault();
});

/* ---------- Boîtes de dialogue ---------- */
const Dlg=(function(){
  function open(o){
    return new Promise(res=>{
      const prev=document.activeElement;
      const sc=document.createElement('div'); sc.className='dscrim';
      sc.innerHTML=`<div class="dlg" role="dialog" aria-modal="true" aria-label="${esc(o.title||'')}"><h3>${esc(o.title||'')}</h3>${o.text?`<p>${esc(o.text)}</p>`:''}${o.input?'<input class="dinp" spellcheck="false" autocomplete="off">':''}
        <div class="dbtns">${o.hideCancel?'':`<button class="ghostb" data-r="0">${esc(o.cancel||'Annuler')}</button>`}<button class="primb${o.danger?' dng':''}" data-r="1">${esc(o.ok||'OK')}</button></div></div>`;
      document.body.appendChild(sc);
      const inp=sc.querySelector('.dinp'); if(inp) inp.value=o.value||'';
      const done=v=>{ sc.classList.add('out'); setTimeout(()=>sc.remove(),180); removeEventListener('keydown',key,true); try{prev&&prev.focus&&prev.focus()}catch(e){} res(v) };
      const ok=()=>done(o.input?inp.value:true), no=()=>done(o.input?null:false);
      const key=e=>{ if(e.key==='Escape'){e.preventDefault();e.stopPropagation();no()} else if(e.key==='Enter'&&!e.target.closest('.ghostb')){e.preventDefault();e.stopPropagation();ok()} };
      addEventListener('keydown',key,true);
      sc.addEventListener('pointerdown',e=>{ if(e.target===sc&&!o.hideCancel) no() });
      sc.querySelectorAll('[data-r]').forEach(b=>b.onclick=()=>b.dataset.r==='1'?ok():no());
      requestAnimationFrame(()=>{ sc.classList.add('on'); const f=inp||sc.querySelector('.primb'); f.focus(); if(inp) inp.select() });
    });
  }
  return {confirm:o=>open(o), prompt:o=>open(Object.assign({input:true},o)), alert:o=>open(Object.assign({hideCancel:true},o))};
})();

/* ---------- Calcul sûr (sans eval) ---------- */
function safeEval(src){
  const s=src.replace(/×|x(?=\s*[\d(])/gi,'*').replace(/÷/g,'/').replace(/,/g,'.').replace(/\s+/g,'');
  let i=0;
  const peek=()=>s[i];
  function num(){ const m=/^\d*\.?\d+(e[+-]?\d+)?/i.exec(s.slice(i)); if(!m) throw 0; i+=m[0].length; return parseFloat(m[0]) }
  function atom(){
    if(peek()==='('){ i++; const v=expr(); if(peek()!==')') throw 0; i++; return post(v) }
    if(peek()==='-'){ i++; return -atom() } if(peek()==='+'){ i++; return atom() }
    return post(num());
  }
  function post(v){ while(peek()==='%'){ i++; v=v/100 } return v }
  function pow(){ let v=atom(); while(peek()==='^'){ i++; v=Math.pow(v,atom()) } return v }
  function term(){ let v=pow(); while(peek()==='*'||peek()==='/'){ const o=s[i++]; const r=pow(); v=o==='*'?v*r:v/r } return v }
  function expr(){ let v=term(); while(peek()==='+'||peek()==='-'){ const o=s[i++]; const r=term(); v=o==='+'?v+r:v-r } return v }
  const v=expr(); if(i!==s.length||!isFinite(v)) throw 0; return v;
}

/* ---------- Recherche globale (Ctrl+Espace) ---------- */
const Spot=(function(){
  let box=null, input, list, flat=[], sel=0, isOpen=false;
  const ACTIONS=()=>[
    {t:'Verrouiller',s:'Action système',i:IC.lock,run:()=>power('lock')},{t:'Mettre en veille',s:'Action système',i:IC.moon,run:()=>power('sleep')},
    {t:'Redémarrer',s:'Action système',i:IC.refresh,run:()=>power('restart')},{t:'Éteindre',s:'Action système',i:IC.power,run:()=>power('shutdown')},
    {t:'Vue des tâches',s:'Fenêtres',i:IC.grid,run:()=>TaskView.open()},
    {t:'Thème sombre',s:'Réglage',i:IC.moon,run:()=>Baura.settings.set({theme:'dark'})},{t:'Thème clair',s:'Réglage',i:IC.sun,run:()=>Baura.settings.set({theme:'light'})},
    {t:'Thème automatique',s:'Réglage',i:IC.sun,run:()=>Baura.settings.set({theme:'auto'})},
    {t:'Changer le fond d\'écran',s:'Réglage',i:IC.image,run:()=>openApp('params',{section:'perso'})},
    {t:'Nouvelle note',s:'Action',i:byId.notes.d,run:()=>{const id=FS.create({name:'Note.txt',type:'file',parent:'documents',content:''});openApp('notes',{file:id})}},
    {t:'Ouvrir la corbeille',s:'Fichiers',i:IC.trash,run:()=>openApp('fichiers',{folder:'trash'})},
    {t:'Vider la corbeille',s:'Fichiers',i:IC.trash,run:()=>emptyTrashAsk()},
  ];
  function build(){
    box=document.createElement('div'); box.className='spscrim'; box.hidden=true;
    box.innerHTML='<div class="spbox" role="dialog" aria-label="Recherche"><div class="spin">'+svgi(IC.search)+'<input id="spq" placeholder="Rechercher des applis, fichiers, réglages… ou calculer" autocomplete="off" spellcheck="false" aria-label="Rechercher"><kbd>Échap</kbd></div><div class="splist" id="splist" role="listbox"></div></div>';
    document.body.appendChild(box); input=box.querySelector('#spq'); list=box.querySelector('#splist');
    box.addEventListener('pointerdown',e=>{ if(e.target===box) close() });
    input.addEventListener('input',render);
    input.addEventListener('keydown',e=>{
      if(e.key==='ArrowDown'){e.preventDefault();sel=Math.min(flat.length-1,sel+1);paintSel()} else if(e.key==='ArrowUp'){e.preventDefault();sel=Math.max(0,sel-1);paintSel()}
      else if(e.key==='Enter'){e.preventDefault();run(sel)} else if(e.key==='Escape'){e.preventDefault();close()}
    });
    list.addEventListener('click',e=>{ const r=e.target.closest('.spr'); if(r) run(+r.dataset.i) });
    list.addEventListener('mousemove',e=>{ const r=e.target.closest('.spr'); if(r&&+r.dataset.i!==sel){ sel=+r.dataset.i; paintSel() } });
  }
  function gather(q){
    const ql=q.toLowerCase().trim(), out=[];
    if(!ql){
      out.push(...APPS.slice(0,6).map(a=>({g:'Applications',t:a.n,s:DESC[a.id],ico:appIco(a,34),run:()=>openApp(a.id)})));
      out.push(...ACTIONS().slice(0,4).map(a=>({g:'Actions',t:a.t,s:a.s,ico:icoHTML(G('#94A3C4','#5B6794'),a.i,34),run:a.run})));
      return out;
    }
    if(/^[\d\s+\-*/().,%^x×÷e]+$/i.test(q)&&/[+\-*/%^×÷x]/i.test(q.replace(/^\s*-/,''))){
      try{ const v=safeEval(q); const txt=String(+v.toPrecision(12)).replace('.',',');
        out.push({g:'Calcul',t:'= '+txt,s:q.trim()+' · Entrée pour copier',ico:icoHTML(G('#A78BFA','#6D28D9'),IC.calc,34),run:()=>{ try{navigator.clipboard.writeText(txt.replace(',','.'))}catch(e){}; notify({app:'calc',title:'Résultat copié',text:q.trim()+' = '+txt}) }}) }catch(e){}
    }
    APPS.filter(a=>a.n.toLowerCase().includes(ql)||DESC[a.id].toLowerCase().includes(ql)).forEach(a=>out.push({g:'Applications',t:a.n,s:DESC[a.id],ico:appIco(a,34),run:()=>openApp(a.id)}));
    FS.search(ql).slice(0,6).forEach(f=>out.push({g:'Fichiers',t:f.name,s:FS.path(f.parent).map(p=>p.name).join(' › ')||'BAURA',ico:fileIco(f,34),run:()=>openItem(f.id)}));
    ACTIONS().filter(a=>a.t.toLowerCase().includes(ql)).forEach(a=>out.push({g:'Actions & réglages',t:a.t,s:a.s,ico:icoHTML(G('#94A3C4','#5B6794'),a.i,34),run:a.run}));
    out.push({g:'Web',t:'Rechercher « '+q.trim()+' » sur le web',s:'S\'ouvre dans un nouvel onglet',ico:icoHTML(G('#38BDF8','#4F46E5'),IC.globe,34),run:()=>window.open('https://duckduckgo.com/?q='+encodeURIComponent(q.trim()),'_blank','noopener')});
    return out;
  }
  function render(){
    flat=gather(input.value); sel=0; let g='', h='';
    flat.forEach((r,i)=>{ if(r.g!==g){ g=r.g; h+=`<div class="spg">${esc(g)}</div>` }
      h+=`<button class="spr${i===0?' on':''}" role="option" data-i="${i}">${r.ico}<span class="tx"><b>${esc(r.t)}</b><small>${esc(r.s||'')}</small></span><kbd>↵</kbd></button>` });
    list.innerHTML=h;
  }
  function paintSel(){ list.querySelectorAll('.spr').forEach((r,i)=>r.classList.toggle('on',i===sel)); const e=list.querySelector('.spr.on'); if(e) e.scrollIntoView({block:'nearest'}) }
  function run(i){ const r=flat[i]; if(!r) return; close(); setTimeout(()=>r.run(),60) }
  function open(){ if(!box) build(); if(typeof closePanels==='function') closePanels(); Ctx.hide(); box.hidden=false; isOpen=true; input.value=''; render(); requestAnimationFrame(()=>{box.classList.add('on');input.focus()}) }
  function close(){ if(!box||!isOpen) return; isOpen=false; box.classList.remove('on'); setTimeout(()=>{box.hidden=true},180) }
  return {open,close,toggle:()=>isOpen?close():open(),get isOpen(){return isOpen}};
})();

/* ---------- Vue des tâches (Alt+W) ---------- */
const TaskView=(function(){
  let el=null, ids=[], sel=0, open=false, byKey=false;
  function list(){ return [...wins.entries()].sort((a,b)=>b[1].el.style.zIndex-a[1].el.style.zIndex).map(e=>e[0]) }
  function build(){ el=document.createElement('div'); el.className='tvscrim'; el.hidden=true; document.body.appendChild(el);
    el.addEventListener('pointerdown',e=>{ if(e.target===el) close(false) });
    el.addEventListener('click',e=>{
      const x=e.target.closest('.tvx'); if(x){ closeApp(x.dataset.id); render(); if(!ids.length) close(false); return }
      const c=e.target.closest('.tvc'); if(c){ select(c.dataset.id) } });
  }
  function render(){
    ids=list(); if(sel>=ids.length) sel=Math.max(0,ids.length-1);
    el.innerHTML=ids.length?`<div class="tvgrid">${ids.map((id,i)=>{const a=byId[id],w=wins.get(id);
      return `<div class="tvc${i===sel?' on':''}" data-id="${id}" tabindex="0"><button class="tvx" data-id="${id}" aria-label="Fermer ${esc(a.n)}">${svgi(IC.x)}</button>${appIco(a,64)}<b>${esc(w.el.querySelector('.wname').textContent)}</b><small>${w.minimized?'Réduite':'Ouverte'}</small></div>`}).join('')}</div>`
      :'<div class="tvempty">Aucune fenêtre ouverte</div>';
  }
  function select(id){ close(false); focusWin(id) }
  function openIt(viaKey){ if(!el) build(); if(typeof closePanels==='function') closePanels(); Ctx.hide(); open=true; byKey=!!viaKey; sel=ids.length>1?1:0; ids=list(); sel=ids.length>1?1:0; el.hidden=false; render(); requestAnimationFrame(()=>el.classList.add('on')) }
  function close(confirm){ if(!open) return; open=false; if(confirm&&ids[sel]) focusWin(ids[sel]); el.classList.remove('on'); setTimeout(()=>{el.hidden=true},160) }
  function cycle(dir){ if(!ids.length) return; sel=(sel+(dir||1)+ids.length)%ids.length; el.querySelectorAll('.tvc').forEach((c,i)=>c.classList.toggle('on',i===sel)) }
  addEventListener('keydown',e=>{
    if(e.altKey&&!e.ctrlKey&&!e.metaKey&&(e.key==='w'||e.key==='W'||e.key==='∑')){ e.preventDefault(); if(!open) openIt(true); else cycle(e.shiftKey?-1:1) }
    else if(open&&e.key==='Escape'){ e.preventDefault(); close(false) }
    else if(open&&(e.key==='ArrowRight'||e.key==='ArrowDown')){ e.preventDefault(); cycle(1) }
    else if(open&&(e.key==='ArrowLeft'||e.key==='ArrowUp')){ e.preventDefault(); cycle(-1) }
    else if(open&&e.key==='Enter'){ e.preventDefault(); close(true) }
  });
  addEventListener('keyup',e=>{ if(open&&byKey&&e.key==='Alt') close(true) });
  return {open:()=>openIt(false),close,get isOpen(){return open}};
})();

/* ---------- Corbeille ---------- */
async function emptyTrashAsk(){
  const n=FS.trashCount(); if(!n){ Dlg.alert({title:'Corbeille',text:'La corbeille est déjà vide.'}); return }
  if(await Dlg.confirm({title:'Vider la corbeille ?',text:`${n} élément${n>1?'s seront supprimés':' sera supprimé'} définitivement.`,ok:'Vider',danger:true})) FS.emptyTrash();
}
function trashItem(id){ FS.trash(id) }
/* Types de documents que l'on peut créer (clic droit → Nouveau…) : [nom par défaut, libellé, contenu initial] */
const NEWT={file:['Nouveau document.txt','Document texte',''],skrib:['Document.skrib','Document Skrib',''],numbro:['Classeur.numbro','Classeur Numbro',''],
  wepoint:['Présentation.wepoint','Présentation WePoint',''],data:['Base.data','Base de données Data',''],crea:['Création.crea','Création Crea','']};
async function newItem(parent,kind){
  const isF=kind==='folder', T=NEWT[kind]||NEWT.file;
  const name=await Dlg.prompt({title:isF?'Nouveau dossier':'Nouveau — '+T[1],text:'Donne un nom :',value:isF?'Nouveau dossier':T[0],ok:'Créer'});
  if(!name||!name.trim()) return null;
  if(isF) return FS.create({name:name.trim(),type:'folder',parent});
  const ext=FS.ext(T[0]), n=name.trim();
  return FS.create({name:/\.[a-z0-9]+$/i.test(n)?n:n+'.'+ext,type:'file',parent,content:T[2]});
}
function newMenu(parent,after){ return Object.keys(NEWT).map(k=>({label:NEWT[k][1],icon:k==='file'?IC.file:byId[FT_EXT[FS.ext(NEWT[k][0])]].d,run:async()=>{const id=await newItem(parent,k); if(id&&after) after(id)}})) }
async function renameItem(id){
  const f=FS.get(id); if(!f) return;
  const n=await Dlg.prompt({title:'Renommer',text:'Nouveau nom :',value:f.name,ok:'Renommer'});
  if(n&&n.trim()) FS.rename(id,n);
}

/* ---------- Panneaux de la barre : calendrier & réglages rapides ---------- */
window.extraPanels=[];
function makePanel(name,id,cls){
  const el=document.createElement('section'); el.className='panel pop '+(cls||''); el.id=id; el.setAttribute('aria-label',name); document.body.appendChild(el);
  const api={el,name,show(){ closePanels(name); el.classList.add('show'); openPanel=name; body.classList.add('pop-open') },hide(){ el.classList.remove('show') },
    toggle(){ el.classList.contains('show')?closePanels():api.show() }};
  extraPanels.push(api); return api;
}

const Cal=(function(){
  const p=makePanel('calendrier','calPop','calp'); let y,m;
  const MONTHS=['janvier','février','mars','avril','mai','juin','juillet','août','septembre','octobre','novembre','décembre'];
  function render(){
    const now=new Date(), first=new Date(y,m,1), start=(first.getDay()+6)%7, days=new Date(y,m+1,0).getDate(), prevDays=new Date(y,m,0).getDate();
    let cells='';
    for(let i=0;i<42;i++){
      let d=i-start+1, cls='', yy=y, mm=m;
      if(d<1){ d=prevDays+d; cls='o'; mm=m-1 } else if(d>days){ d=d-days; cls='o'; mm=m+1 }
      const t=new Date(yy,mm,d); if(t.toDateString()===now.toDateString()) cls+=' today';
      cells+=`<button class="cd ${cls}" data-d="${t.getFullYear()}-${t.getMonth()}-${t.getDate()}">${d}</button>`;
    }
    const dayName=new Intl.DateTimeFormat(locale,{weekday:'long'}).format(now);
    p.el.innerHTML=`<div class="calhead"><div><b>${dayName.charAt(0).toUpperCase()+dayName.slice(1)}</b><span>${new Intl.DateTimeFormat(locale,{day:'numeric',month:'long',year:'numeric'}).format(now)}</span></div></div>
      <div class="calnav"><button id="cPrev" aria-label="Mois précédent">${svgi('<path d="M14 6l-6 6 6 6"/>')}</button><b>${MONTHS[m].charAt(0).toUpperCase()+MONTHS[m].slice(1)} ${y}</b><button id="cNext" aria-label="Mois suivant">${svgi('<path d="M10 6l6 6-6 6"/>')}</button></div>
      <div class="calg">${['L','M','M','J','V','S','D'].map(d=>`<i>${d}</i>`).join('')}${cells}</div>
      <button class="calnow" id="cNow">Aujourd'hui</button>`;
    p.el.querySelector('#cPrev').onclick=()=>{m--;if(m<0){m=11;y--}render()};
    p.el.querySelector('#cNext').onclick=()=>{m++;if(m>11){m=0;y++}render()};
    p.el.querySelector('#cNow').onclick=()=>{const n=new Date();y=n.getFullYear();m=n.getMonth();render()};
    p.el.querySelectorAll('.cd').forEach(b=>b.onclick=()=>{ p.el.querySelectorAll('.cd').forEach(x=>x.classList.remove('sel')); b.classList.add('sel') });
  }
  const _show=p.show; p.show=function(){ const n=new Date(); y=n.getFullYear(); m=n.getMonth(); render(); _show() };
  $('#clock').addEventListener('click',e=>{ e.stopPropagation(); p.toggle() });
  return p;
})();

const QS={wifi:true,bt:true,air:false,saver:false};
window.qsState=QS;
const Quick=(function(){
  const p=makePanel('réglages rapides','qsPop','qsp');
  function tile(id,label,ic,on){ return `<button class="qt${on?' on':''}" data-q="${id}" aria-pressed="${!!on}"><span class="qic">${svgi(ic)}</span><span>${label}</span></button>` }
  function render(){
    const s=Baura.settings.get(), dark=Baura.settings.isDark(s);
    p.el.innerHTML=`<div class="qgrid">
      ${tile('wifi','Wi‑Fi',IC.wifi,QS.wifi&&!QS.air)}${tile('bt','Bluetooth',IC.bt,QS.bt&&!QS.air)}${tile('air','Mode avion',IC.plane,QS.air)}
      ${tile('night','Veilleuse',IC.moon,s.night)}${tile('saver','Économie',IC.leaf,s.saver)}${tile('theme',dark?'Thème sombre':'Thème clair',dark?IC.moon:IC.sun,dark)}</div>
      <div class="qsl"><span>${svgi(IC.sun)}</span><input type="range" id="qBright" min="20" max="100" value="${s.bright}" aria-label="Luminosité"></div>
      <div class="qsl"><span id="qVolIc">${svgi(SPK+WAVE1+WAVE2)}</span><input type="range" id="qVol" min="0" max="100" value="${muted?0:vol}" aria-label="Volume"></div>
      <div class="qfoot"><span id="qBatt">${$('#pct').textContent}${$('#batt').classList.contains('chg')?' · en charge':''}</span><button class="fbtn" id="qSet" aria-label="Paramètres">${svgi(IC.gear)}</button></div>`;
    paintRange(p.el.querySelector('#qBright')); paintRange(p.el.querySelector('#qVol'));
  }
  p.el.addEventListener('click',e=>{
    const t=e.target.closest('.qt'); if(t){
      const k=t.dataset.q, s=Baura.settings.get();
      if(k==='wifi'){ if(QS.air){QS.air=false} QS.wifi=!QS.wifi; netChanged() } else if(k==='bt'){ if(QS.air) QS.air=false; QS.bt=!QS.bt }
      else if(k==='air'){ QS.air=!QS.air; netChanged() }
      else if(k==='night') Baura.settings.set({night:!s.night});
      else if(k==='saver') Baura.settings.set({saver:!s.saver});
      else if(k==='theme') Baura.settings.set({theme:Baura.settings.isDark(s)?'light':'dark'});
      render(); return }
    if(e.target.closest('#qSet')){ closePanels(); openApp('params') }
  });
  p.el.addEventListener('input',e=>{
    if(e.target.id==='qBright'){ Baura.settings.set({bright:+e.target.value}); paintRange(e.target) }
    else if(e.target.id==='qVol'){ muted=false; setVol(+e.target.value,true); paintRange(e.target) }
  });
  const _show=p.show; p.show=function(){ render(); _show() };
  [$('#net'),$('#batt')].forEach(b=>b.addEventListener('click',e=>{ e.stopPropagation(); p.toggle() }));
  Baura.on('settings',()=>{ if(p.el.classList.contains('show')) render() });
  return p;
})();
function netChanged(){ setNet(); notify({app:'web',title:QS.air?'Mode avion activé':(QS.wifi?'Wi‑Fi activé':'Wi‑Fi désactivé'),text:QS.air?'Les connexions sans fil sont coupées.':(QS.wifi?'Tu es de nouveau connecté.':'Tu n\'es plus connecté au réseau.')}) }
