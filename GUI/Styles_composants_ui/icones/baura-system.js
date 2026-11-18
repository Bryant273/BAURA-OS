/* ===== BAURA OS – système partagé =====
   1. Navigation et alimentation (démarrage → connexion → bureau, verrouillage, veille, redémarrage, extinction)
   2. Stockage partagé (Baura.store), réglages (Baura.settings), compte (Baura.account) : enregistrés dans localStorage
      et synchronisés entre tous les cadres (index.html relaie les messages « sync »).
   3. Application du thème : clair/sombre/auto, accent (--a1/--a2), fond d'écran, luminosité, veilleuse, économie, verre, contraste, animations. */
(function(){
  'use strict';
  var ROUTES={boot:'boot-baura.html',login:'login-baura.html',desktop:'taskbar-and-desktop-tools-ui-baura-os.html',system:'index.html',installer:'installer-baura.html'};
  function framed(){ return window.parent!==window }
  function post(w,m){ try{ w.postMessage(m,'*') }catch(e){} }
  var listeners={store:[],settings:[]};
  function emit(ev,a){ (listeners[ev]||[]).slice().forEach(function(f){ try{f(a)}catch(e){} }) }

/* ================= Écran noir (veille / redémarrage / extinction) ================= */
  var overlay=null;
  function offCss(){
    if(document.getElementById('baura-off-css')) return;
    var s=document.createElement('style'); s.id='baura-off-css';
    s.textContent='.baura-off{position:fixed;inset:0;z-index:2147483000;background:#000;display:grid;place-items:center;opacity:0;transition:opacity .6s ease;font-family:Inter,"Segoe UI",system-ui,sans-serif}'+
      '.baura-off.on{opacity:1}.baura-off .pw{display:none;flex-direction:column;align-items:center;gap:14px;color:rgba(255,255,255,.55);font-size:13px}.baura-off .pw.show{display:flex}'+
      '.baura-off .pw button{width:64px;height:64px;border-radius:50%;border:1.5px solid rgba(255,255,255,.35);background:none;display:grid;place-items:center;color:#fff;cursor:pointer;transition:background .2s,transform .2s}'+
      '.baura-off .pw button:hover{background:rgba(255,255,255,.1);transform:scale(1.05)}.baura-off .pw svg{width:26px;height:26px;fill:none;stroke:#fff;stroke-width:1.9;stroke-linecap:round;stroke-linejoin:round}'+
      '.baura-off .sp{display:none;width:22px;height:22px;border-radius:50%;border:2px solid rgba(255,255,255,.2);border-top-color:#fff;animation:baura-rot .9s linear infinite}.baura-off .sp.show{display:block}@keyframes baura-rot{to{transform:rotate(360deg)}}';
    document.head.appendChild(s);
  }
  function screenOff(mode,done){
    if(overlay) return; offCss();
    var el=overlay=document.createElement('div'); el.className='baura-off';
    el.innerHTML='<div class="sp"></div><div class="pw"><button aria-label="Allumer"><svg viewBox="0 0 24 24"><path d="M12 3v8"/><path d="M6.3 6.8a8 8 0 1 0 11.4 0"/></svg></button><span>Appuie pour allumer</span></div>';
    document.documentElement.appendChild(el);
    var sp=el.querySelector('.sp'), pw=el.querySelector('.pw');
    requestAnimationFrame(function(){requestAnimationFrame(function(){el.classList.add('on')})});
    function back(){ done&&done(); setTimeout(function(){el.classList.remove('on');setTimeout(function(){el.remove();overlay=null},650)},500) }
    if(mode==='sleep'){
      setTimeout(function(){
        function wake(){removeEventListener('pointerdown',wake,true);removeEventListener('keydown',wake,true);back()}
        addEventListener('pointerdown',wake,true); addEventListener('keydown',wake,true);
      },800);
    }else if(mode==='restart'){
      setTimeout(function(){sp.classList.add('show')},700);
      setTimeout(function(){sp.classList.remove('show');back()},2100);
    }else{
      setTimeout(function(){pw.classList.add('show');pw.querySelector('button').focus()},1000);
      pw.querySelector('button').addEventListener('click',function(){pw.classList.remove('show');sp.classList.add('show');setTimeout(function(){sp.classList.remove('show');back()},700)});
    }
  }
  function handlePower(action,hooks){
    if(action==='lock') hooks.lock();
    else if(action==='sleep') screenOff('sleep',hooks.lock);
    else if(action==='restart') screenOff('restart',hooks.reboot);
    else if(action==='shutdown') screenOff('shutdown',hooks.reboot);
  }
  var navHooks={lock:function(){location.href=ROUTES.login},reboot:function(){location.href=ROUTES.boot}};

  
  /* ================= Stockage partagé ================= */
  var PFX='baura.', mem={};
  try{ for(var i=0;i<localStorage.length;i++){ var k=localStorage.key(i); if(k&&k.indexOf(PFX)===0){ try{ mem[k.slice(PFX.length)]=JSON.parse(localStorage.getItem(k)) }catch(e){} } } }catch(e){}
  function persist(k){ try{ localStorage.setItem(PFX+k,JSON.stringify(mem[k])) }catch(e){} }
  var store={
    get:function(k,d){ return mem[k]===undefined?d:mem[k] },
    set:function(k,v,quiet){ mem[k]=v; persist(k); emit('store',k); if(k==='settings'){ applyTheme(); emit('settings') } if(!quiet&&framed()) post(window.parent,{baura:'sync',k:k,v:v}) },
    clear:function(){ mem={}; try{ Object.keys(localStorage).forEach(function(k){ if(k.indexOf(PFX)===0) localStorage.removeItem(k) }) }catch(e){} applyTheme(); emit('store','*'); emit('settings'); if(framed()) post(window.parent,{baura:'sync-clear'}) }
  };
  addEventListener('message',function(e){
    var m=e.data; if(!m||!m.baura) return;
    if(m.baura==='sync'&&m.k){ if(JSON.stringify(mem[m.k])===JSON.stringify(m.v)) return; mem[m.k]=m.v; persist(m.k); emit('store',m.k); if(m.k==='settings'){ applyTheme(); emit('settings') } }
    else if(m.baura==='sync-clear'){ mem={}; applyTheme(); emit('store','*'); emit('settings') }
  });
  addEventListener('storage',function(e){ if(!e.key||e.key.indexOf(PFX)!==0) return; var k=e.key.slice(PFX.length); try{ mem[k]=e.newValue==null?undefined:JSON.parse(e.newValue) }catch(x){ return } emit('store',k); if(k==='settings'){ applyTheme(); emit('settings') } });

  /* ================= Réglages ================= */
  var ACCENTS={ indigo:['#4F46E5','#8B5CF6'], ocean:['#0369A1','#38BDF8'], rose:['#BE185D','#F472B6'], emeraude:['#047857','#34D399'], or:['#8A6D0B','#D0AE2A'], ardoise:['#475569','#94A3B8'] };
  var WALLS=[{id:'aura',n:'Aura'},{id:'aurore',n:'Aurore'},{id:'brume',n:'Brume'},{id:'minuit',n:'Minuit'},{id:'ocean',n:'Océan'}];
  var DEF={theme:'auto',accent:'indigo',wall:'aura',wallCustom:null,bright:100,night:false,saver:false,glass:true,contrast:false,motion:false};
  function isDark(s){ s=s||settings.get(); if(s.theme==='dark') return true; if(s.theme==='light') return false; return !!(window.matchMedia&&matchMedia('(prefers-color-scheme: dark)').matches) }
  function wallFile(s){ s=s||settings.get(); return s.wall==='aura'?(isDark(s)?'wallpaper-dark.svg':'wallpaper.svg'):'wallpaper-'+s.wall+'.svg' }
  var settings={
    accents:ACCENTS, walls:WALLS, isDark:isDark, wallFile:wallFile,
    get:function(){ return Object.assign({},DEF,store.get('settings',{})) },
    set:function(p){ store.set('settings',Object.assign({},store.get('settings',{}),p)) },
    reset:function(){ store.set('settings',{}) }
  };
  var account={ get:function(){ return store.get('account',null) }, set:function(o){ store.set('account',o) } };

  function applyTheme(){
    var s=settings.get(), r=document.documentElement, dark=isDark(s);
    if(s.theme==='auto') r.removeAttribute('data-theme'); else r.setAttribute('data-theme',s.theme);
    r.setAttribute('data-mode',dark?'dark':'light');
    var ac=ACCENTS[s.accent]||ACCENTS.indigo;
    r.style.setProperty('--a1',ac[0]); r.style.setProperty('--a2',ac[1]); r.style.setProperty('--indigo',ac[0]); r.style.setProperty('--violet',ac[1]);
    var sv=!!s.saver;
    r.setAttribute('data-glass',(s.glass===false||sv)?'off':'on');
    r.setAttribute('data-contrast',s.contrast?'high':'normal');
    r.setAttribute('data-motion',(s.motion||sv)?'reduce':'normal');
    r.setAttribute('data-saver',sv?'on':'off');
    var url=(s.wall==='custom'&&s.wallCustom)?s.wallCustom:wallFile(s);
    r.style.setProperty('--wall','url("'+url+'")');
    r.style.setProperty('--dim',((100-Math.max(20,Math.min(100,s.bright)))/100*.75).toFixed(3));
    r.style.setProperty('--night',s.night?'.22':'0');
  }
  if(window.matchMedia){ try{ matchMedia('(prefers-color-scheme: dark)').addEventListener('change',applyTheme) }catch(e){} }
  applyTheme();

  var B=window.Baura={
    ROUTES:ROUTES, screenOff:screenOff, handlePower:handlePower, applyTheme:applyTheme,
    store:store, settings:settings, account:account,
    on:function(ev,fn){ (listeners[ev]=listeners[ev]||[]).push(fn) },
    send:function(type,data){
      data=data||{};
      if(framed()){ var m={baura:type}; for(var k in data) m[k]=data[k]; post(window.parent,m); return; }
      if(type==='boot-done') location.href=ROUTES.login;
      else if(type==='login-ok') location.href=ROUTES.desktop+'?user='+encodeURIComponent(data.user||'');
      else if(type==='install-done') location.href=ROUTES.boot;
      else if(type==='power') handlePower(data.action,navHooks);
    },
    power:function(action){ B.send('power',{action:action}) },
    onMessage:function(fn){ addEventListener('message',function(e){ if(e.data&&e.data.baura) fn(e.data.baura,e.data,e) }) },
    installed:function(){ return !!store.get('installed',null) },
    param:function(k){ try{return new URLSearchParams(location.search).get(k)}catch(e){return null} }
  };
  Object.defineProperty(B,'framed',{get:framed});
})();
