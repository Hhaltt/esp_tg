const $=id=>document.getElementById(id);
function bytes(v){if(v<1024)return v+' B';if(v<1048576)return (v/1024).toFixed(1)+' KB';return (v/1048576).toFixed(2)+' MB'}
async function load(){
  try{
    const r=await fetch('/api/status',{cache:'no-store'});
    if(!r.ok)throw new Error('HTTP '+r.status);
    const d=await r.json();
    document.title=d.deviceName;
    $('deviceName').textContent=d.deviceName;
    $('networkStatus').textContent=d.linkUp?'Online':'Offline';
    $('ip').textContent=d.ip;
    $('gateway').textContent=d.gateway;
    $('speed').textContent=d.linkSpeed+' Mbps';
    $('duplex').textContent=d.fullDuplex?'Full':'Half';
    $('time').textContent=d.time;
    $('totalUptime').textContent=d.totalUptime;
    $('currentUptime').textContent=d.currentUptime;
    $('bootCount').textContent=d.bootCount;
    $('errors').textContent=d.errors;
    $('chatCount').textContent=d.chatCount;
    $('messagesReceived').textContent=d.messagesReceived;
    $('messagesSent').textContent=d.messagesSent;
    $('commandsReceived').textContent=d.commandsReceived;
    $('commandsExecuted').textContent=d.commandsExecuted;
    $('commandErrors').textContent=d.commandErrors;
    $('reminderCount').textContent=d.reminderCount;
    $('sdStatus').textContent=d.sdAvailable?'Available':'Unavailable';
    $('sdTotal').textContent=d.sdAvailable?bytes(d.sdTotal):'—';
    $('sdUsed').textContent=d.sdAvailable?bytes(d.sdUsed):'—';
  }catch(e){console.error(e);$('networkStatus').textContent='API error'}
}
$('refresh').addEventListener('click',load);
load();
setInterval(load,5000);
