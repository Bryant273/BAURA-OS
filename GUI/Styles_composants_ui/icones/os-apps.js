/* ===== BAURA OS – applications : Fichiers, Notes, Calculatrice, Terminal, Photos, Musique, Navigateur ===== */
const fmtDate=ts=>{ const d=new Date(ts), n=new Date(); const same=d.toDateString()===n.toDateString();
  return same?'Aujourd\'hui '+d.toLocaleTimeString('fr-FR',{hour:'2-digit',minute:'2-digit'}):d.toLocaleDateString('fr-FR',{day:'numeric',month:'short',year:d.getFullYear()===n.getFullYear()?undefined:'numeric'}) };
const fmtSize=f=>{ if(f.type==='folder') return '—'; if(f.size) return f.size; const n=(f.content||'').length; return n?(n<1024?n+' o':(n/1024).toFixed(1).replace('.',',')+' Ko'):(f.src?'Image':'0 o') };

/* ====================== FICHIERS ====================== */
AppViews.fichiers=function(root,ctx){
  const PLACES=[['bureau','Bureau',IC.grid],['documents','Documents',IC.file],['images','Images',IC.image],['musique','Musique',byId.musique.d],['telechargements','Téléchargements','<path d="M12 4v11M7 11l5 5 5-5M5 20h14"/>'],['trash','Corbeille',IC.trash]];
  const DISK='<rect x="3" y="8" width="18" height="8" rx="3"/><path d="M7 12h.01M11 12h6"/>';
  const places=()=>PLACES.slice(0,5).concat([['root','Disque local (C:)',DISK]],FS.get('drive-d')?[['drive-d',FS.get('drive-d').name+' (D:)',DISK]]:[],[PLACES[5]]);
  let cwd='documents', hist=['documents'], hi=0, sel=new Set(), view=Baura.store.get('filesView','grid'), q='';
  root.innerHTML=`<div class="fm"><aside class="fmside" aria-label="Emplacements"></aside><section class="fmmain">
    <div class="fmbar"><button class="tb" data-a="back" aria-label="Précédent">${svgi('<path d="M14 6l-6 6 6 6"/>')}</button><button class="tb" data-a="fwd" aria-label="Suivant">${svgi('<path d="M10 6l6 6-6 6"/>')}</button><button class="tb" data-a="up" aria-label="Dossier parent">${svgi('<path d="M6 14l6-6 6 6"/>')}</button>
      <div class="crumbs" aria-label="Chemin"></div>
      <label class="fmsearch">${svgi(IC.search)}<input placeholder="Filtrer" aria-label="Filtrer"></label>
      <button class="tb" data-a="view" aria-label="Changer l'affichage"></button>
      <button class="tb txt" data-a="nfolder">${svgi('<path d="M12 8v8M8 12h8"/>')}Dossier</button><button class="tb txt" data-a="nfile">${svgi('<path d="M12 8v8M8 12h8"/>')}Fichier</button><button class="tb txt dng" data-a="empty" hidden>Vider</button></div>
    <div class="fmlist" tabindex="0" role="listbox" aria-multiselectable="true"></div><div class="fmstat"></div></section></div>`;
  const side=root.querySelector('.fmside'), crumbs=root.querySelector('.crumbs'), listEl=root.querySelector('.fmlist'), stat=root.querySelector('.fmstat'), search=root.querySelector('.fmsearch input');

  function go(id,push=true){
    if(!FS.get(id)) id='documents'; cwd=id; sel.clear(); q=''; search.value='';
    if(push){ hist=hist.slice(0,hi+1); hist.push(id); hi=hist.length-1 }
    render();
  }
  function renderSide(){
    side.innerHTML=places().map(([id,n,d])=>`<button class="pl${(cwd===id)?' on':''}${id==='trash'&&FS.trashCount()?' full':''}" data-p="${id}" data-drop="${id}">${svgi(d)}<span>${n}</span>${id==='trash'&&FS.trashCount()?`<em>${FS.trashCount()}</em>`:''}</button>`).join('');
  }
  function renderBar(){
    const path=FS.path(cwd); const dl=FS.drive(cwd); const parts=[{id:dl==='D'?'drive-d':'root',name:dl+':'},...path];
    crumbs.innerHTML=parts.map((p,i)=>`<button data-c="${p.id}"${i===parts.length-1?' class="cur"':''}>${esc(p.name)}</button>`).join('<i>›</i>');
    const t=cwd==='trash'||FS.inTrash(cwd);
    root.querySelector('[data-a=back]').disabled=hi<=0; root.querySelector('[data-a=fwd]').disabled=hi>=hist.length-1; root.querySelector('[data-a=up]').disabled=cwd==='root'||cwd==='drive-d';
    root.querySelector('[data-a=nfolder]').hidden=t; root.querySelector('[data-a=nfile]').hidden=t; root.querySelector('[data-a=empty]').hidden=!(cwd==='trash');
    root.querySelector('[data-a=view]').innerHTML=svgi(view==='grid'?'<path d="M5 7h14M5 12h14M5 17h14"/>':IC.grid);
  }
  function itemsNow(){
    let a=FS.list(cwd); if(cwd==='root') a=a.filter(i=>i.id!=='trash'); if(q) a=a.filter(i=>i.name.toLowerCase().includes(q.toLowerCase())); return a;
  }
  function render(){
    renderSide(); renderBar(); const a=itemsNow(); ctx.setTitle((cwd==='root'?'Disque local (C:)':FS.get(cwd)?.name)||'Fichiers');
    if(!a.length){ listEl.className='fmlist empty'; listEl.innerHTML=`<div class="fmempty">${svgi(cwd==='trash'?IC.trash:IC.folder)}<b>${q?'Aucun résultat':cwd==='trash'?'La corbeille est vide':'Ce dossier est vide'}</b><span>${q?'Essaie un autre nom.':cwd==='trash'?'Les éléments supprimés apparaissent ici.':'Clique droit pour créer un élément.'}</span></div>` }
    else if(view==='grid'){ listEl.className='fmlist grid'; listEl.innerHTML=a.map(f=>`<div class="fi${sel.has(f.id)?' sel':''}" role="option" aria-selected="${sel.has(f.id)}" draggable="true" data-id="${f.id}"${f.type==='folder'?` data-drop="${f.id}"`:''}>${fileIco(f,52)}<span>${esc(f.name)}</span></div>`).join('') }
    else { listEl.className='fmlist rows'; listEl.innerHTML=`<div class="fh"><span>Nom</span><span>Modifié</span><span>Type</span><span>Taille</span></div>`+a.map(f=>`<div class="fr${sel.has(f.id)?' sel':''}" role="option" aria-selected="${sel.has(f.id)}" draggable="true" data-id="${f.id}"${f.type==='folder'?` data-drop="${f.id}"`:''}><span class="n">${fileIco(f,26)}${esc(f.name)}</span><span>${f.mod?fmtDate(f.mod):'—'}</span><span>${fileKind(f).label}</span><span>${fmtSize(f)}</span></div>`).join('') }
    const n=a.length, s=sel.size; stat.textContent=`${n} élément${n>1?'s':''}`+(s?` · ${s} sélectionné${s>1?'s':''}`:'');
  }
  const up=()=>{ const p=FS.get(cwd)?.parent; if(p) go(p) };
  function del(ids){ ids.forEach(id=>cwd==='trash'?null:trashItem(id)); if(cwd==='trash'&&ids.length){ Dlg.confirm({title:'Supprimer définitivement ?',text:ids.length>1?`${ids.length} éléments seront supprimés pour toujours.`:'Cet élément sera supprimé pour toujours.',ok:'Supprimer',danger:true}).then(ok=>{ if(ok) ids.forEach(FS.purge) }) } sel.clear() }

  root.addEventListener('click',async e=>{
    const a=e.target.closest('[data-a]')?.dataset.a;
    if(a==='back'&&hi>0){ hi--; go(hist[hi],false) } else if(a==='fwd'&&hi<hist.length-1){ hi++; go(hist[hi],false) } else if(a==='up') up();
    else if(a==='view'){ view=view==='grid'?'rows':'grid'; Baura.store.set('filesView',view); render() }
    else if(a==='nfolder'){ const id=await newItem(cwd,'folder'); if(id){ sel=new Set([id]); render() } }
    else if(a==='nfile'){ const r=e.target.closest('[data-a]').getBoundingClientRect(); Ctx.show(newMenu(cwd,id=>{ sel=new Set([id]); render() }),r.left,r.bottom+4) }
    else if(a==='empty') emptyTrashAsk();
    const p=e.target.closest('[data-p]'); if(p) go(p.dataset.p);
    const c=e.target.closest('[data-c]'); if(c) go(c.dataset.c);
  });
  listEl.addEventListener('click',e=>{
    const it=e.target.closest('[data-id]'); if(!it){ if(!e.ctrlKey){ sel.clear(); render() } return }
    const id=it.dataset.id;
    if(e.ctrlKey||e.metaKey){ sel.has(id)?sel.delete(id):sel.add(id) } else if(e.shiftKey&&sel.size){
      const ids=itemsNow().map(x=>x.id), last=[...sel].pop(), a=ids.indexOf(last), b=ids.indexOf(id); ids.slice(Math.min(a,b),Math.max(a,b)+1).forEach(x=>sel.add(x)) }
    else sel=new Set([id]);
    render();
  });
  listEl.addEventListener('dblclick',e=>{ const it=e.target.closest('[data-id]'); if(!it) return; const f=FS.get(it.dataset.id); if(f.type==='folder'&&!FS.inTrash(f.id)||f.id==='trash') go(f.id); else openItem(f.id) });
  listEl.addEventListener('contextmenu',e=>{
    e.preventDefault(); e.stopPropagation(); const it=e.target.closest('[data-id]');
    if(it){
      const id=it.dataset.id; if(!sel.has(id)){ sel=new Set([id]); render() } const ids=[...sel], one=ids.length===1?FS.get(ids[0]):null, t=cwd==='trash';
      Ctx.show(t?[{label:'Restaurer',icon:IC.restore,run:()=>ids.forEach(FS.restore)},{label:'Supprimer définitivement',icon:IC.trash,danger:true,run:()=>del(ids)}]:[
        one&&{label:'Ouvrir',icon:IC.open,kbd:'Entrée',run:()=>one.type==='folder'?go(one.id):openItem(one.id)}, one&&{label:'Renommer',icon:IC.rename,kbd:'F2',run:()=>renameItem(one.id)}, one&&'sep',
        {label:'Mettre à la corbeille',icon:IC.trash,kbd:'Suppr',danger:true,run:()=>del(ids)}],e.clientX,e.clientY);
    } else {
      sel.clear(); render();
      Ctx.show(cwd==='trash'?[{label:'Vider la corbeille',icon:IC.trash,danger:true,disabled:!FS.trashCount(),run:emptyTrashAsk}]:
        [{label:'Nouveau dossier',icon:IC.folder,run:async()=>{const id=await newItem(cwd,'folder'); if(id){sel=new Set([id]);render()}}},...newMenu(cwd,id=>{sel=new Set([id]);render()}),'sep',
         {label:view==='grid'?'Affichage en liste':'Affichage en grille',icon:IC.grid,run:()=>root.querySelector('[data-a=view]').click()},{label:'Actualiser',icon:IC.refresh,run:render}],e.clientX,e.clientY);
    }
  });
  search.addEventListener('input',()=>{ q=search.value; render() });
  listEl.addEventListener('keydown',e=>{
    const ids=[...sel], one=ids.length===1?FS.get(ids[0]):null;
    if(e.key==='Enter'&&one){ one.type==='folder'?go(one.id):openItem(one.id) }
    else if(e.key==='Delete'&&ids.length){ del(ids) } else if(e.key==='F2'&&one&&cwd!=='trash'){ renameItem(one.id) }
    else if(e.key==='Backspace'){ up() } else if((e.ctrlKey||e.metaKey)&&e.key.toLowerCase()==='a'){ e.preventDefault(); sel=new Set(itemsNow().map(x=>x.id)); render() }
  });
  // glisser-déposer vers un dossier / la corbeille
  root.addEventListener('dragstart',e=>{ const it=e.target.closest('[data-id]'); if(!it) return; if(!sel.has(it.dataset.id)){ sel=new Set([it.dataset.id]); render() } e.dataTransfer.setData('text/baura',JSON.stringify([...sel])); e.dataTransfer.effectAllowed='move' });
  root.addEventListener('dragover',e=>{ const t=e.target.closest('[data-drop]'); if(t){ e.preventDefault(); root.querySelectorAll('.dz').forEach(x=>x.classList.remove('dz')); t.classList.add('dz') } });
  root.addEventListener('dragleave',e=>{ e.target.closest&&e.target.closest('[data-drop]')?.classList.remove('dz') });
  root.addEventListener('drop',e=>{
    const t=e.target.closest('[data-drop]'); root.querySelectorAll('.dz').forEach(x=>x.classList.remove('dz')); if(!t) return; e.preventDefault();
    let ids=[]; try{ ids=JSON.parse(e.dataTransfer.getData('text/baura')||'[]') }catch(x){}
    const dest=t.dataset.drop; ids.forEach(id=>{ if(dest==='trash') FS.trash(id); else FS.move(id,dest) }); sel.clear();
  });
  FS.on(()=>{ if(root.isConnected){ [...sel].forEach(id=>{ if(!FS.get(id)) sel.delete(id) }); render() } });
  go(ctx.args.folder||'documents'); setTimeout(()=>listEl.focus({preventScroll:true}),80);
  return {open(a){ if(a&&a.folder) go(a.folder) }};
};

/* ====================== NOTES ====================== */
AppViews.notes=function(root,ctx){
  let cur=null, timer=0, q='';
  root.innerHTML=`<div class="nt"><aside class="ntside"><div class="nthead"><label class="fmsearch">${svgi(IC.search)}<input placeholder="Rechercher" aria-label="Rechercher une note"></label><button class="tb" id="nNew" aria-label="Nouvelle note">${svgi('<path d="M12 5v14M5 12h14"/>')}</button></div><div class="ntlist" role="listbox"></div></aside>
    <section class="ntmain"><div class="ntbar"><input class="nttitle" aria-label="Titre" placeholder="Titre"><button class="tb" id="nDel" aria-label="Supprimer la note">${svgi(IC.trash)}</button></div><textarea class="ntedit" placeholder="Écris ici…" aria-label="Contenu de la note" spellcheck="true"></textarea><div class="ntstat"><span id="nCount"></span><span id="nSaved">Enregistré</span></div></section></div>`;
  const listEl=root.querySelector('.ntlist'), title=root.querySelector('.nttitle'), edit=root.querySelector('.ntedit'), saved=root.querySelector('#nSaved'), count=root.querySelector('#nCount');
  const notes=()=>FS.all().filter(f=>f.type==='file'&&['txt','md','log'].includes(FS.ext(f.name))&&!FS.inTrash(f.id)&&(!q||f.name.toLowerCase().includes(q)||(f.content||'').toLowerCase().includes(q))).sort((a,b)=>(b.mod||0)-(a.mod||0));
  const base=n=>n.replace(/\.[^.]+$/,'');
  function renderList(){
    const a=notes(); listEl.innerHTML=a.length?a.map(f=>`<button class="ni2${f.id===cur?' on':''}" data-id="${f.id}" role="option"><b>${esc(base(f.name))}</b><span>${esc((f.content||'').split('\n').find(l=>l.trim())||'Note vide').slice(0,60)}</span><small>${fmtDate(f.mod||Date.now())}</small></button>`).join(''):'<div class="nempty2">Aucune note</div>';
  }
  function load(id){
    flush(); cur=id; const f=FS.get(id);
    title.value=f?base(f.name):''; edit.value=f?(f.content||''):''; title.disabled=edit.disabled=!f; root.querySelector('#nDel').disabled=!f;
    ctx.setTitle(f?base(f.name):'Notes'); saved.textContent='Enregistré'; upCount(); renderList();
  }
  const upCount=()=>{ const w=edit.value.trim()?edit.value.trim().split(/\s+/).length:0; count.textContent=`${w} mot${w>1?'s':''} · ${edit.value.length} caractère${edit.value.length>1?'s':''}` };
  function flush(){ if(timer){ clearTimeout(timer); timer=0; if(cur&&FS.get(cur)){ FS.update(cur,{content:edit.value},true); saved.textContent='Enregistré' } } }
  edit.addEventListener('input',()=>{ saved.textContent='Enregistrement…'; upCount(); clearTimeout(timer); timer=setTimeout(()=>{ timer=0; if(cur&&FS.get(cur)){ FS.update(cur,{content:edit.value},true); saved.textContent='Enregistré'; renderList() } },350) });
  const rn=()=>{ if(!cur) return; const f=FS.get(cur); if(!f) return; const n=title.value.trim(); if(!n){ title.value=base(f.name); return } const e=FS.ext(f.name); FS.rename(cur,n+(e?'.'+e:'.txt')); }
  title.addEventListener('change',rn); title.addEventListener('keydown',e=>{ if(e.key==='Enter'){ e.preventDefault(); edit.focus() } });
  listEl.addEventListener('click',e=>{ const b=e.target.closest('[data-id]'); if(b) load(b.dataset.id) });
  root.querySelector('#nNew').onclick=()=>{ flush(); const id=FS.create({name:'Nouvelle note.txt',type:'file',parent:'documents',content:''}); load(id); title.focus(); title.select() };
  root.querySelector('#nDel').onclick=()=>{ if(!cur) return; flush(); const id=cur, a=notes(), i=a.findIndex(f=>f.id===id); FS.trash(id); const rest=notes(); load(rest[Math.min(i,rest.length-1)]?.id||null) };
  root.querySelector('.nthead input').addEventListener('input',e=>{ q=e.target.value.toLowerCase(); renderList() });
  FS.on(()=>{ if(!root.isConnected) return; const f=cur&&FS.get(cur); if(cur&&(!f||FS.inTrash(cur))){ load(notes()[0]?.id||null) } else { if(f&&document.activeElement!==title) title.value=base(f.name); renderList() } });
  const a0=ctx.args.file&&FS.get(ctx.args.file)?ctx.args.file:(notes()[0]?.id||null); load(a0);
  return {open(a){ if(a&&a.file) load(a.file) }, destroy(){ flush() }};
};

/* ====================== CALCULATRICE ====================== */
AppViews.calc=function(root,ctx){
  let cur='0', acc=null, op=null, fresh=true, expr='', err=false;
  const SYM={'+':'+','-':'−','*':'×','/':'÷'};
  const keys=[['C','fn'],['±','fn'],['%','fn'],['/','op'],['7'],['8'],['9'],['*','op'],['4'],['5'],['6'],['-','op'],['1'],['2'],['3'],['+','op'],['0','zero'],[','],['=','eq']];
  root.innerHTML=`<div class="calc" tabindex="0"><div class="cdisp" aria-live="polite"><small class="cexpr"></small><div class="cval">0</div></div><div class="ckeys">${keys.map(([k,c])=>`<button class="ck ${c||''}" data-k="${k}">${SYM[k]||k}</button>`).join('')}</div></div>`;
  const val=root.querySelector('.cval'), ex=root.querySelector('.cexpr'), box=root.querySelector('.calc');
  const fmt=s=>{ if(s==='Erreur') return s; const neg=s.startsWith('-'), [i,d]=s.replace('-','').split('.'); const ii=i.replace(/\B(?=(\d{3})+(?!\d))/g,' '); return (neg?'−':'')+ii+(d!==undefined?','+d:'') };
  function paint(){ val.textContent=fmt(cur); ex.textContent=expr; val.style.fontSize=cur.length>12?'30px':cur.length>9?'38px':'48px'; root.querySelector('[data-k=C]').textContent=(cur==='0'&&acc===null)?'C':'C' }
  function compute(){ const a=acc,b=parseFloat(cur); let r; if(op==='+') r=a+b; else if(op==='-') r=a-b; else if(op==='*') r=a*b; else r=b===0?NaN:a/b;
    if(!isFinite(r)){ cur='Erreur'; err=true; acc=null; op=null; return } acc=r; cur=String(+r.toPrecision(12)); op=null }
  function press(k){
    if(err&&k!=='C'){ return } if(err){ err=false }
    if(/^\d$/.test(k)){ if(fresh){ cur=k; fresh=false } else if(cur==='0') cur=k; else if(cur.replace(/[-.]/g,'').length<14) cur+=k }
    else if(k===','||k==='.'){ if(fresh){ cur='0.'; fresh=false } else if(!cur.includes('.')) cur+='.' }
    else if('+-*/'.includes(k)&&k.length===1){ if(op&&!fresh){ compute(); if(err){paint();return} } else acc=parseFloat(cur); op=k; fresh=true; expr=fmt(String(acc))+' '+SYM[k] }
    else if(k==='='||k==='Enter'){ if(op){ const b=cur; expr=fmt(String(acc))+' '+SYM[op]+' '+fmt(b)+' ='; compute(); fresh=true } }
    else if(k==='C'||k==='Escape'){ cur='0'; acc=null; op=null; fresh=true; expr=''; err=false }
    else if(k==='±'){ if(cur!=='0') cur=cur.startsWith('-')?cur.slice(1):'-'+cur }
    else if(k==='%'){ const v=parseFloat(cur); cur=String(+((op&&acc!==null?acc*v/100:v/100)).toPrecision(12)); fresh=false }
    else if(k==='Backspace'){ if(!fresh){ cur=cur.slice(0,-1); if(cur===''||cur==='-') cur='0' } }
    paint();
  }
  root.querySelector('.ckeys').addEventListener('click',e=>{ const b=e.target.closest('[data-k]'); if(b){ press(b.dataset.k); box.focus({preventScroll:true}) } });
  box.addEventListener('keydown',e=>{
    if(e.ctrlKey||e.altKey||e.metaKey) return; let k=e.key;
    if(k==='x'||k==='X') k='*'; if(k===':') k='/';
    if(/^[\d+\-*/.,=%]$/.test(k)||['Enter','Backspace','Escape'].includes(k)){ e.preventDefault(); press(k==='.'?',':k) }
  });
  setTimeout(()=>box.focus({preventScroll:true}),80); paint();
};

/* ====================== TERMINAL ====================== */
AppViews.terminal=function(root,ctx){
  const acct=Baura.account.get(), user=((acct&&acct.name)||'utilisateur').toLowerCase().replace(/\s+/g,'');
  let cwd='root', hist=[], hp=-1; const t0=Date.now();
  root.innerHTML=`<div class="term" tabindex="-1"><div class="tout" role="log" aria-live="polite"></div><div class="tline"><span class="tp"></span><input class="tin" spellcheck="false" autocomplete="off" aria-label="Commande"></div></div>`;
  const out=root.querySelector('.tout'), inp=root.querySelector('.tin'), tp=root.querySelector('.tp'), box=root.querySelector('.term');
  const drv=id=>FS.drive(id)+':', pathStr=id=>drv(id)+'/'+FS.path(id).map(p=>p.name).join('/');
  const who=()=>ctx.admin?'admin':user;
  const ps=()=>`<span class="pd">${drv(cwd)}</span><span class="ps">/</span><b>${esc(who())}@baura</b> <i>${esc(id2s(cwd))}</i> <em class="pc${ctx.admin?' adm':''}">❯</em>&nbsp;`;
  const prompt=()=>{ tp.innerHTML=ps() };
  const id2s=id=>(id==='root'||id==='drive-d')?'':'/'+FS.path(id).map(p=>p.name).join('/');
  const print=(t,c)=>{ const d=document.createElement('div'); d.className='tl '+(c||''); d.innerHTML=t===''?'&nbsp;':(c==='raw'?t:esc(t)); out.appendChild(d); out.scrollTop=out.scrollHeight };
  const NEO=String.raw`   ▐█▌ ▟█▙
   ▐█▌ ▜██▙
   ▐█▌ ▟██▛
   ▐█▌ ▜█▛`;
  const appNames=()=>Object.fromEntries(APPS.map(a=>[a.n.toLowerCase(),a.id]).concat(APPS.map(a=>[a.id,a.id])));
  const need=adm=>{ if(adm||ctx.admin) return true; print('Droits administrateur requis. Essaie : sudo <commande>','err'); return false };
  const findApp=n=>{ n=(n||'').toLowerCase(); return Object.values(byId).find(a=>a.id===n||a.n.toLowerCase()===n) };
  const CMDS={
    help(){ ['Commandes disponibles :','  ls [chemin]      liste le contenu','  cd <chemin>      change de dossier','  pwd              affiche le dossier courant','  cat <fichier>    affiche un fichier texte','  mkdir <nom>      crée un dossier','  touch <nom>      crée un fichier','  rm <nom>         met à la corbeille','  echo <texte>     affiche du texte','  open <appli|fichier>  ouvre une appli ou un fichier','  sudo <commande>  exécute en administrateur','  install / uninstall <appli>  (avec sudo) · store : liste','  notify [--warn|--ok|--err] <texte>','  apps             liste les applications','  theme auto|clair|sombre','  accent indigo|rose|teal|ambre|ardoise','  notify <texte>   envoie une notification','  neofetch · date · uptime · whoami · uname','  lock · sleep · reboot · shutdown','  history · clear'].forEach(l=>print(l)) },
    clear(){ out.innerHTML='' },
    pwd(){ print(pathStr(cwd).replace(/\/$/,'')+(FS.path(cwd).length?'':'/')) },
    drives(){ FS.drives().forEach(d=>print(d+':/   '+(d==='C'?'Disque système':FS.get('drive-d').name),'mut')) },
    ls(a){ const p=a[0]&&!a[0].startsWith('-')?a[0]:'.', id=FS.byPath(p,cwd); if(!id||FS.get(id).type!=='folder') return print('ls : '+p+' : dossier introuvable','err');
      const l=FS.list(id).filter(f=>!(id==='root'&&f.id==='trash')); if(!l.length) return print('(vide)','mut');
      print(l.map(f=>f.type==='folder'?`<span class="tdir">${esc(f.name)}/</span>`:esc(f.name)).join('   '),'raw') },
    cd(a){ const id=a[0]?FS.byPath(a.join(' '),cwd):'root'; if(!id||FS.get(id).type!=='folder') return print('cd : '+a.join(' ')+' : dossier introuvable','err'); cwd=id; prompt() },
    cat(a){ const id=FS.byPath(a.join(' '),cwd); const f=id&&FS.get(id); if(!f||f.type!=='file') return print('cat : fichier introuvable','err'); if(f.content===undefined) return print('cat : ce fichier n\'est pas du texte','err'); String(f.content).split('\n').forEach(l=>print(l)) },
    mkdir(a){ if(!a.length) return print('mkdir : nom manquant','err'); FS.create({name:a.join(' '),type:'folder',parent:cwd}) },
    touch(a){ if(!a.length) return print('touch : nom manquant','err'); FS.create({name:a.join(' '),type:'file',parent:cwd,content:''}) },
    rm(a){ const id=FS.byPath(a.join(' '),cwd); if(!id||id==='root') return print('rm : introuvable','err'); FS.trash(id); print('Déplacé dans la corbeille.','mut') },
    echo(a){ print(a.join(' ')) },
    date(){ print(new Date().toLocaleString('fr-FR',{dateStyle:'full',timeStyle:'medium'})) },
    whoami(){ print(who()) }, uname(){ print('BAURA OS 0.2 (web) · glassmorphism') },
    uptime(){ const s=Math.floor((Date.now()-t0)/1000); print(`session ouverte depuis ${Math.floor(s/60)} min ${s%60} s`) },
    apps(){ APPS.forEach(a=>print('  '+a.id.padEnd(10)+a.n)) },
    open(a){ const n=a.join(' ').toLowerCase(); if(appNames()[n]){ openApp(appNames()[n]); return } const id=FS.byPath(a.join(' '),cwd); if(id&&id!=='root'){ openItem(id); return } print('open : « '+a.join(' ')+' » introuvable','err') },
    theme(a){ const m={auto:'auto',clair:'light',light:'light',sombre:'dark',dark:'dark'}; if(!m[a[0]]) return print('Usage : theme auto|clair|sombre','err'); Baura.settings.set({theme:m[a[0]]}); print('Thème : '+a[0],'mut') },
    accent(a){ if(!Baura.settings.accents[a[0]]) return print('Usage : accent '+Object.keys(Baura.settings.accents).join('|'),'err'); Baura.settings.set({accent:a[0]}); print('Accent : '+a[0],'mut') },
    notify(a){ const w=a[0]==='--warn'||a[0]==='--err'||a[0]==='--ok'; const lv=w?a[0].slice(2):undefined; const tx=(w?a.slice(1):a).join(' ')||'Bonjour !'; if(lv) Sys.popup({app:'terminal',title:'Terminal',text:tx,level:lv}); else notify({app:'terminal',title:'Terminal',text:tx}) },
    store(){ Object.values(byId).forEach(x=>print('  '+x.id.padEnd(10)+x.n.padEnd(14)+(APPS.includes(x)?'installée':'disponible'))) },
    async sudo(a){ if(!a.length) return print('Usage : sudo <commande>','err'); const c=a[0].toLowerCase(); if(!CMDS[c]) return print(c+' : commande introuvable','err');
      const ok=ctx.admin||await Sys.elevate({title:'Terminal',reason:'Exécuter « '+a.join(' ')+' » en tant qu\'administrateur ?'}); if(!ok) return print('sudo : autorisation refusée','err'); await CMDS[c](a.slice(1),true) },
    async install(a,adm){ const x=findApp(a[0]); if(!x) return print('Usage : sudo install <appli>  (voir « store »)','err'); if(!need(adm)) return; if(APPS.includes(x)) return print(x.n+' est déjà installée.','mut');
      for(const l of ['Téléchargement de '+x.n+'…','Vérification de la signature…','Installation…']){ print(l,'mut'); await new Promise(r=>setTimeout(r,500)) } Apps.add(x.id); print('OK · '+x.n+' installée.'); Sys.popup({title:x.n+' installée',text:'Installée depuis le terminal.',level:'ok',app:x.id,actions:[{label:'Ouvrir',run:()=>openApp(x.id)}]}) },
    async uninstall(a,adm){ const x=findApp(a[0]); if(!x||!APPS.includes(x)) return print('Usage : sudo uninstall <appli>','err'); if(!need(adm)) return; if(Sys.isSystem(x.id)) return print(x.n+' est une application système.','err');
      print('Suppression de '+x.n+'…','mut'); await new Promise(r=>setTimeout(r,600)); Apps.remove(x.id); print('OK · '+x.n+' désinstallée.'); Sys.popup({title:x.n+' désinstallée',text:'Retirée depuis le terminal.',level:'ok',app:'store',noOpen:true}) },
    neofetch(){ print(`<span class="tn">${esc(NEO)}</span>`,'raw'); [`${user}@baura`,'-----------','OS      : BAURA OS 0.2','Thème   : '+(Baura.settings.isDark(Baura.settings.get())?'sombre':'clair')+' · glass','Fenêtres: '+wins.size,'Fichiers: '+FS.all().filter(f=>f.type==='file').length].forEach(l=>print(l)) },
    history(){ hist.forEach((h,i)=>print(String(i+1).padStart(3)+'  '+h)) },
    lock(){ power('lock') }, sleep(){ power('sleep') }, reboot(){ power('restart') }, restart(){ power('restart') }, shutdown(){ power('shutdown') },
  };
  function run(line){
    print(`<span class="tp2">${ps()}</span>${esc(line)}`,'raw');
    const parts=(line.match(/"[^"]*"|\S+/g)||[]).map(s=>s.replace(/^"|"$/g,'')); if(!parts.length) return;
    const c=parts[0].toLowerCase(), a=parts.slice(1);
    if(CMDS[c]) CMDS[c](a,false); else print(c+' : commande introuvable. Tape « help ».','err');
  }
  inp.addEventListener('keydown',e=>{
    if(e.key==='Enter'){ const l=inp.value; inp.value=''; if(l.trim()){ hist.push(l); } hp=hist.length; run(l) }
    else if(e.key==='ArrowUp'){ e.preventDefault(); if(hp>0){ hp--; inp.value=hist[hp] } }
    else if(e.key==='ArrowDown'){ e.preventDefault(); if(hp<hist.length-1){ hp++; inp.value=hist[hp] } else { hp=hist.length; inp.value='' } }
    else if(e.key==='Tab'){ e.preventDefault(); const v=inp.value, sp=v.lastIndexOf(' '), w=v.slice(sp+1).toLowerCase();
      const pool=sp<0?Object.keys(CMDS):FS.list(cwd).map(f=>f.name); const m=pool.filter(n=>n.toLowerCase().startsWith(w)); if(m.length===1) inp.value=v.slice(0,sp+1)+m[0]; else if(m.length>1) print(m.join('   ')) }
    else if(e.key==='l'&&e.ctrlKey){ e.preventDefault(); CMDS.clear() }
  });
  box.addEventListener('pointerdown',e=>{ if(!window.getSelection().toString()) setTimeout(()=>inp.focus({preventScroll:true}),0) });
  print('BAURA OS Terminal — tape « help » pour voir les commandes.','mut'); prompt(); setTimeout(()=>inp.focus({preventScroll:true}),80);
};

/* ====================== PHOTOS ====================== */
AppViews.photos=function(root,ctx){
  let tab='lib', view=null;
  root.innerHTML=`<div class="ph2"><div class="phbar"><div class="seg"><button data-t="lib" class="on">Bibliothèque</button><button data-t="walls">Fonds d'écran</button></div><span class="grow"></span><button class="tb txt" id="phImp">${svgi('<path d="M12 4v11M7 9l5-5 5 5M5 20h14"/>')}Importer</button><input type="file" id="phFile" accept="image/*" hidden></div><div class="phbody"></div></div>`;
  const bodyEl=root.querySelector('.phbody');
  const imgs=()=>FS.all().filter(f=>f.type==='file'&&(f.kind==='image'||['svg','png','jpg','jpeg','webp','gif'].includes(FS.ext(f.name)))&&!FS.inTrash(f.id));
  function render(){
    if(view&&FS.get(view)){
      const f=FS.get(view), a=imgs(), i=a.findIndex(x=>x.id===view);
      bodyEl.innerHTML=`<div class="viewer"><div class="vimg"><img src="${esc(f.src||'')}" alt="${esc(f.name)}"></div><div class="vbar"><button class="tb" data-v="back">${svgi('<path d="M14 6l-6 6 6 6"/>')}Retour</button><b>${esc(f.name)}</b><span class="grow"></span><button class="tb" data-v="prev" aria-label="Précédente" ${a.length<2?'disabled':''}>${svgi('<path d="M14 6l-6 6 6 6"/>')}</button><button class="tb" data-v="next" aria-label="Suivante" ${a.length<2?'disabled':''}>${svgi('<path d="M10 6l6 6-6 6"/>')}</button><button class="tb txt" data-v="wall">${svgi(IC.image)}Définir comme fond</button><button class="tb dng" data-v="del" aria-label="Supprimer">${svgi(IC.trash)}</button></div></div>`;
      ctx.setTitle(f.name); return }
    ctx.setTitle('Photos'); view=null;
    if(tab==='lib'){ const a=imgs(); bodyEl.innerHTML=a.length?`<div class="pgrid">${a.map(f=>`<button class="pth" data-id="${f.id}" title="${esc(f.name)}"><img src="${esc(f.src||'')}" alt=""><span>${esc(f.name)}</span></button>`).join('')}</div>`:'<div class="fmempty">'+svgi(IC.image)+'<b>Aucune image</b><span>Importe une image avec le bouton ci-dessus.</span></div>' }
    else { const s=Baura.settings.get(); bodyEl.innerHTML=`<div class="pgrid">${Baura.settings.walls.map(w=>`<button class="pth${s.wall===w.id?' cur':''}" data-w="${w.id}"><img src="${w.id==='aura'?Baura.settings.wallFile(s):'wallpaper-'+w.id+'.svg'}" alt=""><span>${w.n}${s.wall===w.id?' · actuel':''}</span></button>`).join('')}</div>` }
  }
  root.addEventListener('click',e=>{
    const t=e.target.closest('[data-t]'); if(t){ tab=t.dataset.t; view=null; root.querySelectorAll('[data-t]').forEach(x=>x.classList.toggle('on',x===t)); render(); return }
    const th=e.target.closest('.pth[data-id]'); if(th){ view=th.dataset.id; render(); return }
    const w=e.target.closest('.pth[data-w]'); if(w){ Baura.settings.set({wall:w.dataset.w}); render(); return }
    const v=e.target.closest('[data-v]')?.dataset.v; if(!v) return; const a=imgs(), i=a.findIndex(x=>x.id===view);
    if(v==='back'){ view=null; render() } else if(v==='prev'){ view=a[(i-1+a.length)%a.length].id; render() } else if(v==='next'){ view=a[(i+1)%a.length].id; render() }
    else if(v==='wall'){ const f=FS.get(view); Baura.settings.set({wall:'custom',wallCustom:f.src}); notify({app:'photos',title:'Fond d\'écran modifié',text:f.name+' est maintenant ton fond d\'écran.'}) }
    else if(v==='del'){ FS.trash(view); view=null; render() }
  });
  root.querySelector('#phImp').onclick=()=>root.querySelector('#phFile').click();
  root.querySelector('#phFile').onchange=e=>{
    const file=e.target.files[0]; if(!file) return; const rd=new FileReader();
    rd.onload=()=>{ const im=new Image(); im.onload=()=>{
      const k=Math.min(1,1600/Math.max(im.width,im.height)), c=document.createElement('canvas'); c.width=Math.round(im.width*k); c.height=Math.round(im.height*k);
      c.getContext('2d').drawImage(im,0,0,c.width,c.height); const url=c.toDataURL('image/jpeg',.82);
      FS.create({name:file.name.replace(/\.[^.]+$/,'')+'.jpg',type:'file',parent:'images',kind:'image',src:url}); tab='lib'; render() };
      im.onerror=()=>Dlg.alert({title:'Import impossible',text:'Ce fichier n\'est pas une image valide.'}); im.src=rd.result };
    rd.readAsDataURL(file); e.target.value='';
  };
  FS.on(()=>{ if(root.isConnected) render() }); Baura.on('settings',()=>{ if(root.isConnected&&tab==='walls') render() });
  if(ctx.args.file) view=ctx.args.file; render();
  return {open(a){ if(a&&a.file){ view=a.file; render() } }};
};

/* ====================== MUSIQUE ====================== */
window.mediaHooks=window.mediaHooks||[];
AppViews.musique=function(root,ctx){
  root.innerHTML=`<div class="mu"><div class="munow"><div class="mcov" id="muCov">${svgi('<path d="M9 18V6l10-2v12"/><circle cx="7" cy="18" r="2"/><circle cx="17" cy="16" r="2"/>')}</div><div class="mtx"><b id="muT"></b><span id="muA"></span></div></div>
    <div class="mprog"><input type="range" id="muSeek" min="0" max="1000" value="0" aria-label="Position"></div><div class="times"><span id="muC">0:00</span><span id="muD">0:00</span></div>
    <div class="mctl"><button class="mb" id="muPrev" aria-label="Précédent"><svg viewBox="0 0 24 24"><path class="fill" d="M18 5.5v13L9.5 12z"/><path d="M6 5.5v13"/></svg></button><button class="mb play" id="muPlay"><svg viewBox="0 0 24 24" id="muIc"></svg></button><button class="mb" id="muNext" aria-label="Suivant"><svg viewBox="0 0 24 24"><path class="fill" d="M6 5.5v13L14.5 12z"/><path d="M18 5.5v13"/></svg></button></div>
    <div class="mlist" role="listbox"></div></div>`;
  const $$=s=>root.querySelector(s), seek=$$('#muSeek');
  function paint(){
    const t=TRACKS[ti]; $$('#muT').textContent=t.t; $$('#muA').textContent=t.a; $$('#muCov').style.background=`linear-gradient(145deg,${t.g[0]},${t.g[1]})`;
    seek.value=Math.round(pos/t.d*1000); paintRange(seek); $$('#muC').textContent=mm(pos); $$('#muD').textContent=mm(t.d); $$('#muIc').innerHTML=playing?PAUSE:PLAY;
    $$('#muPlay').setAttribute('aria-label',playing?'Pause':'Lecture'); root.querySelector('.mu').classList.toggle('playing',playing);
    $$('.mlist').innerHTML=TRACKS.map((x,i)=>`<button class="mrow${i===ti?' on':''}" data-i="${i}" role="option"><span class="mn">${i===ti&&playing?'<i class="eq2"><b></b><b></b><b></b></i>':i+1}</span><span class="mt"><b>${esc(x.t)}</b><small>${esc(x.a)}</small></span><span>${mm(x.d)}</span></button>`).join('');
  }
  const hook=()=>{ if(root.isConnected) paint() }; mediaHooks.push(hook);
  $$('#muPlay').onclick=()=>$('#mPlay').click(); $$('#muPrev').onclick=()=>$('#mPrev').click(); $$('#muNext').onclick=()=>$('#mNext').click();
  seek.addEventListener('input',e=>{ pos=e.target.value/1000*TRACKS[ti].d; paintMedia() });
  root.addEventListener('click',e=>{ const r=e.target.closest('.mrow'); if(r){ loadTrack(+r.dataset.i); playing=true; paintMedia() } });
  function open(a){ if(a&&a.track!==undefined){ loadTrack(a.track); if(a.play){ playing=true; paintMedia() } } }
  open(ctx.args); paint();
  return {open,destroy(){ const i=mediaHooks.indexOf(hook); if(i>-1) mediaHooks.splice(i,1) }};
};

/* ====================== NAVIGATEUR ====================== */
AppViews.web=function(root,ctx){
  const TILES=[['Wikipédia','https://fr.wikipedia.org','#4F46E5'],['Anthropic','https://www.anthropic.com','#AB8E18'],['GitHub','https://github.com','#334155'],['OpenStreetMap','https://www.openstreetmap.org','#0D9488'],['MDN','https://developer.mozilla.org','#8B5CF6'],['Météo','https://www.meteofrance.com','#0EA5E9']];
  root.innerHTML=`<div class="br"><div class="bbar"><span class="burl">${svgi(IC.globe)}<input id="bIn" placeholder="Rechercher ou saisir une adresse" aria-label="Adresse" spellcheck="false" autocomplete="off"></span></div>
    <div class="bhome"><svg class="bl" viewBox="150 29 199 420"><rect x="150" y="29" width="48" height="420" rx="24" fill="var(--b1)"/><path d="M248 29A101 102.5 0 0 1 248 234Z" fill="var(--b2)"/><path d="M248 245A101 102 0 0 1 248 449Z" fill="var(--b3)"/><rect x="316" y="221" width="33" height="36" rx="15" fill="var(--b4)"/></svg>
    <h2>Que veux-tu découvrir ?</h2><div class="btiles">${TILES.map(([n,u,c])=>`<a class="bt" href="${u}" target="_blank" rel="noopener noreferrer"><span style="background:${c}">${n[0]}</span>${n}</a>`).join('')}</div>
    <p class="bnote">Les sites s'ouvrent dans un nouvel onglet de ton vrai navigateur.</p></div></div>`;
  const inp=root.querySelector('#bIn');
  inp.addEventListener('keydown',e=>{ if(e.key!=='Enter') return; const v=inp.value.trim(); if(!v) return;
    const url=/^https?:\/\//i.test(v)?v:(/^[\w-]+(\.[\w-]+)+(\/\S*)?$/.test(v)?'https://'+v:'https://duckduckgo.com/?q='+encodeURIComponent(v)); window.open(url,'_blank','noopener,noreferrer'); inp.value='' });
  setTimeout(()=>inp.focus({preventScroll:true}),80);
};
