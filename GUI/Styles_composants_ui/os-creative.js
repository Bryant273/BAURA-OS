/* ===== BAURA OS – Création : StageKit (éditeur d'éléments), WePoint, Crea ===== */
window.StageKit=(function(){
  let uid=0; const nid=()=>'e'+Date.now().toString(36)+(uid++).toString(36);
  const clone=o=>JSON.parse(JSON.stringify(o));
  const FONTS=['Inter','Georgia','Times New Roman','Arial','Courier New','Trebuchet MS','Impact','Verdana'];
  const bgCss=bg=>!bg?'#fff':bg.c2?`linear-gradient(${bg.ang==null?135:bg.ang}deg,${bg.c1},${bg.c2})`:bg.c1;
  const fillCss=e=>e.fill2?`linear-gradient(135deg,${e.fill},${e.fill2})`:(e.fill||'transparent');
  const STAR='50% 0,61% 35%,98% 35%,68% 57%,79% 91%,50% 70%,21% 91%,32% 57%,2% 35%,39% 35%';
  const filt=e=>{ const f=e.fx||{}; return [f.br!=null?`brightness(${f.br}%)`:'',f.ct!=null?`contrast(${f.ct}%)`:'',f.sat!=null?`saturate(${f.sat}%)`:'',f.bl?`blur(${f.bl}px)`:'',f.gray?`grayscale(${f.gray}%)`:''].filter(Boolean).join(' ')||'none' };
  function defaults(t){ const b={id:nid(),t,x:80,y:80,w:240,h:160,op:100};
    if(t==='text') return Object.assign(b,{w:420,h:80,txt:'Votre texte',fs:40,ff:'Inter',b:false,i:false,u:false,al:'left',col:'#1e1b4b',lh:1.25});
    if(t==='rect') return Object.assign(b,{fill:'#6366F1',r:0,stroke:'#312e81',sw:0}); if(t==='ellipse') return Object.assign(b,{fill:'#A78BFA',stroke:'#312e81',sw:0,w:200,h:200});
    if(t==='tri') return Object.assign(b,{fill:'#F5B84B',w:200,h:180}); if(t==='star') return Object.assign(b,{fill:'#F5B84B',w:200,h:200});
    if(t==='line') return Object.assign(b,{fill:'#1e1b4b',h:6,w:300}); if(t==='img') return Object.assign(b,{src:'',r:0,fit:'cover'}); return b }
  function build(e){ const n=document.createElement('div'); n.className='stel st-'+e.t; n.dataset.id=e.id; apply(n,e); return n }
  function apply(n,e){
    const s=n.style; s.left=e.x+'px'; s.top=e.y+'px'; s.width=e.w+'px'; s.height=e.h+'px'; s.opacity=(e.op==null?100:e.op)/100; s.display=e.hide?'none':''; s.transform=e.rot?`rotate(${e.rot}deg)`:''; s.pointerEvents=e.lock?'none':'';
    if(e.t==='text'){ if(!n._ed){ n.textContent=e.txt } s.fontSize=e.fs+'px'; s.fontFamily=`"${e.ff}",system-ui,sans-serif`; s.fontWeight=e.b?800:400; s.fontStyle=e.i?'italic':'normal'; s.textDecoration=e.u?'underline':'none'; s.textAlign=e.al; s.color=e.col; s.lineHeight=e.lh||1.25; s.padding='6px'; s.whiteSpace='pre-wrap'; s.overflow='hidden'; s.overflowWrap='anywhere' }
    else if(e.t==='img'){ n.innerHTML=e.src?`<img src="${e.src}" draggable="false" alt="">`:'<div class="stph">Image</div>'; const im=n.firstChild; if(e.src){ im.style.objectFit=e.fit||'cover'; im.style.filter=filt(e) } s.borderRadius=(e.r||0)+'px'; s.overflow='hidden' }
    else if(e.t==='line'){ s.background=fillCss(e); s.borderRadius='99px' }
    else { s.background=fillCss(e); s.borderRadius=e.t==='ellipse'?'50%':(e.r||0)+'px'; s.boxShadow=e.sw?`inset 0 0 0 ${e.sw}px ${e.stroke||'#000'}`:''; s.clipPath=e.t==='tri'?'polygon(50% 0,100% 100%,0 100%)':e.t==='star'?`polygon(${STAR})`:'' }
  }
  /* rendu statique (miniatures, présentation) */
  function thumb(page,W,H,width){ const wrap=document.createElement('div'); wrap.className='stthumb'; wrap.style.cssText=`width:${width}px;height:${width*H/W}px;position:relative;overflow:hidden`; const p=document.createElement('div'); p.className='stpage'; p.style.cssText=`width:${W}px;height:${H}px;transform:scale(${width/W});transform-origin:0 0;position:absolute;left:0;top:0;background:${bgCss(page.bg)}`; page.els.forEach(e=>p.appendChild(build(e))); wrap.appendChild(p); return wrap }
  /* rendu canvas (export PNG / JPG / PDF) */
  function loadImg(src){ return new Promise(r=>{ const i=new Image(); i.onload=()=>r(i); i.onerror=()=>r(null); i.src=src }) }
  async function toCanvas(page,W,H,scale){
    const c=document.createElement('canvas'); c.width=Math.round(W*scale); c.height=Math.round(H*scale); const x=c.getContext('2d'); x.scale(scale,scale);
    const grad=(bx,by,bw,bh,c1,c2,ang)=>{ const a=((ang==null?135:ang)-90)*Math.PI/180, cx=bx+bw/2, cy=by+bh/2, d=Math.abs(bw*Math.cos(a))+Math.abs(bh*Math.sin(a)); const g=x.createLinearGradient(cx-Math.cos(a)*d/2,cy-Math.sin(a)*d/2,cx+Math.cos(a)*d/2,cy+Math.sin(a)*d/2); g.addColorStop(0,c1); g.addColorStop(1,c2); return g };
    const bg=page.bg||{c1:'#fff'}; x.fillStyle=bg.c2?grad(0,0,W,H,bg.c1,bg.c2,bg.ang):bg.c1; x.fillRect(0,0,W,H);
    for(const e of page.els){ if(e.hide) continue; x.save(); x.globalAlpha=(e.op==null?100:e.op)/100; if(e.rot){ x.translate(e.x+e.w/2,e.y+e.h/2); x.rotate(e.rot*Math.PI/180); x.translate(-(e.x+e.w/2),-(e.y+e.h/2)) }
      const fill=e.fill2?grad(e.x,e.y,e.w,e.h,e.fill,e.fill2,135):(e.fill||'transparent');
      const path=()=>{ x.beginPath(); if(e.t==='ellipse') x.ellipse(e.x+e.w/2,e.y+e.h/2,e.w/2,e.h/2,0,0,7); else if(e.t==='tri'){ x.moveTo(e.x+e.w/2,e.y); x.lineTo(e.x+e.w,e.y+e.h); x.lineTo(e.x,e.y+e.h); x.closePath() } else if(e.t==='star'){ STAR.split(',').forEach((p,i)=>{ const [a,b]=p.trim().split(' ').map(v=>parseFloat(v)/100); i?x.lineTo(e.x+a*e.w,e.y+b*e.h):x.moveTo(e.x+a*e.w,e.y+b*e.h) }); x.closePath() } else if(x.roundRect) x.roundRect(e.x,e.y,e.w,e.h,e.t==='line'?99:(e.r||0)); else x.rect(e.x,e.y,e.w,e.h) };
      if(['rect','ellipse','tri','star','line'].includes(e.t)){ path(); x.fillStyle=fill; x.fill(); if(e.sw&&(e.t==='rect'||e.t==='ellipse')){ x.save(); x.clip(); x.lineWidth=e.sw*2; x.strokeStyle=e.stroke||'#000'; x.stroke(); x.restore() } }
      else if(e.t==='img'&&e.src){ const im=await loadImg(e.src); if(im){ path(); x.clip(); const f=filt(e); if(f!=='none') x.filter=f; const fit=e.fit||'cover'; let dw=e.w,dh=e.h,dx=e.x,dy=e.y; if(fit!=='fill'){ const k=fit==='cover'?Math.max(e.w/im.width,e.h/im.height):Math.min(e.w/im.width,e.h/im.height); dw=im.width*k; dh=im.height*k; dx=e.x+(e.w-dw)/2; dy=e.y+(e.h-dh)/2 } x.drawImage(im,dx,dy,dw,dh) } }
      else if(e.t==='text'){ x.beginPath(); x.rect(e.x,e.y,e.w,e.h); x.clip(); x.fillStyle=e.col; x.font=`${e.i?'italic ':''}${e.b?'800 ':'400 '}${e.fs}px "${e.ff}",system-ui,sans-serif`; x.textBaseline='top'; const lh=e.fs*(e.lh||1.25), maxW=e.w-12; let y=e.y+6;
        for(const para of String(e.txt).split('\n')){ let line=''; const flush=l=>{ const w=x.measureText(l).width; const px=e.al==='center'?e.x+6+(maxW-w)/2:e.al==='right'?e.x+6+maxW-w:e.x+6; x.fillText(l,px,y+(lh-e.fs)/2); if(e.u) x.fillRect(px,y+lh*.88,w,Math.max(1,e.fs/16)); y+=lh };
          for(const word of para.split(/(\s+)/)){ const t=line+word; if(x.measureText(t).width>maxW&&line.trim()){ flush(line.trimEnd()); line=word.trimStart() } else line=t } flush(line) } }
      x.restore() }
    return c;
  }
  /* ---------- éditeur ---------- */
  class Stage{
    constructor(host,o){ this.host=host; this.W=o.w; this.H=o.h; this.o=o; this.page={bg:{c1:'#fff'},els:[]}; this.selId=null; this.s=1;
      host.classList.add('stghost'); host.tabIndex=0; host.innerHTML='<div class="stwrap"><div class="stpage"><div class="stbox" hidden>'+['nw','n','ne','e','se','s','sw','w'].map(d=>`<i data-h="${d}"></i>`).join('')+'</div><i class="stg stgv" hidden></i><i class="stg stgh" hidden></i></div></div>';
      this.wrap=host.querySelector('.stwrap'); this.pg=host.querySelector('.stpage'); this.selEl=host.querySelector('.stbox'); this.gv=host.querySelector('.stgv'); this.gh=host.querySelector('.stgh');
      this.ro=new ResizeObserver(()=>this.fit()); this.ro.observe(host); this.bind() }
    size(w,h){ this.W=w; this.H=h; this.fit() }
    fit(){ const r={width:this.host.clientWidth,height:this.host.clientHeight}; if(!r.width) return; const m=this.o.pad==null?28:this.o.pad; this.s=Math.max(.05,Math.min((r.width-m*2)/this.W,(r.height-m*2)/this.H,this.o.max||1.5)); this.wrap.style.width=this.W*this.s+'px'; this.wrap.style.height=this.H*this.s+'px'; this.pg.style.cssText+=`;width:${this.W}px;height:${this.H}px;transform:scale(${this.s});--s:${this.s}`; this.place() }
    set(page){ this.page=page; this.selId=null; this.render() }
    get(id){ return this.page.els.find(e=>e.id===id) }
    get sel(){ return this.get(this.selId) }
    render(){ this.pg.style.background=bgCss(this.page.bg); [...this.pg.querySelectorAll('.stel')].forEach(n=>n.remove()); this.page.els.forEach(e=>this.pg.insertBefore(build(e),this.selEl)); this.fit(); this.place() }
    refresh(e){ const n=this.pg.querySelector(`.stel[data-id="${e.id}"]`); if(n) apply(n,e); this.pg.style.background=bgCss(this.page.bg); this.place() }
    place(){ const e=this.sel; if(!e||e.hide){ this.selEl.hidden=true; return } const s=this.selEl.style; s.hidden=false; this.selEl.hidden=false; s.left=e.x+'px'; s.top=e.y+'px'; s.width=e.w+'px'; s.height=e.h+'px'; s.transform=e.rot?`rotate(${e.rot}deg)`:'' }
    select(id){ this.selId=id; this.place(); this.o.onSel&&this.o.onSel(this.sel) }
    change(kind){ this.o.onChange&&this.o.onChange(kind) }
    add(e){ this.page.els.push(e); this.render(); this.select(e.id); this.change('add'); return e }
    remove(){ const e=this.sel; if(!e) return; this.page.els=this.page.els.filter(x=>x!==e); this.render(); this.select(null); this.change('del') }
    dup(){ const e=this.sel; if(!e) return; const c=clone(e); c.id=nid(); c.x+=24; c.y+=24; this.add(c) }
    order(d){ const a=this.page.els, e=this.sel; if(!e) return; const i=a.indexOf(e); a.splice(i,1); const j=d==='front'?a.length:d==='back'?0:Math.max(0,Math.min(a.length,i+d)); a.splice(j,0,e); this.render(); this.place(); this.change('order') }
    patch(p,quiet){ const e=this.sel; if(!e) return; Object.assign(e,p); this.refresh(e); if(!quiet) this.change('patch') }
    edit(id){ const e=this.get(id||this.selId); if(!e||e.t!=='text') return; const n=this.pg.querySelector(`.stel[data-id="${e.id}"]`); n._ed=true; n.contentEditable='plaintext-only'; if(n.contentEditable!=='plaintext-only') n.contentEditable='true'; n.classList.add('editing'); n.focus(); const r=document.createRange(); r.selectNodeContents(n); const s=getSelection(); s.removeAllRanges(); s.addRange(r);
      const done=()=>{ n.removeEventListener('blur',done); n._ed=false; n.contentEditable='false'; n.classList.remove('editing'); e.txt=n.innerText.replace(/\n$/,''); apply(n,e); this.change('text') }; n.addEventListener('blur',done) }
    bind(){
      const pg=this.pg;
      pg.addEventListener('pointerdown',ev=>{
        if(ev.button) return; const h=ev.target.closest('[data-h]'), n=ev.target.closest('.stel'); if(n&&n._ed) return;
        if(h&&this.sel){ this.drag(ev,'r',h.dataset.h); return }
        if(n){ this.select(n.dataset.id); this.host.focus({preventScroll:true}); this.drag(ev,'m'); return }
        this.select(null); this.host.focus({preventScroll:true}) });
      pg.addEventListener('dblclick',ev=>{ const n=ev.target.closest('.stel'); if(n&&n.classList.contains('st-text')) this.edit(n.dataset.id); else if(n&&n.classList.contains('st-img')&&this.o.onImgDbl) this.o.onImgDbl(this.get(n.dataset.id)) });
      this.host.addEventListener('keydown',ev=>{ if(ev.target.closest('[contenteditable=true],[contenteditable=plaintext-only],input,textarea,select')) return; const e=this.sel, k=ev.key, mod=ev.ctrlKey||ev.metaKey;
        if(mod&&k.toLowerCase()==='z'){ ev.preventDefault(); this.o.onUndo&&this.o.onUndo(ev.shiftKey) } else if(mod&&k.toLowerCase()==='y'){ ev.preventDefault(); this.o.onUndo&&this.o.onUndo(true) }
        else if(mod&&k.toLowerCase()==='d'){ ev.preventDefault(); this.dup() }
        else if(!e) return; else if(k==='Delete'||k==='Backspace'){ ev.preventDefault(); this.remove() } else if(k==='Enter'&&e.t==='text'){ ev.preventDefault(); this.edit() }
        else if(k==='Escape') this.select(null);
        else if(k.startsWith('Arrow')){ ev.preventDefault(); const d=ev.shiftKey?10:1; if(k==='ArrowLeft') e.x-=d; if(k==='ArrowRight') e.x+=d; if(k==='ArrowUp') e.y-=d; if(k==='ArrowDown') e.y+=d; this.refresh(e); this.change('move') } });
    }
    drag(ev,mode,dir){ const e=this.sel; if(!e||e.lock) return; ev.preventDefault(); const s=this.s, sx=ev.clientX, sy=ev.clientY, o={x:e.x,y:e.y,w:e.w,h:e.h}, W=this.W, H=this.H, th=6/s; let moved=false;
      const snap=(v,cands)=>{ for(const c of cands) if(Math.abs(v-c)<th) return c; return null };
      const mv=m=>{ const dx=(m.clientX-sx)/s, dy=(m.clientY-sy)/s; if(!moved&&Math.abs(dx)+Math.abs(dy)<2) return; moved=true; let gx=null, gy=null;
        if(mode==='m'){ let x=o.x+dx, y=o.y+dy; const sxl=snap(x,[0]), sxc=snap(x+o.w/2,[W/2]), sxr=snap(x+o.w,[W]), syt=snap(y,[0]), syc=snap(y+o.h/2,[H/2]), syb=snap(y+o.h,[H]);
          if(sxl!==null){ x=0; gx=0 } else if(sxc!==null){ x=W/2-o.w/2; gx=W/2 } else if(sxr!==null){ x=W-o.w; gx=W } if(syt!==null){ y=0; gy=0 } else if(syc!==null){ y=H/2-o.h/2; gy=H/2 } else if(syb!==null){ y=H-o.h; gy=H } e.x=Math.round(x); e.y=Math.round(y) }
        else { let {x,y,w,h}=o; const ar=o.w/o.h; if(dir.includes('e')) w=o.w+dx; if(dir.includes('s')) h=o.h+dy; if(dir.includes('w')){ w=o.w-dx; x=o.x+dx } if(dir.includes('n')){ h=o.h-dy; y=o.y+dy }
          if((m.shiftKey||e.t==='img')&&dir.length===2){ if(Math.abs(w-o.w)>Math.abs((h-o.h)*ar)) h=w/ar; else w=h*ar; if(dir.includes('w')) x=o.x+o.w-w; if(dir.includes('n')) y=o.y+o.h-h }
          if(w<12){ if(dir.includes('w')) x-=12-w; w=12 } if(h<4){ if(dir.includes('n')) y-=4-h; h=4 } e.x=Math.round(x); e.y=Math.round(y); e.w=Math.round(w); e.h=Math.round(h) }
        this.gv.hidden=gx===null; this.gh.hidden=gy===null; if(gx!==null) this.gv.style.left=gx+'px'; if(gy!==null) this.gh.style.top=gy+'px'; this.refresh(e); this.o.onGeom&&this.o.onGeom(e) };
      const up=()=>{ removeEventListener('pointermove',mv); removeEventListener('pointerup',up); this.gv.hidden=this.gh.hidden=true; if(moved) this.change('move') };
      addEventListener('pointermove',mv); addEventListener('pointerup',up) }
    destroy(){ this.ro.disconnect() }
  }
  /* ---------- inspecteur partagé ---------- */
  function inspector(host,stage,extra){
    const F=(l,h)=>`<div class="ins-r"><span>${l}</span>${h}</div>`;
    const num=(k,v,min,max,step)=>`<input type="number" data-k="${k}" value="${Math.round(v*100)/100}" ${min!=null?`min="${min}"`:''} ${max!=null?`max="${max}"`:''} step="${step||1}">`;
    const rng=(k,v,min,max)=>`<input type="range" data-k="${k}" min="${min}" max="${max}" value="${v}">`;
    const col=(k,v)=>`<input type="color" data-k="${k}" value="${/^#[0-9a-f]{6}$/i.test(v||'')?v:'#6366f1'}">`;
    function draw(){ const e=stage.sel; if(!e){ host.innerHTML=(extra?extra():'')+'<p class="ins-none">Sélectionne un élément pour le modifier.</p>'; return }
      let h=(extra?extra():'')+`<h6>${{text:'Texte',rect:'Rectangle',ellipse:'Cercle',tri:'Triangle',star:'Étoile',line:'Ligne',img:'Image'}[e.t]}</h6>`;
      if(e.t==='text') h+=F('Police',`<select data-k="ff">${FONTS.map(f=>`<option${f===e.ff?' selected':''}>${f}</option>`).join('')}</select>`)+F('Taille',num('fs',e.fs,6,400))+F('Style',`<span class="ins-b"><button data-t="b" class="${e.b?'on':''}"><b>G</b></button><button data-t="i" class="${e.i?'on':''}"><i>I</i></button><button data-t="u" class="${e.u?'on':''}"><u>S</u></button><button data-al="left" class="${e.al==='left'?'on':''}">⇤</button><button data-al="center" class="${e.al==='center'?'on':''}">↔</button><button data-al="right" class="${e.al==='right'?'on':''}">⇥</button></span>`)+F('Couleur',col('col',e.col))+F('Interligne',rng('lh',e.lh||1.25,.8,2.5).replace('<input','<input step="0.05"'));
      else if(e.t==='img'){ h+=F('Ajustement',`<select data-k="fit"><option value="cover"${e.fit!=='contain'&&e.fit!=='fill'?' selected':''}>Remplir</option><option value="contain"${e.fit==='contain'?' selected':''}>Contenir</option><option value="fill"${e.fit==='fill'?' selected':''}>Étirer</option></select>`)+F('Arrondi',rng('r',e.r||0,0,Math.min(e.w,e.h)/2))+F('Luminosité',rng('fx.br',(e.fx||{}).br??100,20,200))+F('Contraste',rng('fx.ct',(e.fx||{}).ct??100,20,200))+F('Saturation',rng('fx.sat',(e.fx||{}).sat??100,0,300))+F('Flou',rng('fx.bl',(e.fx||{}).bl||0,0,20))+F('Gris',rng('fx.gray',(e.fx||{}).gray||0,0,100)); }
      else { h+=F('Remplissage',col('fill',e.fill)+`<label class="ins-ck"><input type="checkbox" data-g ${e.fill2?'checked':''}>dégradé</label>`+(e.fill2?col('fill2',e.fill2):''));
        if(e.t==='rect'||e.t==='ellipse') h+=F('Bordure',col('stroke',e.stroke)+num('sw',e.sw||0,0,60)); if(e.t==='rect') h+=F('Arrondi',rng('r',e.r||0,0,Math.min(e.w,e.h)/2)) }
      h+=F('Opacité',rng('op',e.op==null?100:e.op,5,100))+F('Rotation',num('rot',e.rot||0,-180,180))+F('Position',`<span class="ins-xy">${num('x',e.x)}${num('y',e.y)}</span>`)+F('Taille',`<span class="ins-xy">${num('w',e.w,4)}${num('h',e.h,4)}</span>`)+
        `<div class="ins-act"><button data-o="front" title="Premier plan">⤒</button><button data-o="back" title="Arrière-plan">⤓</button><button data-o="1" title="Avancer">↑</button><button data-o="-1" title="Reculer">↓</button><button data-x="dup" title="Dupliquer">⧉</button><button data-x="lock" title="Verrouiller">${e.lock?'🔒':'🔓'}</button><button data-x="del" class="dng" title="Supprimer">🗑</button></div>`;
      host.innerHTML=h; host.querySelectorAll('input[type=range]').forEach(r=>{ r.style.setProperty('--p',((r.value-r.min)/(r.max-r.min)*100)+'%') }) }
    host.addEventListener('input',ev=>{ const t=ev.target, k=t.dataset.k; if(!k||!stage.sel) return; let v=t.type==='number'||t.type==='range'?+t.value:t.value; if(t.type==='range') t.style.setProperty('--p',((t.value-t.min)/(t.max-t.min)*100)+'%');
      if(k.startsWith('fx.')){ const fx=Object.assign({},stage.sel.fx||{}); fx[k.slice(3)]=v; stage.patch({fx},true) } else stage.patch({[k]:v},true); if(['x','y','w','h','rot'].includes(k)) stage.place(); });
    host.addEventListener('change',ev=>{ const t=ev.target; if(t.dataset.g!==undefined){ stage.patch(t.checked?{fill2:'#F5B84B'}:{fill2:null}); draw() } else if(t.dataset.k) stage.change('patch') });
    host.addEventListener('click',ev=>{ const b=ev.target.closest('button'); if(!b||!stage.sel) return; const e=stage.sel;
      if(b.dataset.t){ stage.patch({[b.dataset.t]:!e[b.dataset.t]}); draw() } else if(b.dataset.al){ stage.patch({al:b.dataset.al}); draw() }
      else if(b.dataset.o){ stage.order(isNaN(+b.dataset.o)?b.dataset.o:+b.dataset.o) } else if(b.dataset.x==='dup') stage.dup(); else if(b.dataset.x==='del') stage.remove(); else if(b.dataset.x==='lock'){ stage.patch({lock:!e.lock}); draw() } });
    return {draw};
  }
  return {Stage,thumb,toCanvas,defaults,inspector,clone,nid,bgCss,FONTS};
})();

/* =============================== WEPOINT =============================== */
(function(){
const {later,bind,download,pick}=OffKit, SK=StageKit, W=960, H=540;
SK.shrink=function(dataUrl,max,keepAlpha){ return new Promise(res=>{ if(/^data:image\/svg/.test(dataUrl)||dataUrl.length<250000) return res(dataUrl); const i=new Image(); i.onload=()=>{ const k=Math.min(1,max/Math.max(i.width,i.height)), c=document.createElement('canvas'); c.width=Math.round(i.width*k); c.height=Math.round(i.height*k); c.getContext('2d').drawImage(i,0,0,c.width,c.height); res(c.toDataURL(keepAlpha?'image/png':'image/jpeg',.86)) }; i.onerror=()=>res(dataUrl); i.src=dataUrl }) };
const TH={
  indigo:{n:'Indigo',bg:{c1:'#312e81',c2:'#6d28d9',ang:135},t:'#ffffff',b:'#e0e7ff',a:'#F5B84B'},
  nuit:{n:'Nuit',bg:{c1:'#0b1020',c2:'#1e1b4b',ang:160},t:'#f8fafc',b:'#cbd5e1',a:'#38BDF8'},
  lavande:{n:'Lavande',bg:{c1:'#f5f3ff',c2:'#ddd6fe',ang:135},t:'#312e81',b:'#4c4a72',a:'#7c3aed'},
  or:{n:'Or doré',bg:{c1:'#fffbeb',c2:'#fde68a',ang:135},t:'#78350f',b:'#92400e',a:'#d97706'},
  clair:{n:'Clair',bg:{c1:'#ffffff'},t:'#111827',b:'#4b5563',a:'#6366F1'}
};
const tx=(o,th,k)=>Object.assign(SK.defaults('text'),{col:k==='t'?th.t:th.b,ff:'Inter',b:k==='t'},o);
const LAYOUTS={
  titre:['Titre',th=>[tx({x:90,y:170,w:780,h:130,txt:'Titre de la présentation',fs:64,al:'left',lh:1.1},th,'t'),tx({x:90,y:320,w:780,h:60,txt:'Sous-titre · Auteur · Date',fs:26},th,'b'),Object.assign(SK.defaults('rect'),{x:96,y:150,w:90,h:8,fill:th.a,r:4})]],
  contenu:['Titre et contenu',th=>[tx({x:60,y:40,w:840,h:90,txt:'Titre de la diapositive',fs:44,lh:1.1},th,'t'),Object.assign(SK.defaults('rect'),{x:66,y:126,w:70,h:6,fill:th.a,r:3}),tx({x:60,y:160,w:840,h:330,txt:'• Premier point\n• Deuxième point\n• Troisième point',fs:30,lh:1.5},th,'b')]],
  deux:['Deux colonnes',th=>[tx({x:60,y:40,w:840,h:90,txt:'Comparaison',fs:44},th,'t'),tx({x:60,y:160,w:405,h:330,txt:'Colonne A\n• Idée\n• Idée',fs:26,lh:1.5},th,'b'),tx({x:495,y:160,w:405,h:330,txt:'Colonne B\n• Idée\n• Idée',fs:26,lh:1.5},th,'b')]],
  section:['Section',th=>[Object.assign(SK.defaults('rect'),{x:0,y:0,w:36,h:540,fill:th.a}),tx({x:100,y:200,w:760,h:140,txt:'Nouvelle section',fs:60,lh:1.1},th,'t')]],
  vide:['Vide',()=>[]]
};
const mkSlide=(k,th)=>({els:LAYOUTS[k][1](TH[th]),notes:'',bg:null});
const sample=()=>({v:1,theme:'indigo',slides:[mkSlide('titre','indigo'),mkSlide('contenu','indigo'),mkSlide('deux','indigo'),mkSlide('section','indigo')].map((s,i)=>{ if(i===0){ s.els[0].txt='Bienvenue dans WePoint'; s.els[1].txt='Des diapositives simples, belles et rapides' } if(i===1){ s.els[0].txt='Ce que tu peux faire'; s.els[2].txt='• Ajouter du texte, des formes et des images\n• Glisser les éléments, les aligner (guides magnétiques)\n• Changer de thème en un clic\n• Présenter en plein écran avec F5' } if(i===2){ s.els[0].txt='Avant / Après'; s.els[1].txt='Avant\n• Fichiers dispersés\n• Outils séparés'; s.els[2].txt='Après\n• Tout dans BAURA OS\n• Skrib, Numbro, WePoint' } if(i===3){ s.els[1].txt='Merci !' } return s })});

AppViews.wepoint=function(root,ctx){
  const B=bind(ctx,'wepoint','Présentation');
  let D=sample(); const load=()=>{ try{ const p=JSON.parse(B.raw()); if(p&&Array.isArray(p.slides)&&p.slides.length) D=p; else if(B.raw().trim()===''&&B.id) D={v:1,theme:'indigo',slides:[mkSlide('titre','indigo')]} }catch(e){ if(B.id) D={v:1,theme:'indigo',slides:[mkSlide('titre','indigo')]} } };
  load(); if(!B.id) D=sample();
  let cur=0, hist=[], hi=-1;
  root.innerHTML=`<div class="of wp"><div class="oftool"><input class="oftitle" aria-label="Nom de la présentation" spellcheck="false">
    <button class="tb" data-a="add" title="Nouvelle diapositive">＋ Diapo</button><button class="tb" data-a="dupS" title="Dupliquer la diapositive">⧉</button><button class="tb" data-a="delS" title="Supprimer la diapositive">🗑</button><i class="sep"></i>
    <button class="tb" data-a="text" title="Zone de texte"><b>T</b></button><button class="tb" data-s="rect" title="Rectangle">▬</button><button class="tb" data-s="ellipse" title="Cercle">●</button><button class="tb" data-s="tri" title="Triangle">▲</button><button class="tb" data-s="star" title="Étoile">★</button><button class="tb" data-s="line" title="Ligne">―</button><button class="tb" data-a="img" title="Image">🖼</button><i class="sep"></i>
    <select class="ofsel" data-a="theme" aria-label="Thème">${Object.entries(TH).map(([k,t])=>`<option value="${k}">${t.n}</option>`).join('')}</select><label class="clr" title="Fond de la diapositive"><span style="background:linear-gradient(135deg,#6d28d9,#F5B84B)"></span><input type="color" data-a="bgc" value="#6d28d9"></label><i class="sep"></i>
    <button class="tb" data-a="undo">↶</button><button class="tb" data-a="redo">↷</button><span style="flex:1"></span><button class="tb" data-a="png" title="Exporter la diapositive en PNG">PNG</button><button class="tb" data-a="pdf" title="Exporter en PDF">PDF</button><button class="stbtn pres" data-a="present">▶ Présenter</button></div>
    <div class="wpbody"><aside class="wpthumbs"></aside><section class="wpmain"><div class="wpstage"></div><textarea class="wpnotes" placeholder="Notes du présentateur…" spellcheck="true"></textarea></section><aside class="wpins"></aside></div>
    <div class="ofstat"><span class="stsel"></span><span class="stsave">Enregistré</span></div></div>`;
  const $=s=>root.querySelector(s), thumbs=$('.wpthumbs'), notes=$('.wpnotes'), title=$('.oftitle'), statSave=$('.stsave'), statSel=$('.stsel');
  title.value=B.name(); ctx.setTitle(B.name());
  const save=later(()=>{ try{ const s=JSON.stringify(D); B.last=s; B.save(s); statSave.textContent='Enregistré' }catch(e){ statSave.textContent='Espace plein !' } },450);
  const snap=()=>{ hist=hist.slice(0,hi+1); hist.push(JSON.stringify(D)); if(hist.length>60) hist.shift(); hi=hist.length-1 };
  const th=()=>TH[D.theme]||TH.indigo;
  const pageOf=s=>({bg:s.bg||th().bg,els:s.els});
  const stage=new SK.Stage($('.wpstage'),{w:W,h:H,pad:20,
    onChange:k=>{ D.slides[cur].els=stage.page.els; if(k!=='move'||true) snap(); statSave.textContent='Enregistrement…'; save(); drawThumb(cur); ins.draw() },
    onSel:()=>ins.draw(), onGeom:()=>{ const e=stage.sel; if(e){ const xs=ins.host; xs.querySelectorAll('[data-k=x]').forEach(i=>i.value=e.x); xs.querySelectorAll('[data-k=y]').forEach(i=>i.value=e.y); xs.querySelectorAll('[data-k=w]').forEach(i=>i.value=e.w); xs.querySelectorAll('[data-k=h]').forEach(i=>i.value=e.h) } },
    onUndo:redo=>undo(redo)});
  const insHost=$('.wpins'); const ins=SK.inspector(insHost,stage); ins.host=insHost;
  function drawThumbs(){ thumbs.innerHTML=D.slides.map((s,i)=>`<div class="wpth${i===cur?' on':''}" data-i="${i}" draggable="true"><b>${i+1}</b><div class="tc"></div></div>`).join(''); D.slides.forEach((s,i)=>drawThumb(i)) }
  function drawThumb(i){ const c=thumbs.querySelectorAll('.wpth .tc')[i]; if(!c) return; const s=D.slides[i]; c.innerHTML=''; c.appendChild(SK.thumb(pageOf(s),W,H,148)) }
  function show(i){ cur=Math.max(0,Math.min(D.slides.length-1,i)); stage.set(pageOf(D.slides[cur])); notes.value=D.slides[cur].notes||''; thumbs.querySelectorAll('.wpth').forEach((t,k)=>t.classList.toggle('on',k===cur)); statSel.textContent=`Diapositive ${cur+1} / ${D.slides.length}`; ins.draw(); const t=thumbs.querySelector('.wpth.on'); t&&t.scrollIntoView({block:'nearest'}) }
  function full(i){ drawThumbs(); show(i==null?cur:i); root.querySelector('[data-a=theme]').value=D.theme }
  function undo(redo){ if(redo?hi<hist.length-1:hi>0){ hi+=redo?1:-1; D=JSON.parse(hist[hi]); cur=Math.min(cur,D.slides.length-1); full(); statSave.textContent='Enregistrement…'; save() } }
  const structural=()=>{ snap(); statSave.textContent='Enregistrement…'; save() };
  thumbs.addEventListener('click',e=>{ const t=e.target.closest('.wpth'); if(t) show(+t.dataset.i) });
  let dragI=null; thumbs.addEventListener('dragstart',e=>{ const t=e.target.closest('.wpth'); if(t){ dragI=+t.dataset.i; e.dataTransfer.effectAllowed='move'; e.dataTransfer.setData('text/plain','s') } });
  thumbs.addEventListener('dragover',e=>{ if(dragI!==null) e.preventDefault() });
  thumbs.addEventListener('drop',e=>{ const t=e.target.closest('.wpth'); if(dragI===null||!t) return; e.preventDefault(); const j=+t.dataset.i; const [s]=D.slides.splice(dragI,1); D.slides.splice(j,0,s); dragI=null; structural(); full(j) });
  notes.addEventListener('input',()=>{ D.slides[cur].notes=notes.value; statSave.textContent='Enregistrement…'; save() });
  notes.addEventListener('change',snap);
  async function addImg(){ pick('image/*',async(d,f)=>{ d=await SK.shrink(d,1600,f.type==='image/png'); const im=new Image(); im.onload=()=>{ const k=Math.min(1,500/im.width,380/im.height); stage.add(Object.assign(SK.defaults('img'),{src:d,x:(W-im.width*k)/2|0,y:(H-im.height*k)/2|0,w:im.width*k|0,h:im.height*k|0,fit:'fill'})) }; im.src=d }) }
  root.querySelector('.oftool').addEventListener('click',e=>{ const b=e.target.closest('button'); if(!b) return; const a=b.dataset.a, s=b.dataset.s;
    if(s){ const el=SK.defaults(s); if(s==='line'){ el.x=330; el.y=260 } el.fill=th().a; if(s==='line') el.fill=th().t; el.x=el.x+Math.random()*40|0; stage.add(el) }
    else if(a==='text'){ const el=SK.defaults('text'); el.col=th().b; el.x=120; el.y=200; el.fs=32; stage.add(el); stage.edit(el.id) }
    else if(a==='img') addImg();
    else if(a==='add'){ const r=b.getBoundingClientRect(); Ctx.show(Object.keys(LAYOUTS).map(k=>({label:LAYOUTS[k][0],run:()=>{ D.slides.splice(cur+1,0,mkSlide(k,D.theme)); structural(); full(cur+1) }})),r.left,r.bottom+4) }
    else if(a==='dupS'){ D.slides.splice(cur+1,0,JSON.parse(JSON.stringify(D.slides[cur]))); D.slides[cur+1].els.forEach(x=>x.id=SK.nid()); structural(); full(cur+1) }
    else if(a==='delS'){ if(D.slides.length<2){ D.slides[0]=mkSlide('vide',D.theme) } else D.slides.splice(cur,1); structural(); full(Math.min(cur,D.slides.length-1)) }
    else if(a==='undo') undo(false); else if(a==='redo') undo(true);
    else if(a==='present') present(); else if(a==='png') exportPNG(); else if(a==='pdf') exportPDF() });
  root.querySelector('.oftool').addEventListener('change',e=>{ const a=e.target.dataset.a;
    if(a==='theme'){ const old=th(), nt=TH[e.target.value]; D.slides.forEach(s=>s.els.forEach(x=>{ if(x.t==='text'){ if(x.col===old.t) x.col=nt.t; else if(x.col===old.b) x.col=nt.b } else if(x.fill===old.a) x.fill=nt.a })); D.theme=e.target.value; structural(); full() }
    else if(a==='bgc'){ D.slides[cur].bg={c1:e.target.value}; structural(); full() } });
  title.addEventListener('change',()=>{ B.rename(title.value); title.value=B.name(); ctx.setTitle(B.name()) });
  title.addEventListener('keydown',e=>{ if(e.key==='Enter') title.blur() });
  /* exports */
  async function exportPNG(){ const c=await SK.toCanvas(pageOf(D.slides[cur]),W,H,2); c.toBlob(b=>download(`${B.name()}-${cur+1}.png`,'image/png',b),'image/png') }
  async function exportPDF(){ const w=window.open('','_blank'); if(!w){ Dlg.alert({title:'Export PDF',text:'Autorise les fenêtres pop-up, puis choisis « Enregistrer au format PDF » dans la boîte d\'impression.'}); return } const imgs=[]; for(const s of D.slides) imgs.push((await SK.toCanvas(pageOf(s),W,H,1.5)).toDataURL('image/jpeg',.92));
    w.document.write(`<!doctype html><title>${esc(B.name())}</title><style>@page{size:297mm 167mm;margin:0}body{margin:0}img{display:block;width:297mm;height:167mm;page-break-after:always}</style>${imgs.map(s=>`<img src="${s}">`).join('')}<script>onload=()=>setTimeout(()=>print(),400)<\/script>`); w.document.close() }
  /* présentation plein écran */
  function present(){ let i=cur; const ov=document.createElement('div'); ov.className='wppres'; ov.tabIndex=0; ov.innerHTML='<div class="wpslide"></div><div class="wpbar"><span class="wpn"></span><i class="wpdots"></i></div><button class="wpx" aria-label="Quitter">×</button>'; document.body.appendChild(ov);
    try{ ov.requestFullscreen&&ov.requestFullscreen() }catch(e){}
    const sl=ov.querySelector('.wpslide'); const paint=(dir)=>{ const w=Math.min(innerWidth,innerHeight*16/9); sl.innerHTML=''; const t=SK.thumb(pageOf(D.slides[i]),W,H,w); t.className+=' wpin'; sl.appendChild(t); ov.querySelector('.wpn').textContent=`${i+1} / ${D.slides.length}`; ov.querySelector('.wpdots').style.setProperty('--p',((i+1)/D.slides.length*100)+'%') };
    const go=d=>{ const n=i+d; if(n<0) return; if(n>=D.slides.length){ close(); return } i=n; paint() };
    const close=()=>{ removeEventListener('resize',paint); if(document.fullscreenElement) document.exitFullscreen().catch(()=>{}); ov.remove(); show(i); stage.host.focus() };
    ov.addEventListener('keydown',e=>{ if(['ArrowRight','ArrowDown',' ','PageDown','Enter'].includes(e.key)){ e.preventDefault(); go(1) } else if(['ArrowLeft','ArrowUp','PageUp','Backspace'].includes(e.key)){ e.preventDefault(); go(-1) } else if(e.key==='Escape'){ close() } else if(e.key==='Home'){ i=0; paint() } else if(e.key==='End'){ i=D.slides.length-1; paint() } });
    ov.addEventListener('click',e=>{ if(e.target.closest('.wpx')) close(); else go(1) }); ov.addEventListener('contextmenu',e=>{ e.preventDefault(); go(-1) });
    document.addEventListener('fullscreenchange',function f(){ if(!document.fullscreenElement){ document.removeEventListener('fullscreenchange',f); if(ov.isConnected) close() } }); addEventListener('resize',paint); paint(); ov.focus() }
  root.addEventListener('keydown',e=>{ if(e.key==='F5'){ e.preventDefault(); present() } });
  snap(); full(0);
  return {open(a){ if(a&&a.file&&FS.get(a.file)){ B.id=a.file; load(); title.value=B.name(); ctx.setTitle(B.name()); hist=[]; hi=-1; snap(); full(0) } }, destroy(){ save.now(); stage.destroy() }};
};
OfficeSamples.wepoint={name:'Découvrir WePoint.wepoint',parent:'documents',content:JSON.stringify(sample())};
})();

/* =============================== CREA =============================== */
(function(){
const {later,bind,download,pick}=OffKit, SK=StageKit;
const FORMATS={post:['Post carré',1080,1080],story:['Story',1080,1920],aff:['Affiche A4',794,1123],ban:['Bannière',1500,500],card:['Carte de visite',1050,600],yt:['Miniature vidéo',1280,720]};
const T=(o)=>Object.assign(SK.defaults('text'),o), R=(t,o)=>Object.assign(SK.defaults(t),o);
const TPL={
  citation:['Citation',()=>({fmt:'post',bg:{c1:'#312e81',c2:'#7c3aed',ang:150},els:[R('ellipse',{x:760,y:-120,w:520,h:520,fill:'#F5B84B',op:90}),T({x:90,y:300,w:900,h:420,txt:'« Le meilleur moment\npour commencer,\nc\'est maintenant. »',fs:76,col:'#fff',b:true,lh:1.2}),T({x:90,y:800,w:700,h:60,txt:'— BAURA OS',fs:36,col:'#e0e7ff'})]})],
  concert:['Affiche concert',()=>({fmt:'aff',bg:{c1:'#0b1020',c2:'#312e81',ang:170},els:[R('ellipse',{x:120,y:150,w:560,h:560,fill:'#7c3aed',fill2:'#F5B84B',op:85}),T({x:60,y:690,w:674,h:260,txt:'AURA\nLIVE',fs:120,col:'#fff',b:true,al:'center',lh:.95}),T({x:60,y:960,w:674,h:60,txt:'Samedi 14 novembre · 20 h',fs:34,col:'#F5B84B',al:'center'}),T({x:60,y:1020,w:674,h:50,txt:'Salle Indigo · Abidjan',fs:26,col:'#e0e7ff',al:'center'})]})],
  promo:['Promotion',()=>({fmt:'post',bg:{c1:'#fff7ed'},els:[R('star',{x:230,y:120,w:620,h:620,fill:'#fb923c',fill2:'#ef4444'}),T({x:230,y:340,w:620,h:200,txt:'-30%',fs:170,col:'#fff',b:true,al:'center'}),T({x:80,y:790,w:920,h:110,txt:'Soldes d\'été',fs:88,col:'#7c2d12',b:true,al:'center'}),T({x:80,y:920,w:920,h:60,txt:'Jusqu\'au 31 juillet, en ligne et en boutique',fs:34,col:'#9a3412',al:'center'})]})],
  carte:['Carte de visite',()=>({fmt:'card',bg:{c1:'#ffffff'},els:[R('rect',{x:0,y:0,w:36,h:600,fill:'#6366F1',fill2:'#A78BFA'}),R('ellipse',{x:820,y:60,w:150,h:150,fill:'#F5B84B'}),T({x:100,y:150,w:700,h:90,txt:'Amina Koné',fs:70,b:true,col:'#1e1b4b'}),T({x:100,y:250,w:700,h:50,txt:'Designer produit',fs:34,col:'#6366F1'}),T({x:100,y:400,w:800,h:130,txt:'amina@baura.mail\n+225 01 02 03 04 05\nAbidjan',fs:28,col:'#4b5563',lh:1.5})]})],
  banniere:['Bannière',()=>({fmt:'ban',bg:{c1:'#6366F1',c2:'#A78BFA',ang:120},els:[R('ellipse',{x:1180,y:-100,w:420,h:420,fill:'#F5B84B',op:90}),T({x:80,y:140,w:1000,h:130,txt:'Une idée. Un design.',fs:96,b:true,col:'#fff'}),T({x:80,y:300,w:1000,h:60,txt:'Crée tes visuels avec Crea',fs:38,col:'#ede9fe'})]})],
  story:['Story',()=>({fmt:'story',bg:{c1:'#0f172a',c2:'#4c1d95',ang:180},els:[R('rect',{x:90,y:250,w:900,h:900,fill:'#F5B84B',r:60,op:95}),T({x:140,y:560,w:800,h:300,txt:'Nouveau !',fs:150,b:true,col:'#1e1b4b',al:'center'}),T({x:90,y:1350,w:900,h:200,txt:'Glisse vers le haut\npour découvrir',fs:60,col:'#e0e7ff',al:'center',lh:1.3})]})]
};
AppViews.crea=function(root,ctx){
  const B=bind(ctx,'crea','Création');
  let D={v:1,fmt:'post',w:1080,h:1080,page:{bg:{c1:'#ffffff'},els:[]}};
  const load=()=>{ try{ const p=JSON.parse(B.raw()); if(p&&p.page&&p.w) D=p; else if(!B.id) throw 0 }catch(e){ const t=TPL.citation[1](); D={v:1,fmt:t.fmt,w:FORMATS[t.fmt][1],h:FORMATS[t.fmt][2],page:{bg:t.bg,els:t.els}} } };
  load(); let hist=[],hi=-1;
  root.innerHTML=`<div class="of cr"><div class="oftool"><input class="oftitle" aria-label="Nom de la création" spellcheck="false">
    <select class="ofsel" data-a="fmt" aria-label="Format">${Object.entries(FORMATS).map(([k,v])=>`<option value="${k}">${v[0]}</option>`).join('')}</select><button class="tb" data-a="tpl">✨ Modèles</button><i class="sep"></i>
    <button class="tb" data-a="text" title="Texte"><b>T</b></button><button class="tb" data-s="rect">▬</button><button class="tb" data-s="ellipse">●</button><button class="tb" data-s="tri">▲</button><button class="tb" data-s="star">★</button><button class="tb" data-s="line">―</button><button class="tb" data-a="img" title="Image">🖼</button><i class="sep"></i>
    <button class="tb" data-a="undo">↶</button><button class="tb" data-a="redo">↷</button><span style="flex:1"></span><button class="tb" data-a="png">PNG</button><button class="tb" data-a="jpg">JPG</button><button class="stbtn pres" data-a="toimg">→ Images</button></div>
    <div class="crbody"><section class="crstage"></section><aside class="crside"><div class="crins"></div><h6>Calques</h6><div class="crlay"></div></aside></div>
    <div class="ofstat"><span class="stsel"></span><span class="stsave">Enregistré</span></div></div>`;
  const $=s=>root.querySelector(s), title=$('.oftitle'), statSave=$('.stsave'), laysEl=$('.crlay'), insHost=$('.crins');
  title.value=B.name(); ctx.setTitle(B.name());
  const save=later(()=>{ try{ const s=JSON.stringify(D); B.last=s; B.save(s); statSave.textContent='Enregistré' }catch(e){ statSave.textContent='Espace plein !' } },450);
  const snap=()=>{ hist=hist.slice(0,hi+1); hist.push(JSON.stringify(D)); if(hist.length>60) hist.shift(); hi=hist.length-1 };
  const stage=new SK.Stage($('.crstage'),{w:D.w,h:D.h,pad:24,max:1,
    onChange:()=>{ D.page=stage.page; snap(); statSave.textContent='Enregistrement…'; save(); drawLayers(); ins.draw() },
    onSel:()=>{ ins.draw(); drawLayers() }, onUndo:r=>undo(r),
    onGeom:()=>{ const e=stage.sel; if(e) ['x','y','w','h'].forEach(k=>insHost.querySelectorAll(`[data-k=${k}]`).forEach(i=>i.value=e[k])) } });
  const bgUI=()=>{ const b=D.page.bg||{c1:'#ffffff'}; return `<h6>Fond</h6><div class="ins-r"><span>Couleur</span><input type="color" data-bg="c1" value="${b.c1}"><label class="ins-ck"><input type="checkbox" data-bgg ${b.c2?'checked':''}>dégradé</label>${b.c2?`<input type="color" data-bg="c2" value="${b.c2}">`:''}</div>`+(b.c2?`<div class="ins-r"><span>Angle</span><input type="range" data-bg="ang" min="0" max="360" value="${b.ang==null?135:b.ang}"></div>`:'') };
  const ins=SK.inspector(insHost,stage,bgUI);
  insHost.addEventListener('input',e=>{ const k=e.target.dataset.bg; if(!k) return; D.page.bg=Object.assign({},D.page.bg,{[k]:k==='ang'?+e.target.value:e.target.value}); stage.pg.style.background=SK.bgCss(D.page.bg) });
  insHost.addEventListener('change',e=>{ if(e.target.dataset.bg!==undefined){ stage.change('bg') } else if(e.target.dataset.bgg!==undefined){ const b=D.page.bg; D.page.bg=e.target.checked?Object.assign({},b,{c2:'#F5B84B',ang:135}):{c1:b.c1}; stage.render(); stage.change('bg') } });
  function drawLayers(){ const els=stage.page.els; laysEl.innerHTML=els.length?[...els].reverse().map(e=>`<div class="cli${e.id===stage.selId?' on':''}${e.hide?' off':''}" data-id="${e.id}"><span class="cic">${{text:'T',rect:'▬',ellipse:'●',tri:'▲',star:'★',line:'―',img:'🖼'}[e.t]}</span><span class="cn">${esc(e.t==='text'?e.txt.split('\n')[0]||'Texte':e.t==='img'?'Image':({rect:'Rectangle',ellipse:'Cercle',tri:'Triangle',star:'Étoile',line:'Ligne'})[e.t])}</span><button data-v title="Afficher/masquer">${e.hide?'🚫':'👁'}</button><button data-l title="Verrouiller">${e.lock?'🔒':'🔓'}</button></div>`).join(''):'<p class="ins-none">Aucun élément.</p>'; $('.stsel').textContent=`${D.w} × ${D.h} px · ${els.length} élément${els.length>1?'s':''}` }
  laysEl.addEventListener('click',e=>{ const r=e.target.closest('.cli'); if(!r) return; const el=stage.get(r.dataset.id); if(e.target.closest('[data-v]')){ el.hide=!el.hide; stage.render(); stage.change('vis') } else if(e.target.closest('[data-l]')){ el.lock=!el.lock; stage.render(); stage.change('lock') } else { stage.select(el.id) } });
  function apply(){ $('[data-a=fmt]').value=D.fmt; stage.size(D.w,D.h); stage.set(D.page); drawLayers(); ins.draw() }
  function undo(redo){ if(redo?hi<hist.length-1:hi>0){ hi+=redo?1:-1; D=JSON.parse(hist[hi]); apply(); save() } }
  async function addImgData(d,f){ d=await SK.shrink(d,1800,f&&f.type==='image/png'); const im=new Image(); im.onload=()=>{ const k=Math.min(1,D.w*.7/im.width,D.h*.7/im.height); stage.add(Object.assign(SK.defaults('img'),{src:d,w:im.width*k|0,h:im.height*k|0,x:(D.w-im.width*k)/2|0,y:(D.h-im.height*k)/2|0,fit:'fill'})) }; im.src=d }
  const stageEl=$('.crstage'); stageEl.addEventListener('dragover',e=>e.preventDefault()); stageEl.addEventListener('drop',e=>{ const f=e.dataTransfer.files&&e.dataTransfer.files[0]; if(f&&f.type.startsWith('image/')){ e.preventDefault(); const r=new FileReader(); r.onload=()=>addImgData(r.result,f); r.readAsDataURL(f) } });
  root.querySelector('.oftool').addEventListener('click',e=>{ const b=e.target.closest('button'); if(!b) return; const a=b.dataset.a, s=b.dataset.s, r=b.getBoundingClientRect();
    if(s){ const el=SK.defaults(s); el.x=(D.w-el.w)/2|0; el.y=(D.h-el.h)/2|0; stage.add(el) }
    else if(a==='text'){ const el=SK.defaults('text'); el.x=(D.w-el.w)/2|0; el.y=(D.h-el.h)/2|0; el.fs=Math.round(D.w/16); stage.add(el); stage.edit(el.id) }
    else if(a==='img'){ const imgs=FS.all().filter(f=>f.type==='file'&&f.kind==='image'&&f.src&&!FS.inTrash(f.id)).slice(0,12); Ctx.show([{label:'Depuis l\'ordinateur…',run:()=>pick('image/*',(d,f)=>addImgData(d,f))},'sep',...imgs.map(f=>({label:f.name,run:()=>{ const k=Math.min(1,D.w*.5/400); stage.add(Object.assign(SK.defaults('img'),{src:f.src,w:400*k|0,h:400*k|0,x:(D.w-400*k)/2|0,y:(D.h-400*k)/2|0,fit:'contain'})) }}))],r.left,r.bottom+4) }
    else if(a==='tpl'){ Ctx.show(Object.entries(TPL).map(([k,[n,fn]])=>({label:n,run:async()=>{ if(stage.page.els.length&&!await Dlg.confirm({title:'Utiliser ce modèle ?',text:'Ta création actuelle sera remplacée.',ok:'Remplacer'})) return; const t=fn(); D.fmt=t.fmt; D.w=FORMATS[t.fmt][1]; D.h=FORMATS[t.fmt][2]; D.page={bg:t.bg,els:t.els}; apply(); snap(); save() }})),r.left,r.bottom+4) }
    else if(a==='undo') undo(false); else if(a==='redo') undo(true);
    else if(a==='png') exp('png'); else if(a==='jpg') exp('jpg'); else if(a==='toimg') exp('save') });
  root.querySelector('.oftool').addEventListener('change',e=>{ if(e.target.dataset.a==='fmt'){ const f=FORMATS[e.target.value], ow=D.w, oh=D.h; D.fmt=e.target.value; D.w=f[1]; D.h=f[2]; const k=Math.min(D.w/ow,D.h/oh); D.page.els.forEach(x=>{ x.x=Math.round(x.x*D.w/ow); x.y=Math.round(x.y*D.h/oh); x.w=Math.round(x.w*k); x.h=Math.round(x.h*k); if(x.t==='text') x.fs=Math.round(x.fs*k) }); apply(); snap(); save() } });
  title.addEventListener('change',()=>{ B.rename(title.value); title.value=B.name(); ctx.setTitle(B.name()) }); title.addEventListener('keydown',e=>{ if(e.key==='Enter') title.blur() });
  async function exp(kind){ try{ const sel=stage.selId; const c=await SK.toCanvas(D.page,D.w,D.h,kind==='save'?Math.min(1,1280/Math.max(D.w,D.h)):1);
      if(kind==='save'){ const url=c.toDataURL('image/jpeg',.88); FS.create({name:B.name()+'.jpg',type:'file',parent:'images',kind:'image',src:url}); notify({app:'crea',title:'Enregistré dans Images',text:B.name()+'.jpg est disponible dans Photos et Fichiers.',level:'ok',actions:[{label:'Voir dans Photos',run:()=>openApp('photos')}]}) }
      else c.toBlob(b=>download(B.name()+'.'+(kind==='jpg'?'jpg':'png'),kind==='jpg'?'image/jpeg':'image/png',b),kind==='jpg'?'image/jpeg':'image/png',.92) }
    catch(err){ Dlg.alert({title:'Export impossible',text:'Une image externe empêche l\'export ('+(err.message||err)+'). Ajoute-la depuis l\'ordinateur et réessaie.'}) } }
  snap(); apply(); setTimeout(()=>stage.host.focus({preventScroll:true}),100);
  return {open(a){ if(a&&a.file&&FS.get(a.file)){ B.id=a.file; load(); title.value=B.name(); ctx.setTitle(B.name()); hist=[]; hi=-1; snap(); apply() } }, destroy(){ save.now(); stage.destroy() }};
};
OfficeSamples.crea={name:'Affiche exemple.crea',parent:'documents',content:JSON.stringify((()=>{ const t=TPL.concert[1](); return {v:1,fmt:t.fmt,w:FORMATS[t.fmt][1],h:FORMATS[t.fmt][2],page:{bg:t.bg,els:t.els}} })())};
})();
