/* ===== BAURA OS – application Paramètres ===== */
AppViews.params=function(root,ctx){
  const SECTIONS=[
    ['perso','Personnalisation','<path d="M12 3a9 9 0 1 0 0 18c1.4 0 2-1 1.6-2-.5-1.2.3-2.5 1.7-2.5H17a4 4 0 0 0 4-4C21 6.6 17 3 12 3z"/><circle cx="8" cy="11" r="1"/><circle cx="12" cy="7.5" r="1"/><circle cx="16" cy="10" r="1"/>'],
    ['ecran','Affichage','<rect x="3" y="4" width="18" height="12" rx="3"/><path d="M8 20h8M12 16v4"/>'],
    ['access','Accessibilité','<circle cx="12" cy="5" r="2"/><path d="M5 9l7 1.5L19 9M12 10.5V15l-3 5M12 15l3 5"/>'],
    ['apps','Applications','<rect x="4" y="4" width="7" height="7" rx="2"/><rect x="13" y="4" width="7" height="7" rx="2"/><rect x="4" y="13" width="7" height="7" rx="2"/><path d="M16.5 13v7M13 16.5h7"/>'],
    ['compte','Compte','<circle cx="12" cy="8" r="4"/><path d="M4 21c0-4.4 3.6-7 8-7s8 2.6 8 7"/>'],
    ['sys','Système','<circle cx="12" cy="12" r="9"/><path d="M12 8v4M12 16h.01"/>']
  ];
  let sec=ctx.args.section&&SECTIONS.some(s=>s[0]===ctx.args.section)?ctx.args.section:'perso';
  root.innerHTML='<div class="st"><nav class="stside" aria-label="Sections"></nav><section class="stmain"></section></div>';
  const side=root.querySelector('.stside'), main=root.querySelector('.stmain');
  const tog=(k,v)=>`<button class="sw2" role="switch" aria-checked="${!!v}" data-t="${k}" aria-label="${k}"></button>`;
  const row=(t,s,c)=>`<div class="sr"><div class="tx"><b>${t}</b>${s?`<small>${s}</small>`:''}</div>${c||''}</div>`;

  function renderSide(){
    const a=Baura.account.get(), n=(a&&a.name)||'Utilisateur';
    side.innerHTML=`<div class="who"><span class="av">${esc(n[0].toUpperCase())}</span><div><b>${esc(n)}</b><small>Compte local</small></div></div>`+
      SECTIONS.map(([id,n,d])=>`<button class="pl${id===sec?' on':''}" data-s="${id}">${svgi(d)}<span>${n}</span></button>`).join('');
  }
  function renderMain(){
    const s=Baura.settings.get(), acc=Baura.account.get()||{}, t=SECTIONS.find(x=>x[0]===sec)[1];
    let h=`<h2>${t}</h2>`;
    if(sec==='perso'){
      h+=`<div class="sg">${row('Thème','Clair, sombre, ou selon ton appareil',`<div class="themes">${[['auto','Auto','a'],['light','Clair','l'],['dark','Sombre','d']].map(([k,n,c])=>`<button class="tcard${s.theme===k?' on':''}" data-th="${k}"><i class="${c}"></i>${n}</button>`).join('')}</div>`)}
        ${row('Couleur d\'accent','Boutons, curseurs et sélections',`<div class="accs">${Object.entries(Baura.settings.accents).map(([k,v])=>`<button class="sw${s.accent===k?' on':''}" data-acc="${k}" aria-label="${k}" style="background:linear-gradient(145deg,${v[1]},${v[0]})"></button>`).join('')}</div>`)}</div>
        <div class="sg"><div class="sr" style="display:block"><div class="tx"><b>Fond d'écran</b><small>Clique sur une image pour l'appliquer</small></div>
        <div class="pgrid" style="margin-top:12px">${Baura.settings.walls.map(w=>`<button class="pth${s.wall===w.id?' cur':''}" data-w="${w.id}"><img src="${w.id==='aura'?Baura.settings.wallFile(s):'wallpaper-'+w.id+'.svg'}" alt=""><span>${w.n}</span></button>`).join('')}
        ${s.wall==='custom'&&s.wallCustom?`<button class="pth cur" data-w="custom"><img src="${esc(s.wallCustom)}" alt=""><span>Personnalisé</span></button>`:''}</div>
        <p style="margin:12px 0 0;font-size:12.5px;color:var(--mut)">Pour utiliser tes propres images, ouvre-en une dans Photos puis choisis « Définir comme fond d'écran ».</p></div></div>`;
    } else if(sec==='ecran'){
      h+=`<div class="sg">${row('Luminosité','',`<input type="range" min="20" max="100" value="${s.bright}" data-r="bright" aria-label="Luminosité">`)}
        ${row('Veilleuse','Réchauffe les couleurs le soir',tog('night',s.night))}
        ${row('Économie d\'énergie','Réduit la transparence et les animations',tog('saver',s.saver))}</div>`;
    } else if(sec==='access'){
      h+=`<div class="sg">${row('Transparence du verre','Désactive-la pour des panneaux plus lisibles',tog('glass',s.glass!==false))}
        ${row('Contraste élevé','Bordures et textes plus marqués',tog('contrast',s.contrast))}
        ${row('Réduire les animations','Moins de mouvements dans l\'interface',tog('motion',s.motion))}</div>
        <div class="sg">${row('Raccourcis clavier','',`<span style="font-size:12.5px;color:var(--mut);text-align:right;line-height:1.7">Ctrl+Espace · recherche<br>Alt+W / F3 · vue des tâches<br>Ctrl+Alt+L · verrouiller<br>Échap · fermer un panneau</span>`)}</div>`;
    } else if(sec==='apps'){
      h+=`<div class="sg">${APPS.map(a=>{ const m=Sys.meta[a.id]||{}; return row(esc(a.n),esc((m.pub||'')+' · '+(m.size||'')+(m.system?' · Système':'')),`<div style="display:flex;gap:8px;align-items:center">${appIco(a,30)}${m.system?'':`<button class="stbtn dng" data-un="${a.id}">Désinstaller</button>`}</div>`) }).join('')}</div>
        <div class="sg">${row('Ajouter des applications','Parcours la Boutique',`<button class="stbtn" data-a="store">Ouvrir la Boutique</button>`)}</div>`;
    } else if(sec==='compte'){
      h+=`<div class="sg">${row('Nom','Affiché sur l\'écran de connexion',`<button class="stbtn" data-a="name">${esc(acc.name||'Utilisateur')} — modifier</button>`)}
        ${row('Mot de passe',acc.pass?'Défini':'Aucun mot de passe',`<button class="stbtn" data-a="pass">Modifier</button>`)}</div>
        <div class="sg">${row('Verrouiller maintenant','',`<button class="stbtn" data-a="lock">Verrouiller</button>`)}</div>`;
    } else {
      h+=`<div class="sg"><div class="about"><svg viewBox="150 29 199 420"><rect x="150" y="29" width="48" height="420" rx="24" fill="var(--b1)"/><path d="M248 29A101 102.5 0 0 1 248 234Z" fill="var(--b2)"/><path d="M248 245A101 102 0 0 1 248 449Z" fill="var(--b3)"/><rect x="316" y="221" width="33" height="36" rx="15" fill="var(--b4)"/></svg><div><b>BAURA OS</b><small>Prototype d'interface · HTML / CSS / JS</small><small>${esc(((Baura.store.get('device')||{}).name)||'Appareil')} · ${(Baura.store.get('device')||{}).diskGb?'Disque '+Baura.store.get('device').diskGb+' Go':'disque virtuel'} · ${FS.all().filter(f=>f.type==='file').length} fichiers</small></div></div></div>
        <div class="sg">${row('Réinstaller BAURA OS','Relance l\'installateur graphique (efface tout)',`<button class="stbtn dng" data-a="reinstall">Réinstaller</button>`)}
        ${row('Réinitialiser les réglages','Thème, accent, fond d\'écran, accessibilité',`<button class="stbtn" data-a="resetset">Réinitialiser</button>`)}
        ${row('Réinitialiser BAURA OS','Efface tes fichiers, ton compte et tes réglages',`<button class="stbtn dng" data-a="resetall">Tout effacer</button>`)}</div>`;
    }
    main.innerHTML=h; main.querySelectorAll('input[type=range]').forEach(paintRange);
  }
  function render(){ renderSide(); renderMain() }
  root.addEventListener('click',async e=>{
    const sb=e.target.closest('[data-s]'); if(sb){ sec=sb.dataset.s; render(); return }
    const th=e.target.closest('[data-th]'); if(th){ Baura.settings.set({theme:th.dataset.th}); return }
    const ac=e.target.closest('[data-acc]'); if(ac){ Baura.settings.set({accent:ac.dataset.acc}); return }
    const w=e.target.closest('[data-w]'); if(w){ if(w.dataset.w!=='custom') Baura.settings.set({wall:w.dataset.w}); return }
    const t=e.target.closest('[data-t]'); if(t){ const k=t.dataset.t, s=Baura.settings.get(); Baura.settings.set({[k]:k==='glass'?s.glass===false:!s[k]}); return }
    const un=e.target.closest('[data-un]'); if(un){ Sys.uninstall(un.dataset.un); return }
    const a=e.target.closest('[data-a]')?.dataset.a; if(!a) return;
    if(a==='store'){ openApp('store'); return }
    const acc=Baura.account.get()||{name:'Utilisateur',pass:''};
    if(a==='name'){ const n=await Dlg.prompt({title:'Nom du compte',text:'Comment t\'appelles-tu ?',value:acc.name,ok:'Enregistrer'}); if(n&&n.trim()){ Baura.account.set(Object.assign({},acc,{name:n.trim()})); notify({app:'params',title:'Compte mis à jour',text:'Ton nom est maintenant '+n.trim()+'.'}) } }
    else if(a==='pass'){ const p=await Dlg.prompt({title:'Mot de passe',text:'Nouveau mot de passe (laisse vide pour aucun) :',value:'',ok:'Enregistrer'}); if(p!==null){ Baura.account.set(Object.assign({},acc,{pass:p})); renderMain() } }
    else if(a==='lock') power('lock');
    else if(a==='reinstall'){ if(await Dlg.confirm({title:'Réinstaller BAURA OS ?',text:'Le disque virtuel sera effacé et l\'installateur se lancera au redémarrage.',ok:'Réinstaller',danger:true})&&await Sys.elevate({title:'Réinstallation de BAURA OS',reason:'Effacer le disque et relancer l\'installation ?'})){ FS.reset(); Baura.store.clear(); power('restart') } }
    else if(a==='resetset'){ Baura.settings.reset(); }
    else if(a==='resetall'){ if(await Dlg.confirm({title:'Tout effacer ?',text:'Fichiers, compte et réglages seront supprimés, puis BAURA OS redémarrera.',ok:'Effacer et redémarrer',danger:true})){ FS.reset(); Baura.store.clear(); power('restart') } }
  });
  root.addEventListener('input',e=>{ const r=e.target.dataset.r; if(r){ Baura.settings.set({[r]:+e.target.value}); paintRange(e.target) } });
  const refresh=()=>{ if(root.isConnected&&!root.contains(document.activeElement&&document.activeElement.type==='range'?document.activeElement:null)) render() };
  Baura.on('settings',refresh); Baura.on('store',k=>{ if(k==='account'||k==='apps'||k==='*') refresh() });
  render();
  return {open(a){ if(a&&a.section&&SECTIONS.some(s=>s[0]===a.section)){ sec=a.section; render() } }};
};
