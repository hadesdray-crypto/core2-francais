#pragma once
// Page de réglages du Core2 (en français), servie par le Core2 lui-même.
static const char PAGE_WEB[] PROGMEM = R"RAW(<!doctype html><html lang="fr"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Core2 français</title>
<style>
body{font-family:system-ui,sans-serif;background:#12142a;color:#eef;margin:0;padding:16px;max-width:560px;margin:auto}
h1{font-size:22px}h2{font-size:16px;margin:22px 0 8px;color:#aab}
.c{background:#1e2142;border-radius:12px;padding:12px;margin:8px 0}
label{display:flex;justify-content:space-between;align-items:center;gap:10px;padding:8px 0}
input[type=text],input[type=password],input[type=number]{background:#0d0f22;color:#fff;border:1px solid #445;border-radius:8px;padding:8px;width:55%}
input[type=range]{width:55%}
button{background:#4a5bd0;color:#fff;border:0;border-radius:10px;padding:12px 16px;font-size:16px;margin:4px 0;width:100%}
button.d{background:#8a3b48}
#etat{font-size:14px;color:#bcd}
select{background:#0d0f22;color:#fff;border:1px solid #445;border-radius:8px;padding:10px;width:100%;font-size:15px;margin:6px 0}
</style></head><body>
<h1>Core2 français</h1><div class="c" id="etat">Chargement…</div>
<h2>Petits programmes (modules)</h2><div class="c" id="mods"></div>
<h2>Personnage</h2><div class="c">
<label>Nom <input type="text" id="nom" maxlength="15"></label>
<button onclick="envoyer('nourrir=1')">Donner à manger</button>
<label>Gravité <input type="checkbox" id="gravite" onchange="envoyer('gravite='+(this.checked?1:0))"></label></div>
<h2>Message pour le frigo</h2><div class="c">
<label>Texte <input type="text" id="msg" maxlength="36"></label>
<button onclick="envoyer('message='+encodeURIComponent(msg.value)+'&duree=0')">Afficher le message</button>
<button class="d" onclick="envoyer('message=&duree=0')">Effacer le message</button></div>
<h2>Écran et son</h2><div class="c">
<label>Luminosité <input type="range" id="lum" min="10" max="255" onchange="envoyer('lum='+this.value)"></label>
<label>Volume <input type="range" id="vol" min="0" max="255" onchange="envoyer('vol='+this.value)"></label></div>
<h2>Météo : lieu</h2><div class="c">
<label>Latitude <input type="number" step="0.001" id="lat"></label>
<label>Longitude <input type="number" step="0.001" id="lon"></label>
<button onclick="envoyer('lat='+lat.value+'&lon='+lon.value)">Enregistrer le lieu</button></div>
<h2>Wi-Fi de la maison</h2><div class="c">
<button onclick="chercher()">1. Chercher les réseaux Wi-Fi</button>
<select id="liste" onchange="if(this.value){ssid.value=this.value;ssidModifie=true}"><option value="">(aucune recherche faite)</option></select>
<div style="font-size:13px;color:#9ab">Le Core2 ne voit que le 2,4 GHz : tous les réseaux de cette liste lui conviennent.</div>
<label>Réseau choisi <input type="text" id="ssid" oninput="ssidModifie=true"></label>
<label>Mot de passe <input type="password" id="mdp" placeholder="à taper"></label>
<button onclick="enregistrerWifi()">2. Enregistrer et redémarrer</button><div id="rwifi"></div></div>
<h2>Wi-Fi du téléphone (partage de connexion)</h2><div class="c">
<div style="font-size:13px;color:#9ab">Pour que le Core2 ait Internet hors de la maison. Le partage du téléphone doit être réglé sur 2,4 GHz.</div>
<label>Nom du partage <input type="text" id="ssid2"></label>
<label>Mot de passe <input type="password" id="mdp2" placeholder="à taper"></label>
<button onclick="enregistrerTel()">Enregistrer le Wi-Fi du téléphone</button>
<button class="d" onclick="envoyer('oublier2=1')">Oublier le Wi-Fi du téléphone</button><div id="rtel"></div></div>
<h2>Vérifier la connexion</h2><div class="c"><label>Wi-Fi direct (réseau Core2-francais) <input type="checkbox" id="direct" onchange="envoyer('direct='+(this.checked?1:0))"></label><label>Économie d'énergie Wi-Fi (plus lent à répondre) <input type="checkbox" id="eco" onchange="envoyer('eco='+(this.checked?1:0))"></label><button onclick="tester()">Vérifier la connexion</button><button onclick="chercher()">Chercher les réseaux Wi-Fi</button><div id="rtest"></div></div>
<h2>Mise à jour sans câble</h2><div class="c">
<div id="rnet" style="font-size:14px;color:#bcd">Mise à jour par Internet : pas encore vérifié.</div>
<button onclick="majInternet()">Chercher et installer la mise à jour par Internet</button>
<div style="font-size:13px;color:#9ab;margin-top:8px">Ou bien envoyer un fichier à la main :</div>
<label>Code <input type="text" id="cod" value="666666"></label><input type="file" id="fic" accept=".bin"><button onclick="maj()">Envoyer la nouvelle version</button>
<button class="d" onclick="retour()">Revenir à l'ancienne version</button><div id="rmaj"></div></div>
<script>
let ssidModifie=false,ancienSsid='';
const MODS={meteo:'Météo',wifi:'Wi-Fi',son:'Son',batterie:'Batterie',heure:'Heure et sommeil',gravite:'Gravité',messages:'Messages'};
async function envoyer(c){await fetch('/api/reglages',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:c});charger()}
async function charger(){const e=await (await fetch('/api/etat')).json();
etat.innerHTML=e.nom+' · '+e.phase+' · faim '+e.faim+' % · joie '+e.joie+' %<br>Batterie '+e.batterie+' %'+(e.charge?' (en charge)':'')+' · Wi-Fi '+(e.wifi?'oui':'non')+' · Version '+e.version+(e.reseauActif?' · réseau '+e.reseauActif:'');
mods.innerHTML='';for(const k in MODS){const l=document.createElement('label');l.innerHTML=MODS[k]+'<input type="checkbox" '+(e.modules[k]?'checked':'')+'>';l.querySelector('input').onchange=function(){envoyer(k+'='+(this.checked?1:0))};mods.appendChild(l)}
if(document.activeElement.id!='nom')nom.value=e.nom;nom.onchange=()=>envoyer('nom='+encodeURIComponent(nom.value));
gravite.checked=e.gravite;direct.checked=e.direct;eco.checked=e.eco;lum.value=e.luminosite;vol.value=e.volume;lat.value=e.lat;lon.value=e.lon;ancienSsid=e.ssid;if(document.activeElement.id!='ssid2'&&!ssid2.value)ssid2.value=e.ssid2||'';rnet.textContent=e.majDispo?'Mise à jour '+e.majDispo+' disponible.':'Aucune mise à jour en attente.';if(!ssidModifie&&document.activeElement.id!='ssid')ssid.value=e.ssid}
async function maj(){const f=fic.files[0];if(!f){rmaj.textContent='Choisis d\'abord le fichier de la nouvelle version.';return}
const d=new FormData();d.append('fichier',f);rmaj.textContent='Envoi en cours…';const r=await fetch('/api/maj?code='+encodeURIComponent(cod.value),{method:'POST',body:d});rmaj.textContent=await r.text()}
async function retour(){const r=await fetch('/api/retour?code='+encodeURIComponent(cod.value),{method:'POST'});rmaj.textContent=await r.text()}
async function tester(){rtest.textContent='Vérification…';try{const t=await (await fetch('/api/test')).json();rtest.innerHTML='Version '+t.version+'<br>Wi-Fi de la maison : '+(t.wifiMaison?'connecté ('+t.reseau+', signal '+t.signal+' dBm, adresse '+t.ip+')':'NON connecté')+'<br>Téléphones sur le Wi-Fi direct : '+t.telephonesSurWifiDirect+'<br>Ancienne version de secours : '+(t.ancienneVersionDisponible?'oui':'non')+'<br>Mémoire libre : '+t.memoireLibre+' octets'}catch(e){rtest.textContent='Pas de réponse du Core2.'}}
async function chercher(){rtest.textContent='Recherche des réseaux… (quelques secondes)';try{const l=await (await fetch('/api/reseaux')).json();liste.innerHTML='<option value="">Choisis ton réseau ('+l.length+' trouvés)</option>';l.forEach(n=>{const o=document.createElement('option');o.value=n.nom;const f=n.dbm>-55?'très bon':n.dbm>-67?'bon':n.dbm>-75?'moyen':'faible';o.textContent=n.nom+' — signal '+f+' ('+n.dbm+' dBm) — '+(n.ferme?'protégé':'ouvert')+' — canal '+n.canal+' (2,4 GHz)';liste.appendChild(o)});rtest.textContent=l.length+' réseaux trouvés : ouvre la liste « Choisis ton réseau ».'}catch(e){rtest.textContent='La recherche a échoué, réessaie.'}}
async function enregistrerWifi(){if(!ssid.value){rwifi.textContent='Choisis d\'abord un réseau dans la liste.';return}
if(!mdp.value&&ssid.value!==ancienSsid){rwifi.textContent='Tape le mot de passe de ce réseau.';return}
rwifi.textContent='Enregistrement…';try{await fetch('/api/reglages',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'ssid='+encodeURIComponent(ssid.value)+'&mdp='+encodeURIComponent(mdp.value)});rwifi.textContent='Enregistré : « '+ssid.value+' ». Le Core2 redémarre et se connecte (environ 30 secondes). Ensuite touche « Vérifier la connexion ».'}catch(e){rwifi.textContent='Pas de réponse du Core2 : réessaie.'}}
async function enregistrerTel(){if(!ssid2.value||!mdp2.value){rtel.textContent='Écris le nom du partage et son mot de passe.';return}
rtel.textContent='Enregistrement…';try{await fetch('/api/reglages',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'ssid2='+encodeURIComponent(ssid2.value)+'&mdp2='+encodeURIComponent(mdp2.value)});rtel.textContent='Enregistré. Le Core2 redémarre (environ 30 secondes).'}catch(e){rtel.textContent='Pas de réponse du Core2 : réessaie.'}}
async function majInternet(){rnet.textContent='Demande envoyée…';try{const r=await fetch('/api/majinternet?code='+encodeURIComponent(cod.value),{method:'POST'});rnet.textContent=await r.text()}catch(e){rnet.textContent='Pas de réponse du Core2.'}}
charger();setInterval(charger,10000);
</script></body></html>)RAW";
