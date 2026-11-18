/* ===== BAURA OS – système de fichiers virtuel (dossiers, fichiers, corbeille) =====
   Enregistré dans le stockage partagé (localStorage) : il survit aux redémarrages. */
(function(){
  var KEY='fs', items={}, nid=1, L=[], saving=false;
  function save(){ saving=true; try{ Baura.store.set(KEY,{items:items,nid:nid}) }finally{ saving=false } }
  function emit(){ L.slice().forEach(function(f){try{f()}catch(e){}}) }
  function uid(){ return 'f'+(nid++) }
  function seed(){
    items={}; nid=1;
    var home={bureau:'Bureau',documents:'Documents',images:'Images',musique:'Musique',telechargements:'Téléchargements'};
    items.root={id:'root',name:'BAURA',type:'folder',parent:null};
    for(var k in home) items[k]={id:k,name:home[k],type:'folder',parent:'root'};
    items.trash={id:'trash',name:'Corbeille',type:'folder',parent:'root',system:true};
    function f(name,parent,extra){ var id=uid(); items[id]=Object.assign({id:id,name:name,type:'file',parent:parent,mod:Date.now()},extra||{}); return id }
    f('Bienvenue.txt','bureau',{content:'Bienvenue sur BAURA OS !\n\nQuelques idées pour commencer :\n• Clique droit sur le bureau pour créer un dossier\n• Ouvre Paramètres pour changer le thème et le fond d\'écran\n• Appuie sur Ctrl+Espace pour rechercher n\'importe quoi\n• Alt+W affiche toutes tes fenêtres\n'});
    f('Notes de projet.txt','documents',{content:'Projet BAURA OS\n\n- Logo : barre + deux demi-disques + point doré\n- Style : glassmorphism, simple comme Windows, élégant comme macOS\n- À faire : applis, thèmes, fichiers\n',mod:Date.now()-86400000});
    f('Idées.txt','documents',{content:'Idées en vrac\n\n• Mode concentration\n• Widgets sur le bureau\n• Thèmes saisonniers\n',mod:Date.now()-3*86400000});
    f('Charte graphique BAURA.pdf','documents',{kind:'pdf',size:'1,2 Mo',mod:Date.now()-120000});
    f('baura-symbole.svg','images',{src:'logos/baura-symbole-couleur.svg',kind:'image'});
    f('baura-logo.svg','images',{src:'logos/baura-logo-couleur.svg',kind:'image'});
    f('baura-icone.svg','images',{src:'icones/baura-icone-app.svg',kind:'image'});
    f('Aura.mp3','musique',{kind:'audio',track:0}); f('Heures indigo.mp3','musique',{kind:'audio',track:1});
    f('Lavande.mp3','musique',{kind:'audio',track:2}); f('Point doré.mp3','musique',{kind:'audio',track:3});
    save();
  }
  function load(){
    var d=Baura.store.get(KEY,null);
    if(d&&d.items&&d.items.root){ items=d.items; nid=d.nid||1000 } else seed();
    drives();
  }
  // lecteur D: (créé si l'installateur a prévu une partition de données)
  function drives(){
    var dev=Baura.store.get('device',null), has=dev&&dev.partitions&&dev.partitions.filter(function(p){return p.letter==='D'})[0];
    if(has&&!items['drive-d']){ items['drive-d']={id:'drive-d',name:has.label||'Données',type:'folder',parent:null,system:true,drive:'D'}; var u=nid++; items['f'+u]={id:'f'+u,name:'Lisez-moi.txt',type:'file',parent:'drive-d',mod:Date.now(),content:'Ceci est ton lecteur D:.\nUtilise-le pour tes données, séparées du système.\n'}; save() }
    if(!has&&items['drive-d']){ delete items['drive-d'] }
  }
  var ext=function(n){ var m=/\.([^.]+)$/.exec(n); return m?m[1].toLowerCase():'' };
  var FS={
    on:function(f){L.push(f)},
    ext:ext,
    get:function(id){return items[id]},
    list:function(pid){
      return Object.keys(items).map(function(k){return items[k]}).filter(function(i){return i.parent===pid&&i.id!=='root'})
        .sort(function(a,b){ return (a.type===b.type?0:a.type==='folder'?-1:1)||a.name.localeCompare(b.name,'fr',{numeric:true}) });
    },
    all:function(){ return Object.keys(items).map(function(k){return items[k]}) },
    inTrash:function(id){ var i=items[id]; while(i){ if(i.id==='trash') return true; i=items[i.parent] } return false },
    drive:function(id){ var i=items[id]; while(i&&i.parent){ i=items[i.parent] } return i&&i.id==='drive-d'?'D':'C' },
    drives:function(){ return items['drive-d']?['C','D']:['C'] },
    path:function(id){ var r=[],i=items[id]; while(i&&i.id!=='root'&&i.id!=='drive-d'){ r.unshift(i); i=items[i.parent] } return r },
    unique:function(parent,name,self){
      var taken=function(n){ return FS.list(parent).some(function(i){return i.id!==self&&i.name.toLowerCase()===n.toLowerCase()}) };
      if(!taken(name)) return name;
      var e=ext(name), base=e?name.slice(0,-(e.length+1)):name, n=2;
      while(taken(base+' ('+n+')'+(e?'.'+e:''))) n++;
      return base+' ('+n+')'+(e?'.'+e:'');
    },
    create:function(o){
      var id=uid(), name=FS.unique(o.parent,o.name);
      items[id]=Object.assign({mod:Date.now()},o,{id:id,name:name}); save(); emit(); return id;
    },
    update:function(id,patch,silent){ if(!items[id]) return; Object.assign(items[id],patch,{mod:Date.now()}); save(); if(!silent) emit() },
    rename:function(id,name){ var i=items[id]; if(!i||i.system) return null; name=name.trim(); if(!name) return null; i.name=FS.unique(i.parent,name,id); i.mod=Date.now(); save(); emit(); return i.name },
    move:function(id,parent){ var i=items[id]; if(!i||i.system||id===parent) return; var p=items[parent]; while(p){ if(p.id===id) return; p=items[p.parent] } i.parent=parent; i.name=FS.unique(parent,i.name,id); delete i.orig; save(); emit() },
    trash:function(id){ var i=items[id]; if(!i||i.system||FS.inTrash(id)) return; i.orig=i.parent; i.parent='trash'; i.name=FS.unique('trash',i.name,id); save(); emit() },
    restore:function(id){ var i=items[id]; if(!i||i.parent!=='trash') return; var to=(items[i.orig]&&!FS.inTrash(i.orig))?i.orig:'documents'; i.parent=to; delete i.orig; i.name=FS.unique(to,i.name,id); save(); emit() },
    purge:function(id){
      var kids=FS.list(id); kids.forEach(function(k){FS.purge(k.id)});
      if(id!=='trash'&&items[id]&&!items[id].system) delete items[id]; save(); emit();
    },
    emptyTrash:function(){ FS.list('trash').forEach(function(k){FS.purge(k.id)}); emit() },
    trashCount:function(){ return FS.list('trash').length },
    search:function(q){ q=q.toLowerCase(); return FS.all().filter(function(i){return i.id!=='root'&&!i.system&&!FS.inTrash(i.id)&&i.name.toLowerCase().indexOf(q)>-1}) },
    byPath:function(path,cwd){
      var m=/^([a-zA-Z]):/.exec(path), cur;
      if(m){ var L2=m[1].toUpperCase(); cur=L2==='C'?'root':(L2==='D'&&items['drive-d']?'drive-d':null); if(!cur) return null; path=path.slice(2) || '/' ; if(path[0]!=='/'&&path[0]!=='\\') path='/'+path }
      else cur=path[0]==='/'?'root':(cwd||'root');
      var parts=path.split(/[\/\\]/).filter(Boolean);
      for(var i=0;i<parts.length;i++){
        var p=parts[i]; if(p==='.') continue;
        if(p==='..'){ cur=items[cur].parent||cur; continue }
        var f=FS.list(cur).filter(function(x){return x.name.toLowerCase()===p.toLowerCase()})[0];
        if(!f) return null; cur=f.id;
      }
      return cur;
    },
    reset:function(){ seed(); emit() }
  };
  window.FS=FS;
  load();
  // les autres cadres / un autre onglet ont modifié le disque
  Baura.on('store',function(k){ if(saving) return; if(k===KEY||k==='*'){ load(); emit() } });
})();
