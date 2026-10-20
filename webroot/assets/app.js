/* AY Zygisk original WebUI. Shell requests limited to the AY command dispatcher. */
(() => {
  'use strict';
  const $ = (s) => document.querySelector(s);
  const $$ = (s) => [...document.querySelectorAll(s)];
  const route = `for d in /data/adb/modules/ayzygisk /data/adb/ksu/modules/ayzygisk /data/adb/apatch/modules/ayzygisk; do if [ -f "$d/scripts/ayctl.sh" ]; then /system/bin/sh "$d/scripts/ayctl.sh"`;
  const finish = '; exit $?; fi; done; echo "ERROR|AY controller not installed"';
  const t = {
    ar: {v2Doctor:'مركز فحص V2',doctorIntro:'فحص واقعي للإمكانيات والاعتماديات، مع توضيح العناصر غير المتاحة.',runDoctor:'فحص التوافق والاعتماديات',integrityGuard:'سلامة ملفات AY',integrityIntro:'فحص بصمات SHA-256 لعشرة ملفات أساسية من الحزمة، دون فحص التطبيقات الأخرى.',runIntegrity:'التحقق من ملفات AY',profileBackup:'النسخ الاحتياطي للإعدادات',profileIntro:'نسخ واسترجاع خيارات AY الثلاث محليًا، ولا تُرسل بيانات خارج الجهاز.',saveProfile:'نسخ الإعدادات',restoreProfile:'استرجاع الإعدادات',restoreConfirm:'هل تريد استرجاع إعدادات AY المحفوظة؟',v2Limits:'V2 منصة إدارة وتشخيص مستقلة، وليست محرك Zygote مستقلًا أو وسيلة لتجاوز فحوص الحماية.',eyebrow:'الجيل الجديد من إدارة Zygisk',heroDesc:'مركز تحكم مستقل لتشخيص Zygisk ومراقبة توافق الموديولات، بهوية AY السوداء والذهبية.',checking:'جارِ التحقق من الاتصال',live:'متصل بالنظام',demo:'وضع المعاينة – بيانات توضيحية',rootTools:'Magisk · KernelSU · APatch',bridgeLab:'واجهة Zygisk API',bridgeDesc:'اختبار Native اختياري لواجهة Zygisk API v5 دون تعديل التطبيقات أو إخفاء الروت.',bridgeRefresh:'فحص جسر Zygisk',bridgeDetails:'بيانات التوافق',bridgeNote:'التثبيت وحده لا يثبت تشغيل المحرك؛ يجب البناء بواسطة Android NDK والاختبار على جهاز.',runtimeLab:'مختبر Runtime',runtimeDesc:'فحوص جاهزية Android ومحاكاة دورة حياة إضافات AY داخل عملية معزولة على الكمبيوتر. لا يوجد حقن Zygote.',runtimeRefresh:'فحص جاهزية التشغيل',runtimeDetails:'حالة مكونات Runtime',runtimeHint:'اختبارات تحميل إضافات AY التجريبية تعمل على الكمبيوتر فقط، ولا تنفذ داخل التطبيقات.',nativeLab:'مختبر Native',nativeDesc:'فحص آمن لمكتبات Zygisk ومعمارياتها دون تنفيذها أو حقنها في أي عملية.',nativeModeLabel:'نوع الفحص',nativeStatusLabel:'حالة المكوّن',nativeRun:'ابدأ فحص المكتبات',nativeResults:'نتائج فحص ELF',nativeDisclaimer:'نتائج الفحص التحضيري لا تضمن نجاح تشغيل الموديول؛ هذه النسخة ليست محرك Zygote مستقلًا.',overview:'نظرة عامة',manager:'مدير الروت',detected:'يُكتشف تلقائيًا',zygiskBackend:'محرك Zygisk المكتشف',backendHint:'كشف تثبيت، وليس إثبات تشغيل',androidVersion:'إصدار أندرويد',availableMemory:'الذاكرة المتاحة',dashboard:'لوحة التحكم',modules:'الموديولات',diagnostics:'التشخيص',settings:'الإعدادات',system:'SYSTEM STATUS',healthMonitor:'مراقبة صحة النظام',activity:'ACTIVITY',quickActions:'إجراءات سريعة',healthLabel:'حالة المراقبة',loading:'جارِ تحميل البيانات...',healthDesc:'التشخيص يراقب حالة الموديولات فقط، ولا يغيّر محرك Zygisk.',scanNow:'فحص الموديولات الآن ↗',generateReport:'إنشاء تقرير تشخيص',reportSubtitle:'احفظ ملخصًا عن الجهاز والموديولات',moduleInspector:'فاحص الموديولات',moduleSubtitle:'استعراض الإضافات وحالات التشغيل',configMonitor:'إعدادات المراقبة',configSubtitle:'التحكم في تسجيل الأحداث والفحص',important:'مهم',noEngine:'هذه نسخة تحكم وتشخيص مستقلة. لا تحتوي بعد على محرك Native لحقن Zygote ولا تتجاوز فحوصات أمان التطبيقات.',installedModules:'الموديولات المثبتة',rescan:'إعادة الفحص',modulesDesc:'فحص للقراءة فقط. لن يتم تفعيل أو تعطيل أي موديول آخر.',modulesFound:'الموديولات المكتشفة',healthReport:'تقرير حالة الجهاز',reportPrivacy:'التقرير لا يجمع قائمة التطبيقات أو كلمات المرور أو سجلات استخدامك.',createReport:'إنشاء التقرير الآن',copyReport:'نسخ التقرير',eventHistory:'سجل الأحداث',refreshLogs:'تحديث',reportOutput:'محتوى التقرير',monitorSettings:'إعدادات المراقبة',monitorToggle:'تشغيل مراقبة AY',monitorExplain:'السماح بفحص خفيف عند الإقلاع من أدوات AY فقط',bootToggle:'فحص عند الإقلاع',bootExplain:'كتابة تقرير واحد بعد بدء الجهاز، دون تشغيل خدمة مستمرة',logToggle:'تسجيل أحداث AY',logExplain:'سجل محلي محدود الحجم لأحداث وحدة AY فقط',restartNote:'يتم حفظ الخيارات فورًا. خيارات فحص الإقلاع تسري عند الإقلاع التالي.',monitorOn:'مراقبة AY مفعلة',monitorOff:'مراقبة AY متوقفة',monitorDemo:'عرض تصميم تجريبي',noModules:'لا توجد موديولات أخرى مكتشفة.',notDetected:'غير مكتشف',unknown:'غير معروف',uptime:'مدة التشغيل',minutes:'دقيقة',enabled:'نشط',disabled:'معطّل',removing:'قيد الإزالة',zygiskLib:'Zygisk library',saved:'تم حفظ الإعداد',unavailable:'تحتاج إلى واجهة مدير روت تدعم تنفيذ الأوامر',reported:'تم إنشاء تقرير التشخيص',copied:'تم نسخ التقرير',nothingCopy:'أنشئ التقرير أولًا',scanComplete:'اكتمل فحص الموديولات',backendMissing:'محرك مستقل غير مكتشف',backendInstalled:'تم اكتشاف تثبيت محرك',error:'تعذّر الاتصال بالوحدة'},
    en: {v2Doctor:'V2 Doctor',doctorIntro:'Honest capability and prerequisite audit, never simulated runtime success.',runDoctor:'Check compatibility',integrityGuard:'File integrity guard',integrityIntro:'SHA-256 check of 10 bundled AY project files, not third-party apps.',runIntegrity:'Verify AY files',profileBackup:'Settings backup',profileIntro:'Save and restore only three AY options locally; no data leaves the device.',saveProfile:'Back up settings',restoreProfile:'Restore settings',restoreConfirm:'Restore locally backed-up AY settings?',v2Limits:'V2 is an independent diagnostics controller, not a standalone Zygote engine or security bypass.',eyebrow:'THE NEXT LEVEL OF ZYGISK MANAGEMENT',heroDesc:'An independent Zygisk observability and module compatibility console in signature black and gold.',checking:'Checking connection',live:'Connected to device',demo:'Preview mode – illustrative data',rootTools:'Magisk · KernelSU · APatch',bridgeLab:'Zygisk API Bridge',bridgeDesc:'Optional native API v5 smoke test, without modifying apps or hiding root.',bridgeRefresh:'Check Zygisk bridge',bridgeDetails:'Compatibility details',bridgeNote:'Installation alone does not verify runtime callbacks. Android NDK build and on-device validation are required.',runtimeLab:'Runtime Lab',runtimeDesc:'Android readiness checks and an isolated desktop-only AY plugin lifecycle test. No Zygote injection.',runtimeRefresh:'Check runtime readiness',runtimeDetails:'Runtime component status',runtimeHint:'Desktop plugin tests use AY fixtures only and do not execute inside Android apps.',nativeLab:'Native Lab',nativeDesc:'Safely preflight Zygisk libraries and ABIs without loading or injecting them.',nativeModeLabel:'Inspection mode',nativeStatusLabel:'Component status',nativeRun:'Scan libraries',nativeResults:'ELF scan results',nativeDisclaimer:'Preflight is not proof of runtime compatibility; this build is not a standalone Zygote engine.',overview:'Overview',manager:'Root manager',detected:'Automatically detected',zygiskBackend:'Detected Zygisk engine',backendHint:'Installed marker, not runtime proof',androidVersion:'Android version',availableMemory:'Available memory',dashboard:'Dashboard',modules:'Modules',diagnostics:'Diagnostics',settings:'Settings',system:'SYSTEM STATUS',healthMonitor:'System health monitor',activity:'ACTIVITY',quickActions:'Quick actions',healthLabel:'Monitor status',loading:'Loading diagnostics...',healthDesc:'Read-only module monitoring; your Zygisk engine is never changed.',scanNow:'Scan modules now ↗',generateReport:'Generate diagnostics',reportSubtitle:'Save a local device and modules summary',moduleInspector:'Module inspector',moduleSubtitle:'Explore installed Zygisk add-ons',configMonitor:'Monitoring options',configSubtitle:'Control events and startup scan',important:'IMPORTANT',noEngine:'This is an independent monitoring build. Native Zygote injection and security-integrity bypass are not implemented.',installedModules:'Installed modules',rescan:'Scan again',modulesDesc:'Read-only audit. No other module will be enabled or disabled.',modulesFound:'Modules found',healthReport:'Device health report',reportPrivacy:'Reports never collect installed apps, passwords or your usage history.',createReport:'Generate report',copyReport:'Copy report',eventHistory:'Event history',refreshLogs:'Refresh',reportOutput:'Report output',monitorSettings:'Monitor settings',monitorToggle:'AY monitoring',monitorExplain:'Allow a lightweight AY-only scan at startup',bootToggle:'Boot scan',bootExplain:'Write one report after startup; no continuous daemon',logToggle:'AY event logging',logExplain:'Locally capped events generated by AY only',restartNote:'Settings are saved immediately. Boot scan settings apply on next boot.',monitorOn:'AY monitor enabled',monitorOff:'AY monitor disabled',monitorDemo:'Illustrative preview',noModules:'No other modules detected.',notDetected:'Not detected',unknown:'Unknown',uptime:'Uptime',minutes:'min',enabled:'Active',disabled:'Disabled',removing:'Removing',zygiskLib:'Zygisk library',saved:'Setting saved',unavailable:'Requires root-manager WebUI command bridge',reported:'Diagnostic report saved',copied:'Report copied',nothingCopy:'Generate a report first',scanComplete:'Module scan finished',backendMissing:'No standalone engine detected',backendInstalled:'Engine install marker found',error:'Failed to contact module'}
  };
  let preferredLanguage = 'ar';
  try { preferredLanguage = localStorage.getItem('ay-lang') || 'ar'; } catch (_) { /* file preview */ }
  let lang = preferredLanguage === 'en' ? 'en' : 'ar';
  let live = false, latestReport = '', apiStatus = {}, busy = false;
  const bridge = window.ksu || window.apatch || null;
  const message = (key) => t[lang][key] || key;
  function toast(msg) { const el = $('#toast'); el.textContent = msg; el.classList.add('show'); clearTimeout(toast.timer); toast.timer = setTimeout(() => el.classList.remove('show'), 2900); }
  function translate() {
    document.documentElement.dir = lang === 'ar' ? 'rtl' : 'ltr';
    document.documentElement.lang = lang;
    $$('[data-t]').forEach(el => {const value = t[lang][el.dataset.t]; if(value) el.textContent = value;});
    $('#language').textContent = lang === 'ar' ? 'EN' : 'عربي';
    $('#connectionText').textContent = live ? message('live') : message('demo');
    $('#timeLabel').textContent = new Date().toLocaleString(lang === 'ar' ? 'ar-EG' : 'en-US', {year:'numeric',month:'short',day:'numeric'});
    applyStatus(apiStatus);
  }
  function page(p) { $$('.tab').forEach(x => x.classList.toggle('active', x.dataset.page === p)); $$('.page').forEach(x => x.classList.toggle('active', x.id === 'page-' + p)); if(p === 'modules') loadModules(); if(p === 'diagnostics') loadLogs(); if(p === 'native') loadNative(); if(p === 'runtime') loadRuntime(); if(p === 'bridge') loadBridge(); if(p === 'doctor') loadDoctor(); }
  async function execCommand(args) {
    if (!bridge || typeof bridge.exec !== 'function') throw new Error('WebUI bridge unavailable');
    // Only compile a fixed, validated command list; never incorporate user shell text.
    if (!/^status$|^scan$|^native$|^runtime$|^bridge$|^doctor$|^verify$|^backup-settings$|^restore-settings$|^config$|^report$|^logs$|^set AY_(MONITOR|LOGGING|BOOT_SCAN) [01]$/.test(args)) throw new Error('Invalid command');
    const cmd = route + ' ' + args + finish;
    return new Promise((resolve, reject) => {
      const cb = '__aycb_' + Date.now() + '_' + Math.floor(Math.random()*1e6);
      let ended = false;
      const end = (err, val) => { if(ended) return; ended = true; clearTimeout(timer); delete window[cb]; if(err) reject(err); else resolve(String(val ?? '')); };
      window[cb] = (code,stdout,stderr) => { if(Number(code) !== 0) end(new Error(String(stderr || code))); else end(null,stdout); };
      const timer = setTimeout(() => end(new Error('Bridge timed out')), 10000);
      try {
        const result = bridge.exec(cmd, cb);
        if(typeof result === 'string') end(null,result);
        else if(result && typeof result.then === 'function') result.then(res => end(null, typeof res === 'string' ? res : res?.stdout)).catch(e => end(e));
        else if(result && typeof result === 'object' && 'stdout' in result) { if(result.errno) end(new Error(result.stderr)); else end(null,result.stdout); }
      } catch (e) { end(e); }
    });
  }
  const parsed = (output,prefix) => output.split(/\r?\n/).filter(line => line.startsWith(prefix+'|')).map(line => line.split('|').slice(1));
  function applyStatus(s={}) {
    $('#managerValue').textContent=s.manager || (live?message('unknown'):'—');
    $('#engineValue').textContent=s.backend === 'not-detected' ? message('notDetected') : s.backend || '—';
    $('#androidValue').textContent=s.android || '—';
    $('#abiValue').textContent = s.architecture || '—';
    $('#memoryValue').textContent = s.memory_mb ? s.memory_mb + ' MB' : '—';
    $('#uptimeValue').textContent = s.uptime ? message('uptime') + ': ' + Math.round(Number(s.uptime)/60) + ' '+message('minutes') : '—';
    const monitor = s.monitor === '1';
    $('#healthTitle').textContent = !live?message('monitorDemo') : monitor?message('monitorOn'):message('monitorOff');
    $('#healthScore').textContent= !live?'—':monitor?'ON':'OFF';
    $('#connectionText').textContent=live?message('live'):message('demo');
  }
  function defaultPreview() { apiStatus = {manager:'PREVIEW',backend:'not-detected',android:'—',architecture:'—',memory_mb:'',uptime:'',monitor:'0'}; applyStatus(apiStatus); $('#moduleList').innerHTML = ''; const n=document.createElement('div'); n.className='empty'; n.textContent=message('unavailable'); $('#moduleList').appendChild(n); }
  async function loadStatus() {
    if(!bridge) { live=false; defaultPreview(); return; }
    try {
      const output = await execCommand('status');
      if(!output.includes('STATUS|manager|')) throw new Error('Missing output');
      apiStatus = Object.fromEntries(parsed(output,'STATUS').map(([k,...v])=>[k,v.join('|')]));
      live = true; applyStatus(apiStatus);
      await loadConfig();
    } catch(e) { live=false; defaultPreview(); console.warn('AY bridge:',e); }
  }
  async function loadModules() {
    const container=$('#moduleList'); container.textContent='';
    if(!live){ const p=document.createElement('div');p.className='empty';p.textContent=message('unavailable');container.appendChild(p);return; }
    try {
      const output=await execCommand('scan'), rows=parsed(output,'MODULE'), warnings=parsed(output,'WARNING');
      $('#moduleCount').textContent=rows.length;
      $('#warnings').textContent='';
      warnings.forEach(w=>{const p=document.createElement('div');p.className='warning';p.textContent='⚠ '+w.join('|');$('#warnings').appendChild(p);});
      if(!rows.length){const p=document.createElement('div');p.className='empty';p.textContent=message('noModules');container.appendChild(p);}
      for(const [id,name,status,zygisk] of rows){
        const row=document.createElement('div');row.className='module-row';
        const icon=document.createElement('div');icon.className='mod-icon';icon.textContent=zygisk==='yes'?'ϟ':'◈';
        const info=document.createElement('div');info.className='mod-description';const label=document.createElement('b');label.textContent=name;const detail=document.createElement('small');detail.textContent=id+(zygisk==='yes'?' · '+message('zygiskLib'):'');info.append(label,detail);
        const badge=document.createElement('span');badge.className='mod-status '+(status==='disabled'?'disabled':status==='removing'?'removing':'');badge.textContent=message(status);
        row.append(icon,info,badge);container.appendChild(row);
      }
    } catch(e){ toast(message('error')); }
  }
  async function loadConfig(){ if(!live) return; try {const out=await execCommand('config');const cfg=Object.fromEntries(parsed(out,'CONFIG').map(([k,v])=>[k,v]));$('#setMonitor').checked=cfg.AY_MONITOR==='1';$('#setBoot').checked=cfg.AY_BOOT_SCAN==='1';$('#setLogging').checked=cfg.AY_LOGGING==='1';} catch(e){ console.warn('config read',e); } }
  async function saveConfig(k,v){ if(!live){toast(message('unavailable'));await loadConfig();return;}try{const result=await execCommand(`set ${k} ${v}`);if(!result.startsWith('OK|')) throw new Error(result);toast(message('saved'));await loadStatus();}catch(e){toast(message('error'));await loadConfig();} }
  async function report(){ if(!live){toast(message('unavailable'));return;}if(busy)return;busy=true;try{const out=await execCommand('report');latestReport=out;$('#reportOutput').textContent=out;toast(message('reported'));page('diagnostics');}catch(e){toast(message('error'));}finally{busy=false;} }
  async function loadLogs(){ if(!live)return;try {$('#logOutput').textContent=await execCommand('logs')||'No events recorded.';}catch(e){$('#logOutput').textContent=String(e.message);} }
  async function loadNative() {
    const result = $('#nativeOutput');
    if (!live) {
      result.textContent=message('unavailable');
      $('#nativeState').textContent=message('notDetected');
      $('#nativeMode').textContent='—';
      return;
    }
    try {
      result.textContent=message('loading');
      const data=await execCommand('native');
      const metadata=Object.fromEntries(parsed(data,'NATIVE').map(([k,...v])=>[k,v.join('|')]));
      $('#nativeState').textContent=metadata.state||message('unknown');
      $('#nativeMode').textContent=metadata.mode||'—';
      // textContent is used because diagnostic strings may contain module names.
      result.textContent=data||'No Zygisk libraries found.';
    } catch(e) { result.textContent=message('error')+': '+e.message; }
  }

  async function loadRuntime() {
    const output = $('#runtimeOutput');
    if (!live) { output.textContent=message('unavailable'); return; }
    try {
      output.textContent=message('loading');
      const data=await execCommand('runtime');
      output.textContent=data || 'No runtime information';
      const meta=Object.fromEntries(parsed(data,'RUNTIME').map(([k,...v])=>[k,v.join('|')]));
      $('#runtimeEngineState').textContent=meta.android_zygote_hook||'—';
      $('#runtimeAbi').textContent=meta.cpu_abi||'—';
      $('#runtimeInspector').textContent=meta.android_inspector||'—';
    } catch(e) { output.textContent=message('error')+': '+e.message; }
  }
  async function loadBridge() {
    const output = $('#bridgeOutput');
    if (!live) {output.textContent = message('unavailable'); return;}
    try {
      output.textContent=message('loading');
      const data=await execCommand('bridge');
      output.textContent=data || 'No bridge data available';
      const meta=Object.fromEntries(parsed(data,'BRIDGE').map(([k,...v])=>[k,v.join('|')]));
      $('#bridgeBuild').textContent=meta.build||'—';
      $('#bridgeBackend').textContent=meta.backend_marker||'—';
      $('#bridgeCallbacks').textContent=meta.live_callbacks||'—';
    } catch(e) {output.textContent=message('error')+': '+e.message;}
  }
  async function loadDoctor() {
    const result=$('#doctorOutput');
    if(!live){result.textContent=message('unavailable');return;}
    try { result.textContent=message('loading'); result.textContent=await execCommand('doctor'); }
    catch(e){result.textContent=message('error')+': '+e.message;}
  }
  async function checkIntegrity(){
    const result=$('#integrityOutput');
    if(!live){result.textContent=message('unavailable');return;}
    try {result.textContent=message('loading');result.textContent=await execCommand('verify');}
    catch(e){result.textContent=message('error')+': '+e.message;}
  }
  async function profileAction(verb){
    if(!live){toast(message('unavailable'));return;}
    if(verb==='restore-settings' && !window.confirm(message('restoreConfirm')))return;
    try{const result=await execCommand(verb); if(!result.startsWith('BACKUP|'))throw new Error(result);
      toast(message('saved'));await loadConfig();}catch(e){toast(message('error'));}
  }
  $('#runDoctor').addEventListener('click',loadDoctor);
  $('#runIntegrity').addEventListener('click',checkIntegrity);
  $('#saveProfile').addEventListener('click',()=>profileAction('backup-settings'));
  $('#restoreProfile').addEventListener('click',()=>profileAction('restore-settings'));
  $$('.tab').forEach(el=>el.addEventListener('click',()=>page(el.dataset.page)));
  $('#viewModules').addEventListener('click',()=>page('modules'));
  $('#openSettings').addEventListener('click',()=>page('settings'));
  $('#generateReport').addEventListener('click',report);$('#createReport').addEventListener('click',report);
  $('#copyReport').addEventListener('click',async()=>{if(!latestReport){toast(message('nothingCopy'));return;}try{await navigator.clipboard.writeText(latestReport);toast(message('copied'));}catch(e){toast(message('error'));}});
  $('#runScan').addEventListener('click',()=>page('modules'));
  $('#refreshModules').addEventListener('click',loadModules);
  $('#loadLogs').addEventListener('click',loadLogs);
  $('#runNative').addEventListener('click',loadNative);
  $('#runRuntime').addEventListener('click',loadRuntime);
  $('#runBridge').addEventListener('click',loadBridge);
  $('#refresh').addEventListener('click',loadStatus);
  $('#language').addEventListener('click',()=>{lang=lang==='ar'?'en':'ar';try{localStorage.setItem('ay-lang',lang);}catch(_){} translate();});
  [['#setMonitor','AY_MONITOR'],['#setBoot','AY_BOOT_SCAN'],['#setLogging','AY_LOGGING']].forEach(([selector,key])=>$(selector).addEventListener('change',e=>saveConfig(key,e.target.checked?'1':'0')));
  translate();loadStatus();
})();
