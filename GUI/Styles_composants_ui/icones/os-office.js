/* ===== BAURA OS – Bureautique : Numbro (tableur), Data (bases), Skrib (texte) ===== */
window.OffKit=(function(){
  const later=(fn,ms)=>{ let t; const f=()=>{ clearTimeout(t); t=setTimeout(fn,ms) }; f.now=()=>{ clearTimeout(t); fn() }; return f };
  function bind(ctx,ext,label){
    let id=ctx.args.file&&FS.get(ctx.args.file)&&FS.get(ctx.args.file).type==='file'?ctx.args.file:null;
    const o={
      get id(){ return id }, set id(v){ id=v },
      raw(){ const f=id&&FS.get(id); return f?(f.content||''):'' },
      name(){ const f=id&&FS.get(id); return f?f.name.replace(/\.[^.]+$/,''):'Sans titre' },
      save(s){ if(!id||!FS.get(id)){ id=FS.create({name:label+'.'+ext,type:'file',parent:'documents',content:s}) } else FS.update(id,{content:s},true) },
      rename(n){ n=(n||'').trim(); if(!n) return; if(!id){ o.save(o.last||'') } FS.rename(id,n+'.'+ext) }
    }; return o;
  }
  function download(name,mime,data){ const a=document.createElement('a'); a.href=URL.createObjectURL(new Blob([data],{type:mime})); a.download=name; document.body.appendChild(a); a.click(); setTimeout(()=>{ URL.revokeObjectURL(a.href); a.remove() },500) }
  function pick(accept,cb,text){ const i=document.createElement('input'); i.type='file'; i.accept=accept; i.onchange=()=>{ const f=i.files[0]; if(!f) return; const r=new FileReader(); r.onload=()=>cb(r.result,f); text?r.readAsText(f):r.readAsDataURL(f) }; i.click() }
  const PAL=['#6366F1','#A78BFA','#F5B84B','#34D399','#F87171','#38BDF8','#F472B6','#FB923C'];
  return {later,bind,download,pick,PAL};
})();

/* =============================== NUMBRO =============================== */
(function(){
const {later,bind,download,pick,PAL}=OffKit;
class SErr{ constructor(c){ this.c=c } }
const colN=s=>{ let n=0; for(const ch of s) n=n*26+ch.charCodeAt(0)-64; return n-1 };
const cn=i=>(i>=26?String.fromCharCode(64+(i/26|0)):'')+String.fromCharCode(65+i%26);
const rc=(c,r)=>cn(c)+(r+1);
const pr=s=>{ const m=/^\$?([A-Z]{1,2})\$?(\d+)$/.exec(s); return m?{c:colN(m[1]),r:+m[2]-1}:null };
const TOK=/\s*(?:(\d+\.?\d*(?:e[-+]?\d+)?|\.\d+)|("(?:[^"]|"")*")|(\$?[A-Z]{1,2}\$?\d+(?::\$?[A-Z]{1,2}\$?\d+)?)(?![A-Za-z_(])|([A-Za-z_][A-Za-z0-9_.]*)\s*(\()?|(<>|<=|>=|[-+*\/^&=<>(),%;]))/y;
const thr=c=>{ throw new SErr(c) };
const num=v=>{ if(v===null||v===undefined||v==='') return 0; if(typeof v==='boolean') return +v; if(typeof v==='number') return v; const t=String(v).trim().replace(',','.'); if(t!==''&&!isNaN(+t)) return +t; thr('#VALEUR!') };
const str=v=>v===null||v===undefined?'':typeof v==='boolean'?(v?'VRAI':'FAUX'):String(v);
const sc=v=>Array.isArray(v)?(v.length===1?v[0]:thr('#VALEUR!')):v;
const truthy=v=>{ v=sc(v); return typeof v==='string'?v.toUpperCase()==='VRAI':!!v };
const cmpv=(a,b)=>{ if(a===null) a=typeof b==='string'?'':0; if(b===null) b=typeof a==='string'?'':0; if(typeof a==='number'&&typeof b==='number') return a-b; if(typeof a==='boolean'||typeof b==='boolean') return num(a)-num(b); const x=String(a).toLowerCase(), y=String(b).toLowerCase(); return x<y?-1:x>y?1:0 };
const flat=a=>a.flatMap(t=>{ const v=t(); return Array.isArray(v)?v:[v] });
const nums=a=>flat(a).filter(v=>typeof v==='number'||typeof v==='boolean').map(Number);
const FN={
  SUM:a=>nums(a).reduce((x,y)=>x+y,0), SOMME:a=>FN.SUM(a),
  AVERAGE:a=>{ const n=nums(a); if(!n.length) thr('#DIV/0!'); return n.reduce((x,y)=>x+y,0)/n.length }, MOYENNE:a=>FN.AVERAGE(a),
  MIN:a=>{ const n=nums(a); return n.length?Math.min(...n):0 }, MAX:a=>{ const n=nums(a); return n.length?Math.max(...n):0 },
  COUNT:a=>nums(a).length, NB:a=>FN.COUNT(a), COUNTA:a=>flat(a).filter(v=>v!==null&&v!=='').length, NBVAL:a=>FN.COUNTA(a),
  IF:a=>truthy(a[0]())?(a[1]?a[1]():true):(a[2]?a[2]():false), SI:a=>FN.IF(a),
  ROUND:a=>{ const p=Math.pow(10,a[1]?num(sc(a[1]())):0); return Math.round(num(sc(a[0]()))*p)/p }, ARRONDI:a=>FN.ROUND(a),
  ABS:a=>Math.abs(num(sc(a[0]()))), SQRT:a=>{ const v=num(sc(a[0]())); if(v<0) thr('#NOMBRE!'); return Math.sqrt(v) }, RACINE:a=>FN.SQRT(a),
  POWER:a=>Math.pow(num(sc(a[0]())),num(sc(a[1]()))), PUISSANCE:a=>FN.POWER(a),
  MOD:a=>{ const d=num(sc(a[1]())); if(!d) thr('#DIV/0!'); return num(sc(a[0]()))%d },
  INT:a=>Math.floor(num(sc(a[0]()))), ENT:a=>FN.INT(a), PI:()=>Math.PI,
  CONCAT:a=>flat(a).map(str).join(''), CONCATENER:a=>FN.CONCAT(a),
  LEN:a=>str(sc(a[0]())).length, NBCAR:a=>FN.LEN(a), UPPER:a=>str(sc(a[0]())).toUpperCase(), MAJUSCULE:a=>FN.UPPER(a), LOWER:a=>str(sc(a[0]())).toLowerCase(), MINUSCULE:a=>FN.LOWER(a),
  TODAY:()=>new Date().toLocaleDateString('fr-FR'), AUJOURDHUI:()=>FN.TODAY(), NOW:()=>new Date().toLocaleString('fr-FR'), MAINTENANT:()=>FN.NOW(),
  AND:a=>flat(a).every(truthy), ET:a=>FN.AND(a), OR:a=>flat(a).some(truthy), OU:a=>FN.OR(a), NOT:a=>!truthy(a[0]()), NON:a=>FN.NOT(a),
  SUMIF:a=>{ const r=a[0](), cr=sc(a[1]()), s=a[2]?a[2]():r; return crit(r,cr).reduce((t,i)=>t+(typeof s[i]==='number'?s[i]:0),0) }, 'SOMME.SI':a=>FN.SUMIF(a),
  COUNTIF:a=>crit(a[0](),sc(a[1]())).length, 'NB.SI':a=>FN.COUNTIF(a)
};
function crit(r,c){ r=Array.isArray(r)?r:[r]; const m=/^(<=|>=|<>|<|>|=)?(.*)$/.exec(String(c)), op=m[1]||'=', t=m[2], tn=t!==''&&!isNaN(+t)?+t:t; const out=[]; r.forEach((v,i)=>{ const d=cmpv(v,tn); if(op==='='?d===0:op==='<>'?d!==0:op==='<'?d<0:op==='>'?d>0:op==='<='?d<=0:d>=0) out.push(i) }); return out }

function Engine(cells){
  const cache=new Map(), stack=new Set();
  function cell(ref){
    if(cache.has(ref)){ const v=cache.get(ref); if(v instanceof SErr) throw v; return v }
    if(stack.has(ref)) throw new SErr('#CYCLE!');
    const c=cells[ref]; let v=null;
    if(c&&c.v!==undefined&&c.v!==''){ const raw=c.v;
      if(typeof raw==='string'&&raw[0]==='='&&raw.length>1){ stack.add(ref); try{ v=run(raw.slice(1)) }catch(e){ stack.delete(ref); const er=e instanceof SErr?e:new SErr('#ERR!'); cache.set(ref,er); throw er } stack.delete(ref); if(Array.isArray(v)) v=v[0] }
      else if(typeof raw==='number') v=raw;
      else { const t=String(raw).trim().replace(',','.'); if(/^[-+]?(\d+\.?\d*|\.\d+)(e[-+]?\d+)?$/i.test(t)) v=+t; else if(/^[-+]?(\d+\.?\d*|\.\d+)\s*%$/.test(t)) v=parseFloat(t)/100; else v=String(raw) } }
    cache.set(ref,v); return v;
  }
  function run(src){
    const toks=[]; let pos=0, m;
    while(pos<src.length){ TOK.lastIndex=pos; m=TOK.exec(src); if(!m){ if(/^\s*$/.test(src.slice(pos))) break; thr('#ERR!') } pos=TOK.lastIndex;
      if(m[1]!==undefined) toks.push({t:'n',v:parseFloat(m[1])}); else if(m[2]!==undefined) toks.push({t:'s',v:m[2].slice(1,-1).replace(/""/g,'"')});
      else if(m[3]!==undefined) toks.push({t:'r',v:m[3].replace(/\$/g,'')}); else if(m[4]!==undefined) toks.push(m[5]?{t:'f',v:m[4].toUpperCase()}:{t:'w',v:m[4].toUpperCase()}); else toks.push({t:'o',v:m[6]}) }
    let i=0; const O=v=>toks[i]&&toks[i].t==='o'&&toks[i].v===v;
    const cmp=()=>{ let a=cat(); while(toks[i]&&toks[i].t==='o'&&['=','<>','<','>','<=','>='].includes(toks[i].v)){ const op=toks[i++].v, b=cat(), l=a; a=()=>{ const d=cmpv(sc(l()),sc(b())); return op==='='?d===0:op==='<>'?d!==0:op==='<'?d<0:op==='>'?d>0:op==='<='?d<=0:d>=0 } } return a };
    const cat=()=>{ let a=add(); while(O('&')){ i++; const b=add(), l=a; a=()=>str(sc(l()))+str(sc(b())) } return a };
    const add=()=>{ let a=mul(); while(O('+')||O('-')){ const op=toks[i++].v, b=mul(), l=a; a=()=>op==='+'?num(sc(l()))+num(sc(b())):num(sc(l()))-num(sc(b())) } return a };
    const mul=()=>{ let a=pow(); while(O('*')||O('/')){ const op=toks[i++].v, b=pow(), l=a; a=()=>{ const x=num(sc(l())), y=num(sc(b())); if(op==='/'){ if(y===0) thr('#DIV/0!'); return x/y } return x*y } } return a };
    const pow=()=>{ let a=un(); while(O('^')){ i++; const b=un(), l=a; a=()=>Math.pow(num(sc(l())),num(sc(b()))) } return a };
    const un=()=>{ if(O('-')){ i++; const a=un(); return ()=>-num(sc(a())) } if(O('+')){ i++; return un() } let a=prim(); while(O('%')){ i++; const l=a; a=()=>num(sc(l()))/100 } return a };
    const prim=()=>{ const t=toks[i++]; if(!t) thr('#ERR!');
      if(t.t==='n') return ()=>t.v; if(t.t==='s') return ()=>t.v;
      if(t.t==='w'){ if(t.v==='VRAI'||t.v==='TRUE') return ()=>true; if(t.v==='FAUX'||t.v==='FALSE') return ()=>false; thr('#NOM?') }
      if(t.t==='r'){ if(t.v.includes(':')){ const [a,b]=t.v.split(':').map(pr); if(!a||!b) thr('#REF!'); const c1=Math.min(a.c,b.c),c2=Math.max(a.c,b.c),r1=Math.min(a.r,b.r),r2=Math.max(a.r,b.r); return ()=>{ const o=[]; for(let r=r1;r<=r2;r++) for(let c=c1;c<=c2;c++) o.push(cell(rc(c,r))); return o } }
        if(!pr(t.v)) thr('#REF!'); return ()=>cell(t.v) }
      if(t.t==='f'){ const args=[]; if(!O(')')){ args.push(cmp()); while(O(',')||O(';')){ i++; args.push(cmp()) } } if(!O(')')) thr('#ERR!'); i++; const f=FN[t.v]; if(!f) thr('#NOM?'); return ()=>f(args) }
      if(t.t==='o'&&t.v==='('){ const a=cmp(); if(!O(')')) thr('#ERR!'); i++; return a }
      thr('#ERR!') };
    const e=cmp(); if(i<toks.length) thr('#ERR!'); return e();
  }
  return {get(ref){ try{ return {v:cell(ref)} }catch(e){ return {err:e instanceof SErr?e.c:'#ERR!'} } }};
}
// décale les références relatives d'une formule (copier/coller, remplissage)
function shiftF(f,dc,dr){ if(typeof f!=='string'||f[0]!=='=') return f; return f.replace(/("(?:[^"]|"")*")|(\$?)([A-Z]{1,2})(\$?)(\d+)/g,(m,s,d1,c,d2,r)=>{ if(s) return s; const cc=d1?colN(c):colN(c)+dc, rr=d2?+r-1:+r-1+dr; return (cc<0||rr<0)?'#REF!':(d1+cn(cc)+d2+(rr+1)) }) }

const FM={g:'Général',n2:'Nombre (0,00)',int:'Entier',eur:'Monnaie (€)',pct:'Pourcentage'};
const nf={n2:new Intl.NumberFormat('fr-FR',{minimumFractionDigits:2,maximumFractionDigits:2}),int:new Intl.NumberFormat('fr-FR',{maximumFractionDigits:0}),eur:new Intl.NumberFormat('fr-FR',{style:'currency',currency:'EUR'}),pct:new Intl.NumberFormat('fr-FR',{style:'percent',minimumFractionDigits:0,maximumFractionDigits:2}),g:new Intl.NumberFormat('fr-FR',{maximumFractionDigits:10})};
const fmtV=(r,fm)=>{ if(r.err) return r.err; const v=r.v; if(v===null) return ''; if(typeof v==='boolean') return v?'VRAI':'FAUX'; if(typeof v==='number') return isFinite(v)?(nf[fm]||nf.g).format(v):'#NOMBRE!'; return String(v) };

AppViews.numbro=function(root,ctx){
  const B=bind(ctx,'numbro','Classeur');
  let M={v:1,cells:{},rows:100,chart:null}, hist=[], hi=-1, cur={c:0,r:0}, end={c:0,r:0}, eng=null, editing=null;
  const COLS=26;
  try{ const p=JSON.parse(B.raw()); if(p&&p.cells) M=Object.assign(M,p) }catch(e){}
  root.innerHTML=`<div class="of nb">
    <div class="oftool"><input class="oftitle" aria-label="Nom du classeur" spellcheck="false">
      <button class="tb" data-a="undo" title="Annuler (Ctrl+Z)">${BI.i('undo')}</button><button class="tb" data-a="redo" title="Rétablir (Ctrl+Y)">${BI.i('redo')}</button><i class="sep"></i>
      <button class="tb" data-a="b" title="Gras">${BI.i('bold')}</button><button class="tb" data-a="i" title="Italique">${BI.i('italic')}</button>
      <button class="tb" data-a="al-l" title="Aligner à gauche">${BI.i('alignL')}</button><button class="tb" data-a="al-c" title="Centrer">${BI.i('alignC')}</button><button class="tb" data-a="al-r" title="Aligner à droite">${BI.i('alignR')}</button><i class="sep"></i>
      <select class="ofsel" data-a="fm" aria-label="Format">${Object.entries(FM).map(([k,v])=>`<option value="${k}">${v}</option>`).join('')}</select>
      <label class="clr" title="Couleur de fond"><span style="background:#FDE68A"></span><input type="color" data-a="bg" value="#fde68a"></label>
      <label class="clr" title="Couleur du texte"><span class="t">A</span><input type="color" data-a="co" value="#4f46e5"></label><i class="sep"></i>
      <button class="tb" data-a="sum" title="Somme automatique">${BI.i('sigma')}</button><button class="tb" data-a="sortA" title="Trier A→Z">${BI.i('sortAZ')}</button><button class="tb" data-a="sortD" title="Trier Z→A">${BI.i('sortZA')}</button><button class="tb" data-a="chart" title="Graphique">${BI.i('chart')}</button><i class="sep"></i>
      <button class="tb" data-a="csvo" title="Importer un CSV">${BI.i('arrowUp')}CSV</button><button class="tb" data-a="csvx" title="Exporter en CSV">${BI.i('arrowDown')}CSV</button></div>
    <div class="nbfx"><span class="nbname">A1</span><span class="fxl">${BI.i('func')}</span><input class="nbfxin" aria-label="Formule" spellcheck="false" autocomplete="off"></div>
    <div class="nbgrid" tabindex="0"><table class="nbt"></table><input class="nbed" hidden spellcheck="false" autocomplete="off"></div>
    <div class="nbchart" hidden><div class="ncbar"><b>Graphique</b><select class="ofsel" data-a="ctype"><option value="bar">Barres</option><option value="line">Courbe</option><option value="pie">Secteurs</option></select><span class="ncrange"></span><button class="tb" data-a="cclose">×</button></div><canvas></canvas></div>
    <div class="ofstat"><span class="stsel">Prêt</span><span class="stsave">Enregistré</span></div></div>`;
  const $=s=>root.querySelector(s), gridEl=$('.nbgrid'), tbl=$('.nbt'), ed=$('.nbed'), fxin=$('.nbfxin'), nameEl=$('.nbname'), title=$('.oftitle'), statSave=$('.stsave'), statSel=$('.stsel'), chartEl=$('.nbchart');
  title.value=B.name(); ctx.setTitle(B.name());
  const save=later(()=>{ const s=JSON.stringify(M); B.last=s; B.save(s); statSave.textContent='Enregistré' },400);
  const touch=()=>{ statSave.textContent='Enregistrement…'; save() };
  const snap=()=>{ hist=hist.slice(0,hi+1); hist.push(JSON.stringify({cells:M.cells,rows:M.rows})); if(hist.length>60) hist.shift(); hi=hist.length-1 };
  const restore=i=>{ const s=JSON.parse(hist[i]); M.cells=s.cells; M.rows=s.rows; build(); recalc(); touch() };
  snap();
  const rect=()=>({c1:Math.min(cur.c,end.c),c2:Math.max(cur.c,end.c),r1:Math.min(cur.r,end.r),r2:Math.max(cur.r,end.r)});
  const cellOf=(c,r)=>M.cells[rc(c,r)]||(M.cells[rc(c,r)]={});
  const tdOf=(c,r)=>tbl.rows[r+1]&&tbl.rows[r+1].cells[c+1];

  function build(){
    let h='<thead><tr><th class="corner"></th>'+Array.from({length:COLS},(_,c)=>`<th class="ch" data-c="${c}">${cn(c)}</th>`).join('')+'</tr></thead><tbody>';
    for(let r=0;r<M.rows;r++){ h+=`<tr><th class="rh" data-r="${r}">${r+1}</th>`+Array.from({length:COLS},(_,c)=>`<td data-c="${c}" data-r="${r}"></td>`).join('')+'</tr>' }
    tbl.innerHTML=h+'</tbody>'; recalc(); paintSel();
  }
  function paintCell(c,r){
    const td=tdOf(c,r); if(!td) return; const ref=rc(c,r), x=M.cells[ref]||{}, res=eng.get(ref), t=fmtV(res,x.fm);
    td.textContent=t; td.className=(typeof res.v==='number'?'num ':'')+(res.err?'err ':'')+(x.b?'b ':'')+(x.i?'it ':''); td.style.textAlign=x.al==='l'?'left':x.al==='c'?'center':x.al==='r'?'right':''; td.style.background=x.bg||''; td.style.color=x.co||'';
    if(td.dataset.in==='1') td.classList.add('in'); if(cur.c===c&&cur.r===r) td.classList.add('cur');
  }
  function recalc(){ eng=Engine(M.cells); for(let r=0;r<M.rows;r++) for(let c=0;c<COLS;c++) paintCell(c,r); drawChart() }
  let lastR=null;
  function paintSel(){
    if(lastR) for(let r=lastR.r1;r<=lastR.r2;r++) for(let c=lastR.c1;c<=lastR.c2;c++){ const td=tdOf(c,r); if(td){ td.dataset.in=''; td.classList.remove('in','cur') } }
    const R=rect(); lastR=R;
    for(let r=R.r1;r<=R.r2;r++) for(let c=R.c1;c<=R.c2;c++){ const td=tdOf(c,r); if(td){ td.dataset.in='1'; td.classList.add('in') } }
    const t=tdOf(cur.c,cur.r); if(t) t.classList.add('cur');
    tbl.querySelectorAll('.ch.on,.rh.on').forEach(e=>e.classList.remove('on')); for(let c=R.c1;c<=R.c2;c++) tbl.tHead.rows[0].cells[c+1].classList.add('on'); for(let r=R.r1;r<=R.r2;r++){ const h=tbl.rows[r+1]&&tbl.rows[r+1].cells[0]; if(h) h.classList.add('on') }
    nameEl.textContent=(R.c1===R.c2&&R.r1===R.r2)?rc(cur.c,cur.r):rc(R.c1,R.r1)+':'+rc(R.c2,R.r2);
    const x=M.cells[rc(cur.c,cur.r)]||{}; if(document.activeElement!==fxin) fxin.value=x.v===undefined?'':x.v; root.querySelector('[data-a=fm]').value=x.fm||'g';
    const ns=[]; for(let r=R.r1;r<=R.r2;r++) for(let c=R.c1;c<=R.c2;c++){ const v=eng.get(rc(c,r)).v; if(typeof v==='number') ns.push(v) }
    statSel.textContent=ns.length>1?`Somme ${nf.g.format(ns.reduce((a,b)=>a+b,0))} · Moyenne ${nf.g.format(ns.reduce((a,b)=>a+b,0)/ns.length)} · ${ns.length} nombres`:'Prêt';
    const td=tdOf(cur.c,cur.r); if(td&&!editing){ const g=gridEl.getBoundingClientRect(), b=td.getBoundingClientRect(); if(b.bottom>g.bottom-4) gridEl.scrollTop+=b.bottom-g.bottom+30; else if(b.top<g.top+26) gridEl.scrollTop-=g.top+30-b.top; if(b.right>g.right-4) gridEl.scrollLeft+=b.right-g.right+30; else if(b.left<g.left+44) gridEl.scrollLeft-=g.left+48-b.left }
  }
  function select(c,r,extend){ c=Math.max(0,Math.min(COLS-1,c)); r=Math.max(0,Math.min(M.rows-1,r)); if(extend){ end={c,r} } else { cur={c,r}; end={c,r} } if(r>=M.rows-3&&M.rows<500){ M.rows=Math.min(500,M.rows+50); build() } paintSel() }
  function setRaw(c,r,v){ const x=cellOf(c,r); if(v===''||v===undefined) delete x.v; else x.v=v }
  function commit(c,r,v){ if((M.cells[rc(c,r)]||{}).v===v||(v===''&&!M.cells[rc(c,r)])) return; setRaw(c,r,v); snap(); recalc(); touch(); paintSel() }
  function startEdit(init,sel){ const td=tdOf(cur.c,cur.r), g=gridEl.getBoundingClientRect(), b=td.getBoundingClientRect(); const x=M.cells[rc(cur.c,cur.r)]||{};
    ed.hidden=false; ed.style.left=(b.left-g.left+gridEl.scrollLeft)+'px'; ed.style.top=(b.top-g.top+gridEl.scrollTop)+'px'; ed.style.width=Math.max(b.width,140)+'px'; ed.style.height=b.height+'px';
    ed.value=init!==undefined?init:(x.v===undefined?'':x.v); editing={c:cur.c,r:cur.r}; ed.focus(); if(init===undefined&&!sel) ed.setSelectionRange(ed.value.length,ed.value.length); else if(init===undefined) ed.select() }
  function endEdit(save,mv){ if(!editing) return; const e=editing; editing=null; ed.hidden=true; const v=ed.value; if(save) commit(e.c,e.r,v); gridEl.focus({preventScroll:true}); if(mv) select(e.c+mv[0],e.r+mv[1]); else paintSel() }
  ed.addEventListener('keydown',e=>{ if(e.key==='Enter'){ e.preventDefault(); endEdit(true,[0,1]) } else if(e.key==='Tab'){ e.preventDefault(); endEdit(true,[e.shiftKey?-1:1,0]) } else if(e.key==='Escape'){ e.preventDefault(); endEdit(false) } e.stopPropagation() });
  ed.addEventListener('input',()=>{ fxin.value=ed.value });
  ed.addEventListener('blur',()=>{ if(editing&&!formulaPick) endEdit(true) });
  let formulaPick=false;
  fxin.addEventListener('keydown',e=>{ if(e.key==='Enter'){ e.preventDefault(); commit(cur.c,cur.r,fxin.value); gridEl.focus() } else if(e.key==='Escape'){ paintSel(); gridEl.focus() } });

  // souris
  let drag=false;
  tbl.addEventListener('mousedown',e=>{
    const td=e.target.closest('td'), ch=e.target.closest('.ch'), rh=e.target.closest('.rh');
    if(td){ const c=+td.dataset.c, r=+td.dataset.r;
      if(editing&&ed.value[0]==='='&&/[-+*\/^&=<>(,;]$/.test(ed.value)){ e.preventDefault(); formulaPick=true; ed.value+=rc(c,r); ed.focus(); formulaPick=false; return }
      if(editing) endEdit(true); drag=true; select(c,r,e.shiftKey); gridEl.focus({preventScroll:true}); e.preventDefault() }
    else if(ch){ if(editing) endEdit(true); const c=+ch.dataset.c; cur={c,r:0}; end={c,r:M.rows-1}; paintSel(); gridEl.focus() }
    else if(rh){ if(editing) endEdit(true); const r=+rh.dataset.r; cur={c:0,r}; end={c:COLS-1,r}; paintSel(); gridEl.focus() } });
  tbl.addEventListener('mouseover',e=>{ if(!drag) return; const td=e.target.closest('td'); if(td) select(+td.dataset.c,+td.dataset.r,true) });
  addEventListener('mouseup',()=>{ drag=false });
  tbl.addEventListener('dblclick',e=>{ const td=e.target.closest('td'); if(td){ select(+td.dataset.c,+td.dataset.r); startEdit() } });

  // clavier
  const mv=(dc,dr,e)=>{ if(e.shiftKey) select(end.c+dc,end.r+dr,true); else select((e.ctrlKey?cur.c:cur.c)+dc,cur.r+dr) };
  gridEl.addEventListener('keydown',e=>{
    if(editing) return; const k=e.key, mod=e.ctrlKey||e.metaKey;
    if(mod&&k.toLowerCase()==='z'){ e.preventDefault(); if(hi>0) restore(--hi); return }
    if(mod&&(k.toLowerCase()==='y')){ e.preventDefault(); if(hi<hist.length-1) restore(++hi); return }
    if(mod&&k.toLowerCase()==='b'){ e.preventDefault(); fmtSel(x=>x.b=!x.b); return }
    if(mod&&k.toLowerCase()==='i'){ e.preventDefault(); fmtSel(x=>x.i=!x.i); return }
    if(mod&&k.toLowerCase()==='d'){ e.preventDefault(); fillDown(); return }
    if(mod&&k.toLowerCase()==='a'){ e.preventDefault(); cur={c:0,r:0}; end={c:COLS-1,r:M.rows-1}; paintSel(); return }
    if(mod) return;
    if(k==='ArrowDown'){ e.preventDefault(); mv(0,1,e) } else if(k==='ArrowUp'){ e.preventDefault(); mv(0,-1,e) } else if(k==='ArrowLeft'){ e.preventDefault(); mv(-1,0,e) } else if(k==='ArrowRight'){ e.preventDefault(); mv(1,0,e) }
    else if(k==='Tab'){ e.preventDefault(); select(cur.c+(e.shiftKey?-1:1),cur.r) } else if(k==='Enter'){ e.preventDefault(); startEdit() } else if(k==='F2'){ e.preventDefault(); startEdit() }
    else if(k==='Delete'||k==='Backspace'){ e.preventDefault(); const R=rect(); for(let r=R.r1;r<=R.r2;r++) for(let c=R.c1;c<=R.c2;c++){ const x=M.cells[rc(c,r)]; if(x) delete x.v } snap(); recalc(); touch(); paintSel() }
    else if(k==='Home'){ select(0,cur.r) } else if(k==='PageDown'){ select(cur.c,cur.r+15) } else if(k==='PageUp'){ select(cur.c,cur.r-15) }
    else if(k.length===1&&!e.altKey){ e.preventDefault(); startEdit(k) } });
  // copier / coller (TSV)
  gridEl.addEventListener('copy',e=>{ const R=rect(), rows=[]; for(let r=R.r1;r<=R.r2;r++){ const row=[]; for(let c=R.c1;c<=R.c2;c++){ const x=M.cells[rc(c,r)]; row.push(x&&x.v!==undefined?x.v:'') } rows.push(row.join('\t')) } e.clipboardData.setData('text/plain',rows.join('\n')); e.clipboardData.setData('application/x-numbro',JSON.stringify({c:R.c1,r:R.r1,t:rows.join('\n')})); e.preventDefault() });
  gridEl.addEventListener('cut',e=>{ gridEl.dispatchEvent(new ClipboardEvent('copy',{clipboardData:e.clipboardData})); e.preventDefault(); const R=rect(); for(let r=R.r1;r<=R.r2;r++) for(let c=R.c1;c<=R.c2;c++){ const x=M.cells[rc(c,r)]; if(x) delete x.v } snap(); recalc(); touch(); paintSel() });
  gridEl.addEventListener('paste',e=>{ e.preventDefault(); const t=e.clipboardData.getData('text/plain'); if(!t) return; let o=null; try{ o=JSON.parse(e.clipboardData.getData('application/x-numbro')||'null') }catch(_){}
    const dc=o&&o.t===t?cur.c-o.c:0, dr=o&&o.t===t?cur.r-o.r:0; t.replace(/\r/g,'').replace(/\n$/,'').split('\n').forEach((line,j)=>line.split('\t').forEach((v,i)=>{ if(cur.c+i<COLS&&cur.r+j<M.rows) setRaw(cur.c+i,cur.r+j,shiftF(v,dc,dr)) })); snap(); recalc(); touch(); paintSel() });
  function fillDown(){ const R=rect(); if(R.r1===R.r2) return; for(let c=R.c1;c<=R.c2;c++){ const s=M.cells[rc(c,R.r1)]; for(let r=R.r1+1;r<=R.r2;r++){ const d=cellOf(c,r); if(s&&s.v!==undefined) d.v=shiftF(s.v,0,r-R.r1); else delete d.v } } snap(); recalc(); touch(); paintSel() }
  function fmtSel(fn){ const R=rect(); for(let r=R.r1;r<=R.r2;r++) for(let c=R.c1;c<=R.c2;c++) fn(cellOf(c,r)); snap(); recalc(); touch(); paintSel() }

  // barre d'outils
  function sortSel(desc){ const R=rect(); let r1=R.r1,r2=R.r2,c1=R.c1,c2=R.c2; if(r1===r2){ r1=0; r2=M.rows-1; for(let r=0;r<M.rows;r++){ if(M.cells[rc(c1,r)]&&M.cells[rc(c1,r)].v!==undefined) { r2=r } } c1=0; c2=COLS-1; for(let c=COLS-1;c>=0;c--){ let any=false; for(let r=r1;r<=r2&&!any;r++) any=M.cells[rc(c,r)]&&M.cells[rc(c,r)].v!==undefined; if(any){ c2=c; break } } }
    const key=R.c1, rows=[]; for(let r=r1;r<=r2;r++){ rows.push({k:eng.get(rc(key,r)).v,cells:Array.from({length:c2-c1+1},(_,i)=>M.cells[rc(c1+i,r)]?Object.assign({},M.cells[rc(c1+i,r)]):null)}) }
    rows.sort((a,b)=>{ const d=a.k===null?(b.k===null?0:1):b.k===null?-1:cmpv(a.k,b.k); return desc?-d:d });
    rows.forEach((row,j)=>row.cells.forEach((x,i)=>{ const ref=rc(c1+i,r1+j); if(x) M.cells[ref]=shiftRow(x,j+r1,row,rows,r1); else delete M.cells[ref] })); snap(); recalc(); touch(); paintSel() }
  const shiftRow=x=>x;
  function autoSum(){ const R=rect(); if(R.r1!==R.r2||R.c1!==R.c2){ // plage sélectionnée : somme dans la cellule sous la plage
      const r=R.r2+1; setRaw(R.c1,r,`=SUM(${rc(R.c1,R.r1)}:${rc(R.c2,R.r2)})`); snap(); recalc(); touch(); select(R.c1,r); return }
    let r=cur.r-1; while(r>=0&&typeof eng.get(rc(cur.c,r)).v==='number') r--; const r1=r+1; if(r1<cur.r) setRaw(cur.c,cur.r,`=SUM(${rc(cur.c,r1)}:${rc(cur.c,cur.r-1)})`); else { let c=cur.c-1; while(c>=0&&typeof eng.get(rc(c,cur.r)).v==='number') c--; if(c+1<cur.c) setRaw(cur.c,cur.r,`=SUM(${rc(c+1,cur.r)}:${rc(cur.c-1,cur.r)})`) }
    snap(); recalc(); touch(); paintSel() }
  const toCSV=()=>{ let maxR=0,maxC=0; Object.keys(M.cells).forEach(k=>{ const p=pr(k); if(p&&M.cells[k].v!==undefined){ maxR=Math.max(maxR,p.r); maxC=Math.max(maxC,p.c) } }); const out=[]; for(let r=0;r<=maxR;r++){ const row=[]; for(let c=0;c<=maxC;c++){ const t=fmtV(eng.get(rc(c,r)),'g'); row.push(/[;"\n]/.test(t)?'"'+t.replace(/"/g,'""')+'"':t) } out.push(row.join(';')) } return out.join('\n') };
  function fromCSV(t){ const rows=[]; let row=[],cell='',q=false; const sep=(t.split('\n')[0].match(/;/g)||[]).length>=(t.split('\n')[0].match(/,/g)||[]).length?';':',';
    for(let i=0;i<t.length;i++){ const ch=t[i]; if(q){ if(ch==='"'&&t[i+1]==='"'){ cell+='"'; i++ } else if(ch==='"') q=false; else cell+=ch } else if(ch==='"') q=true; else if(ch===sep){ row.push(cell); cell='' } else if(ch==='\n'){ row.push(cell); rows.push(row); row=[]; cell='' } else if(ch!=='\r') cell+=ch } if(cell||row.length){ row.push(cell); rows.push(row) }
    M.cells={}; M.rows=Math.max(100,rows.length+20); rows.forEach((r,j)=>r.forEach((v,i)=>{ if(v!==''&&i<COLS) M.cells[rc(i,j)]={v} })); snap(); build(); touch() }
  root.querySelector('.oftool').addEventListener('click',e=>{ const a=e.target.closest('[data-a]')?.dataset.a; if(!a) return;
    if(a==='undo'){ if(hi>0) restore(--hi) } else if(a==='redo'){ if(hi<hist.length-1) restore(++hi) }
    else if(a==='b') fmtSel(x=>x.b=!x.b); else if(a==='i') fmtSel(x=>x.i=!x.i); else if(a==='al-l') fmtSel(x=>x.al='l'); else if(a==='al-c') fmtSel(x=>x.al='c'); else if(a==='al-r') fmtSel(x=>x.al='r');
    else if(a==='sum') autoSum(); else if(a==='sortA') sortSel(false); else if(a==='sortD') sortSel(true);
    else if(a==='chart'){ const R=rect(); if(R.c1!==R.c2||R.r1!==R.r2) M.chart=Object.assign({type:'bar'},M.chart||{},{range:rc(R.c1,R.r1)+':'+rc(R.c2,R.r2)}); else M.chart=M.chart||null; if(!M.chart){ Dlg.alert({title:'Graphique',text:'Sélectionne d\'abord une plage de cellules (avec des étiquettes et des nombres).'}); return } chartEl.hidden=false; M.chartOn=true; drawChart(); touch() }
    else if(a==='csvx'){ download(B.name()+'.csv','text/csv;charset=utf-8','﻿'+toCSV()) } else if(a==='csvo'){ pick('.csv,.txt,text/csv',t=>fromCSV(t),true) } });
  root.querySelector('.oftool').addEventListener('change',e=>{ const a=e.target.dataset.a; if(a==='fm') fmtSel(x=>{ x.fm=e.target.value==='g'?undefined:e.target.value }); else if(a==='bg') fmtSel(x=>x.bg=e.target.value); else if(a==='co') fmtSel(x=>x.co=e.target.value) });
  title.addEventListener('change',()=>{ B.rename(title.value); title.value=B.name(); ctx.setTitle(B.name()) });
  title.addEventListener('keydown',e=>{ if(e.key==='Enter'){ title.blur(); gridEl.focus() } });
  fxin.addEventListener('focus',()=>{ fxin.select() });

  // graphique
  const cvs=chartEl.querySelector('canvas'), cx=cvs.getContext('2d');
  function drawChart(){
    if(!M.chart||!M.chartOn){ chartEl.hidden=true; return } chartEl.hidden=false; chartEl.querySelector('[data-a=ctype]').value=M.chart.type; chartEl.querySelector('.ncrange').textContent=M.chart.range;
    const [a,b]=M.chart.range.split(':').map(pr); const w=cvs.clientWidth||600, h=cvs.clientHeight||220, dpr=devicePixelRatio||1; cvs.width=w*dpr; cvs.height=h*dpr; cx.setTransform(dpr,0,0,dpr,0,0); cx.clearRect(0,0,w,h);
    const get=(c,r)=>eng.get(rc(c,r)).v; const c1=Math.min(a.c,b.c),c2=Math.max(a.c,b.c),r1=Math.min(a.r,b.r),r2=Math.max(a.r,b.r);
    const labCol=typeof get(c1,r1+1)==='string'||(typeof get(c1,r1+1)!=='number'); const hdr=typeof get(c1+(labCol?1:0),r1)==='string'; const sc0=c1+(labCol?1:0), rs=r1+(hdr?1:0);
    const series=[]; for(let c=sc0;c<=c2;c++){ series.push({n:hdr?String(get(c,r1)):'Série '+(series.length+1),v:Array.from({length:r2-rs+1},(_,i)=>{ const x=get(c,rs+i); return typeof x==='number'?x:0 })}) }
    const labs=Array.from({length:r2-rs+1},(_,i)=>labCol?str(get(c1,rs+i)):String(i+1)); if(!series.length) return;
    const cs=getComputedStyle(root), fg=cs.color||'#222', mut='rgba(128,128,150,.35)'; cx.font='12px Inter,system-ui,sans-serif'; cx.fillStyle=fg;
    if(M.chart.type==='pie'){ const v=series[0].v, tot=v.reduce((x,y)=>x+Math.max(0,y),0)||1; let ang=-Math.PI/2; const R=Math.min(h/2-14,w/4); v.forEach((x,i)=>{ const d=Math.max(0,x)/tot*Math.PI*2; cx.beginPath(); cx.moveTo(w/3,h/2); cx.arc(w/3,h/2,R,ang,ang+d); cx.closePath(); cx.fillStyle=PAL[i%PAL.length]; cx.fill(); ang+=d; cx.fillRect(w*.62,16+i*20,12,12); cx.fillStyle=fg; cx.fillText(`${labs[i]} (${Math.round(x/tot*100)} %)`,w*.62+20,26+i*20) }); return }
    const L=48,Rr=16,T=16,Bm=34, pw=w-L-Rr, ph=h-T-Bm; const all=series.flatMap(s=>s.v), mx=Math.max(0,...all), mn=Math.min(0,...all), span=(mx-mn)||1;
    const Y=v=>T+ph-(v-mn)/span*ph; cx.strokeStyle=mut; cx.fillStyle=fg; cx.textAlign='right'; for(let i=0;i<=4;i++){ const v=mn+span*i/4, y=Y(v); cx.beginPath(); cx.moveTo(L,y); cx.lineTo(w-Rr,y); cx.stroke(); cx.fillText(nf.g.format(Math.round(v*100)/100),L-6,y+4) }
    cx.textAlign='center'; const n=labs.length, step=pw/n;
    labs.forEach((l,i)=>cx.fillText(l.length>9?l.slice(0,8)+'…':l,L+step*(i+.5),h-14));
    if(M.chart.type==='bar'){ const bw=step*.7/series.length; series.forEach((s,k)=>{ cx.fillStyle=PAL[k%PAL.length]; s.v.forEach((v,i)=>{ const x=L+step*i+step*.15+bw*k, y=Y(Math.max(v,0)), y0=Y(Math.min(0,Math.max(v,0)>0?0:v)); cx.beginPath(); cx.roundRect?cx.roundRect(x,Math.min(y,Y(0)),bw-2,Math.abs(Y(0)-Y(v))||1,3):cx.rect(x,Math.min(y,Y(0)),bw-2,Math.abs(Y(0)-Y(v))||1); cx.fill() }) }) }
    else series.forEach((s,k)=>{ cx.strokeStyle=PAL[k%PAL.length]; cx.lineWidth=2.5; cx.beginPath(); s.v.forEach((v,i)=>{ const x=L+step*(i+.5), y=Y(v); i?cx.lineTo(x,y):cx.moveTo(x,y) }); cx.stroke(); cx.fillStyle=PAL[k%PAL.length]; s.v.forEach((v,i)=>{ cx.beginPath(); cx.arc(L+step*(i+.5),Y(v),3.5,0,7); cx.fill() }) });
    cx.textAlign='left'; if(series.length>1||hdr){ series.forEach((s,k)=>{ cx.fillStyle=PAL[k%PAL.length]; cx.fillRect(L+k*110,2,10,10); cx.fillStyle=fg; cx.fillText(s.n.slice(0,12),L+k*110+14,11) }) } cx.lineWidth=1;
  }
  chartEl.addEventListener('click',e=>{ if(e.target.closest('[data-a=cclose]')){ M.chartOn=false; chartEl.hidden=true; touch() } });
  chartEl.addEventListener('change',e=>{ if(e.target.dataset.a==='ctype'){ M.chart.type=e.target.value; drawChart(); touch() } });
  if(M.chart&&M.chartOn) chartEl.hidden=false;
  const ro=new ResizeObserver(()=>{ if(!chartEl.hidden) drawChart() }); ro.observe(chartEl);
  build(); setTimeout(()=>gridEl.focus({preventScroll:true}),100);
  FS.on(()=>{ if(root.isConnected&&B.id&&FS.get(B.id)) title.value!==B.name()&&document.activeElement!==title&&(title.value=B.name(),ctx.setTitle(B.name())) });
  return {open(a){ if(a&&a.file&&FS.get(a.file)){ B.id=a.file; try{ const p=JSON.parse(B.raw()); M=Object.assign({v:1,cells:{},rows:100,chart:null},p&&p.cells?p:{}) }catch(e){ M={v:1,cells:{},rows:100,chart:null} } title.value=B.name(); ctx.setTitle(B.name()); hist=[]; snap(); build() } }, destroy(){ save.now(); ro.disconnect() }};
};
const budget={v:1,rows:100,chart:{type:'bar',range:'A1:C7'},chartOn:true,cells:{}};
[['Mois','Revenus','Dépenses'],['Janvier',2400,1850],['Février',2400,1720],['Mars',2550,1990],['Avril',2400,1640],['Mai',2600,2100],['Juin',2600,1880]].forEach((r,j)=>r.forEach((v,i)=>{ const x={v}; if(j===0){ x.b=true; x.bg='#e0e7ff' } else if(i>0) x.fm='eur'; budget.cells[rc(i,j)]=x }));
budget.cells.A8={v:'Total',b:true}; budget.cells.B8={v:'=SUM(B2:B7)',b:true,fm:'eur'}; budget.cells.C8={v:'=SUM(C2:C7)',b:true,fm:'eur'};
budget.cells.A9={v:'Épargne',b:true}; budget.cells.B9={v:'=B8-C8',b:true,fm:'eur',co:'#059669'}; budget.cells.A10={v:'Taux d\'épargne'}; budget.cells.B10={v:'=B9/B8',fm:'pct'};
OfficeSamples.numbro={name:'Budget 2026.numbro',parent:'documents',content:JSON.stringify(budget)};
})();

/* =============================== DATA =============================== */
(function(){
const {later,bind,download,pick}=OffKit;
const TYPES={text:'Texte',number:'Nombre',date:'Date',bool:'Oui/Non'};
const lc=s=>String(s).toLowerCase();
/* ---- mini SQL ---- */
const TK=/\s*(?:('(?:[^']|'')*')|("(?:[^"]|"")*")|(\d+\.?\d*)|([A-Za-zÀ-ÿ_][\wÀ-ÿ]*(?:\.[\wÀ-ÿ]+)?)|(<>|<=|>=|!=|[=<>(),*+\-\/%;]))/y;
function tokenize(s){ const t=[]; let p=0,m; while(p<s.length){ TK.lastIndex=p; m=TK.exec(s); if(!m){ if(/^\s*$/.test(s.slice(p))) break; throw new Error('Caractère inattendu près de « '+s.slice(p,p+10).trim()+' »') } p=TK.lastIndex;
  if(m[1]!==undefined) t.push({t:'s',v:m[1].slice(1,-1).replace(/''/g,"'")}); else if(m[2]!==undefined) t.push({t:'i',v:m[2].slice(1,-1).replace(/""/g,'"'),q:1}); else if(m[3]!==undefined) t.push({t:'n',v:+m[3]}); else if(m[4]!==undefined) t.push({t:'i',v:m[4]}); else t.push({t:'o',v:m[5]}) } return t }
const AGG=['COUNT','SUM','AVG','MIN','MAX'];
function runSQL(db,sql){
  const T=tokenize(sql.trim()); let i=0;
  const kw=w=>T[i]&&T[i].t==='i'&&!T[i].q&&T[i].v.toUpperCase()===w, op=v=>T[i]&&T[i].t==='o'&&T[i].v===v;
  const eatKw=w=>{ if(kw(w)){ i++; return true } return false }, need=w=>{ if(!eatKw(w)) throw new Error('« '+w+' » attendu'+(T[i]?' avant « '+T[i].v+' »':'')) };
  const tbl=n=>{ const t=db.tables.find(x=>lc(x.name)===lc(n)); if(!t) throw new Error('Table introuvable : '+n); return t };
  const tname=()=>{ const t=T[i++]; if(!t||t.t!=='i') throw new Error('Nom de table attendu'); return t.v };
  const ctxFor=t=>row=>name=>{ const k=t.fields.findIndex(f=>lc(f.n)===lc(name.includes('.')?name.split('.')[1]:name)); if(k<0) throw new Error('Colonne introuvable : '+name); return row[k] };
  // expressions → fonctions (get,row)
  function expr(){ return orE() }
  function orE(){ let a=andE(); while(eatKw('OR')){ const b=andE(), l=a; a=(g)=>l(g)||b(g) } return a }
  function andE(){ let a=notE(); while(eatKw('AND')){ const b=notE(), l=a; a=(g)=>l(g)&&b(g) } return a }
  function notE(){ if(eatKw('NOT')){ const a=notE(); return g=>!a(g) } return cmpE() }
  const cv=(a,b)=>{ if(a==null||b==null) return a==null&&b==null?0:NaN; if(typeof a==='number'&&typeof b==='number') return a-b; const x=lc(a),y=lc(b); return x<y?-1:x>y?1:0 };
  function cmpE(){ let a=addE();
    if(kw('IS')){ i++; const neg=eatKw('NOT'); need('NULL'); const l=a; return g=>{ const v=l(g); const n=v==null||v===''; return neg?!n:n } }
    if(eatKw('LIKE')){ const b=addE(), l=a; return g=>{ const p=String(b(g)).replace(/[.*+?^${}()|[\]\\]/g,'\\$&').replace(/%/g,'.*').replace(/_/g,'.'); return new RegExp('^'+p+'$','i').test(String(l(g)??'')) } }
    if(eatKw('IN')){ if(!op('(')) throw new Error('( attendue'); i++; const vs=[expr()]; while(op(',')){ i++; vs.push(expr()) } i++; const l=a; return g=>{ const v=l(g); return vs.some(f=>cv(v,f(g))===0) } }
    if(T[i]&&T[i].t==='o'&&['=','<>','!=','<','>','<=','>='].includes(T[i].v)){ const o=T[i++].v, b=addE(), l=a; return g=>{ const d=cv(l(g),b(g)); if(isNaN(d)) return o==='<>'||o==='!='; return o==='='?d===0:(o==='<>'||o==='!=')?d!==0:o==='<'?d<0:o==='>'?d>0:o==='<='?d<=0:d>=0 } }
    return a }
  function addE(){ let a=mulE(); while(op('+')||op('-')){ const o=T[i++].v, b=mulE(), l=a; a=g=>o==='+'?(typeof l(g)==='string'?String(l(g))+String(b(g)):+l(g)+ +b(g)):+l(g)- +b(g) } return a }
  function mulE(){ let a=prim(); while(op('*')||op('/')||op('%')){ const o=T[i++].v, b=prim(), l=a; a=g=>o==='*'?l(g)*b(g):o==='/'?l(g)/b(g):l(g)%b(g) } return a }
  function prim(){ const t=T[i++]; if(!t) throw new Error('Expression incomplète');
    if(t.t==='n') return ()=>t.v; if(t.t==='s') return ()=>t.v; if(t.t==='o'&&t.v==='-'){ const a=prim(); return g=>-a(g) }
    if(t.t==='o'&&t.v==='('){ const a=expr(); if(!op(')')) throw new Error(') attendue'); i++; return a }
    if(t.t==='i'){ const U=t.v.toUpperCase();
      if(!t.q&&op('(')){ i++; const args=[]; if(!op(')')){ args.push(op('*')?(i++,()=>1):expr()); while(op(',')){ i++; args.push(expr()) } } if(!op(')')) throw new Error(') attendue'); i++;
        if(AGG.includes(U)) return {agg:U,arg:args[0]};
        const F={UPPER:v=>String(v??'').toUpperCase(),LOWER:v=>String(v??'').toLowerCase(),LENGTH:v=>String(v??'').length,ROUND:(v,d)=>{ const p=Math.pow(10,d||0); return Math.round(v*p)/p },ABS:v=>Math.abs(v),YEAR:v=>+String(v).slice(0,4)}[U]; if(!F) throw new Error('Fonction inconnue : '+t.v); return g=>F(...args.map(a=>a(g))) }
      if(!t.q&&U==='TRUE') return ()=>true; if(!t.q&&U==='FALSE') return ()=>false; if(!t.q&&U==='NULL') return ()=>null;
      const name=t.v; return g=>g(name) }
    throw new Error('Expression invalide près de « '+t.v+' »') }
  const where=(t)=>{ if(!eatKw('WHERE')) return ()=>true; const e=expr(); return row=>!!e(ctxFor(t)(row)) };
  const coerce=(f,v)=>{ if(v===null||v===undefined||v==='') return f.t==='bool'?false:(f.t==='number'?null:''); if(f.t==='number'){ const n=+v; return isNaN(n)?null:n } if(f.t==='bool') return v===true||/^(1|oui|true|vrai)$/i.test(String(v)); return String(v) };
  if(kw('SELECT')){ i++; const distinct=eatKw('DISTINCT'); const items=[];
    do{ if(op(',')) i++; if(op('*')){ i++; items.push({star:1}); continue }
      const s=i; const e=expr(); let alias=null; if(eatKw('AS')) alias=T[i++].v; const label=alias||T.slice(s,i-(alias?2:0)).map(x=>x.v).join('').replace(/\s+/g,'');
      items.push({e,alias:label}) }while(op(','));
    need('FROM'); const t=tbl(tname()); if(T[i]&&T[i].t==='i'&&!['WHERE','GROUP','ORDER','LIMIT'].includes(T[i].v.toUpperCase())) i++;
    const wh=where(t); let gb=null; if(eatKw('GROUP')){ need('BY'); gb=[]; do{ if(op(',')) i++; gb.push(T[i++].v) }while(op(',')) }
    const ord=[]; if(eatKw('ORDER')){ need('BY'); do{ if(op(',')) i++; const n=T[i++].v; let d=false; if(eatKw('DESC')) d=true; else eatKw('ASC'); ord.push([n,d]) }while(op(',')) }
    let lim=null; if(eatKw('LIMIT')) lim=T[i++].v;
    const G=ctxFor(t); let src=t.rows.filter(wh); let cols=[], out=[];
    const hasAgg=items.some(x=>x.e&&x.e.agg);
    if(items.some(x=>x.star)){ t.fields.forEach(f=>cols.push(f.n)) } items.filter(x=>!x.star).forEach(x=>cols.push(x.alias));
    // ordre des colonnes : * d'abord s'il est en tête
    const evalItem=(x,rows)=>{ if(x.e&&x.e.agg){ const a=x.e.agg, vs=rows.map(r=>x.e.arg?x.e.arg(G(r)):1).filter(v=>v!==null&&v!==undefined&&v!==''); if(a==='COUNT') return vs.length; const ns=vs.map(Number).filter(n=>!isNaN(n)); if(a==='SUM') return ns.reduce((p,q)=>p+q,0); if(a==='AVG') return ns.length?Math.round(ns.reduce((p,q)=>p+q,0)/ns.length*100)/100:null; return ns.length?(a==='MIN'?Math.min(...ns):Math.max(...ns)):null } return rows.length?x.e(G(rows[0])):null };
    if(hasAgg||gb){ const groups=new Map(); src.forEach(r=>{ const k=gb?gb.map(c=>G(r)(c)).join('\u0001'):'*'; if(!groups.has(k)) groups.set(k,[]); groups.get(k).push(r) }); if(!gb&&!groups.size) groups.set('*',[]);
      groups.forEach(rows=>{ const o={}; items.forEach(x=>{ if(!x.star) o[x.alias]=evalItem(x,rows) }); out.push(o) }) }
    else out=src.map(r=>{ const o={}, g=G(r); items.forEach(x=>{ if(x.star) t.fields.forEach((f,k)=>o[f.n]=r[k]); else o[x.alias]=x.e(g) }); Object.defineProperty(o,'__s',{value:g}); return o });
    if(ord.length) out.sort((a,b)=>{ for(const [n,d] of ord){ const k=cols.find(c=>lc(c)===lc(n)); let x,y; if(k){ x=a[k]; y=b[k] } else { x=a.__s?a.__s(n):null; y=b.__s?b.__s(n):null } const c=cv(x,y); if(c&&!isNaN(c)) return d?-c:c } return 0 });
    if(distinct){ const seen=new Set(); out=out.filter(o=>{ const k=JSON.stringify(cols.map(c=>o[c])); if(seen.has(k)) return false; seen.add(k); return true }) }
    if(lim!==null) out=out.slice(0,+lim);
    return {cols,rows:out.map(o=>cols.map(c=>o[c])),msg:out.length+' ligne'+(out.length>1?'s':'')} }
  if(kw('INSERT')){ i++; need('INTO'); const t=tbl(tname()); let cs=t.fields.map(f=>f.n); if(op('(')){ i++; cs=[]; do{ if(op(',')) i++; cs.push(T[i++].v) }while(op(',')); i++ } need('VALUES'); let n=0;
    do{ if(op(',')) i++; if(!op('(')) throw new Error('( attendue'); i++; const vs=[]; do{ if(op(',')) i++; vs.push(expr()(()=>null)) }while(op(',')); i++; const row=t.fields.map(f=>{ const k=cs.findIndex(c=>lc(c)===lc(f.n)); return coerce(f,k>=0?vs[k]:null) }); t.rows.push(row); n++ }while(op(','));
    return {changed:true,msg:n+' enregistrement'+(n>1?'s':'')+' ajouté'+(n>1?'s':'')} }
  if(kw('DELETE')){ i++; need('FROM'); const t=tbl(tname()), wh=where(t), b=t.rows.length; t.rows=t.rows.filter(r=>!wh(r)); return {changed:true,msg:(b-t.rows.length)+' enregistrement(s) supprimé(s)'} }
  if(kw('UPDATE')){ i++; const t=tbl(tname()); need('SET'); const sets=[]; do{ if(op(',')) i++; const c=T[i++].v; if(!op('=')) throw new Error('= attendu'); i++; sets.push([c,expr()]) }while(op(',')); const wh=where(t); let n=0;
    t.rows.forEach(r=>{ if(!wh(r)) return; const g=ctxFor(t)(r); const vals=sets.map(([c,e])=>[c,e(g)]); vals.forEach(([c,v])=>{ const k=t.fields.findIndex(f=>lc(f.n)===lc(c)); if(k<0) throw new Error('Colonne introuvable : '+c); r[k]=coerce(t.fields[k],v) }); n++ }); return {changed:true,msg:n+' enregistrement(s) modifié(s)'} }
  if(kw('CREATE')){ i++; need('TABLE'); const n=tname(); if(db.tables.some(t=>lc(t.name)===lc(n))) throw new Error('Cette table existe déjà'); if(!op('(')) throw new Error('( attendue'); i++; const fields=[]; do{ if(op(',')) i++; const fn=T[i++].v; let ty=T[i++]; ty=ty?lc(ty.v):'text'; fields.push({n:fn,t:['number','int','integer','float','real'].includes(ty)?'number':ty==='date'?'date':['bool','boolean'].includes(ty)?'bool':'text'}) }while(op(',')); i++; db.tables.push({name:n,fields,rows:[]}); return {changed:true,msg:'Table « '+n+' » créée'} }
  throw new Error('Commande non reconnue. Utilise SELECT, INSERT, UPDATE, DELETE ou CREATE TABLE.');
}

AppViews.data=function(root,ctx){
  const B=bind(ctx,'data','Base de données');
  let D={v:1,tables:[{name:'Table1',fields:[{n:'Nom',t:'text'},{n:'Valeur',t:'number'}],rows:[]}],queries:[]};
  const load=()=>{ try{ const p=JSON.parse(B.raw()); if(p&&Array.isArray(p.tables)) D=Object.assign({queries:[]},p) }catch(e){} };
  load();
  let sel={k:'t',i:0}, view='sheet', rec=0, flt='', sortc=null, qres=null;
  root.innerHTML=`<div class="of dt"><div class="oftool"><input class="oftitle" aria-label="Nom de la base" spellcheck="false">
      <button class="tb" data-a="nt">${BI.i('plus')}Table</button><button class="tb" data-a="nq">${BI.i('plus')}Requête</button><i class="sep"></i><button class="tb" data-a="csvo">${BI.i('arrowUp')}CSV</button><button class="tb" data-a="csvx">${BI.i('arrowDown')}CSV</button></div>
    <div class="dtbody"><aside class="dtside"></aside><section class="dtmain"></section></div><div class="ofstat"><span class="stsel"></span><span class="stsave">Enregistré</span></div></div>`;
  const $=s=>root.querySelector(s), side=$('.dtside'), main=$('.dtmain'), title=$('.oftitle'), stat=$('.stsel'), statSave=$('.stsave');
  title.value=B.name(); ctx.setTitle(B.name());
  const save=later(()=>{ const s=JSON.stringify(D); B.last=s; B.save(s); statSave.textContent='Enregistré' },400);
  const touch=()=>{ statSave.textContent='Enregistrement…'; save() };
  const T=()=>sel.k==='t'?D.tables[sel.i]:null, Q=()=>sel.k==='q'?D.queries[sel.i]:null;
  const E=esc;
  function renderSide(){
    side.innerHTML=`<h6>Tables</h6>`+D.tables.map((t,i)=>`<button class="dti${sel.k==='t'&&sel.i===i?' on':''}" data-k="t" data-i="${i}">${BI.i('table2')} ${E(t.name)}<small>${t.rows.length}</small></button>`).join('')+`<h6>Requêtes</h6>`+(D.queries.map((q,i)=>`<button class="dti${sel.k==='q'&&sel.i===i?' on':''}" data-k="q" data-i="${i}">${BI.i('func')} ${E(q.name)}</button>`).join('')||'<p class="dtnone">Aucune requête</p>');
  }
  const inputFor=(f,v,r,c)=>f.t==='bool'?`<input type="checkbox" data-r="${r}" data-c="${c}" ${v?'checked':''}>`:`<input type="${f.t==='number'?'number':f.t==='date'?'date':'text'}" step="any" data-r="${r}" data-c="${c}" value="${E(v===null||v===undefined?'':v)}">`;
  function renderMain(){
    const t=T(), q=Q();
    if(t){
      ctx.setTitle(B.name());
      let h=`<div class="dtview"><div class="seg"><button class="${view==='sheet'?'on':''}" data-v="sheet">Feuille</button><button class="${view==='form'?'on':''}" data-v="form">Formulaire</button><button class="${view==='struct'?'on':''}" data-v="struct">Structure</button></div>`+(view==='sheet'?`<label class="fmsearch"><input data-flt placeholder="Filtrer…" value="${E(flt)}"></label>`:'')+`<span style="flex:1"></span><b class="dtname">${E(t.name)}</b></div>`;
      if(view==='sheet'){
        let idx=t.rows.map((r,k)=>k).filter(k=>!flt||t.rows[k].some(v=>lc(v??'').includes(lc(flt)))); if(sortc!==null) idx.sort((a,b)=>{ const x=t.rows[a][sortc.c], y=t.rows[b][sortc.c], d=(x==null?'':x)>(y==null?'':y)?1:(x==null?'':x)<(y==null?'':y)?-1:0; return sortc.d?-d:d });
        h+=`<div class="dtgrid"><table><thead><tr><th class="n">#</th>${t.fields.map((f,c)=>`<th data-s="${c}">${E(f.n)}<small>${TYPES[f.t]}${sortc&&sortc.c===c?' '+BI.i(sortc.d?'arrowDown':'arrowUp'):''}</small></th>`).join('')}<th class="x"></th></tr></thead><tbody>`+idx.map((k,j)=>`<tr><td class="n">${j+1}</td>${t.rows[k].map((v,c)=>`<td>${inputFor(t.fields[c],v,k,c)}</td>`).join('')}<td class="x"><button class="tb" data-del="${k}" title="Supprimer">×</button></td></tr>`).join('')+`<tr class="add"><td class="n">${BI.i('plus')}</td><td colspan="${t.fields.length+1}"><button class="dtadd" data-addrow>${BI.i('plus')} Nouvel enregistrement</button></td></tr></tbody></table></div>`;
        stat.textContent=idx.length+' / '+t.rows.length+' enregistrement(s)';
      } else if(view==='form'){
        rec=Math.max(0,Math.min(rec,t.rows.length-1)); const r=t.rows[rec];
        h+=`<div class="dtform">`+(r?`<div class="dtfnav"><button class="tb" data-nav="0">${BI.i('skipBack')}</button><button class="tb" data-nav="${rec-1}">${BI.i('chevL')}</button><span>Enregistrement ${rec+1} sur ${t.rows.length}</span><button class="tb" data-nav="${rec+1}">${BI.i('chevR')}</button><button class="tb" data-nav="${t.rows.length-1}">${BI.i('skipFwd')}</button><span style="flex:1"></span><button class="tb" data-del="${rec}">Supprimer</button></div><div class="dtfields">${t.fields.map((f,c)=>`<label><span>${E(f.n)}</span>${inputFor(f,r[c],rec,c)}</label>`).join('')}</div>`:`<p class="dtnone">Cette table est vide.</p>`)+`<button class="stbtn" data-addrow>${BI.i('plus')} Nouvel enregistrement</button></div>`;
        stat.textContent=t.rows.length?`Enregistrement ${rec+1} / ${t.rows.length}`:'Table vide';
      } else {
        h+=`<div class="dtstruct"><label class="dtl">Nom de la table<input data-tname value="${E(t.name)}"></label><table><thead><tr><th>Champ</th><th>Type</th><th></th></tr></thead><tbody>${t.fields.map((f,c)=>`<tr><td><input data-fn="${c}" value="${E(f.n)}"></td><td><select data-ft="${c}">${Object.entries(TYPES).map(([k,v])=>`<option value="${k}"${f.t===k?' selected':''}>${v}</option>`).join('')}</select></td><td><button class="tb" data-fd="${c}" ${t.fields.length<2?'disabled':''}>×</button></td></tr>`).join('')}</tbody></table>
          <div class="row"><button class="stbtn" data-addfield>${BI.i('plus')} Ajouter un champ</button><button class="stbtn dng" data-deltable>Supprimer la table</button></div></div>`;
        stat.textContent=t.fields.length+' champ(s)';
      }
      main.innerHTML=h;
    } else if(q){
      ctx.setTitle(B.name());
      main.innerHTML=`<div class="dtview"><input class="dtqn" value="${E(q.name)}" aria-label="Nom de la requête"><span style="flex:1"></span><button class="stbtn" data-qrun>${BI.i('play')} Exécuter</button><button class="tb" data-qdel title="Supprimer la requête">×</button></div>
        <textarea class="dtsql" spellcheck="false" placeholder="SELECT * FROM Table1">${E(q.sql)}</textarea><div class="dtqres"></div>`; runQ(false);
    } else { main.innerHTML='<p class="dtnone" style="padding:30px">Sélectionne une table ou une requête.</p>' }
    if(!t&&!q) stat.textContent='';
  }
  function showRes(r,box){ if(r.error){ box.innerHTML=`<div class="dterr">${BI.i('warn')} ${E(r.error)}</div>`; stat.textContent='Erreur'; return } qres=r;
    box.innerHTML=(r.cols?`<div class="dtgrid ro"><table><thead><tr>${r.cols.map(c=>`<th>${E(c)}</th>`).join('')}</tr></thead><tbody>${r.rows.map(row=>`<tr>${row.map(v=>`<td>${E(v===null||v===undefined?'':typeof v==='boolean'?(v?'Oui':'Non'):v)}</td>`).join('')}</tr>`).join('')}</tbody></table></div>`:`<div class="dtok">${BI.i('check')} ${E(r.msg)}</div>`); stat.textContent=r.msg }
  function runQ(write){ const q=Q(); if(!q) return; const box=main.querySelector('.dtqres'); const sql=main.querySelector('.dtsql').value; q.sql=sql; let r; try{ r=runSQL(D,sql) }catch(e){ r={error:e.message} } showRes(r,box); if(r.changed){ renderSide(); touch() } else if(write) touch() }
  function render(){ renderSide(); renderMain() }
  side.addEventListener('click',e=>{ const b=e.target.closest('.dti'); if(b){ sel={k:b.dataset.k,i:+b.dataset.i}; rec=0; flt=''; sortc=null; render() } });
  root.querySelector('.oftool').addEventListener('click',async e=>{ const a=e.target.closest('[data-a]')?.dataset.a; if(!a) return;
    if(a==='nt'){ const n=await Dlg.prompt({title:'Nouvelle table',text:'Nom de la table :',value:'Table'+(D.tables.length+1),ok:'Créer'}); if(n&&n.trim()){ D.tables.push({name:n.trim(),fields:[{n:'Nom',t:'text'}],rows:[]}); sel={k:'t',i:D.tables.length-1}; view='struct'; touch(); render() } }
    else if(a==='nq'){ const first=D.tables[0]; D.queries.push({name:'Requête'+(D.queries.length+1),sql:'SELECT * FROM '+(first?first.name:'Table1')}); sel={k:'q',i:D.queries.length-1}; touch(); render() }
    else if(a==='csvx'){ const t=T(); let cols,rows; if(t){ cols=t.fields.map(f=>f.n); rows=t.rows } else if(qres&&qres.cols){ cols=qres.cols; rows=qres.rows } else return; const l=v=>{ const s=v===null||v===undefined?'':String(v); return /[;"\n]/.test(s)?'"'+s.replace(/"/g,'""')+'"':s }; download((t?t.name:Q().name)+'.csv','text/csv;charset=utf-8','\ufeff'+[cols.map(l).join(';'),...rows.map(r=>r.map(l).join(';'))].join('\n')) }
    else if(a==='csvo'){ pick('.csv,.txt',txt=>{ const lines=txt.replace(/\r/g,'').split('\n').filter(x=>x.trim()); if(lines.length<1) return; const sep=lines[0].includes(';')?';':','; const split=l=>{ const o=[]; let c='',q=false; for(let i=0;i<l.length;i++){ const ch=l[i]; if(q){ if(ch==='"'&&l[i+1]==='"'){ c+='"'; i++ } else if(ch==='"') q=false; else c+=ch } else if(ch==='"') q=true; else if(ch===sep){ o.push(c); c='' } else c+=ch } o.push(c); return o };
      const hd=split(lines[0]), data=lines.slice(1).map(split); const fields=hd.map((n,c)=>({n:n||'Champ'+(c+1),t:data.length&&data.every(r=>r[c]===''||(!isNaN(+r[c].replace(',','.'))))?'number':'text'})); const nm='Import'+(D.tables.length+1);
      D.tables.push({name:nm,fields,rows:data.map(r=>fields.map((f,c)=>f.t==='number'?(r[c]===''||r[c]===undefined?null:+r[c].replace(',','.')):(r[c]??'')))}); sel={k:'t',i:D.tables.length-1}; view='sheet'; touch(); render() },true) } });
  title.addEventListener('change',()=>{ B.rename(title.value); title.value=B.name(); ctx.setTitle(B.name()) });
  main.addEventListener('click',async e=>{ const t=T(), q=Q(); const g=a=>e.target.closest('['+a+']');
    if(g('data-v')){ view=g('data-v').dataset.v; render() }
    else if(g('data-s')){ const c=+g('data-s').dataset.s; sortc=sortc&&sortc.c===c?(sortc.d?null:{c,d:true}):{c,d:false}; renderMain() }
    else if(g('data-addrow')&&t){ t.rows.push(t.fields.map(f=>f.t==='bool'?false:f.t==='number'?null:'')); if(view==='form') rec=t.rows.length-1; touch(); render(); const l=main.querySelector('.dtgrid tbody tr:nth-last-child(2) input'); if(l&&view==='sheet') l.focus() }
    else if(g('data-del')&&t){ const k=+g('data-del').dataset.del; if(t.rows[k]){ t.rows.splice(k,1); touch(); render() } }
    else if(g('data-nav')){ rec=+g('data-nav').dataset.nav; renderMain() }
    else if(g('data-addfield')&&t){ t.fields.push({n:'Champ'+(t.fields.length+1),t:'text'}); t.rows.forEach(r=>r.push('')); touch(); renderMain() }
    else if(g('data-fd')&&t){ const c=+g('data-fd').dataset.fd; if(await Dlg.confirm({title:'Supprimer le champ ?',text:'Les données de « '+t.fields[c].n+' » seront perdues.',ok:'Supprimer',danger:true})){ t.fields.splice(c,1); t.rows.forEach(r=>r.splice(c,1)); touch(); renderMain() } }
    else if(g('data-deltable')&&t){ if(await Dlg.confirm({title:'Supprimer la table ?',text:'« '+t.name+' » et ses '+t.rows.length+' enregistrements seront supprimés.',ok:'Supprimer',danger:true})){ D.tables.splice(sel.i,1); sel=D.tables.length?{k:'t',i:0}:{k:'q',i:-1}; touch(); render() } }
    else if(g('data-qrun')) runQ(true);
    else if(g('data-qdel')&&q){ D.queries.splice(sel.i,1); sel=D.tables.length?{k:'t',i:0}:{k:'q',i:-1}; touch(); render() } });
  const edit=e=>{ const t=T(); if(!t) return; const el=e.target;
    if(el.dataset.r!==undefined&&el.dataset.c!==undefined){ const r=+el.dataset.r, c=+el.dataset.c, f=t.fields[c]; t.rows[r][c]=f.t==='bool'?el.checked:f.t==='number'?(el.value===''?null:+el.value):el.value; touch(); if(e.type==='change'&&sortc!==null) renderMain() }
    else if(el.dataset.flt!==undefined){ flt=el.value; const p=el.selectionStart; renderMain(); const n=main.querySelector('[data-flt]'); if(n){ n.focus(); n.setSelectionRange(p,p) } }
    else if(el.dataset.tname!==undefined&&e.type==='change'){ t.name=el.value.trim()||t.name; touch(); render() }
    else if(el.dataset.fn!==undefined&&e.type==='change'){ t.fields[+el.dataset.fn].n=el.value.trim()||t.fields[+el.dataset.fn].n; touch() }
    else if(el.dataset.ft!==undefined&&e.type==='change'){ const c=+el.dataset.ft, ty=el.value; t.fields[c].t=ty; t.rows.forEach(r=>{ const v=r[c]; r[c]=ty==='number'?(isNaN(+v)||v===''?null:+v):ty==='bool'?!!v&&v!=='false':(v===null?'':String(v)) }); touch() } };
  main.addEventListener('input',e=>{ if(e.target.matches('[data-flt],input[data-r],input[data-c]')) edit(e); const q=Q(); if(q&&e.target.classList.contains('dtsql')){ q.sql=e.target.value; touch() } });
  main.addEventListener('change',e=>{ edit(e); const q=Q(); if(q&&e.target.classList.contains('dtqn')){ q.name=e.target.value.trim()||q.name; touch(); renderSide() } });
  main.addEventListener('keydown',e=>{ if(e.key==='Enter'&&(e.ctrlKey||e.metaKey)&&e.target.classList.contains('dtsql')){ e.preventDefault(); runQ(true) } else if(e.key==='Enter'&&e.target.matches('.dtgrid input[type=text],.dtgrid input[type=number]')){ const tr=e.target.closest('tr').nextElementSibling; const i=[...e.target.closest('tr').querySelectorAll('input')].indexOf(e.target); const n=tr&&tr.querySelectorAll('input')[i]; if(n){ e.preventDefault(); n.focus() } } });
  render();
  return {open(a){ if(a&&a.file&&FS.get(a.file)){ B.id=a.file; D={v:1,tables:[{name:'Table1',fields:[{n:'Nom',t:'text'}],rows:[]}],queries:[]}; load(); title.value=B.name(); sel={k:'t',i:0}; render() } }, destroy(){ save.now() }};
};
const cl=['Amina','Karim','Sofia','Yann','Léa','Moussa','Chloé','Ibrahim'], vi=['Abidjan','Paris','Lyon','Dakar','Marseille','Casablanca','Lille','Bamako'];
const sample={v:1,tables:[
 {name:'Clients',fields:[{n:'Nom',t:'text'},{n:'Ville',t:'text'},{n:'Âge',t:'number'},{n:'VIP',t:'bool'},{n:'Depuis',t:'date'}],rows:cl.map((n,i)=>[n,vi[i],[29,41,35,52,24,38,47,31][i],i%3===0,'202'+(i%5)+'-0'+(i%9+1)+'-1'+(i%9)])},
 {name:'Commandes',fields:[{n:'Client',t:'text'},{n:'Produit',t:'text'},{n:'Montant',t:'number'},{n:'Date',t:'date'}],rows:[['Amina','Clavier',89,'2026-01-12'],['Karim','Écran',240,'2026-01-15'],['Amina','Souris',35,'2026-02-02'],['Sofia','Casque',129,'2026-02-11'],['Yann','Écran',240,'2026-03-03'],['Karim','Webcam',75,'2026-03-09'],['Amina','Écran',240,'2026-03-20'],['Léa','Clavier',89,'2026-04-01']]}],
 queries:[{name:'Total par client',sql:'SELECT Client, COUNT(*) AS Commandes, SUM(Montant) AS Total\nFROM Commandes\nGROUP BY Client\nORDER BY Total DESC'},{name:'Clients VIP',sql:"SELECT Nom, Ville FROM Clients WHERE VIP = TRUE OR Âge > 45 ORDER BY Nom"}]};
OfficeSamples.data={name:'Clients et commandes.data',parent:'documents',content:JSON.stringify(sample)};
})();

/* =============================== SKRIB =============================== */
(function(){
const {later,bind,download,pick}=OffKit;
AppViews.skrib=function(root,ctx){
  const B=bind(ctx,'skrib','Document');
  const FONTS=['Inter','Georgia','Times New Roman','Arial','Courier New','Trebuchet MS','Verdana'];
  root.innerHTML=`<div class="of sk"><div class="oftool sktool"><input class="oftitle" aria-label="Nom du document" spellcheck="false">
    <button class="tb" data-c="undo" title="Annuler">${BI.i('undo')}</button><button class="tb" data-c="redo" title="Rétablir">${BI.i('redo')}</button><i class="sep"></i>
    <select class="ofsel" data-s="style" aria-label="Style"><option value="p">Paragraphe</option><option value="h1">Titre 1</option><option value="h2">Titre 2</option><option value="h3">Titre 3</option><option value="blockquote">Citation</option></select>
    <select class="ofsel" data-s="font" aria-label="Police">${FONTS.map(f=>`<option style="font-family:${f}">${f}</option>`).join('')}</select>
    <select class="ofsel" data-s="size" aria-label="Taille">${[1,2,3,4,5,6,7].map(n=>`<option value="${n}"${n===3?' selected':''}>${[8,10,12,14,18,24,36][n-1]} pt</option>`).join('')}</select><i class="sep"></i>
    <button class="tb" data-c="bold" title="Gras (Ctrl+B)">${BI.i('bold')}</button><button class="tb" data-c="italic" title="Italique">${BI.i('italic')}</button><button class="tb" data-c="underline" title="Souligné">${BI.i('underline')}</button><button class="tb" data-c="strikeThrough" title="Barré">${BI.i('strike')}</button>
    <label class="clr" title="Couleur du texte"><span class="t">A</span><input type="color" data-k="foreColor" value="#4f46e5"></label><label class="clr" title="Surlignage"><span style="background:#FDE68A"></span><input type="color" data-k="hiliteColor" value="#fde68a"></label><i class="sep"></i>
    <button class="tb" data-c="justifyLeft" title="Aligner à gauche">${BI.i('alignL')}</button><button class="tb" data-c="justifyCenter" title="Centrer">${BI.i('alignC')}</button><button class="tb" data-c="justifyRight" title="Aligner à droite">${BI.i('alignR')}</button><button class="tb" data-c="justifyFull" title="Justifier">${BI.i('justify')}</button><i class="sep"></i>
    <button class="tb" data-c="insertUnorderedList" title="Liste à puces">${BI.i('listUl')}</button><button class="tb" data-c="insertOrderedList" title="Liste numérotée">${BI.i('listOl')}</button><button class="tb" data-c="outdent">${BI.i('outdent')}</button><button class="tb" data-c="indent">${BI.i('indent')}</button><i class="sep"></i>
    <button class="tb" data-a="table" title="Insérer un tableau">${BI.i('table')}</button><button class="tb" data-a="img" title="Insérer une image">${BI.i('image')}</button><button class="tb" data-a="link" title="Lien">${BI.i('link')}</button><button class="tb" data-a="hr" title="Ligne">${BI.i('minus')}</button><i class="sep"></i>
    <button class="tb" data-a="find" title="Rechercher / remplacer (Ctrl+F)">${BI.i('search')}</button><button class="tb" data-a="pdf" title="Exporter en PDF">${BI.i('external')}PDF</button><button class="tb" data-a="html" title="Exporter en HTML">HTML</button></div>
    <div class="skfind" hidden><input data-f placeholder="Rechercher" spellcheck="false"><input data-r placeholder="Remplacer par" spellcheck="false"><button class="stbtn" data-a="next">Suivant</button><button class="stbtn" data-a="rep">Remplacer</button><button class="stbtn" data-a="repall">Tout</button><span class="skcnt"></span><button class="tb" data-a="fclose">×</button></div>
    <div class="skdesk"><div class="skpage" contenteditable="true" spellcheck="true" aria-label="Document" style="--z:1"></div></div>
    <div class="ofstat"><span class="stsel">0 mot</span><span class="stz"><button class="tb" data-a="zm">${BI.i('minus')}</button><span class="zv">100 %</span><button class="tb" data-a="zp">${BI.i("plus")}</button></span><span class="stsave">Enregistré</span></div></div>`;
  const $=s=>root.querySelector(s), page=$('.skpage'), title=$('.oftitle'), stat=$('.stsel'), statSave=$('.stsave'), fbar=$('.skfind');
  let zoom=1;
  const load=()=>{ const r=B.raw(); page.innerHTML=r&&r.trim()?r:'<h1>Sans titre</h1><p>Commence à écrire…</p>' };
  load(); title.value=B.name(); ctx.setTitle(B.name());
  const save=later(()=>{ const s=page.innerHTML; B.last=s; B.save(s); statSave.textContent='Enregistré' },500);
  const count=()=>{ const t=page.innerText.trim(), w=t?t.split(/\s+/).length:0; stat.textContent=`${w} mot${w>1?'s':''} · ${t.length} caractères · ${Math.max(1,Math.ceil(page.scrollHeight/1123))} page${page.scrollHeight>1123*1.02?'s':''}` };
  page.addEventListener('input',()=>{ statSave.textContent='Enregistrement…'; save(); count() }); count();
  document.execCommand('styleWithCSS',false,true);
  const keepSel=fn=>{ page.focus(); fn() };
  const sync=()=>{ try{ const b=document.queryCommandValue('formatBlock').toLowerCase().replace(/[<>]/g,''); const s=$('[data-s=style]'); s.value=['h1','h2','h3','blockquote'].includes(b)?b:'p' }catch(e){}
    ['bold','italic','underline','strikeThrough'].forEach(c=>{ const el=$(`[data-c=${c}]`); el&&el.classList.toggle('on',document.queryCommandState(c)) }) };
  document.addEventListener('selectionchange',()=>{ if(root.isConnected&&page.contains(document.getSelection().anchorNode)) sync() });
  root.querySelector('.sktool').addEventListener('mousedown',e=>{ if(e.target.closest('button')) e.preventDefault() });
  root.querySelector('.sktool').addEventListener('click',e=>{ const b=e.target.closest('button'); if(!b) return; const c=b.dataset.c, a=b.dataset.a;
    if(c){ keepSel(()=>document.execCommand(c,false,null)); sync(); touch() }
    else if(a==='table'){ const n=prompt0(); }
    else if(a==='img'){ const sel=saveSel(); pick('image/*',(d)=>{ restoreSel(sel); page.focus(); document.execCommand('insertHTML',false,`<img src="${d}" style="max-width:100%">`); touch() }) }
    else if(a==='link'){ const sel=saveSel(); Dlg.prompt({title:'Insérer un lien',text:'Adresse (https://…)',value:'https://',ok:'Insérer'}).then(u=>{ if(!u) return; restoreSel(sel); page.focus(); document.execCommand('createLink',false,u); touch() }) }
    else if(a==='hr'){ keepSel(()=>document.execCommand('insertHorizontalRule')); touch() }
    else if(a==='find'){ fbar.hidden=!fbar.hidden; if(!fbar.hidden) fbar.querySelector('[data-f]').focus() }
    else if(a==='pdf'){ exportPDF() } else if(a==='html'){ download(B.name()+'.html','text/html',docHTML()) } });
  async function prompt0(){ const sel=saveSel(); const v=await Dlg.prompt({title:'Insérer un tableau',text:'Lignes × colonnes (ex. 3x4)',value:'3x3',ok:'Insérer'}); if(!v) return; const m=/^(\d+)\s*[x×*]\s*(\d+)$/i.exec(v.trim()); if(!m) return; const r=Math.min(30,+m[1]), c=Math.min(12,+m[2]); restoreSel(sel); page.focus(); document.execCommand('insertHTML',false,`<table class="sktb"><tbody>${('<tr>'+'<td><br></td>'.repeat(c)+'</tr>').repeat(r)}</tbody></table><p><br></p>`); touch() }
  let saved=null; const saveSel=()=>{ const s=getSelection(); saved=s.rangeCount&&page.contains(s.anchorNode)?s.getRangeAt(0).cloneRange():null; return saved }; const restoreSel=r=>{ if(!r) return; const s=getSelection(); s.removeAllRanges(); s.addRange(r) };
  const touch=()=>{ statSave.textContent='Enregistrement…'; save(); count() };
  root.querySelector('.sktool').addEventListener('change',e=>{ const s=e.target.dataset.s, k=e.target.dataset.k;
    if(k){ keepSel(()=>document.execCommand(k,false,e.target.value)); touch() }
    else if(s==='style'){ keepSel(()=>document.execCommand('formatBlock',false,e.target.value==='p'?'p':e.target.value)); touch() }
    else if(s==='font'){ keepSel(()=>document.execCommand('fontName',false,e.target.value)); touch() }
    else if(s==='size'){ keepSel(()=>document.execCommand('fontSize',false,e.target.value)); touch() } });
  title.addEventListener('change',()=>{ B.rename(title.value); title.value=B.name(); ctx.setTitle(B.name()) });
  title.addEventListener('keydown',e=>{ if(e.key==='Enter'){ e.preventDefault(); page.focus() } });
  // zoom
  const setZoom=z=>{ zoom=Math.max(.5,Math.min(2,z)); page.style.setProperty('--z',zoom); $('.zv').textContent=Math.round(zoom*100)+' %' };
  root.querySelector('.ofstat').addEventListener('click',e=>{ const a=e.target.closest('[data-a]')?.dataset.a; if(a==='zm') setZoom(zoom-.1); if(a==='zp') setZoom(zoom+.1) });
  page.addEventListener('wheel',e=>{ if(e.ctrlKey){ e.preventDefault(); setZoom(zoom+(e.deltaY<0?.1:-.1)) } },{passive:false});
  // rechercher / remplacer
  let hits=[], hp=-1;
  const clearHL=()=>{ page.querySelectorAll('mark.skhit').forEach(m=>{ const p=m.parentNode; while(m.firstChild) p.insertBefore(m.firstChild,m); p.removeChild(m); p.normalize() }) };
  function find(q){ clearHL(); hits=[]; hp=-1; if(!q){ fbar.querySelector('.skcnt').textContent=''; return }
    const w=document.createTreeWalker(page,NodeFilter.SHOW_TEXT), nodes=[]; while(w.nextNode()) nodes.push(w.currentNode); const re=new RegExp(q.replace(/[.*+?^${}()|[\]\\]/g,'\\$&'),'gi');
    nodes.forEach(n=>{ const t=n.nodeValue; let m, last=0; const frag=document.createDocumentFragment(); let any=false; while((m=re.exec(t))){ any=true; frag.append(t.slice(last,m.index)); const mk=document.createElement('mark'); mk.className='skhit'; mk.textContent=m[0]; frag.append(mk); hits.push(mk); last=m.index+m[0].length; if(!m[0].length) re.lastIndex++ } if(any){ frag.append(t.slice(last)); n.replaceWith(frag) } });
    fbar.querySelector('.skcnt').textContent=hits.length+' résultat'+(hits.length>1?'s':''); step(1) }
  function step(d){ if(!hits.length) return; hits[hp]&&hits[hp].classList.remove('cur'); hp=(hp+d+hits.length)%hits.length; hits[hp].classList.add('cur'); hits[hp].scrollIntoView({block:'center'}) }
  fbar.addEventListener('input',e=>{ if(e.target.dataset.f!==undefined) find(e.target.value) });
  fbar.addEventListener('keydown',e=>{ if(e.key==='Enter'){ e.preventDefault(); step(1) } else if(e.key==='Escape'){ fbar.hidden=true; clearHL(); page.focus() } });
  fbar.addEventListener('click',e=>{ const a=e.target.closest('[data-a]')?.dataset.a; if(!a) return; const rep=fbar.querySelector('[data-r]').value;
    if(a==='next') step(1); else if(a==='fclose'){ fbar.hidden=true; clearHL() }
    else if(a==='rep'&&hits[hp]){ hits[hp].replaceWith(document.createTextNode(rep)); hits.splice(hp,1); hp--; step(1); touch() }
    else if(a==='repall'){ hits.forEach(h=>h.replaceWith(document.createTextNode(rep))); hits=[]; fbar.querySelector('.skcnt').textContent='Remplacé'; page.normalize(); touch() } });
  page.addEventListener('keydown',e=>{ if((e.ctrlKey||e.metaKey)&&e.key.toLowerCase()==='f'){ e.preventDefault(); fbar.hidden=false; fbar.querySelector('[data-f]').focus() } });
  page.addEventListener('paste',e=>{ const t=e.clipboardData.getData('text/html'); if(!t){ return } e.preventDefault(); const d=new DOMParser().parseFromString(t,'text/html'); d.querySelectorAll('script,style,meta,link').forEach(x=>x.remove()); d.querySelectorAll('*').forEach(x=>{ [...x.attributes].forEach(a=>{ if(!['href','src','colspan','rowspan'].includes(a.name)&&a.name!=='style') x.removeAttribute(a.name); if(a.name==='style') x.setAttribute('style',x.getAttribute('style').replace(/(?:^|;)\s*(?!font-weight|font-style|text-decoration|color|text-align)[^;]+/g,'')) }) }); document.execCommand('insertHTML',false,d.body.innerHTML) });
  page.addEventListener('drop',e=>{ const f=e.dataTransfer.files&&e.dataTransfer.files[0]; if(f&&f.type.startsWith('image/')){ e.preventDefault(); const r=new FileReader(); r.onload=()=>{ page.focus(); document.execCommand('insertHTML',false,`<img src="${r.result}" style="max-width:100%">`); touch() }; r.readAsDataURL(f) } });
  const docHTML=()=>`<!doctype html><html lang="fr"><meta charset="utf-8"><title>${esc(B.name())}</title><style>body{font:12pt/1.6 Inter,Georgia,serif;max-width:21cm;margin:2cm auto;color:#111}table{border-collapse:collapse}td{border:1px solid #999;padding:6px 10px}img{max-width:100%}blockquote{border-left:4px solid #a78bfa;margin-left:0;padding-left:14px;color:#555}</style><body>${page.innerHTML.replace(/<mark class="skhit[^>]*>(.*?)<\/mark>/g,'$1')}</body></html>`;
  function exportPDF(){ const w=window.open('','_blank'); if(!w){ Dlg.alert({title:'Export PDF',text:'Autorise les fenêtres pop-up pour exporter, puis choisis « Enregistrer au format PDF » dans la boîte d\'impression.'}); return } w.document.write(docHTML()+'<script>onload=()=>setTimeout(()=>print(),300)<\/script>'); w.document.close() }
  setTimeout(()=>{ page.focus({preventScroll:true}) },100);
  return {open(a){ if(a&&a.file&&FS.get(a.file)){ B.id=a.file; load(); title.value=B.name(); ctx.setTitle(B.name()); count() } }, destroy(){ save.now() }};
};
const T=`<h1>Lettre de motivation</h1><p style="color:#6b6b80">Skrib · exemple de document</p><p>Madame, Monsieur,</p><p>Je vous écris pour vous proposer ma candidature. Skrib permet de mettre en forme du <b>gras</b>, de l'<i>italique</i>, du <u>souligné</u>, des <span style="color:#4f46e5">couleurs</span> et même des <mark style="background:#fde68a">surlignages</mark>.</p><h2>Ce que Skrib sait faire</h2><ul><li>Titres, listes et citations</li><li>Tableaux et images (glisse-les dans la page)</li><li>Rechercher / remplacer avec Ctrl+F</li><li>Export en PDF ou en HTML</li></ul><table class="sktb"><tbody><tr><td><b>Outil</b></td><td><b>Rôle</b></td></tr><tr><td>Numbro</td><td>Tableur</td></tr><tr><td>WePoint</td><td>Présentations</td></tr></tbody></table><p><br></p><blockquote>Écrire, c'est déjà avancer.</blockquote><p>Cordialement,</p>`;
OfficeSamples.skrib={name:'Lettre exemple.skrib',parent:'documents',content:T};
})();
