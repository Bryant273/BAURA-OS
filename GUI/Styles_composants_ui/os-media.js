/* ===== BAURA OS – Vidéos : clips BAURA (canvas) + lecteur de vidéos réelles ===== */
(function(){
const TAU=Math.PI*2;
const CLIPS={
  aurore:{n:'Aurore',d:46,sub:'Rubans de lumière',draw(x,t,w,h){ const g=x.createLinearGradient(0,0,0,h); g.addColorStop(0,'#0b1020'); g.addColorStop(1,'#1e1b4b'); x.fillStyle=g; x.fillRect(0,0,w,h);
    for(let k=0;k<5;k++){ x.beginPath(); x.moveTo(0,h); for(let i=0;i<=w;i+=12){ const y=h*.45+Math.sin(i/w*5+t*.7+k)*h*.12+Math.sin(i/w*11-t*.4+k*2)*h*.05+k*h*.05; x.lineTo(i,y) } x.lineTo(w,h); x.closePath(); const c=['99,102,241','167,139,250','52,211,153','245,184,75','56,189,248'][k]; const gg=x.createLinearGradient(0,h*.3,0,h); gg.addColorStop(0,`rgba(${c},.55)`); gg.addColorStop(1,`rgba(${c},0)`); x.fillStyle=gg; x.fill() }
    for(let i=0;i<60;i++){ const sx=(i*137.5)%w, sy=(i*71.3)%(h*.5); x.fillStyle=`rgba(255,255,255,${.4+.4*Math.sin(t*2+i)})`; x.fillRect(sx,sy,2,2) } }},
  vagues:{n:'Vagues',d:38,sub:'Océan indigo',draw(x,t,w,h){ const g=x.createLinearGradient(0,0,0,h); g.addColorStop(0,'#fde68a'); g.addColorStop(.45,'#fb923c'); g.addColorStop(.5,'#312e81'); g.addColorStop(1,'#0f172a'); x.fillStyle=g; x.fillRect(0,0,w,h);
    x.fillStyle='#fff7d6'; x.beginPath(); x.arc(w*.5,h*.42,h*.1,0,TAU); x.fill();
    for(let k=0;k<7;k++){ x.beginPath(); x.moveTo(0,h); for(let i=0;i<=w;i+=8) x.lineTo(i,h*.52+k*h*.07+Math.sin(i/(60+k*14)+t*(.8+k*.12)+k)*(6+k*3)); x.lineTo(w,h); x.closePath(); x.fillStyle=`rgba(${60+k*10},${50+k*8},${150+k*12},${.35+k*.08})`; x.fill() } }},
  orbites:{n:'Orbites',d:52,sub:'Le symbole en mouvement',draw(x,t,w,h){ x.fillStyle='#0b0820'; x.fillRect(0,0,w,h); const cx=w/2, cy=h/2, R=Math.min(w,h)*.42;
    const cols=['#6366F1','#A78BFA','#34D399','#F5B84B'];
    for(let k=0;k<4;k++){ const r=R*(.35+k*.22); x.strokeStyle='rgba(167,139,250,.25)'; x.lineWidth=1.5; x.beginPath(); x.arc(cx,cy,r,0,TAU); x.stroke(); const a=t*(.9-k*.18)+k*1.7; x.fillStyle=cols[k]; x.shadowColor=cols[k]; x.shadowBlur=24; x.beginPath(); x.arc(cx+Math.cos(a)*r,cy+Math.sin(a)*r,10+k*3,0,TAU); x.fill(); x.shadowBlur=0 }
    const gr=x.createRadialGradient(cx,cy,0,cx,cy,R*.3); gr.addColorStop(0,'#F5B84B'); gr.addColorStop(1,'rgba(245,184,75,0)'); x.fillStyle=gr; x.beginPath(); x.arc(cx,cy,R*.3*(1+.05*Math.sin(t*3)),0,TAU); x.fill() }},
  neons:{n:'Pluie de néons',d:34,sub:'Lignes lumineuses',draw(x,t,w,h){ x.fillStyle='#07040f'; x.fillRect(0,0,w,h); x.lineCap='round';
    for(let i=0;i<70;i++){ const sp=.25+((i*37)%10)/14, X=(i*53.7)%w, L=60+((i*29)%110), Y=((t*sp*240+i*131)%(h+L*2))-L; const c=['#6366F1','#A78BFA','#34D399','#F5B84B','#38BDF8'][i%5]; const g=x.createLinearGradient(0,Y-L,0,Y); g.addColorStop(0,'rgba(0,0,0,0)'); g.addColorStop(1,c); x.strokeStyle=g; x.lineWidth=3; x.beginPath(); x.moveTo(X,Y-L); x.lineTo(X,Y); x.stroke() } }},
  symbole:{n:'Symbole BAURA',d:30,sub:'Animation du logo',draw(x,t,w,h){ const g=x.createLinearGradient(0,0,w,h); g.addColorStop(0,'#eef2ff'); g.addColorStop(1,'#ddd6fe'); x.fillStyle=g; x.fillRect(0,0,w,h); const s=h/420*.8, cx=w/2, cy=h/2; x.save(); x.translate(cx,cy); x.scale(s,s); const p=(Math.sin(t*1.4)+1)/2, lift=Math.sin(t*1.4)*14;
    x.fillStyle='#4F46E5'; x.beginPath(); x.roundRect(-99,-210+lift,48,420,24); x.fill();
    x.fillStyle='#8B5CF6'; x.beginPath(); x.moveTo(-1,-210-lift); x.arc(-1,-103-lift,101,-Math.PI/2,Math.PI/2); x.closePath(); x.fill();
    x.fillStyle='#A78BFA'; x.beginPath(); x.moveTo(-1,5+lift); x.arc(-1,107+lift,101,-Math.PI/2,Math.PI/2); x.closePath(); x.fill();
    x.fillStyle='#F5B84B'; x.beginPath(); x.roundRect(67+p*10,-8,33,36,15); x.fill(); x.restore() }}
};
const mm=s=>{ s=Math.max(0,s|0); return (s/60|0)+':'+String(s%60).padStart(2,'0') };
const imported=[]; // vidéos importées pendant la session
AppViews.videos=function(root,ctx){
  root.innerHTML=`<div class="vd"><aside class="vdside"><div class="vdhead"><b>Vidéos</b><button class="stbtn" data-a="open">＋ Ouvrir</button></div><div class="vdlist"></div><p class="vdnote">Les vidéos ouvertes depuis ton ordinateur restent disponibles tant que la session est active.</p></aside>
    <section class="vdmain"><div class="vdstage" tabindex="0"><canvas></canvas><video playsinline hidden></video><div class="vdbig" hidden>▶</div><div class="vdtitle"></div></div>
    <div class="vdctl"><button class="vb" data-a="play" aria-label="Lecture / pause">▶</button><span class="vdt">0:00</span><input type="range" class="vdseek" min="0" max="1000" value="0" aria-label="Position"><span class="vdd">0:00</span>
      <select class="ofsel" data-a="rate" aria-label="Vitesse"><option value=".5">0,5×</option><option value="1" selected>1×</option><option value="1.5">1,5×</option><option value="2">2×</option></select>
      <button class="vb" data-a="mute" aria-label="Son">🔊</button><input type="range" class="vdvol" min="0" max="100" value="80" aria-label="Volume"><button class="vb" data-a="loop" aria-label="Boucle" title="Boucle">🔁</button><button class="vb" data-a="pip" aria-label="Image dans l'image" title="Image dans l'image">⧉</button><button class="vb" data-a="fs" aria-label="Plein écran">⛶</button></div></section></div>`;
  const $=s=>root.querySelector(s), cv=$('canvas'), vid=$('video'), cx=cv.getContext('2d'), stage=$('.vdstage'), seek=$('.vdseek'), vol=$('.vdvol'), big=$('.vdbig');
  let cur=null, playing=false, t=0, rate=1, loop=false, last=0, raf=0, muted=false;
  const items=()=>{ const l=Object.entries(CLIPS).map(([k,c])=>({key:'clip:'+k,n:c.n,sub:c.sub+' · '+mm(c.d),clip:k})); imported.forEach((v,i)=>l.push({key:'imp:'+i,n:v.name,sub:'Vidéo importée',src:v.url,imp:true})); FS.all().filter(f=>f.type==='file'&&!FS.inTrash(f.id)&&(f.src||/^clip:/.test(f.content||''))&&['mp4','webm','mov','mkv','ogv','clip'].includes(FS.ext(f.name))).forEach(f=>{ const isC=/^clip:/.test(f.content||''); if(isC&&false) return; l.push({key:'fs:'+f.id,n:f.name.replace(/\.[^.]+$/,''),sub:'Fichier',clip:isC?f.content.slice(5):null,src:f.src||null,fs:f.id}) }); return l };
  const seen=new Set();
  function drawList(){ const a=items().filter(i=>{ if(i.fs&&i.clip&&Object.keys(CLIPS).includes(i.clip)&&false) return false; return true }); $('.vdlist').innerHTML=a.map(i=>`<button class="vdi${cur&&cur.key===i.key?' on':''}" data-k="${i.key}"><span class="vdth">${i.clip?'<i></i>':'🎬'}</span><span><b>${esc(i.n)}</b><small>${esc(i.sub)}</small></span></button>`).join('') }
  function dur(){ return cur?(cur.clip?CLIPS[cur.clip].d:(vid.duration||0)):0 }
  function draw(){ const w=cv.clientWidth, h=cv.clientHeight, d=devicePixelRatio||1; if(cv.width!==(w*d|0)||cv.height!==(h*d|0)){ cv.width=w*d; cv.height=h*d } cx.setTransform(d,0,0,d,0,0); if(cur&&cur.clip) CLIPS[cur.clip].draw(cx,t,w,h); else if(!cur){ cx.fillStyle='#0b0820'; cx.fillRect(0,0,w,h) } }
  function loopFn(now){ raf=requestAnimationFrame(loopFn); const dt=Math.min(.1,(now-last)/1000); last=now; if(!root.isConnected) return;
    if(cur&&cur.clip){ if(playing){ t+=dt*rate; if(t>=dur()){ if(loop) t=0; else { t=dur(); setPlaying(false) } } } draw() }
    if(cur&&!cur.clip){ t=vid.currentTime }
    if(cur){ const d=dur(); if(!seeking) seek.value=d?Math.round(t/d*1000):0; paint(seek); $('.vdt').textContent=mm(t); $('.vdd').textContent=mm(d) } }
  const paint=el=>el.style.setProperty('--p',((el.value-el.min)/(el.max-el.min)*100)+'%');
  function setPlaying(p){ playing=p; $('[data-a=play]').textContent=p?'⏸':'▶'; big.hidden=p||!cur; if(cur&&!cur.clip){ p?vid.play().catch(()=>{}):vid.pause() } }
  function play(i){ cur=i; t=0; seen.add(i.key); stage.querySelector('.vdtitle').textContent=i.n; ctx.setTitle(i.n);
    if(i.clip){ vid.hidden=true; cv.hidden=false; vid.removeAttribute('src'); vid.load() } else { cv.hidden=true; vid.hidden=false; vid.src=i.src; vid.playbackRate=rate; vid.volume=vol.value/100; vid.loop=loop }
    drawList(); setPlaying(true) }
  vid.addEventListener('ended',()=>{ if(!loop) setPlaying(false) }); vid.addEventListener('error',()=>{ if(cur&&!cur.clip){ Dlg.alert({title:'Lecture impossible',text:'Ce format de vidéo n\'est pas pris en charge par le navigateur. Essaie un fichier MP4 (H.264) ou WebM.'}); setPlaying(false) } });
  $('.vdlist').addEventListener('click',e=>{ const b=e.target.closest('.vdi'); if(!b) return; const i=items().find(x=>x.key===b.dataset.k); if(i) play(i) });
  const toggle=()=>{ if(!cur){ play(items()[0]); return } if(cur.clip&&t>=dur()) t=0; setPlaying(!playing) };
  stage.addEventListener('click',()=>{ toggle(); stage.focus() });
  stage.addEventListener('dblclick',()=>$('[data-a=fs]').click());
  stage.addEventListener('keydown',e=>{ if(e.key===' '){ e.preventDefault(); toggle() } else if(e.key==='ArrowRight') seekTo(t+5); else if(e.key==='ArrowLeft') seekTo(t-5); else if(e.key==='f') $('[data-a=fs]').click(); else if(e.key==='m') $('[data-a=mute]').click() });
  const seekTo=s=>{ const d=dur(); t=Math.max(0,Math.min(d,s)); if(cur&&!cur.clip) vid.currentTime=t };
  let seeking=false; seek.addEventListener('input',()=>{ seeking=true; seekTo(seek.value/1000*dur()); paint(seek) }); seek.addEventListener('change',()=>{ seeking=false });
  vol.addEventListener('input',()=>{ vid.volume=vol.value/100; paint(vol); muted=false; vid.muted=false; $('[data-a=mute]').textContent=vol.value==0?'🔇':'🔊' }); paint(vol);
  root.querySelector('.vdctl').addEventListener('click',e=>{ const b=e.target.closest('[data-a]'); if(!b) return; const a=b.dataset.a;
    if(a==='play') toggle(); else if(a==='mute'){ muted=!muted; vid.muted=muted; b.textContent=muted?'🔇':'🔊' } else if(a==='loop'){ loop=!loop; vid.loop=loop; b.classList.toggle('on',loop) }
    else if(a==='pip'){ if(cur&&!cur.clip&&document.pictureInPictureEnabled){ (document.pictureInPictureElement?document.exitPictureInPicture():vid.requestPictureInPicture()).catch(()=>{}) } else toast&&toast('Image dans l\'image : disponible pour les vidéos importées.') }
    else if(a==='fs'){ if(document.fullscreenElement) document.exitFullscreen(); else stage.requestFullscreen&&stage.requestFullscreen().catch(()=>{}) } });
  root.querySelector('.vdctl').addEventListener('change',e=>{ if(e.target.dataset.a==='rate'){ rate=+e.target.value; vid.playbackRate=rate } });
  function importFile(f){ if(!f||!f.type.startsWith('video/')) return; imported.push({name:f.name.replace(/\.[^.]+$/,''),url:URL.createObjectURL(f)}); drawList(); play(items().find(i=>i.key==='imp:'+(imported.length-1))) }
  $('[data-a=open]').onclick=()=>{ const i=document.createElement('input'); i.type='file'; i.accept='video/*'; i.onchange=()=>importFile(i.files[0]); i.click() };
  root.addEventListener('dragover',e=>e.preventDefault()); root.addEventListener('drop',e=>{ const f=e.dataTransfer.files&&e.dataTransfer.files[0]; if(f){ e.preventDefault(); importFile(f) } });
  FS.on(()=>{ if(root.isConnected) drawList() });
  drawList();
  const start=ctx.args.file&&FS.get(ctx.args.file); const it=start?items().find(i=>i.fs===start.id):null; if(it) play(it); else { cur=null; draw(); big.hidden=false }
  raf=requestAnimationFrame(loopFn);
  return {open(a){ if(a&&a.file){ const i=items().find(x=>x.fs===a.file); if(i) play(i) } }, destroy(){ cancelAnimationFrame(raf); vid.pause(); vid.removeAttribute('src') }};
};
OfficeSamples.clipA={name:'Aurore.clip',parent:'telechargements',content:'clip:aurore'};
OfficeSamples.clipB={name:'Orbites.clip',parent:'telechargements',content:'clip:orbites'};
})();
