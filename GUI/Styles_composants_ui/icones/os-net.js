/* ===== BAURA OS – Réseau & organisation : HOUB (FintIt), Bmail, Calendrier, Contacts, Moniteur ===== */
const NetKit=(function(){
  const E=esc, day=d=>{ const x=new Date(d); return x.getFullYear()+'-'+String(x.getMonth()+1).padStart(2,'0')+'-'+String(x.getDate()).padStart(2,'0') };
  const me=()=>{ const a=Baura.account.get(); const n=((a&&a.name)||'utilisateur'); return {name:n,email:n.toLowerCase().normalize('NFD').replace(/[̀-ͯ]/g,'').replace(/[^a-z0-9]+/g,'.').replace(/^\.|\.$/g,'')+'@baura.mail'} };
  const SEED_CONTACTS=[{id:'c1',name:'Amina Koné',email:'amina@baura.mail',phone:'+225 01 02 03 04 05',note:'Design produit'},{id:'c2',name:'Karim Benali',email:'karim@baura.mail',phone:'+212 6 12 34 56 78',note:'Développement'},{id:'c3',name:'Sofia Marti',email:'sofia@baura.mail',phone:'',note:'Marketing'},{id:'c4',name:'Équipe BAURA',email:'equipe@baura.mail',phone:'',note:'Support'}];
  const contacts=()=>{ let c=Baura.store.get('contacts',null); if(!Array.isArray(c)){ c=SEED_CONTACTS; Baura.store.set('contacts',c) } return c };
  const saveContacts=c=>Baura.store.set('contacts',c);
  return {E,day,me,contacts,saveContacts};
})();

/* =============================== HOUB + FINTIT =============================== */
(function(){
const {E,me}=NetKit;
const SITES=[
 ['Wikipédia','https://fr.wikipedia.org','L\'encyclopédie libre, en français.','encyclopédie savoir article wikipedia culture histoire'],
 ['OpenStreetMap','https://www.openstreetmap.org','La carte du monde, libre et collaborative.','carte map plan itinéraire gps lieu'],
 ['GitHub','https://github.com','Héberge et partage du code source.','code programmation développeur git projet open source'],
 ['MDN Web Docs','https://developer.mozilla.org/fr/','Documentation HTML, CSS et JavaScript.','html css javascript web documentation développeur'],
 ['Anthropic','https://www.anthropic.com','Recherche et produits d\'IA fiable : Claude.','claude ia intelligence artificielle anthropic'],
 ['Claude','https://claude.ai','Un assistant IA pour écrire, coder et analyser.','claude assistant ia chat'],
 ['Internet Archive','https://archive.org','Bibliothèque numérique : livres, vidéos, sites.','archive livres wayback bibliothèque'],
 ['Project Gutenberg','https://www.gutenberg.org','Des milliers de livres du domaine public.','livres ebooks lecture roman gratuit'],
 ['Khan Academy','https://fr.khanacademy.org','Cours gratuits de maths, sciences et plus.','cours école apprendre maths sciences éducation'],
 ['Stack Overflow','https://stackoverflow.com','Questions et réponses de programmation.','code question erreur bug programmation aide'],
 ['DuckDuckGo','https://duckduckgo.com','Moteur de recherche respectueux de la vie privée.','recherche moteur web vie privée'],
 ['Wikimedia Commons','https://commons.wikimedia.org','Images et médias libres de droits.','images photos médias libres'],
 ['NASA','https://www.nasa.gov','Espace, missions et sciences de la Terre.','espace nasa planète fusée astronomie'],
 ['Météo-France','https://meteofrance.com','Prévisions météo en France.','météo temps pluie prévisions climat'],
 ['The Conversation','https://theconversation.com/fr','Analyses de chercheurs, accessibles à tous.','actualité analyse recherche science'],
 ['Can I use','https://caniuse.com','Compatibilité des fonctions web par navigateur.','navigateur compatibilité css javascript support'],
 ['Unsplash','https://unsplash.com','Photos gratuites de haute qualité.','photos images fond écran wallpaper'],
 ['Figma','https://www.figma.com','Design d\'interfaces collaboratif.','design interface ui maquette'],
 ['Coolors','https://coolors.co','Générateur de palettes de couleurs.','couleurs palette design'],
 ['Free Music Archive','https://freemusicarchive.org','Musique libre à écouter et télécharger.','musique son audio libre']
];
const TILES=[['Wikipédia',0,'#4F46E5'],['OpenStreetMap',1,'#10B981'],['GitHub',2,'#334155'],['MDN',3,'#0EA5E9'],['Claude',5,'#D97706'],['Unsplash',16,'#EC4899']];
const hist=()=>Baura.store.get('houb.history',[]), bms=()=>Baura.store.get('houb.bm',[]);
const addHist=(u,t)=>{ const h=hist().filter(x=>x.u!==u); h.unshift({u,t,d:Date.now()}); Baura.store.set('houb.history',h.slice(0,120)) };
const norm=s=>s.toLowerCase().normalize('NFD').replace(/[̀-ͯ]/g,'');
function instant(q){
  const out=[], n=norm(q.trim());
  if(/^[\d\s+\-*\/().,%^]+$/.test(q.trim())&&/[+\-*\/^%]/.test(q.trim().replace(/^-/,''))){ try{ const ex=q.replace(/,/g,'.').replace(/\^/g,'**').replace(/(\d+(?:\.\d+)?)%/g,'($1/100)'); const v=Function('"use strict";return ('+ex+')')(); if(isFinite(v)) out.push({k:'Calcul',big:String(Math.round(v*1e10)/1e10).replace('.',','),sub:q.trim()+' ='}) }catch(e){} }
  if(/\b(heure|time|quelle heure)\b/.test(n)) out.push({k:'Heure',big:new Date().toLocaleTimeString('fr-FR',{hour:'2-digit',minute:'2-digit',timeZone:(Baura.store.get('device')||{}).tz||undefined}),sub:'Heure actuelle de ton appareil'});
  if(/\b(date|aujourd'?hui|jour)\b/.test(n)) out.push({k:'Date',big:new Date().toLocaleDateString('fr-FR',{weekday:'long',day:'numeric',month:'long'}),sub:new Date().getFullYear()+''});
  const m=/^(-?\d+(?:[.,]\d+)?)\s*(km|m|cm|kg|g|c|f|°c|°f)\s*(?:en|to|->)\s*(m|km|cm|mi|miles|g|kg|lb|f|c|°c|°f)$/.exec(n);
  if(m){ const v=parseFloat(m[1].replace(',','.')), a=m[2].replace('°',''), b=m[3].replace('°',''), L={km:1000,m:1,cm:.01,mi:1609.344,miles:1609.344}, W={kg:1000,g:1,lb:453.592}; let r=null;
    if(L[a]&&L[b]) r=v*L[a]/L[b]; else if(W[a]&&W[b]) r=v*W[a]/W[b]; else if(a==='c'&&b==='f') r=v*9/5+32; else if(a==='f'&&b==='c') r=(v-32)*5/9; if(r!==null) out.push({k:'Conversion',big:(Math.round(r*1000)/1000+'').replace('.',',')+' '+m[3],sub:m[1]+' '+m[2]+' ='}) }
  return out;
}
function search(q){
  const n=norm(q), words=n.split(/\s+/).filter(Boolean);
  const apps=APPS.filter(a=>words.every(w=>norm(a.n+' '+(DESC[a.id]||'')).includes(w))).map(a=>({a}));
  const files=FS.all().filter(f=>f.type==='file'&&!FS.inTrash(f.id)&&words.every(w=>norm(f.name).includes(w)||(typeof f.content==='string'&&f.content.length<20000&&norm(f.content).includes(w)))).slice(0,12);
  const web=SITES.map((s,i)=>{ const hay=norm(s[0]+' '+s[2]+' '+s[3]); let sc=0; words.forEach(w=>{ if(norm(s[0]).includes(w)) sc+=5; if(hay.includes(w)) sc+=2 }); return {i,sc} }).filter(x=>x.sc>0).sort((a,b)=>b.sc-a.sc).map(x=>SITES[x.i]);
  return {apps,files,web};
}
const looksUrl=s=>/^(https?:\/\/|www\.)/i.test(s)||/^[\w-]+(\.[\w-]+)+(\/\S*)?$/.test(s)&&!/\s/.test(s);

AppViews.web=function(root,ctx){
  let stack=[{u:'baura://home'}], sp=0;
  root.innerHTML=`<div class="hb"><div class="hbbar"><button class="tb" data-a="back" aria-label="Précédent">${svgi('<path d="M14 6l-6 6 6 6"/>')}</button><button class="tb" data-a="fwd" aria-label="Suivant">${svgi('<path d="M10 6l6 6-6 6"/>')}</button><button class="tb" data-a="reload" aria-label="Actualiser">${svgi(IC.refresh)}</button><button class="tb" data-a="home" aria-label="Accueil">${svgi('<path d="M4 11l8-7 8 7v9H4z"/>')}</button>
    <label class="hburl">${svgi('<circle cx="11" cy="11" r="6.5"/><path d="M16 16l4 4"/>')}<input aria-label="Adresse ou recherche FintIt" placeholder="Rechercher avec FintIt ou saisir une adresse" spellcheck="false" autocomplete="off"></label>
    <button class="tb" data-a="star" aria-label="Favori">${BI.i('star')}</button><button class="tb" data-a="menu" aria-label="Menu">${BI.i('menu')}</button></div>
    <div class="hbview"></div></div>`;
  const $=s=>root.querySelector(s), inp=$('.hburl input'), view=$('.hbview');
  const cur=()=>stack[sp];
  function go(u,push=true){ u=u.trim(); if(!u) return; if(!/^baura:\/\//.test(u)){ if(looksUrl(u)){ if(!/^https?:\/\//i.test(u)) u='https://'+u } else u='baura://search?q='+encodeURIComponent(u) }
    if(push){ stack=stack.slice(0,sp+1); stack.push({u}); sp=stack.length-1 } else stack[sp]={u}; render() }
  function titleOf(u){ if(u==='baura://home') return 'Accueil HOUB'; if(u.startsWith('baura://search')) return new URLSearchParams(u.split('?')[1]).get('q')+' – FintIt'; if(u==='baura://history') return 'Historique'; if(u==='baura://bookmarks') return 'Favoris'; try{ return new URL(u).hostname.replace(/^www\./,'') }catch(e){ return u } }
  function render(){
    const u=cur().u, t=titleOf(u); ctx.setTitle(t); inp.value=u==='baura://home'?'':u.startsWith('baura://search')?decodeURIComponent((u.split('q=')[1]||'')):u;
    $('[data-a=back]').disabled=sp<=0; $('[data-a=fwd]').disabled=sp>=stack.length-1; const bm=bms().some(b=>b.u===u); const st=$('[data-a=star]'); st.innerHTML=BI.i(bm?'starFill':'star'); st.classList.toggle('on',bm);
    if(u==='baura://home') home(); else if(u.startsWith('baura://search')) results(decodeURIComponent((u.split('q=')[1]||'').replace(/\+/g,' '))); else if(u==='baura://history') histPage(); else if(u==='baura://bookmarks') bmPage(); else ext(u,t);
    if(!u.startsWith('baura://')) addHist(u,t); else if(u.startsWith('baura://search')) addHist(u,t);
  }
  const logo=`<svg class="hbl" viewBox="150 29 199 420"><rect x="150" y="29" width="48" height="420" rx="24" fill="var(--b1)"/><path d="M248 29A101 102.5 0 0 1 248 234Z" fill="var(--b2)"/><path d="M248 245A101 102 0 0 1 248 449Z" fill="var(--b3)"/><rect x="316" y="221" width="33" height="36" rx="15" fill="var(--b4)"/></svg>`;
  const fint=`<span class="fint"><b>F</b>int<b>I</b>t</span>`;
  function home(){
    const h=new Date().getHours(), g=h<6?'Bonne nuit':h<12?'Bonjour':h<18?'Bon après-midi':'Bonsoir';
    const user=me().name.split(' ')[0];
    const fav=bms().slice(0,6).map(b=>({n:b.t,u:b.u,c:'#6366F1'}));
    const tiles=[...TILES.map(([n,i,c])=>({n,u:SITES[i][1],c})),...fav].slice(0,10);
    view.innerHTML=`<div class="hbhome">${logo}<h2>${g}, ${E(user)}</h2><form class="hbsf"><span>${fint}</span><input aria-label="Rechercher avec FintIt" placeholder="Que veux-tu savoir ?" autocomplete="off"><button>Chercher</button></form>
      <p class="hbsub">FintIt cherche dans tes applis, tes fichiers et une sélection de sites du web.</p><div class="hbtiles">${tiles.map(t=>`<button class="hbt" data-u="${E(t.u)}"><span style="background:${t.c}">${E(t.n[0])}</span>${E(t.n)}</button>`).join('')}</div></div>`;
    const f=view.querySelector('.hbsf'); f.onsubmit=e=>{ e.preventDefault(); const v=f.querySelector('input').value; if(v.trim()) go(v) }; setTimeout(()=>f.querySelector('input').focus(),50);
  }
  function results(q,tab){
    tab=tab||'all'; const ins=instant(q), r=search(q), total=r.apps.length+r.files.length+r.web.length;
    const ext=(n,u)=>`<a class="hbx" href="${u}" target="_blank" rel="noopener noreferrer">${n} ${BI.i('external')}</a>`;
    const eq=encodeURIComponent(q);
    const secA=r.apps.length?`<h4>Applications</h4>`+r.apps.map(({a})=>`<button class="hbr app" data-app="${a.id}">${appIco(a,34)}<span><b>${E(a.n)}</b><small>${E(DESC[a.id]||'')}</small></span><em>Ouvrir</em></button>`).join(''):'';
    const secF=r.files.length?`<h4>Fichiers</h4>`+r.files.map(f=>{ const k=fileKind(f); return `<button class="hbr" data-file="${f.id}"><span class="fi">${fileIco(f,34)}</span><span><b>${E(f.name)}</b><small>${E(k.label)} · ${E(FS.path(f.id).slice(0,-1).map(p=>p.name).join(' › ')||'BAURA')}</small></span><em>Ouvrir</em></button>` }).join(''):'';
    const secW=r.web.length?`<h4>Sur le web</h4>`+r.web.map(s=>`<button class="hbr web" data-u="${E(s[1])}"><span class="wi">${E(s[0][0])}</span><span><b>${E(s[0])}</b><small class="u">${E(s[1].replace(/^https?:\/\//,''))}</small><small>${E(s[2])}</small></span></button>`).join(''):'';
    view.innerHTML=`<div class="hbres"><div class="hbrh">${fint}<form class="hbsf sm"><input value="${E(q)}" aria-label="Rechercher"><button>Chercher</button></form></div>
      <div class="hbtabs">${[['all','Tout'],['apps','Applis'],['files','Fichiers'],['web','Web']].map(([k,n])=>`<button data-tab="${k}" class="${tab===k?'on':''}">${n}</button>`).join('')}</div>
      ${tab==='all'?ins.map(i=>`<div class="hbi"><small>${i.k}</small><b>${E(i.big)}</b><span>${E(i.sub)}</span></div>`).join(''):''}
      ${tab==='all'||tab==='apps'?secA:''}${tab==='all'||tab==='files'?secF:''}${tab==='all'||tab==='web'?secW:''}
      ${!total&&!ins.length?`<div class="hbnone"><b>Aucun résultat dans BAURA pour « ${E(q)} »</b><span>Essaie d'autres mots, ou poursuis ta recherche sur le web :</span></div>`:''}
      <h4>Continuer sur le web</h4><div class="hbext">${ext('DuckDuckGo','https://duckduckgo.com/?q='+eq)}${ext('Wikipédia','https://fr.wikipedia.org/w/index.php?search='+eq)}${ext('Google','https://www.google.com/search?q='+eq)}${ext('Images','https://duckduckgo.com/?iax=images&ia=images&q='+eq)}</div><p class="hbfoot">FintIt est le moteur intégré de BAURA OS. Pour des raisons de sécurité, les recherches web s'ouvrent dans un nouvel onglet de ton navigateur.</p></div>`;
    const f=view.querySelector('.hbsf'); f.onsubmit=e=>{ e.preventDefault(); const v=f.querySelector('input').value; if(v.trim()) go(v) };
    view.querySelectorAll('[data-tab]').forEach(b=>b.onclick=()=>results(q,b.dataset.tab));
  }
  function ext(u,t){
    view.innerHTML=`<div class="hbpage"><div class="hbnote">${svgi('<circle cx="12" cy="12" r="9"/><path d="M12 8v5M12 16h.01"/>')}<span>Beaucoup de sites interdisent leur affichage dans une fenêtre intégrée. Si la page reste vide, ouvre-la dans ton navigateur.</span><a class="stbtn" href="${E(u)}" target="_blank" rel="noopener noreferrer">Ouvrir ${BI.i('external')}</a></div><iframe src="${E(u)}" title="${E(t)}" referrerpolicy="no-referrer" sandbox="allow-scripts allow-forms allow-popups allow-same-origin"></iframe></div>` }
  function histPage(){ const h=hist(); view.innerHTML=`<div class="hbres"><h3>Historique</h3>${h.length?`<button class="stbtn dng" data-clear>Effacer l'historique</button>`:''}${h.map(x=>`<button class="hbr web" data-u="${E(x.u)}"><span class="wi">${E(x.t[0]||'•')}</span><span><b>${E(x.t)}</b><small class="u">${E(x.u.replace('baura://search?q=','FintIt : '))}</small></span><em>${new Date(x.d).toLocaleTimeString('fr-FR',{hour:'2-digit',minute:'2-digit'})}</em></button>`).join('')||'<p class="hbnone">Aucune page visitée.</p>'}</div>`; const c=view.querySelector('[data-clear]'); if(c) c.onclick=()=>{ Baura.store.set('houb.history',[]); histPage() } }
  function bmPage(){ const b=bms(); view.innerHTML=`<div class="hbres"><h3>Favoris</h3>${b.map((x,i)=>`<div class="hbr web" data-u="${E(x.u)}" role="button" tabindex="0"><span class="wi">${E(x.t[0]||'S')}</span><span><b>${E(x.t)}</b><small class="u">${E(x.u)}</small></span><button class="tb" data-rm="${i}" aria-label="Retirer">×</button></div>`).join('')||'<p class="hbnone">Aucun favori. Clique sur l’étoile pour en ajouter.</p>'}</div>`; view.querySelectorAll('[data-rm]').forEach(x=>x.onclick=e=>{ e.stopPropagation(); const l=bms(); l.splice(+x.dataset.rm,1); Baura.store.set('houb.bm',l); bmPage() }) }
  view.addEventListener('click',e=>{ const a=e.target.closest('[data-app]'); if(a){ openApp(a.dataset.app); return } const f=e.target.closest('[data-file]'); if(f){ openItem(f.dataset.file); return } const u=e.target.closest('[data-u]'); if(u&&!e.target.closest('[data-rm]')){ go(u.dataset.u) } });
  inp.addEventListener('keydown',e=>{ if(e.key==='Enter'){ go(inp.value) } else if(e.key==='Escape'){ inp.blur() } }); inp.addEventListener('focus',()=>inp.select());
  root.querySelector('.hbbar').addEventListener('click',e=>{ const b=e.target.closest('[data-a]'); if(!b) return; const a=b.dataset.a;
    if(a==='back'&&sp>0){ sp--; render() } else if(a==='fwd'&&sp<stack.length-1){ sp++; render() } else if(a==='reload') render(); else if(a==='home') go('baura://home');
    else if(a==='star'){ const u=cur().u; if(u==='baura://home'||u.startsWith('baura://search')) { toast&&toast('Ouvre un site pour l\'ajouter aux favoris.'); return } let l=bms(); if(l.some(x=>x.u===u)) l=l.filter(x=>x.u!==u); else l.unshift({u,t:titleOf(u)}); Baura.store.set('houb.bm',l); render() }
    else if(a==='menu'){ const r=b.getBoundingClientRect(); Ctx.show([{label:'Nouvel onglet',run:()=>openApp('web',{newTab:true})},{label:'Favoris',run:()=>go('baura://bookmarks')},{label:'Historique',run:()=>go('baura://history')},'sep',{label:'Ouvrir dans le navigateur',disabled:cur().u.startsWith('baura://'),run:()=>window.open(cur().u,'_blank','noopener')}],r.right-180,r.bottom+4) } });
  root.addEventListener('keydown',e=>{ if((e.ctrlKey||e.metaKey)&&e.key.toLowerCase()==='l'){ e.preventDefault(); inp.focus() } });
  const a0=ctx.args||{}; if(a0.url) go(a0.url,false); else if(a0.q) go(a0.q,false); else render();
  return {open(a){ if(a&&a.url) go(a.url); else if(a&&a.q) go(a.q) }};
};
})();

/* =============================== BMAIL =============================== */
(function(){
const {E,me,contacts}=NetKit;
const FOLD=[['inbox','Boîte de réception','<path d="M4 13l2-7h12l2 7v5H4z"/><path d="M4 13h5l1 2h4l1-2h5"/>'],['star','Favoris','<path d="M12 4l2.4 5 5.4.7-4 3.8 1 5.4L12 16.2 7.2 19l1-5.4-4-3.8 5.4-.7z"/>'],['sent','Envoyés','<path d="M4 12l16-7-6 16-3-7z"/>'],['drafts','Brouillons','<path d="M5 19l1-4L16 5l3 3L9 18z"/>'],['trash','Corbeille','<path d="M5 7h14M9 7V4h6v3M7 7l1 13h8l1-13"/>']];
const nowM=(m)=>Date.now()-m*60000;
const seed=()=>({nid:20,mails:[
 {id:'m1',f:'inbox',from:{n:'Équipe BAURA',e:'equipe@baura.mail'},to:'me',s:'Bienvenue dans Bmail',b:'Bonjour,\n\nBmail est ta messagerie dans BAURA OS. Tu peux écrire à tes contacts, classer tes messages, mettre des favoris et retrouver tout en un clin d\'œil.\n\nEssaie d\'envoyer un message : tu recevras une réponse automatique dans quelques secondes.\n\nÀ très vite,\nL\'équipe BAURA',d:nowM(8),r:false,st:true},
 {id:'m2',f:'inbox',from:{n:'Amina Koné',e:'amina@baura.mail'},to:'me',s:'Maquettes du projet',b:'Salut !\n\nJ\'ai terminé les maquettes dans Crea. Tu peux les voir dans le dossier Documents (Affiche exemple.crea).\n\nDis-moi ce que tu en penses.\n\nAmina',d:nowM(95),r:false},
 {id:'m3',f:'inbox',from:{n:'Karim Benali',e:'karim@baura.mail'},to:'me',s:'Budget du trimestre',b:'Bonjour,\n\nJe t\'ai préparé un classeur Numbro avec les chiffres : Budget 2026.numbro. Regarde l\'onglet Épargne.\n\nKarim',d:nowM(60*26),r:true},
 {id:'m4',f:'inbox',from:{n:'Sofia Marti',e:'sofia@baura.mail'},to:'me',s:'Présentation de jeudi',b:'Coucou,\n\nPeux-tu relire la présentation (Découvrir WePoint.wepoint) avant jeudi ? Un œil neuf fait toujours du bien.\n\nMerci !\nSofia',d:nowM(60*50),r:true},
 {id:'m5',f:'inbox',from:{n:'App shop',e:'noreply@baura.mail'},to:'me',s:'Nouvelles applications disponibles',b:'Découvre de nouvelles applications dans App shop : Dessin, Tâches, Horloge, Serpent, Calendrier, Contacts et Moniteur.\n\nBonne exploration !',d:nowM(60*72),r:true}
]});
const get=()=>{ let d=Baura.store.get('bmail',null); if(!d||!d.mails){ d=seed(); Baura.store.set('bmail',d) } return d };
const put=d=>Baura.store.set('bmail',d);
const fmt=ts=>{ const d=new Date(ts), n=new Date(); if(d.toDateString()===n.toDateString()) return d.toLocaleTimeString('fr-FR',{hour:'2-digit',minute:'2-digit'}); return d.toLocaleDateString('fr-FR',{day:'numeric',month:'short'}) };
const REPL=['Merci pour ton message ! Je regarde ça et je reviens vers toi très vite.','Bien reçu, on en parle dès demain ?','Super, merci ! Je t\'envoie mes retours dans la journée.','Parfait, c\'est noté. Belle journée !'];
function deliver(m){ const d=get(); d.nid++; const id='m'+d.nid; d.mails.unshift(Object.assign({id,r:false,d:Date.now(),to:'me'},m)); put(d); notify({app:'bmail',title:m.from.n,text:m.s,level:'info',actions:[{label:'Lire',run:()=>openApp('bmail',{mail:id})}]}); return id }
function send(m){ const d=get(); d.nid++; const id='m'+d.nid; d.mails.unshift({id,f:'sent',from:{n:me().name,e:me().email},to:m.to,s:m.s,b:m.b,d:Date.now(),r:true}); put(d);
  const to=m.to.split(/[,;\s]+/).filter(Boolean); to.forEach((addr,i)=>{ const c=contacts().find(x=>x.email.toLowerCase()===addr.toLowerCase()); const name=c?c.name:addr.split('@')[0];
    if(addr.toLowerCase()===me().email) setTimeout(()=>deliver({f:'inbox',from:{n:me().name,e:addr},s:m.s,b:m.b}),600);
    else if(/@baura\.mail$/i.test(addr)||c) setTimeout(()=>deliver({f:'inbox',from:{n:name,e:addr},s:'Re: '+m.s.replace(/^re:\s*/i,''),b:REPL[(m.s.length+i)%REPL.length]+'\n\n'+name.split(' ')[0]+'\n\n> '+m.b.split('\n').slice(0,3).join('\n> ')}),5500+i*2500);
    else setTimeout(()=>deliver({f:'inbox',from:{n:'Système de messagerie',e:'postmaster@baura.mail'},s:'Message non remis : '+m.s,b:'Ton message n\'a pas pu être remis à '+addr+'.\n\nBmail est une messagerie de démonstration : elle ne peut écrire qu\'aux adresses @baura.mail et à tes contacts.'}),4000) }) }
AppViews.bmail=function(root,ctx){
  let fold='inbox', sel=null, q='', comp=null;
  root.innerHTML=`<div class="bm"><aside class="bmside"><button class="bmnew" data-a="new">${BI.i('pen')} Nouveau message</button><nav></nav><div class="bmme"></div></aside><section class="bmlist"><div class="bmsearch"><label class="fmsearch">${svgi(IC.search)}<input placeholder="Rechercher un message" aria-label="Rechercher"></label></div><div class="bml"></div></section><section class="bmread"></section><div class="bmcomp" hidden></div></div>`;
  const $=s=>root.querySelector(s), nav=$('nav'), list=$('.bml'), read=$('.bmread'), cp=$('.bmcomp');
  const mails=()=>get().mails.filter(m=>fold==='star'?(m.st&&m.f!=='trash'):m.f===fold).filter(m=>!q||(m.s+m.b+(m.from?m.from.n:'')+m.to).toLowerCase().includes(q)).sort((a,b)=>b.d-a.d);
  const upd=(id,p)=>{ const d=get(), m=d.mails.find(x=>x.id===id); if(m){ Object.assign(m,p); put(d) } };
  function drawNav(){ const d=get(); nav.innerHTML=FOLD.map(([k,n,ic])=>{ const c=k==='inbox'?d.mails.filter(m=>m.f==='inbox'&&!m.r).length:k==='drafts'?d.mails.filter(m=>m.f==='drafts').length:0; return `<button class="bmf${k===fold?' on':''}" data-f="${k}">${svgi(ic)}<span>${n}</span>${c?`<em>${c}</em>`:''}</button>` }).join(''); $('.bmme').innerHTML=`<b>${E(me().name)}</b><small>${E(me().email)}</small>` }
  function drawList(){ const a=mails(); list.innerHTML=a.length?a.map(m=>{ const who=(m.f==='sent'||m.f==='drafts')?'À : '+(m.to||'(sans destinataire)'):m.from.n; return `<button class="bmi${m.id===sel?' on':''}${!m.r?' unread':''}" data-id="${m.id}"><span class="av" style="background:hsl(${[...who].reduce((x,c)=>x+c.charCodeAt(0),0)%360} 60% 50%)">${E((who.replace('À : ','')[0]||'?').toUpperCase())}</span><span class="bx"><b>${E(who)}</b><small>${E(m.s||'(sans objet)')}</small><span>${E((m.b||'').replace(/\s+/g,' ').slice(0,70))}</span></span><i>${fmt(m.d)}${m.st?' '+BI.i('starFill'):''}</i></button>` }).join(''):`<div class="bmempty">${fold==='inbox'?'Boîte de réception vide':'Aucun message ici'}</div>` }
  function drawRead(){ const m=get().mails.find(x=>x.id===sel); if(!m||(fold!==m.f&&fold!=='star')){ if(!m){ read.innerHTML='<div class="bmempty">Sélectionne un message pour le lire</div>'; return } }
    if(!m.r&&m.f!=='drafts'){ upd(m.id,{r:true}); drawNav() }
    ctx.setTitle(m.s||'Bmail');
    read.innerHTML=`<div class="bmrh"><h2>${E(m.s||'(sans objet)')}</h2><div class="bmact"><button class="tb" data-a="reply" title="Répondre">${BI.i('reply')}</button><button class="tb" data-a="fwd" title="Transférer">${BI.i('forward')}</button><button class="tb ${m.st?'on':''}" data-a="star" title="Favori">${BI.i(m.st?'starFill':'star')}</button><button class="tb" data-a="unread" title="Marquer comme non lu">${BI.i('mailOpen')}</button><button class="tb" data-a="del" title="${m.f==='trash'?'Supprimer définitivement':'Supprimer'}">${BI.i('trash')}</button></div></div>
      <div class="bmmeta"><span class="av" style="background:hsl(${[...(m.from?m.from.n:'x')].reduce((x,c)=>x+c.charCodeAt(0),0)%360} 60% 50%)">${E((m.from.n[0]||'?').toUpperCase())}</span><div><b>${E(m.from.n)}</b> <small>&lt;${E(m.from.e)}&gt;</small><br><small>À : ${E(m.to==='me'?me().email:m.to)} · ${new Date(m.d).toLocaleString('fr-FR',{dateStyle:'long',timeStyle:'short'})}</small></div></div><div class="bmbody">${E(m.b).replace(/\n/g,'<br>')}</div>` }
  function refresh(){ drawNav(); drawList(); if(sel) drawRead(); else read.innerHTML='<div class="bmempty">Sélectionne un message pour le lire</div>' }
  function compose(init){ comp=Object.assign({to:'',s:'',b:'',id:null},init||{}); cp.hidden=false; const cs=contacts();
    cp.innerHTML=`<div class="bmcw"><div class="bmch"><b>Nouveau message</b><button class="tb" data-a="cx">${BI.i("x")}</button></div><label>À<input data-f="to" list="bmc" value="${E(comp.to)}" autocomplete="off" spellcheck="false"><datalist id="bmc">${cs.map(c=>`<option value="${E(c.email)}">${E(c.name)}</option>`).join('')}</datalist></label><label>Objet<input data-f="s" value="${E(comp.s)}"></label><textarea data-f="b" placeholder="Écris ton message…">${E(comp.b)}</textarea><div class="bmcf"><button class="stbtn pri2" data-a="send">Envoyer</button><button class="stbtn" data-a="draft">Enregistrer le brouillon</button><span style="flex:1"></span><button class="stbtn dng" data-a="discard">Supprimer</button></div></div>`; setTimeout(()=>cp.querySelector(init&&init.to?'[data-f=s]':'[data-f=to]').focus(),50) }
  cp.addEventListener('input',e=>{ if(e.target.dataset.f) comp[e.target.dataset.f]=e.target.value });
  cp.addEventListener('click',e=>{ const a=e.target.closest('[data-a]')?.dataset.a; if(!a) return;
    if(a==='send'){ if(!comp.to.trim()){ cp.querySelector('[data-f=to]').focus(); return } if(comp.id){ const d=get(); d.mails=d.mails.filter(m=>m.id!==comp.id); put(d) } send({to:comp.to.trim(),s:comp.s.trim()||'(sans objet)',b:comp.b}); cp.hidden=true; comp=null; toast&&toast('Message envoyé'); fold='sent'; sel=null; refresh() }
    else if(a==='draft'||a==='cx'){ if(comp&&(comp.to||comp.s||comp.b)&&a==='draft'){ const d=get(); if(comp.id){ Object.assign(d.mails.find(m=>m.id===comp.id)||{},{to:comp.to,s:comp.s,b:comp.b,d:Date.now()}) } else { d.nid++; comp.id='m'+d.nid; d.mails.unshift({id:comp.id,f:'drafts',from:{n:me().name,e:me().email},to:comp.to,s:comp.s,b:comp.b,d:Date.now(),r:true}) } put(d); toast&&toast('Brouillon enregistré') } cp.hidden=true; comp=null; refresh() }
    else if(a==='discard'){ if(comp&&comp.id){ const d=get(); d.mails=d.mails.filter(m=>m.id!==comp.id); put(d) } cp.hidden=true; comp=null; refresh() } });
  root.addEventListener('click',e=>{ const f=e.target.closest('[data-f]'); if(f&&f.classList.contains('bmf')){ fold=f.dataset.f; sel=null; refresh(); return }
    const it=e.target.closest('.bmi'); if(it){ const m=get().mails.find(x=>x.id===it.dataset.id); if(m&&m.f==='drafts'){ compose({to:m.to,s:m.s,b:m.b,id:m.id}); return } sel=it.dataset.id; drawList(); drawRead(); return }
    const a=e.target.closest('[data-a]')?.dataset.a; if(!a||cp.contains(e.target)) return; const m=get().mails.find(x=>x.id===sel);
    if(a==='new') compose(); else if(!m) return;
    else if(a==='reply') compose({to:m.f==='sent'?m.to:m.from.e,s:'Re: '+m.s.replace(/^re:\s*/i,''),b:'\n\n— Le '+new Date(m.d).toLocaleDateString('fr-FR')+', '+m.from.n+' a écrit :\n> '+m.b.split('\n').join('\n> ')});
    else if(a==='fwd') compose({s:'Tr: '+m.s,b:'\n\n— Message transféré —\nDe : '+m.from.n+' <'+m.from.e+'>\n\n'+m.b});
    else if(a==='star'){ upd(m.id,{st:!m.st}); refresh() } else if(a==='unread'){ upd(m.id,{r:false}); sel=null; refresh() }
    else if(a==='del'){ const d=get(); if(m.f==='trash'){ d.mails=d.mails.filter(x=>x.id!==m.id) } else { m.of=m.f; m.f='trash' } put(d); sel=null; refresh() } });
  $('.bmsearch input').addEventListener('input',e=>{ q=e.target.value.toLowerCase(); drawList() });
  Baura.on('store',k=>{ if(root.isConnected&&(k==='bmail'||k==='account'||k==='*')){ drawNav(); drawList(); if(sel&&!comp) { const m=get().mails.find(x=>x.id===sel); if(m) drawRead() } } });
  const a0=ctx.args||{}; refresh(); if(a0.compose) compose({to:a0.compose}); if(a0.mail){ sel=a0.mail; fold='inbox'; refresh() }
  return {open(a){ if(a&&a.compose) compose({to:a.compose}); if(a&&a.mail){ fold='inbox'; sel=a.mail; refresh() } }};
};
})();

/* =============================== CONTACTS =============================== */
AppViews.contacts=function(root,ctx){
  const {E,contacts,saveContacts}=NetKit; let sel=null, q='';
  root.innerHTML=`<div class="ct"><aside class="ctside"><div class="cth"><label class="fmsearch">${svgi(IC.search)}<input placeholder="Rechercher" aria-label="Rechercher un contact"></label><button class="tb" data-a="add" aria-label="Nouveau contact">${svgi('<path d="M12 5v14M5 12h14"/>')}</button></div><div class="ctl"></div></aside><section class="ctmain"></section></div>`;
  const list=root.querySelector('.ctl'), main=root.querySelector('.ctmain');
  const col=n=>'hsl('+([...n].reduce((x,c)=>x+c.charCodeAt(0),0)%360)+' 60% 50%)';
  const all=()=>contacts().filter(c=>!q||(c.name+c.email+c.phone).toLowerCase().includes(q)).sort((a,b)=>a.name.localeCompare(b.name,'fr'));
  function drawList(){ const a=all(); list.innerHTML=a.map(c=>`<button class="cti${c.id===sel?' on':''}" data-id="${c.id}"><span class="av" style="background:${col(c.name)}">${E((c.name[0]||'?').toUpperCase())}</span><span><b>${E(c.name||'Sans nom')}</b><small>${E(c.email)}</small></span></button>`).join('')||'<p class="dtnone">Aucun contact</p>' }
  function drawMain(){ const c=contacts().find(x=>x.id===sel); if(!c){ main.innerHTML='<div class="bmempty">Sélectionne un contact ou crées-en un</div>'; return }
    main.innerHTML=`<div class="ctcard"><span class="av big" style="background:${col(c.name)}">${E((c.name[0]||'?').toUpperCase())}</span><h2>${E(c.name||'Sans nom')}</h2><div class="ctbtn"><button class="stbtn" data-a="mail">${BI.i('mail')} Écrire</button><button class="stbtn dng" data-a="del">Supprimer</button></div>
      <label>Nom<input data-k="name" value="${E(c.name)}"></label><label>E-mail<input data-k="email" value="${E(c.email)}" type="email"></label><label>Téléphone<input data-k="phone" value="${E(c.phone||'')}"></label><label>Note<textarea data-k="note" rows="3">${E(c.note||'')}</textarea></label></div>` }
  root.addEventListener('input',e=>{ const k=e.target.dataset.k; if(k&&sel){ const l=contacts(), c=l.find(x=>x.id===sel); c[k]=e.target.value; saveContacts(l); drawList(); if(k==='name') main.querySelector('h2').textContent=c.name||'Sans nom' } });
  root.querySelector('.ctside input').addEventListener('input',e=>{ q=e.target.value.toLowerCase(); drawList() });
  root.addEventListener('click',e=>{ const i=e.target.closest('.cti'); if(i){ sel=i.dataset.id; drawList(); drawMain(); return } const a=e.target.closest('[data-a]')?.dataset.a;
    if(a==='add'){ const l=contacts(), id='c'+Date.now().toString(36); l.push({id,name:'Nouveau contact',email:'',phone:'',note:''}); saveContacts(l); sel=id; q=''; root.querySelector('.ctside input').value=''; drawList(); drawMain(); const n=main.querySelector('[data-k=name]'); n&&(n.focus(),n.select()) }
    else if(a==='mail'){ const c=contacts().find(x=>x.id===sel); if(c&&c.email) openApp('bmail',{compose:c.email}); else toast&&toast('Ajoute d\'abord une adresse e-mail.') }
    else if(a==='del'){ Dlg.confirm({title:'Supprimer ce contact ?',text:'Il sera retiré de ton carnet d\'adresses.',ok:'Supprimer',danger:true}).then(ok=>{ if(ok){ saveContacts(contacts().filter(x=>x.id!==sel)); sel=null; drawList(); drawMain() } }) } });
  Baura.on('store',k=>{ if(root.isConnected&&k==='contacts'&&!main.contains(document.activeElement)){ drawList(); drawMain() } });
  drawList(); sel=(all()[0]||{}).id||null; drawList(); drawMain();
  return {};
};

/* =============================== CALENDRIER =============================== */
(function(){
const {E,day}=NetKit, MONTHS=['Janvier','Février','Mars','Avril','Mai','Juin','Juillet','Août','Septembre','Octobre','Novembre','Décembre'], COL=['#6366F1','#F5B84B','#34D399','#F87171','#38BDF8','#F472B6'];
const evs=()=>Baura.store.get('calendar',null)||(()=>{ const t=new Date(), d=n=>day(new Date(t.getFullYear(),t.getMonth(),t.getDate()+n)); const s=[{id:'e1',date:d(0),time:'15:00',title:'Réunion projet BAURA',color:COL[0],note:''},{id:'e2',date:d(1),time:'10:30',title:'Relire la présentation',color:COL[1],note:''},{id:'e3',date:d(3),time:'',title:'Anniversaire d\'Amina',color:COL[5],note:''},{id:'e4',date:d(6),time:'18:00',title:'Sport',color:COL[2],note:''}]; Baura.store.set('calendar',s); return s })();
const save=l=>Baura.store.set('calendar',l);
AppViews.calendrier=function(root,ctx){
  let y=new Date().getFullYear(), m=new Date().getMonth(), sel=day(new Date());
  root.innerHTML=`<div class="cal"><section class="calm"><div class="calh"><button class="tb" data-a="prev">${svgi('<path d="M14 6l-6 6 6 6"/>')}</button><h2></h2><button class="tb" data-a="next">${svgi('<path d="M10 6l6 6-6 6"/>')}</button><button class="stbtn" data-a="today">Aujourd'hui</button></div><div class="calg"></div></section><aside class="cala"></aside></div>`;
  const grid=root.querySelector('.calg'), side=root.querySelector('.cala'), head=root.querySelector('h2');
  function drawGrid(){ head.textContent=MONTHS[m]+' '+y; const first=new Date(y,m,1), off=(first.getDay()+6)%7, start=new Date(y,m,1-off), td=day(new Date()), all=evs();
    grid.innerHTML=['lun','mar','mer','jeu','ven','sam','dim'].map(d=>`<div class="cld h">${d}</div>`).join('')+Array.from({length:42},(_,i)=>{ const d=new Date(start.getFullYear(),start.getMonth(),start.getDate()+i), k=day(d), es=all.filter(e=>e.date===k).sort((a,b)=>(a.time||'').localeCompare(b.time||'')); return `<button class="cld${d.getMonth()!==m?' o':''}${k===td?' today':''}${k===sel?' sel':''}" data-d="${k}"><b>${d.getDate()}</b>${es.slice(0,3).map(e=>`<i style="background:${e.color}">${E(e.title)}</i>`).join('')}${es.length>3?`<small>+${es.length-3}</small>`:''}</button>` }).join('') }
  function drawSide(){ const d=new Date(sel+'T12:00'), es=evs().filter(e=>e.date===sel).sort((a,b)=>(a.time||'').localeCompare(b.time||''));
    side.innerHTML=`<h3>${d.toLocaleDateString('fr-FR',{weekday:'long',day:'numeric',month:'long'})}</h3>${es.map(e=>`<div class="cev" style="--c:${e.color}"><span>${e.time||'Journée'}</span><b>${E(e.title)}</b><button class="tb" data-del="${e.id}" aria-label="Supprimer">×</button></div>`).join('')||'<p class="dtnone">Aucun événement.</p>'}
      <form class="calf"><h4>Nouvel événement</h4><input name="t" placeholder="Titre" required autocomplete="off"><div class="row"><input name="h" type="time"><div class="calco">${COL.map((c,i)=>`<label><input type="radio" name="c" value="${c}"${i===0?' checked':''}><i style="background:${c}"></i></label>`).join('')}</div></div><button class="stbtn pri2">Ajouter</button></form>` }
  root.addEventListener('click',e=>{ const a=e.target.closest('[data-a]')?.dataset.a, d=e.target.closest('[data-d]'), del=e.target.closest('[data-del]');
    if(a==='prev'){ m--; if(m<0){ m=11; y-- } drawGrid() } else if(a==='next'){ m++; if(m>11){ m=0; y++ } drawGrid() } else if(a==='today'){ const n=new Date(); y=n.getFullYear(); m=n.getMonth(); sel=day(n); drawGrid(); drawSide() }
    else if(d){ sel=d.dataset.d; const dt=new Date(sel+'T12:00'); if(dt.getMonth()!==m){ m=dt.getMonth(); y=dt.getFullYear() } drawGrid(); drawSide() }
    else if(del){ save(evs().filter(x=>x.id!==del.dataset.del)); drawGrid(); drawSide() } });
  root.addEventListener('submit',e=>{ e.preventDefault(); const f=e.target, l=evs(); l.push({id:'e'+Date.now().toString(36),date:sel,time:f.h.value,title:f.t.value.trim(),color:f.c.value,note:''}); save(l); drawGrid(); drawSide() });
  Baura.on('store',k=>{ if(root.isConnected&&k==='calendar'){ drawGrid(); drawSide() } });
  drawGrid(); drawSide(); return {};
};
// rappels : notification quand l'heure d'un événement arrive
setInterval(()=>{ try{ const n=new Date(), k=day(n), hm=String(n.getHours()).padStart(2,'0')+':'+String(n.getMinutes()).padStart(2,'0'); const done=Baura.store.get('calendarDone',[]); let ch=false;
  (Baura.store.get('calendar',[])||[]).forEach(e=>{ if(e.date===k&&e.time&&e.time<=hm&&!done.includes(e.id+k)&&hm<=addMin(e.time,10)){ done.push(e.id+k); ch=true; notify({app:'calendrier',title:e.title,text:'Maintenant · '+e.time,level:'info',actions:[{label:'Ouvrir',run:()=>openApp('calendrier')}]}) } }); if(ch) Baura.store.set('calendarDone',done.slice(-200),true) }catch(e){} },20000);
function addMin(t,m){ const [h,mi]=t.split(':').map(Number), x=h*60+mi+m; return String(Math.min(23,x/60|0)).padStart(2,'0')+':'+String(x%60).padStart(2,'0') }
})();

/* =============================== MONITEUR =============================== */
AppViews.moniteur=function(root,ctx){
  const {E}=NetKit; const t0=Date.now(); let hist=Array(60).fill(8), tm=0, sorted='cpu';
  root.innerHTML=`<div class="mo"><div class="mog"><div class="mok"><span>Processeur</span><b class="moc">0 %</b></div><div class="mok"><span>Mémoire</span><b class="mom">0 Mo</b></div><div class="mok"><span>Disque</span><b class="mod">—</b></div><div class="mok"><span>En marche</span><b class="mou">0:00</b></div></div><canvas class="mocv"></canvas>
    <div class="mot"><div class="moh"><span>Application</span><span>État</span><span>CPU</span><span>Mémoire</span><span></span></div><div class="mol"></div></div></div>`;
  const cv=root.querySelector('canvas'), cx=cv.getContext('2d'), L=root.querySelector('.mol');
  const seedN=s=>{ let x=0; for(const c of s) x=(x*31+c.charCodeAt(0))%997; return x };
  const base={fichiers:60,web:240,notes:40,musique:70,photos:90,videos:160,skrib:110,numbro:120,wepoint:150,data:100,crea:190,bmail:80,calc:25,terminal:35,params:50,store:70,dessin:80,calendrier:60,contacts:40,moniteur:30,taches:35,horloge:30,serpent:45};
  function procs(){ const out=[]; if(typeof wins!=='undefined') wins.forEach((w,id)=>{ const a=byId[id]; if(!a) return; const k=Math.sin(Date.now()/1500+seedN(id))*.5+.5; out.push({id,n:a.n,a,st:w.minimized?'Réduite':'Active',cpu:w.minimized?0.1+k*.3:(1+k*7+(base[id]||50)/60),mem:(base[id]||60)+k*14+(w.tabs?w.tabs.length*18:0),adm:w.admin}) }); return out }
  function tick(){ if(!root.isConnected){ clearInterval(tm); return } const p=procs(); p.sort((a,b)=>sorted==='mem'?b.mem-a.mem:b.cpu-a.cpu); const tot=Math.min(98,4+p.reduce((s,x)=>s+x.cpu,0)+Math.random()*3), mem=140+p.reduce((s,x)=>s+x.mem,0);
    hist.push(tot); hist.shift(); root.querySelector('.moc').textContent=Math.round(tot)+' %'; root.querySelector('.mom').textContent=Math.round(mem)+' Mo'; root.querySelector('.mou').textContent=(()=>{ const s=(Date.now()-t0)/1000|0; return (s/60|0)+':'+String(s%60).padStart(2,'0') })();
    const dv=Baura.store.get('device')||{}; const used=FS.all().reduce((s,f)=>s+(typeof f.content==='string'?f.content.length:(f.src?2000:0)),0)/1048576+8.4; root.querySelector('.mod').textContent=(dv.sysGb?used.toFixed(1)+' / '+dv.sysGb+' Go':used.toFixed(1)+' Go');
    L.innerHTML=p.map(x=>`<div class="mor"><span class="n">${appIco(x.a,22)}${E(x.n)}${x.adm?' <em>Admin</em>':''}</span><span>${x.st}</span><span>${x.cpu.toFixed(1)} %</span><span>${Math.round(x.mem)} Mo</span><button class="stbtn dng" data-end="${x.id}">Terminer</button></div>`).join('')||'<p class="dtnone" style="padding:14px">Aucune application ouverte.</p>';
    const w=cv.clientWidth, h=cv.clientHeight, d=devicePixelRatio||1; cv.width=w*d; cv.height=h*d; cx.setTransform(d,0,0,d,0,0); const ac=getComputedStyle(root).getPropertyValue('--a2')||'#8B5CF6';
    cx.strokeStyle='rgba(128,128,150,.25)'; for(let i=1;i<4;i++){ cx.beginPath(); cx.moveTo(0,h*i/4); cx.lineTo(w,h*i/4); cx.stroke() }
    cx.beginPath(); hist.forEach((v,i)=>{ const x=i/(hist.length-1)*w, y=h-v/100*h; i?cx.lineTo(x,y):cx.moveTo(x,y) }); cx.strokeStyle=ac.trim()||'#8B5CF6'; cx.lineWidth=2.5; cx.stroke(); cx.lineTo(w,h); cx.lineTo(0,h); cx.closePath(); const g=cx.createLinearGradient(0,0,0,h); g.addColorStop(0,(ac.trim()||'#8B5CF6')+'55'); g.addColorStop(1,'transparent'); cx.fillStyle=g; cx.fill() }
  root.addEventListener('click',e=>{ const b=e.target.closest('[data-end]'); if(b){ closeApp(b.dataset.end) } });
  tm=setInterval(tick,1000); tick();
  return {destroy(){ clearInterval(tm) }};
};
