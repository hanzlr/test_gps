const TOKEN = "szWjV74ZQEVeKoRojSZAC8s-pTA97WxV";

// ======================
// STATE
// ======================

let relayState = false;
let buzzerState = false;
let geoState = false;
let fullscreen = false;

// ======================
// MAP
// ======================

const map = L.map("map").setView(
  [-6.2, 106.8],
  18
);

L.tileLayer(
  "https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png",
  {
    attribution:"OpenStreetMap"
  }
).addTo(map);

// ======================
// MARKER
// ======================

const marker = L.marker(
  [-6.2,106.8]
).addTo(map);

marker.bindPopup("GPS TRACKER");

// ======================
// LOAD GPS
// ======================

async function loadGPS(){

  try{

    // LATITUDE

    let lat = await fetch(
      `https://blynk.cloud/external/api/get?token=${TOKEN}&V4`
    ).then(r => r.text());

    // LONGITUDE

    let lon = await fetch(
      `https://blynk.cloud/external/api/get?token=${TOKEN}&V5`
    ).then(r => r.text());

    lat = parseFloat(lat);
    lon = parseFloat(lon);

    // UPDATE MAP

    if(!isNaN(lat) && !isNaN(lon)){

      marker.setLatLng([lat,lon]);

      map.setView([lat,lon]);

      document.getElementById(
        "latText"
      ).innerText = lat.toFixed(6);

      document.getElementById(
        "lonText"
      ).innerText = lon.toFixed(6);

      // GOOGLE MAPS LINK

      document.getElementById(
        "mapsLink"
      ).href =
      `https://www.google.com/maps?q=${lat},${lon}`;
    }

    // SIGNAL

    let signal = await fetch(
      `https://blynk.cloud/external/api/get?token=${TOKEN}&V6`
    ).then(r => r.text());

    document.getElementById(
      "signalText"
    ).innerText = signal;

    // GEOFENCE

    let geo = await fetch(
      `https://blynk.cloud/external/api/get?token=${TOKEN}&V7`
    ).then(r => r.text());

    if(geo == "0" || geo == "1"){
      geoState = geo == "1";
    }

    document.getElementById(
      "geoText"
    ).innerText =
    geoState
    ? "ACTIVE"
    : "OFF";

    updateGeoButton();

    // INTERNET

    let internet = await fetch(
      `https://blynk.cloud/external/api/get?token=${TOKEN}&V8`
    ).then(r => r.text());

    document.getElementById(
      "internetText"
    ).innerText =
    internet == "1"
    ? "ONLINE"
    : "OFFLINE";

  }

  catch(err){

    console.log(err);

  }

}

// ======================
// BUTTON UI
// ======================

function updateRelayButton(){

  const btn =
  document.getElementById(
    "relayBtn"
  );

  btn.classList.remove(
    "btn-danger",
    "btn-success"
  );

  btn.classList.add(
    relayState
    ? "btn-success"
    : "btn-danger"
  );

}

function updateBuzzerButton(){

  const btn =
  document.getElementById(
    "buzzerBtn"
  );

  btn.classList.remove(
    "btn-danger",
    "btn-success"
  );

  btn.classList.add(
    buzzerState
    ? "btn-success"
    : "btn-danger"
  );

}

function updateGeoButton(){

  const btn =
  document.getElementById(
    "geoBtn"
  );

  btn.classList.remove(
    "btn-danger",
    "btn-success"
  );

  btn.classList.add(
    geoState
    ? "btn-success"
    : "btn-danger"
  );

}

// ======================
// RELAY
// ======================

function toggleRelay(){

  relayState = !relayState;

  fetch(
    `https://blynk.cloud/external/api/update?token=${TOKEN}&V2=${relayState ? 1 : 0}`
  );

  updateRelayButton();

}

// ======================
// BUZZER
// ======================

function toggleBuzzer(){

  buzzerState = !buzzerState;

  fetch(
    `https://blynk.cloud/external/api/update?token=${TOKEN}&V1=${buzzerState ? 1 : 0}`
  );

  updateBuzzerButton();

}

// ======================
// GEOFENCE
// ======================

function toggleGeo(){

  geoState = !geoState;

  updateGeoButton();

  fetch(
    `https://blynk.cloud/external/api/update?token=${TOKEN}&V0=${geoState ? 1 : 0}`
  );

}

// ======================
// TRACKING
// ======================

function tracking(){

  fetch(
    `https://blynk.cloud/external/api/update?token=${TOKEN}&V3=1`
  );

  alert(
    "TRACKING REQUEST SENT"
  );

}

// ======================
// FULLSCREEN
// ======================

function toggleFullscreen(){

  fullscreen = !fullscreen;

  if(fullscreen){

    document.body.classList.add(
      "fullscreen"
    );

  }

  else{

    document.body.classList.remove(
      "fullscreen"
    );

  }

  setTimeout(() => {

    map.invalidateSize();

  },300);

}

// ======================
// AUTO LOAD
// ======================

loadGPS();

setInterval(loadGPS,3000);

// ======================
// HIDE ATTRIBUTION
// ======================

document.querySelector(
  ".leaflet-control-attribution"
).style.display = "none";
