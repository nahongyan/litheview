#include "demo_pages.h"

#include <array>
#include <cstddef>

namespace litheview_demo {
namespace {

constexpr char kOverviewHtml[] = R"HTML(<!doctype html><meta charset="utf-8"><title>LitheView SDK Overview</title>)HTML"
R"HTML(<style>*{box-sizing:border-box}html,body{margin:0;min-height:100%;font:14px system-ui,sans-serif;color:#202124;background:#f5f6f7}header{display:flex;align-items:center;justify-content:space-between;gap:16px;padding:16px 22px;background:#202124;color:#fff;border-bottom:3px solid #19a974}h1{margin:0;font-size:20px;letter-spacing:0}h2{margin:0 0 12px;font-size:15px;letter-spacing:0}p{line-height:1.55}main{padding:18px 22px}.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(250px,1fr));gap:12px}section{padding:15px;border:1px solid #d8dce0;border-radius:6px;background:#fff}.button{display:inline-flex;align-items:center;justify-content:center;min-height:34px;padding:0 13px;border:0;border-radius:4px;background:#1769aa;color:#fff;text-decoration:none}.status{display:inline-flex;align-items:center;gap:7px}.dot{width:9px;height:9px;border-radius:50%;background:#23b26d}.muted{color:#5f6368}</style>)HTML"
R"HTML(<header><h1>LitheView SDK</h1><span class="status"><i class="dot"></i>Runtime ready</span></header><main><div class="grid">
<section><h2>Single-view shell</h2><p>A focused native window keeps navigation, rendering and input in one clear workspace.</p></section>
<section><h2>C++ / JavaScript</h2><p>Open the dedicated communication page from More to verify structured messages in both directions.</p></section>
<section><h2>Accelerated offscreen</h2><p>The GPU Output page imports a leased shared texture and presents it without CPU readback.</p></section>
<section><h2>Web platform</h2><p>WebGL, Canvas, CSS, custom elements, dialogs, file selection, downloads, popups and IME have dedicated tests.</p></section>
<section><h2>Official website</h2><p class="muted">Open the project website in the current view.</p><a class="button" href="https://litheview.com/">Open LitheView website</a></section>
<section><h2>Runtime facts</h2><p id="facts" class="muted"></p></section></div></main><script>facts.textContent=`${navigator.userAgent}\n${innerWidth} x ${innerHeight} CSS pixels`;console.info('LitheView overview ready')</script>)HTML";

constexpr char kBridgeHtml[] = R"HTML(<!doctype html><meta charset="utf-8"><title>JS / C++ Communication</title>)HTML"
R"HTML(<style>*{box-sizing:border-box}html,body{margin:0;min-height:100%;font:14px system-ui,sans-serif;color:#202124;background:#f4f6f8}header{display:flex;align-items:center;justify-content:space-between;padding:18px 24px;background:#202124;color:#fff;border-bottom:3px solid #19a974}h1{margin:0;font-size:20px;letter-spacing:0}.ready{display:flex;align-items:center;gap:8px;color:#b8f0d3}.ready i{width:8px;height:8px;border-radius:50%;background:#23b26d}main{padding:20px 24px}.grid{display:grid;grid-template-columns:repeat(2,minmax(280px,1fr));gap:14px}section{padding:18px;border:1px solid #d5dbe2;border-radius:6px;background:#fff}h2{margin:0;font-size:16px}p{margin:8px 0 14px;color:#5f6368;line-height:1.55}button,input,textarea{font:inherit}input,textarea{width:100%;padding:0 10px;border:1px solid #9aa0a6;border-radius:4px}input{height:38px}textarea{min-height:126px;padding:10px;resize:vertical;font:13px ui-monospace,monospace}button{min-height:38px;margin-top:10px;padding:0 15px;border:0;border-radius:4px;background:#1769aa;color:#fff;font-weight:600;cursor:pointer}output{display:block;min-height:92px;margin-top:14px;padding:12px;border-left:3px solid #19a974;background:#eef1f3;font:12px ui-monospace,monospace;white-space:pre-wrap;word-break:break-word}.script,.events{grid-column:1/-1}.script-grid{display:grid;grid-template-columns:minmax(280px,1.2fr) minmax(260px,1fr);gap:14px}.script output{min-height:126px;margin:0}.events output{max-height:180px;overflow:auto;border-left-color:#1769aa}.route{display:flex;align-items:center;gap:8px;margin-bottom:14px;color:#5f6368;font-size:12px}.route strong{color:#202124}.route span{color:#1769aa}@media(max-width:760px){.grid,.script-grid{grid-template-columns:1fr}.script,.events{grid-column:auto}}</style>)HTML"
R"HTML(<header><h1>JS / C++ 通讯测试</h1><span class="ready"><i></i>Bridge ready</span></header><main><div class="grid">
<section><div class="route"><strong>JavaScript</strong><span>→</span><strong>C++</strong></div><h2>JS 发送结构化消息</h2><p>触发 C++ 的消息回调，并等待宿主确认。</p><input id="pageText" value="hello from JavaScript"><button id="sendPage">发送到 C++</button><output id="pageResult">等待发送</output></section>
<section><div class="route"><strong>C++</strong><span>→</span><strong>JavaScript</strong></div><h2>C++ 回传页面消息</h2><p>页面发起测试请求，C++ 收到后通过 Bridge 回传确认。</p><input id="requestText" value="request a reply from C++"><button id="requestHost">请求 C++ 回传</button><output id="hostResult">等待 C++ 消息</output></section>
<section class="script"><h2>执行自定义 JavaScript</h2><p>脚本交给 C++，由 SDK 执行后将返回值或异常回传到页面。</p><div class="script-grid"><div><textarea id="customScript">({ title: document.title, language: navigator.language, viewport: [innerWidth, innerHeight] })</textarea><button id="executeScript">由 C++ 执行</button></div><output id="scriptResult">等待执行</output></div></section>
<section class="events"><h2>通讯事件</h2><p>所有跨边界消息按时间顺序记录在这里。</p><output id="events">Bridge initialized</output></section>
</div></main><script>
let pendingTarget='page';let started=0;let scriptStarted=0;
const append=(direction,value)=>{const time=new Date().toLocaleTimeString();events.value+=`\n[${time}] ${direction}\n${JSON.stringify(value,null,2)}`;events.scrollTop=events.scrollHeight};
sendPage.onclick=()=>{const value={type:'page-message',text:pageText.value,time:Date.now()};pendingTarget='page';started=performance.now();window.litheview.postMessage(value);pageResult.value=`已发送\n${JSON.stringify(value,null,2)}`;append('JavaScript → C++',value)};
requestHost.onclick=()=>{const value={type:'request-cpp-reply',text:requestText.value,time:Date.now()};pendingTarget='host';started=performance.now();window.litheview.postMessage(value);hostResult.value='等待 C++ 回传...';append('JavaScript → C++（请求回传）',value)};
executeScript.onclick=()=>{window.__litheviewCustomScript=customScript.value;scriptStarted=performance.now();scriptResult.value='C++ 正在执行...';const value={type:'execute-custom-javascript'};window.litheview.postMessage(value);append('JavaScript → C++（执行脚本）',value)};
window.litheview.onmessage=event=>{if(event.data?.type==='javascript-result'){const value={roundTrip:`${(performance.now()-scriptStarted).toFixed(1)} ms`,result:event.data.result};scriptResult.value=JSON.stringify(value,null,2);append('C++ → JavaScript（执行结果）',value);return}const elapsed=started?`${(performance.now()-started).toFixed(1)} ms`:'received';const value={roundTrip:elapsed,data:event.data};if(pendingTarget==='host'){hostResult.value=JSON.stringify(value,null,2)}else{pageResult.value=JSON.stringify(value,null,2)}append('C++ → JavaScript',value)};
</script>)HTML";

constexpr char kGpuOutputHtml[] = R"HTML(<!doctype html><meta charset="utf-8"><title>Shared GPU Output</title>)HTML"
R"HTML(<style>*{box-sizing:border-box}html,body{margin:0;width:100%;height:100%;overflow:hidden;background:#101316;color:#fff;font:14px system-ui,sans-serif}canvas{position:absolute;inset:0;width:100%;height:100%}.hud{position:absolute;top:18px;left:20px;padding:12px 14px;border:1px solid #47515a;border-radius:6px;background:#171b1ecc}.hud strong{display:block;font-size:18px;letter-spacing:0}.status{color:#69e3aa}output{display:block;margin-top:6px;font:12px ui-monospace,monospace;color:#c5cbd1}</style>)HTML"
R"HTML(<canvas id="scene"></canvas><div class="hud"><strong>Accelerated offscreen output</strong><span class="status">Shared texture stream active</span><output id="stats"></output></div><script>
const c=scene,x=c.getContext('2d');let frames=0,last=performance.now();function draw(t){const d=devicePixelRatio||1,w=c.width=Math.max(1,innerWidth*d),h=c.height=Math.max(1,innerHeight*d);x.setTransform(d,0,0,d,0,0);const g=x.createLinearGradient(0,0,innerWidth,innerHeight);g.addColorStop(0,'#101316');g.addColorStop(1,'#174b42');x.fillStyle=g;x.fillRect(0,0,innerWidth,innerHeight);for(let i=0;i<48;i++){const a=t*.00045+i*.72,r=35+i*5;x.fillStyle=`hsl(${(i*17+t*.03)%360} 72% 57%)`;x.fillRect(innerWidth/2+Math.cos(a)*r-5,innerHeight/2+Math.sin(a)*r-5,10,10)}frames++;if(t-last>500){stats.value=`${Math.round(frames*1000/(t-last))} fps | ${w} x ${h} physical pixels`;frames=0;last=t}requestAnimationFrame(draw)}requestAnimationFrame(draw);
</script>)HTML";

constexpr char kWebGlHtml[] = R"HTML(<!doctype html><meta charset="utf-8"><title>WebGL 3D</title>)HTML"
R"HTML(<style>html,body{margin:0;width:100%;height:100%;overflow:hidden;background:#11151a;color:#fff;font:14px system-ui,sans-serif}canvas{width:100%;height:100%;display:block}.hud{position:absolute;left:18px;top:18px;padding:11px 13px;background:#171b1ed9;border:1px solid #47515a;border-radius:6px}.hud strong{display:block;font-size:18px;letter-spacing:0}.ok{color:#69e3aa}</style>)HTML"
R"HTML(<canvas id="glcanvas"></canvas><div class="hud"><strong>WebGL 3D pipeline</strong><span id="state" class="ok">Initializing</span></div><script>
const gl=glcanvas.getContext('webgl');
if(!gl){state.textContent='WebGL unavailable';state.className=''}else{
const vs=gl.createShader(gl.VERTEX_SHADER),fs=gl.createShader(gl.FRAGMENT_SHADER);
gl.shaderSource(vs,'attribute vec3 p;attribute vec3 c;uniform float angle;uniform float aspect;varying vec3 v;void main(){float cy=cos(angle),sy=sin(angle),cx=cos(angle*.73),sx=sin(angle*.73);vec3 q=vec3(cy*p.x+sy*p.z,p.y,-sy*p.x+cy*p.z);q=vec3(q.x,cx*q.y-sx*q.z,sx*q.y+cx*q.z);q.z-=4.;float focal=1.8;gl_Position=vec4(q.x*focal/aspect,q.y*focal,-q.z-.2,-q.z);v=c;}');
gl.shaderSource(fs,'precision mediump float;varying vec3 v;void main(){gl_FragColor=vec4(v,1.);}');
gl.compileShader(vs);gl.compileShader(fs);const pr=gl.createProgram();gl.attachShader(pr,vs);gl.attachShader(pr,fs);gl.linkProgram(pr);gl.useProgram(pr);
const verts=new Float32Array([-1,-1,1,1,0,0,1,-1,1,0,1,0,1,1,1,0,0,1,-1,1,1,1,1,0,-1,-1,-1,1,0,1,1,-1,-1,.2,.8,1,1,1,-1,1,1,1,-1,1,-1,0,1,1]);
const idx=new Uint16Array([0,1,2,0,2,3,1,5,6,1,6,2,5,4,7,5,7,6,4,0,3,4,3,7,3,2,6,3,6,7,4,5,1,4,1,0]);
const vb=gl.createBuffer();gl.bindBuffer(gl.ARRAY_BUFFER,vb);gl.bufferData(gl.ARRAY_BUFFER,verts,gl.STATIC_DRAW);const ib=gl.createBuffer();gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER,ib);gl.bufferData(gl.ELEMENT_ARRAY_BUFFER,idx,gl.STATIC_DRAW);
const p=gl.getAttribLocation(pr,'p'),c=gl.getAttribLocation(pr,'c'),angle=gl.getUniformLocation(pr,'angle'),aspect=gl.getUniformLocation(pr,'aspect');gl.enableVertexAttribArray(p);gl.enableVertexAttribArray(c);gl.vertexAttribPointer(p,3,gl.FLOAT,false,24,0);gl.vertexAttribPointer(c,3,gl.FLOAT,false,24,12);
state.textContent=`${gl.getParameter(gl.RENDERER)}`;
function frame(t){const d=devicePixelRatio||1,w=Math.max(1,Math.round(innerWidth*d)),h=Math.max(1,Math.round(innerHeight*d));if(glcanvas.width!==w||glcanvas.height!==h){glcanvas.width=w;glcanvas.height=h}gl.viewport(0,0,w,h);gl.enable(gl.DEPTH_TEST);gl.clearColor(.035,.05,.07,1);gl.clear(gl.COLOR_BUFFER_BIT|gl.DEPTH_BUFFER_BIT);gl.uniform1f(angle,t*.001);gl.uniform1f(aspect,w/h);gl.drawElements(gl.TRIANGLES,36,gl.UNSIGNED_SHORT,0);requestAnimationFrame(frame)}requestAnimationFrame(frame)}
</script>)HTML";

constexpr char kNativeApisHtml[] = R"HTML(<!doctype html><meta charset="utf-8"><title>Input and Native APIs</title>)HTML"
R"HTML(<style>*{box-sizing:border-box}html,body{margin:0;min-height:100%;font:14px system-ui,sans-serif;color:#202124;background:#f5f6f7}header{padding:16px 22px;background:#202124;color:#fff;border-bottom:3px solid #19a974}h1{margin:0;font-size:20px;letter-spacing:0}h2{margin:0 0 12px;font-size:15px;letter-spacing:0}main{padding:18px 22px}.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(250px,1fr));gap:12px}section{padding:15px;border:1px solid #d8dce0;border-radius:6px;background:#fff}button,input,textarea{font:inherit}button{min-height:34px;padding:0 13px;border:0;border-radius:4px;background:#1769aa;color:#fff;cursor:pointer}input,textarea{width:100%;padding:8px;border:1px solid #9aa0a6;border-radius:4px}.row{display:flex;flex-wrap:wrap;gap:8px}.editable{min-height:72px;padding:10px;border:1px solid #9aa0a6;border-radius:4px}</style>)HTML"
R"HTML(<header><h1>Input and Native APIs</h1></header><main><div class="grid">
<section><h2>Keyboard and IME</h2><input placeholder="Type here with an IME"><p>Content editable</p><div class="editable" contenteditable="true">Edit this text</div></section>
<section><h2>Native dialogs</h2><div class="row"><button onclick="alert('Alert from page')">Alert</button><button onclick="this.textContent=confirm('Confirm action?')?'Accepted':'Cancelled'">Confirm</button><button onclick="this.textContent=prompt('Enter a value','LitheView')||'Prompt'">Prompt</button></div></section>
<section><h2>File selection</h2><input type="file" multiple onchange="this.nextElementSibling.textContent=[...this.files].map(f=>f.name).join(', ')||'No file'"><p>No file</p><button onclick="typeof showOpenFilePicker==='function'?showOpenFilePicker().catch(()=>{}):document.querySelector('input[type=file]').click()">Open picker</button></section>
<section><h2>Popup and download</h2><div class="row"><button onclick="open('about:blank','litheview-popup','width=520,height=360')">Open popup</button><button onclick="{const a=document.createElement('a');a.href=URL.createObjectURL(new Blob(['LitheView download test\n'],{type:'text/plain'}));a.download='litheview-test.txt';a.click()}">Download file</button></div></section>
<section><h2>Pointer and focus</h2><input type="range" min="0" max="100" value="35"><textarea rows="4" placeholder="Resize, select, scroll and focus"></textarea></section>
<section><h2>Console</h2><button onclick="console.warn('Native API test event',Date.now())">Emit warning</button></section>
</div></main>)HTML";

constexpr DemoPageContent kExternal = {L"Browser", "about:blank", nullptr,
                                       false};
constexpr DemoPageContent kPages[] = {
    {L"SDK 概览", "https://litheview.local/overview", kOverviewHtml, false},
    {L"JS / C++ 通讯", "https://litheview.local/bridge", kBridgeHtml, false},
    {L"GPU 离屏输出", "https://litheview.local/gpu-output", kGpuOutputHtml,
     true},
    {L"WebGL 3D", "https://litheview.local/webgl-3d", kWebGlHtml, false},
    {L"原生 API", "https://litheview.local/native-apis", kNativeApisHtml,
     false},
};

}  // namespace

const DemoPageContent& GetDemoPageContent(DemoPage page) {
  if (page == DemoPage::kExternal) {
    return kExternal;
  }
  return kPages[static_cast<size_t>(page) -
                static_cast<size_t>(DemoPage::kOverview)];
}

}  // namespace litheview_demo
