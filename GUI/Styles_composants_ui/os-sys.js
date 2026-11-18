/* ===== BAURA OS – pop-ups système : élévation administrateur, installation / désinstallation d'applis, notifications riches, Boutique ===== */
const META={ // éditeur, taille, droits demandés
  fichiers:{pub:'BAURA OS',size:'12 Mo',system:true,perms:['Accès aux fichiers']},
  params:{pub:'BAURA OS',size:'9 Mo',system:true,perms:['Réglages du système','Compte']},
  terminal:{pub:'BAURA OS',size:'3 Mo',system:true,perms:['Accès aux fichiers','Exécution de commandes']},
  store:{pub:'BAURA OS',size:'6 Mo',system:true,perms:['Installation d\'applications']},
  web:{pub:'Houb Labs',size:'48 Mo',perms:['Accès au réseau','Historique']},
  notes:{pub:'BAURA Studio',size:'5 Mo',perms:['Accès aux fichiers']},
  photos:{pub:'BAURA Studio',size:'14 Mo',perms:['Accès aux images','Fond d\'écran']},
  videos:{pub:'BAURA Sessions',size:'31 Mo',perms:['Lecture vidéo','Accès aux fichiers']},
  musique:{pub:'BAURA Sessions',size:'22 Mo',perms:['Lecture audio','Notifications']},
  calc:{pub:'BAURA Studio',size:'2 Mo',perms:[]},
  skrib:{pub:'Skrib Office',size:'38 Mo',perms:['Accès aux fichiers']},
  numbro:{pub:'Skrib Office',size:'41 Mo',perms:['Accès aux fichiers']},
  wepoint:{pub:'Skrib Office',size:'52 Mo',perms:['Accès aux fichiers','Plein écran']},
  data:{pub:'Skrib Office',size:'36 Mo',perms:['Accès aux fichiers']},
  crea:{pub:'Crea Studio',size:'64 Mo',perms:['Accès aux fichiers','Accès aux images']},
  bmail:{pub:'Bmail',size:'18 Mo',perms:['Contacts','Notifications','Accès au réseau']},
  dessin:{pub:'BAURA Studio',size:'8 Mo',perms:['Accès aux images']},
  taches:{pub:'BAURA Studio',size:'3 Mo',perms:['Notifications']},
  horloge:{pub:'BAURA Studio',size:'2 Mo',perms:['Notifications']},
  serpent:{pub:'Orbite Games',size:'4 Mo',perms:[]},
  calendrier:{pub:'BAURA Studio',size:'6 Mo',perms:['Notifications']},
  contacts:{pub:'BAURA Studio',size:'4 Mo',perms:['Contacts']},
  moniteur:{pub:'BAURA OS',size:'3 Mo',perms:['Informations système']}
};
const EXTRA=[
  {id:'dessin',n:'Dessin',g:G('#34D399','#047857'),d:'<path d="M4 20l1-4L16.5 4.5a2.1 2.1 0 0 1 3 3L8 19z"/><path d="M14.5 6.5l3 3"/>',desc:'Dessine librement et enregistre dans Images.'},
  {id:'taches',n:'Tâches',g:G('#60A5FA','#2563EB'),d:'<rect x="4" y="4" width="16" height="16" rx="4"/><path d="M8.5 12.2l2.4 2.4 4.6-5"/>',desc:'Une liste de choses à faire, simple et rapide.'},
  {id:'horloge',n:'Horloge',g:G('#FBBF24','#D97706'),d:'<circle cx="12" cy="12" r="9"/><path d="M12 7v5l3 2"/>',desc:'Heure, chronomètre et minuteur.'},
  {id:'serpent',n:'Serpent',g:G('#A3E635','#4D7C0F'),d:'<path d="M5 18h8a3 3 0 0 0 0-6H9a3 3 0 0 1 0-6h9"/><circle cx="18" cy="6" r="1" fill="currentColor"/>',desc:'Le jeu classique, version BAURA.'},
  {id:'calendrier',n:'Calendrier',g:G('#F87171','#DC2626'),d:'<rect x="4" y="5" width="16" height="15" rx="3"/><path d="M4 10h16M8 3v4M16 3v4"/>',desc:'Tes événements et rappels, jour par jour.'},
  {id:'contacts',n:'Contacts',g:G('#2DD4BF','#0E7490'),d:'<circle cx="12" cy="9" r="3.5"/><path d="M5 20c0-3.6 3-6 7-6s7 2.4 7 6"/>',desc:'Ton carnet d\'adresses, partagé avec Bmail.'},
  {id:'moniteur',n:'Moniteur',g:G('#4ADE80','#0F766E'),d:'<path d="M3 12h4l2-6 4 12 2-6h6"/>',desc:'Applications ouvertes, mémoire et processeur.'}
];
/* Applications du bundle de base (les formats de fichiers sont gérés par os-office.js, os-creative.js, os-media.js, os-net.js) */
const BASEDEF=[
  {id:'videos',n:'Vidéos',g:G('#64748B','#0F172A'),d:'<rect x="3" y="5" width="18" height="14" rx="3.5"/><path d="M10 9.5v5l4.5-2.5z" fill="currentColor"/>',desc:'Regarde tes vidéos et des clips BAURA.'},
  {id:'skrib',n:'Skrib',g:G('#6EA8FE','#2F5FD8'),d:'<path d="M7 3h7l4 4v14H7z"/><path d="M14 3v4h4M9.5 12h6M9.5 15.5h6M9.5 9h2"/>',desc:'Traitement de texte : lettres, rapports, CV.'},
  {id:'numbro',n:'Numbro',g:G('#4ADE80','#15803D'),d:'<rect x="4" y="4" width="16" height="16" rx="3"/><path d="M4 10h16M4 15h16M10 4v16"/>',desc:'Tableur : calculs, formules et graphiques.'},
  {id:'wepoint',n:'WePoint',g:G('#FDBA74','#EA580C'),d:'<rect x="3" y="4" width="18" height="12" rx="2.5"/><path d="M12 16v4M8 20h8M7.5 12l3-3 2.5 2 3-3.5"/>',desc:'Présentations : diapositives et mode plein écran.'},
  {id:'data',n:'Data',g:G('#C084FC','#7E22CE'),d:'<ellipse cx="12" cy="6" rx="7" ry="3"/><path d="M5 6v6c0 1.7 3.1 3 7 3s7-1.3 7-3V6M5 12v6c0 1.7 3.1 3 7 3s7-1.3 7-3v-6"/>',desc:'Bases de données : tables, formulaires et requêtes.'},
  {id:'crea',n:'Crea',g:G('#22D3EE','#E879F9'),d:'<path d="M12 3l2.6 5.4 5.9.9-4.3 4.2 1 5.9L12 16.6 6.8 19.4l1-5.9L3.5 9.3l5.9-.9z"/>',desc:'Design : affiches, posts et illustrations.'},
  {id:'bmail',n:'Bmail',g:G('#2DD4BF','#0F766E'),d:'<rect x="3" y="5" width="18" height="14" rx="3"/><path d="M3.5 7.5l8.5 6 8.5-6"/>',desc:'Ta messagerie : écris, reçois, organise.'}
];
const STORE_APP={id:'store',n:'App shop',g:G('#F472B6','#7C3AED'),d:IC.store,desc:'Installe et gère tes applications.'};
[...EXTRA,...BASEDEF,STORE_APP].forEach(a=>{ byId[a.id]=a; DESC[a.id]=a.desc });
Object.assign(byId.web,{n:'HOUB'}); DESC.web='Navigue avec HOUB et le moteur FintIt.';
Object.assign(byId.calc,{n:'Calculatrice'}); Object.assign(byId.params,{n:'Paramètres'});
const BASE_IDS=['fichiers','web','videos','photos','musique','notes','skrib','numbro','wepoint','data','crea','bmail','calc','terminal','params','store'];

const Apps=(function(){
  const get=()=>{ let l=Baura.store.get('apps',null); if(!Array.isArray(l)) return BASE_IDS.slice();
    if(Baura.store.get('appsRev',0)<2&&!((Baura.store.get('device')||{}).pack==='min')){ BASE_IDS.forEach(id=>{ if(!l.includes(id)) l.push(id) }); Baura.store.set('apps',l); Baura.store.set('appsRev',2) }
    return l.filter(id=>byId[id]) };
  function sync(){
    const ids=get(); APPS.length=0; ids.forEach(id=>APPS.push(byId[id]));
    document.querySelectorAll('.tbtn.app').forEach(b=>{ if(!ids.includes(b.dataset.app)&&!wins.has(b.dataset.app)) b.remove() });
    Baura.store.get('pinned',[]);
    if(typeof fitPill==='function') fitPill();
  }
  function add(id){ const l=get(); if(!l.includes(id)) l.push(id); Baura.store.set('apps',l); sync() }
  function remove(id){
    if(wins.has(id)) closeApp(id);
    Baura.store.set('apps',get().filter(x=>x!==id));
    const p=Baura.store.get('pinned',[]); if(p.includes(id)) Baura.store.set('pinned',p.filter(x=>x!==id));
    sync(); const b=innerL.querySelector(`.tbtn.app[data-app="${id}"]`); if(b&&!wins.has(id)){ b.remove(); fitPill() }
  }
  Baura.on('store',k=>{ if(k==='apps'||k==='*') sync() });
  sync();
  return {get,add,remove,sync,has:id=>get().includes(id)};
})();

const Sys=(function(){
  const sleep=ms=>new Promise(r=>setTimeout(r,ms));
  const fast=()=>document.documentElement.dataset.motion==='reduce';
  const isSystem=id=>!!(META[id]&&META[id].system);
  function layer(cls,html){
    const el=document.createElement('div'); el.className=cls; el.innerHTML=html; document.body.appendChild(el);
    requestAnimationFrame(()=>requestAnimationFrame(()=>el.classList.add('on')));
    const close=()=>{ el.classList.remove('on'); el.classList.add('out'); setTimeout(()=>el.remove(),240) };
    return {el,close};
  }

  /* ---- Contrôle de compte : « Autoriser cette application à modifier ton appareil ? » ---- */
  function elevate(o){
    o=o||{};
    return new Promise(res=>{
      const acc=Baura.account.get()||{}, needPass=!!acc.pass, a=o.app;
      if(typeof closePanels==='function') closePanels();
      Ctx.hide();
      const prev=document.activeElement;
      const L=layer('uac',`<div class="uacbox" role="alertdialog" aria-modal="true" aria-label="Contrôle de compte">
        <div class="uachead">${svgi(IC.shield)}<div><b>Contrôle de compte BAURA</b><small>Une autorisation d'administrateur est nécessaire</small></div></div>
        <div class="uacbody">
          <div class="uacapp">${a?appIco(a,52):icoHTML(G('#94A3C4','#5B6794'),IC.gear,52)}<div><b>${esc(o.title||(a?a.n:'Programme système'))}</b><small>Éditeur vérifié : ${esc((a&&META[a.id]&&META[a.id].pub)||'BAURA OS')}</small></div></div>
          <p>${esc(o.reason||'Cette application souhaite apporter des modifications à ton appareil.')}</p>
          ${needPass?`<label class="uacpw"><span>Mot de passe de ${esc(acc.name||'Utilisateur')}</span><input type="password" autocomplete="off" spellcheck="false" aria-label="Mot de passe"></label><div class="uacerr" role="alert"></div>`:''}
        </div>
        <div class="uacbtns"><button class="primb" data-r="1">Oui</button><button class="ghostb" data-r="0">Non</button></div></div>`);
      const box=L.el.querySelector('.uacbox'), inp=L.el.querySelector('input'), err=L.el.querySelector('.uacerr');
      let done=false;
      const fin=v=>{ if(done) return; done=true; removeEventListener('keydown',key,true); L.close(); try{prev&&prev.focus&&prev.focus()}catch(e){} res(v) };
      const yes=()=>{
        if(needPass&&inp.value!==String(acc.pass)){ err.textContent='Mot de passe incorrect.'; box.classList.remove('shake'); void box.offsetWidth; box.classList.add('shake'); inp.select(); return }
        fin(true);
      };
      const key=e=>{ if(e.key==='Escape'){ e.preventDefault(); e.stopPropagation(); fin(false) } else if(e.key==='Enter'){ e.preventDefault(); e.stopPropagation(); yes() } };
      addEventListener('keydown',key,true);
      L.el.querySelector('[data-r="1"]').onclick=yes; L.el.querySelector('[data-r="0"]').onclick=()=>fin(false);
      setTimeout(()=>(inp||L.el.querySelector('.primb')).focus(),60);
    });
  }
  async function runAsAdmin(id){
    const a=byId[id]; if(!a) return;
    if(!(await elevate({app:a,reason:`Exécuter « ${a.n} » avec les droits d'administrateur ?`}))) return;
    openApp(id,{admin:true});
    popup({title:a.n+' · administrateur',text:'L\'application s\'exécute avec des droits élevés.',level:'warn',app:id,noOpen:true});
  }

  /* ---- Notification système riche (niveaux + boutons d'action) ---- */
  function popup(o){ notify(Object.assign({app:'params',level:'info',urgent:true},o)) }

  /* ---- Assistant d'installation / désinstallation ---- */
  async function progress(box,labels,ms){
    const bar=box.querySelector('.wbar i'), lab=box.querySelector('.wlab'), pct=box.querySelector('.wpct');
    const total=fast()?600:ms, n=labels.length;
    for(let i=0;i<n;i++){
      lab.textContent=labels[i];
      const from=i/n*100, to=(i+1)/n*100, steps=14;
      for(let k=1;k<=steps;k++){ const v=from+(to-from)*k/steps; bar.style.width=v+'%'; pct.textContent=Math.round(v)+' %'; await sleep(total/n/steps*(0.6+Math.random()*.8)) }
    }
    bar.style.width='100%'; pct.textContent='100 %';
  }
  function wizard(id,mode){
    return new Promise(async res=>{
      const a=byId[id], m=META[id]||{pub:'Inconnu',size:'—',perms:[]}, inst=mode==='install';
      const L=layer('wizscrim',`<div class="wiz" role="dialog" aria-modal="true" aria-label="${inst?'Installation':'Désinstallation'} de ${esc(a.n)}"></div>`);
      const box=L.el.querySelector('.wiz');
      const fin=v=>{ removeEventListener('keydown',key,true); L.close(); res(v) };
      let busy=false;
      const key=e=>{ if(e.key==='Escape'&&!busy&&!document.querySelector('.uac')){ e.stopPropagation(); fin(false) } };
      addEventListener('keydown',key,true);
      box.innerHTML=`<div class="wizh">${appIco(a,64)}<div><b>${esc(a.n)}</b><small>${esc(m.pub)} · ${esc(m.size)}</small></div></div>
        <p class="wizd">${inst?esc(a.desc||DESC[id]||''):`Cette application sera retirée de la barre des tâches et du menu Démarrer. Tes fichiers personnels sont conservés.`}</p>
        ${inst&&m.perms.length?`<div class="wizp"><small>Cette application pourra :</small>${m.perms.map(p=>`<span>${svgi(IC.shield)}${esc(p)}</span>`).join('')}</div>`:''}
        <div class="wizb"><button class="ghostb" data-r="0">Annuler</button><button class="primb${inst?'':' dng'}" data-r="1">${inst?'Installer':'Désinstaller'}</button></div>`;
      box.querySelector('[data-r="0"]').onclick=()=>fin(false);
      box.querySelector('[data-r="1"]').onclick=async()=>{
        busy=true; box.querySelectorAll('button').forEach(b=>b.disabled=true);
        const ok=await elevate({app:a,reason:inst?`Installer « ${a.n} » sur cet appareil ?`:`Désinstaller « ${a.n} » de cet appareil ?`,title:(inst?'Installateur · ':'Désinstallateur · ')+a.n});
        if(!ok){ busy=false; box.querySelectorAll('button').forEach(b=>b.disabled=false); return }
        box.innerHTML=`<div class="wizh">${appIco(a,64)}<div><b>${inst?'Installation de':'Désinstallation de'} ${esc(a.n)}</b><small class="wlab"></small></div></div>
          <div class="wbar"><i></i></div><div class="wpct">0 %</div>`;
        await progress(box,inst?['Téléchargement…','Vérification de la signature…','Installation des fichiers…','Finalisation…']:['Fermeture de l\'application…','Suppression des fichiers…','Nettoyage des raccourcis…'],inst?3400:2200);
        if(inst) Apps.add(id); else Apps.remove(id);
        box.innerHTML=`<div class="wizok">${svgi('<path d="M5 12.5l4.5 4.5L19 7.5"/>')}</div><h3>${esc(a.n)} ${inst?'est installée':'a été désinstallée'}</h3>
          <div class="wizb"><button class="ghostb" data-r="0">Fermer</button>${inst?'<button class="primb" data-r="1">Ouvrir</button>':''}</div>`;
        box.querySelector('[data-r="0"]').onclick=()=>fin(true);
        const o=box.querySelector('[data-r="1"]'); if(o){ o.onclick=()=>{ fin(true); openApp(id) }; o.focus() }
        popup({title:inst?a.n+' installée':a.n+' désinstallée',text:inst?'L\'application est prête à être utilisée.':'Elle a été retirée de cet appareil.',level:'ok',app:inst?id:'store',noOpen:!inst,
          actions:inst?[{label:'Ouvrir',run:()=>openApp(id)}]:[{label:'Réinstaller',run:()=>install(id)}]});
      };
      setTimeout(()=>box.querySelector('.primb').focus(),80);
    });
  }
  const install=id=>byId[id]&&!Apps.has(id)?wizard(id,'install'):Promise.resolve(false);
  function uninstall(id){
    if(!byId[id]) return;
    if(isSystem(id)){ Dlg.alert({title:'Application système',text:'« '+byId[id].n+' » fait partie de BAURA OS et ne peut pas être désinstallée.'}); return }
    if(!Apps.has(id)) return;
    return wizard(id,'uninstall');
  }
  function notInstalled(id){
    const a=byId[id]; if(!a) return;
    popup({title:a.n+' n\'est pas installée',text:'Tu peux la réinstaller depuis la Boutique.',level:'warn',app:'store',noOpen:true,
      actions:[{label:'Installer',run:()=>install(id)},{label:'Ouvrir la Boutique',run:()=>openApp('store')}]});
  }
  return {elevate,runAsAdmin,popup,install,uninstall,notInstalled,isSystem,meta:META};
})();

/* ---------- Boutique ---------- */
AppViews.store=function(root,ctx){
  let tab='all', q='';
  root.innerHTML=`<div class="bq"><div class="bqbar"><div class="seg"><button data-t="all" class="on">Toutes</button><button data-t="inst">Installées</button><button data-t="avail">Disponibles</button></div>
    <span class="grow"></span><label class="fmsearch">${svgi(IC.search)}<input placeholder="Rechercher une appli" aria-label="Rechercher"></label></div><div class="bqlist"></div></div>`;
  const list=root.querySelector('.bqlist');
  const all=()=>Object.values(byId).filter(a=>a.id!=='store'||true);
  function render(){
    let a=all().filter(x=>(tab==='all'||(tab==='inst')===Apps.has(x.id))&&(!q||(x.n+' '+(DESC[x.id]||'')).toLowerCase().includes(q)));
    list.innerHTML=a.length?a.map(x=>{ const inst=Apps.has(x.id), m=META[x.id]||{};
      return `<div class="bqc">${appIco(x,52)}<div class="tx"><b>${esc(x.n)}</b><small>${esc(DESC[x.id]||'')}</small><em>${esc(m.pub||'')} · ${esc(m.size||'')}${m.system?' · Système':''}</em></div>
      ${inst?`<button class="stbtn" data-o="${x.id}">Ouvrir</button>${m.system?'':`<button class="stbtn dng" data-u="${x.id}">Désinstaller</button>`}`:`<button class="primb" data-i="${x.id}">Installer</button>`}</div>` }).join(''):'<div class="fmempty"><b>Aucun résultat</b><span>Essaie un autre mot.</span></div>';
  }
  root.addEventListener('click',e=>{
    const t=e.target.closest('[data-t]'); if(t){ tab=t.dataset.t; root.querySelectorAll('[data-t]').forEach(x=>x.classList.toggle('on',x===t)); render(); return }
    const i=e.target.closest('[data-i]'); if(i){ Sys.install(i.dataset.i); return }
    const u=e.target.closest('[data-u]'); if(u){ Sys.uninstall(u.dataset.u); return }
    const o=e.target.closest('[data-o]'); if(o) openApp(o.dataset.o);
  });
  root.querySelector('.fmsearch input').addEventListener('input',e=>{ q=e.target.value.toLowerCase().trim(); render() });
  const h=k=>{ if(root.isConnected&&(k==='apps'||k==='*')) render() }; Baura.on('store',h);
  render();
};

/* ---------- Dessin ---------- */
AppViews.dessin=function(root,ctx){
  const COL=['#17133d','#4F46E5','#8B5CF6','#EC4899','#10B981','#D0AE2A','#EF4444','#ffffff'];
  root.innerHTML=`<div class="ds"><div class="dsbar">${COL.map((c,i)=>`<button class="dsc${i?'':' on'}" data-c="${c}" style="background:${c}" aria-label="Couleur ${c}"></button>`).join('')}
    <input type="range" min="2" max="40" value="8" aria-label="Taille"><span class="grow"></span><button class="tb txt" data-a="clear">Effacer</button><button class="tb txt" data-a="save">Enregistrer</button></div><div class="dscv"><canvas></canvas></div></div>`;
  const cv=root.querySelector('canvas'), box=root.querySelector('.dscv'), c2=cv.getContext('2d'), size=root.querySelector('input');
  let col=COL[0], draw=false, lx=0, ly=0;
  function fit(){ const r=box.getBoundingClientRect(); if(!r.width) return; const keep=cv.width?c2.getImageData(0,0,cv.width,cv.height):null; cv.width=Math.round(r.width); cv.height=Math.round(r.height); c2.fillStyle='#fff'; c2.fillRect(0,0,cv.width,cv.height); if(keep) c2.putImageData(keep,0,0); c2.lineCap='round'; c2.lineJoin='round' }
  new ResizeObserver(fit).observe(box); setTimeout(fit,30);
  const pt=e=>{ const r=cv.getBoundingClientRect(); return [e.clientX-r.left,e.clientY-r.top] };
  cv.addEventListener('pointerdown',e=>{ draw=true; [lx,ly]=pt(e); cv.setPointerCapture(e.pointerId); c2.fillStyle=col; c2.beginPath(); c2.arc(lx,ly,size.value/2,0,7); c2.fill() });
  cv.addEventListener('pointermove',e=>{ if(!draw) return; const [x,y]=pt(e); c2.strokeStyle=col; c2.lineWidth=size.value; c2.beginPath(); c2.moveTo(lx,ly); c2.lineTo(x,y); c2.stroke(); lx=x; ly=y });
  const up=()=>{ draw=false }; cv.addEventListener('pointerup',up); cv.addEventListener('pointercancel',up);
  root.addEventListener('click',e=>{
    const c=e.target.closest('[data-c]'); if(c){ col=c.dataset.c; root.querySelectorAll('.dsc').forEach(x=>x.classList.toggle('on',x===c)); return }
    const a=e.target.closest('[data-a]')?.dataset.a;
    if(a==='clear'){ c2.fillStyle='#fff'; c2.fillRect(0,0,cv.width,cv.height) }
    else if(a==='save'){ FS.create({name:'Dessin.png',type:'file',parent:'images',kind:'image',src:cv.toDataURL('image/png')}); Sys.popup({title:'Dessin enregistré',text:'Retrouve-le dans Images.',level:'ok',app:'dessin',noOpen:true,actions:[{label:'Voir',run:()=>openApp('fichiers',{folder:'images'})}]}) }
  });
};

/* ---------- Tâches ---------- */
AppViews.taches=function(root,ctx){
  root.innerHTML=`<div class="td"><h2>Mes tâches</h2><form class="tdadd"><input placeholder="Ajouter une tâche…" aria-label="Nouvelle tâche" autocomplete="off"><button class="primb" type="submit">Ajouter</button></form><div class="tdl"></div><div class="fmstat"></div></div>`;
  const l=root.querySelector('.tdl'), st=root.querySelector('.fmstat'), inp=root.querySelector('input');
  const get=()=>Baura.store.get('todos',[{t:'Découvrir BAURA OS',d:true},{t:'Installer une appli depuis la Boutique',d:false}]), set=v=>Baura.store.set('todos',v);
  function render(){ const a=get(); l.innerHTML=a.length?a.map((x,i)=>`<div class="tdi${x.d?' done':''}"><button class="tdc" data-k="${i}" role="checkbox" aria-checked="${x.d}" aria-label="Terminer">${x.d?svgi('<path d="M5 12.5l4.5 4.5L19 7.5"/>'):''}</button><span>${esc(x.t)}</span><button class="tb" data-x="${i}" aria-label="Supprimer">${svgi(IC.x)}</button></div>`).join(''):'<div class="fmempty"><b>Rien à faire</b><span>Profites-en !</span></div>'; st.textContent=a.filter(x=>!x.d).length+' à faire · '+a.filter(x=>x.d).length+' terminée(s)' }
  root.querySelector('form').onsubmit=e=>{ e.preventDefault(); const v=inp.value.trim(); if(!v) return; set([...get(),{t:v,d:false}]); inp.value=''; render() };
  l.addEventListener('click',e=>{ const k=e.target.closest('[data-k]'), x=e.target.closest('[data-x]'), a=get(); if(k){ a[+k.dataset.k].d=!a[+k.dataset.k].d; set(a) } else if(x){ a.splice(+x.dataset.x,1); set(a) } render() });
  render();
};

/* ---------- Horloge ---------- */
AppViews.horloge=function(root,ctx){
  root.innerHTML=`<div class="hl"><div class="seg"><button data-t="time" class="on">Heure</button><button data-t="chrono">Chronomètre</button><button data-t="timer">Minuteur</button></div><div class="hlv"></div></div>`;
  const v=root.querySelector('.hlv'); let tab='time', t0=0, acc=0, run=false, left=0, tend=0, tm=0;
  const f=(ms,c)=>{ const s=Math.floor(ms/1000), m=Math.floor(s/60); return String(m).padStart(2,'0')+':'+String(s%60).padStart(2,'0')+(c?'.'+String(Math.floor(ms%1000/10)).padStart(2,'0'):'') };
  function draw(){
    if(tab==='time'){ const n=new Date(); v.innerHTML=`<div class="hlbig">${n.toLocaleTimeString('fr-FR')}</div><div class="hlsub">${n.toLocaleDateString('fr-FR',{weekday:'long',day:'numeric',month:'long'})}</div>` }
    else if(tab==='chrono'){ const ms=acc+(run?Date.now()-t0:0); v.innerHTML=`<div class="hlbig">${f(ms,1)}</div><div class="hlb"><button class="primb" data-a="go">${run?'Pause':'Démarrer'}</button><button class="ghostb" data-a="rz">Réinitialiser</button></div>` }
    else { const ms=run?Math.max(0,tend-Date.now()):left; if(run&&ms<=0){ run=false; left=0; Sys.popup({title:'Minuteur terminé',text:'Le temps est écoulé.',level:'warn',app:'horloge'}) }
      v.innerHTML=`<div class="hlbig">${f(ms)}</div><div class="hlb">${run?'':`<input type="number" min="1" max="180" value="${Math.max(1,Math.round((left||300000)/60000))}" aria-label="Minutes"><span>min</span>`}<button class="primb" data-a="tgo">${run?'Annuler':'Démarrer'}</button></div>` }
  }
  root.addEventListener('click',e=>{
    const t=e.target.closest('[data-t]'); if(t){ tab=t.dataset.t; root.querySelectorAll('[data-t]').forEach(x=>x.classList.toggle('on',x===t)); draw(); return }
    const a=e.target.closest('[data-a]')?.dataset.a;
    if(a==='go'){ if(run){ acc+=Date.now()-t0; run=false } else { t0=Date.now(); run=true } }
    else if(a==='rz'){ acc=0; run=false }
    else if(a==='tgo'){ if(run){ run=false; left=0 } else { const m=+v.querySelector('input').value||5; left=m*60000; tend=Date.now()+left; run=true } }
    draw();
  });
  tm=setInterval(()=>{ if(!root.isConnected) return; if(tab==='time'||run) { const fo=v.querySelector('input'); if(!fo) draw() } },100);
  draw();
  return {destroy(){ clearInterval(tm) }};
};

/* ---------- Serpent ---------- */
AppViews.serpent=function(root,ctx){
  root.innerHTML=`<div class="sn" tabindex="0"><div class="snh"><b>Score : <span id="snS">0</span></b><span class="snm">Flèches pour jouer · Espace pour rejouer</span></div><canvas width="360" height="360"></canvas></div>`;
  const box=root.querySelector('.sn'), cv=root.querySelector('canvas'), c=cv.getContext('2d'), sc=root.querySelector('#snS'), N=18, S=cv.width/N;
  let sn,dir,nd,food,score,over,tm;
  function reset(){ sn=[[9,9],[8,9],[7,9]]; dir=nd=[1,0]; score=0; over=false; food=spawn(); sc.textContent=0 }
  const spawn=()=>{ let p; do{ p=[Math.floor(Math.random()*N),Math.floor(Math.random()*N)] }while(sn.some(s=>s[0]===p[0]&&s[1]===p[1])); return p };
  function step(){
    if(over||!root.isConnected) return; dir=nd; const h=[sn[0][0]+dir[0],sn[0][1]+dir[1]];
    if(h[0]<0||h[1]<0||h[0]>=N||h[1]>=N||sn.some(s=>s[0]===h[0]&&s[1]===h[1])){ over=true; paint(); return }
    sn.unshift(h); if(h[0]===food[0]&&h[1]===food[1]){ score++; sc.textContent=score; food=spawn() } else sn.pop(); paint();
  }
  function paint(){
    const cs=getComputedStyle(document.documentElement); c.fillStyle=Baura.settings.isDark()?'#1b1646':'#efeaff'; c.fillRect(0,0,cv.width,cv.height);
    c.fillStyle='#D0AE2A'; c.beginPath(); c.arc(food[0]*S+S/2,food[1]*S+S/2,S/3,0,7); c.fill();
    sn.forEach((s,i)=>{ c.fillStyle=i?cs.getPropertyValue('--a1')||'#4F46E5':cs.getPropertyValue('--a2')||'#8B5CF6'; c.beginPath(); c.roundRect(s[0]*S+1,s[1]*S+1,S-2,S-2,6); c.fill() });
    if(over){ c.fillStyle='rgba(0,0,0,.5)'; c.fillRect(0,0,cv.width,cv.height); c.fillStyle='#fff'; c.font='700 24px Inter,sans-serif'; c.textAlign='center'; c.fillText('Perdu · '+score+' pt',cv.width/2,cv.height/2); c.font='14px Inter,sans-serif'; c.fillText('Espace pour rejouer',cv.width/2,cv.height/2+28) }
  }
  box.addEventListener('keydown',e=>{
    const m={ArrowUp:[0,-1],ArrowDown:[0,1],ArrowLeft:[-1,0],ArrowRight:[1,0]}[e.key];
    if(m){ e.preventDefault(); if(m[0]!==-dir[0]||m[1]!==-dir[1]) nd=m } else if(e.key===' '&&over){ e.preventDefault(); reset(); paint() }
  });
  box.addEventListener('pointerdown',()=>box.focus());
  reset(); paint(); tm=setInterval(step,130); setTimeout(()=>box.focus(),80);
  return {destroy(){ clearInterval(tm) }};
};

/* ---------- Documents d'exemple (fournis par les applications, créés une seule fois) ---------- */
window.OfficeSamples=window.OfficeSamples||{};
function seedSamples(){
  const done=Baura.store.get('samplesDone',[]); let ch=false;
  Object.entries(OfficeSamples).forEach(([k,s])=>{ if(done.includes(k)||!s||!s.name) return; if(FS.get(s.parent||'documents')) FS.create({name:s.name,type:'file',parent:s.parent||'documents',content:s.content||''}); done.push(k); ch=true });
  if(ch) Baura.store.set('samplesDone',done);
}
