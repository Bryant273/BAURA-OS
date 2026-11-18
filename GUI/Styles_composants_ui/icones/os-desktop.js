/* ===== BAURA OS – icônes du bureau : sélection, glisser, rectangle de sélection, clic droit, corbeille ===== */
const Icons=(function(){
  const W=96, H=104, PAD=12;
  const wrap=document.createElement('div'); wrap.id='icons'; document.body.insertBefore(wrap,document.body.firstChild);
  const marq=document.createElement('div'); marq.className='marq'; marq.hidden=true; wrap.appendChild(marq);
  let sel=new Set(), pos=Baura.store.get('iconpos',{}), items=[], renaming=false;
  const SYS={ 'sys-files':{name:'Mes fichiers',run:()=>openApp('fichiers')}, 'sys-trash':{name:'Corbeille',run:()=>openApp('fichiers',{folder:'trash'})} };
  const BIN='<path d="M4 7h16M9 7V4h6v3M6 7l1 13h10l1-13"/>';
  const rows=()=>Math.max(1,Math.floor((innerHeight-PAD-90)/H));

  function collect(){
    items=[{key:'sys-files',sys:true,name:SYS['sys-files'].name,g:G('#6D66F2','#4F46E5'),d:IC.folder},
           {key:'sys-trash',sys:true,name:SYS['sys-trash'].name,g:FS.trashCount()?G('#A78BFA','#6D28D9'):G('#94A3C4','#5B6794'),d:BIN,full:FS.trashCount()>0}];
    FS.list('bureau').forEach(f=>{ const k=fileKind(f); items.push({key:f.id,fs:f,name:f.name,g:k.g,d:k.d,folder:f.type==='folder'}) });
  }
  function freeCell(taken){
    const R=rows();
    for(let c=0;c<40;c++) for(let r=0;r<R;r++){ if(!taken.has(c+','+r)) return [c,r] }
    return [0,0];
  }
  function layout(){
    const taken=new Set(), R=rows();
    Object.keys(pos).forEach(k=>{ if(!items.find(i=>i.key===k)) delete pos[k] });
    items.forEach(i=>{ const p=pos[i.key]; if(p&&p[1]<R&&!taken.has(p[0]+','+p[1])) taken.add(p[0]+','+p[1]); else delete pos[i.key] });
    items.forEach(i=>{ if(!pos[i.key]){ const c=freeCell(taken); pos[i.key]=c; taken.add(c[0]+','+c[1]) } });
  }
  function render(){
    collect(); layout();
    [...wrap.querySelectorAll('.dicon')].forEach(e=>e.remove());
    items.forEach(i=>{
      const [c,r]=pos[i.key], b=document.createElement('div');
      b.className='dicon'+(sel.has(i.key)?' sel':'')+(i.full?' full':''); b.dataset.key=i.key; b.tabIndex=0; b.setAttribute('role','button'); b.setAttribute('aria-label',i.name);
      b.style.left=(PAD+c*W)+'px'; b.style.top=(PAD+r*H)+'px';
      b.innerHTML=`${icoHTML(i.g,i.d,52)}<span class="dl">${esc(i.name)}</span>`;
      wrap.appendChild(b);
    });
  }
  function save(){ Baura.store.set('iconpos',pos) }
  const byKey=k=>items.find(i=>i.key===k);
  const el=k=>wrap.querySelector(`.dicon[data-key="${k}"]`);
  function setSel(keys){ sel=new Set(keys); wrap.querySelectorAll('.dicon').forEach(e=>e.classList.toggle('sel',sel.has(e.dataset.key))) }
  function clearSel(){ if(sel.size) setSel([]) }

  function open(key){ const i=byKey(key); if(!i) return; if(i.sys) SYS[key].run(); else openItem(key) }

  /* ---- clic droit ---- */
  function menuIcon(key,x,y){
    if(!sel.has(key)) setSel([key]);
    const keys=[...sel], one=keys.length===1?byKey(keys[0]):null, fsKeys=keys.filter(k=>!byKey(k)?.sys);
    if(one&&one.key==='sys-trash') return Ctx.show([{label:'Ouvrir',icon:IC.open,run:()=>open('sys-trash')},{label:'Vider la corbeille',icon:IC.trash,disabled:!FS.trashCount(),run:emptyTrashAsk}],x,y);
    if(one&&one.sys) return Ctx.show([{label:'Ouvrir',icon:IC.open,run:()=>open(one.key)}],x,y);
    Ctx.show([
      one&&{label:'Ouvrir',icon:IC.open,kbd:'Entrée',run:()=>open(one.key)},
      one&&{label:'Renommer',icon:IC.rename,kbd:'F2',run:()=>renameItem(one.key)},
      one&&'sep',
      {label:fsKeys.length>1?`Mettre ${fsKeys.length} éléments à la corbeille`:'Mettre à la corbeille',icon:IC.trash,kbd:'Suppr',danger:true,run:()=>{fsKeys.forEach(trashItem);setSel([])}}
    ],x,y);
  }
  function menuDesktop(x,y){
    Ctx.show([
      {label:'Nouveau dossier',icon:IC.folder,run:async()=>{const id=await newItem('bureau','folder'); if(id) setSel([id])}},
      ...newMenu('bureau',id=>setSel([id])),
      'sep',
      {label:'Réorganiser les icônes',icon:IC.grid,run:()=>{pos={};save();render()}},
      {label:'Actualiser',icon:IC.refresh,kbd:'F5',run:render},
      'sep',
      {label:'Vue des tâches',icon:IC.grid,kbd:'Alt+W',run:()=>TaskView.open()},
      {label:'Rechercher…',icon:IC.search,kbd:'Ctrl+Espace',run:()=>Spot.open()},
      'sep',
      {label:'Changer le fond d\'écran',icon:IC.image,run:()=>openApp('params',{section:'perso'})},
      {label:'Paramètres d\'affichage',icon:IC.gear,run:()=>openApp('params',{section:'ecran'})},
    ],x,y);
  }

  /* ---- souris : sélection, glisser, rectangle ---- */
  let drag=null, band=null;
  wrap.addEventListener('pointerdown',e=>{
    if(e.button===2){ return }
    const ic=e.target.closest('.dicon'); if(typeof setFocus==='function') setFocus(null);
    if(ic){
      const k=ic.dataset.key;
      if(e.ctrlKey||e.metaKey){ const s=new Set(sel); s.has(k)?s.delete(k):s.add(k); setSel([...s]) }
      else if(!sel.has(k)) setSel([k]);
      drag={sx:e.clientX,sy:e.clientY,moved:false,keys:[...sel],origin:Object.fromEntries([...sel].map(x=>[x,pos[x].slice()])),k,pid:e.pointerId};
      wrap.setPointerCapture(e.pointerId);
    } else {
      if(!(e.ctrlKey||e.metaKey)) setSel([]);
      band={sx:e.clientX,sy:e.clientY,base:new Set(sel)}; marq.hidden=false; Object.assign(marq.style,{left:e.clientX+'px',top:e.clientY+'px',width:'0px',height:'0px'});
      wrap.setPointerCapture(e.pointerId);
    }
  });
  wrap.addEventListener('pointermove',e=>{
    if(drag){
      const dx=e.clientX-drag.sx, dy=e.clientY-drag.sy;
      if(!drag.moved&&Math.hypot(dx,dy)>5){ drag.moved=true; wrap.classList.add('dragging'); }
      if(drag.moved) drag.keys.forEach(k=>{ const n=el(k); if(n){ n.style.transform=`translate(${dx}px,${dy}px)`; n.classList.add('lift') } });
      if(drag.moved){ // cible de dépôt (corbeille / dossier)
        wrap.querySelectorAll('.drop').forEach(x=>x.classList.remove('drop'));
        const t=dropTarget(e); if(t) el(t).classList.add('drop');
      }
    } else if(band){
      const x=Math.min(band.sx,e.clientX), y=Math.min(band.sy,e.clientY), w=Math.abs(e.clientX-band.sx), h=Math.abs(e.clientY-band.sy);
      Object.assign(marq.style,{left:x+'px',top:y+'px',width:w+'px',height:h+'px'});
      const hit=[...wrap.querySelectorAll('.dicon')].filter(n=>{const r=n.getBoundingClientRect();return r.right>x&&r.left<x+w&&r.bottom>y&&r.top<y+h}).map(n=>n.dataset.key);
      setSel([...band.base,...hit]);
    }
  });
  function dropTarget(e){
    const under=document.elementsFromPoint(e.clientX,e.clientY).map(n=>n.closest&&n.closest('.dicon')).filter(Boolean);
    for(const n of under){ const k=n.dataset.key, it=byKey(k); if(drag.keys.includes(k)) continue;
      if(k==='sys-trash'||(it&&it.folder)) return k }
    return null;
  }
  wrap.addEventListener('pointerup',e=>{
    if(drag){
      const d=drag; drag=null; wrap.classList.remove('dragging'); wrap.querySelectorAll('.drop').forEach(x=>x.classList.remove('drop'));
      if(d.moved){
        const t=(()=>{ drag=d; const r=dropTarget(e); drag=null; return r })();
        d.keys.forEach(k=>{ const n=el(k); if(n){ n.style.transform=''; n.classList.remove('lift') } });
        if(t==='sys-trash'){ d.keys.filter(k=>!byKey(k)?.sys).forEach(trashItem); setSel([]); return }
        if(t&&byKey(t)&&byKey(t).folder){ d.keys.filter(k=>k!==t&&!byKey(k)?.sys).forEach(k=>FS.move(k,t)); setSel([]); return }
        // dépôt sur la grille
        const dx=e.clientX-d.sx, dy=e.clientY-d.sy, R=rows(), taken=new Set();
        items.forEach(i=>{ if(!d.keys.includes(i.key)) taken.add(pos[i.key].join(',')) });
        d.keys.forEach(k=>{
          let c=Math.max(0,d.origin[k][0]+Math.round(dx/W)), r=Math.min(R-1,Math.max(0,d.origin[k][1]+Math.round(dy/H)));
          if(taken.has(c+','+r)){ const f=freeCell(taken); c=f[0]; r=f[1] }
          pos[k]=[c,r]; taken.add(c+','+r);
        });
        save(); render();
      }
    } else if(band){ band=null; marq.hidden=true }
  });
  wrap.addEventListener('pointercancel',()=>{ if(drag){ drag.keys.forEach(k=>{const n=el(k); if(n){n.style.transform='';n.classList.remove('lift')}}); drag=null; wrap.classList.remove('dragging') } band=null; marq.hidden=true });
  wrap.addEventListener('dblclick',e=>{ const t=document.elementFromPoint(e.clientX,e.clientY); const ic=t&&t.closest&&t.closest('.dicon'); if(ic) open(ic.dataset.key) });
  wrap.addEventListener('contextmenu',e=>{
    e.preventDefault(); const ic=e.target.closest('.dicon');
    if(ic) menuIcon(ic.dataset.key,e.clientX,e.clientY); else { setSel([]); menuDesktop(e.clientX,e.clientY) }
  });

  /* ---- clavier ---- */
  addEventListener('keydown',e=>{
    if(e.target.closest&&e.target.closest('input,textarea,[contenteditable]')) return;
    if(document.querySelector('.dscrim')||Spot.isOpen||TaskView.isOpen||Ctx.isOpen) return;
    if(e.key==='F5'){ e.preventDefault(); render(); return }
    if(!sel.size) return;
    const keys=[...sel], fsKeys=keys.filter(k=>!byKey(k)?.sys);
    if(e.key==='Delete'||e.key==='Backspace'&&e.metaKey){ e.preventDefault(); fsKeys.forEach(trashItem); setSel([]) }
    else if(e.key==='F2'&&keys.length===1&&fsKeys.length){ e.preventDefault(); renameItem(keys[0]) }
    else if(e.key==='Enter'){ e.preventDefault(); keys.forEach(open) }
    else if(e.key==='Escape'){ setSel([]) }
  });
  addEventListener('resize',()=>render());
  FS.on(()=>{ const had=new Set(sel); render(); setSel([...had].filter(k=>byKey(k))) });
  render();
  return {render,clearSel,select:setSel,get selection(){return [...sel]}};
})();
