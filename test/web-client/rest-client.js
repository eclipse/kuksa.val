var requestObj = null;
var requestUrl = '';

// JWT header
var jwtHeader = {
  "alg": "RS256",
  "typ": "JWT"
};

////////////////////////
// Startup configuration

// setup default values for server
document.getElementById('server-address').value = "localhost";
document.getElementById('server-port').value = 8090;
document.getElementById('server-doc-root').value = "vss/api/v1";

// setup event handlers
document.getElementById('make-get-request').onclick = function req() {return makeRestRequest('GET');};
document.getElementById('make-put-request').onclick = function req() {return makeRestRequest("PUT");};
document.getElementById('make-post-request').onclick = function req() {return makeRestRequest("POST");};
document.getElementById('make-auth-request').onclick = function req() {return makeAuthRequest("POST");};

document.getElementById('client-token').addEventListener('paste', onTokenUpdate);
document.getElementById('client-token').addEventListener('keyup', onTokenUpdate);
document.getElementById('client-token').addEventListener('change', onTokenUpdate);

document.getElementById('client-key-cert').addEventListener('paste', onTokenUpdate);
document.getElementById('client-key-cert').addEventListener('keyup', onTokenUpdate);
document.getElementById('client-key-cert').addEventListener('change', onTokenUpdate);

///////////////////
// Implementation

function onTokenUpdate() {
  if((document.getElementById("client-token").value != '') &&
     (document.getElementById('client-key-cert').value != '')) {
    getEncodedToken();
  }
  return false;
}

function onListResourceClick() {
  document.getElementById('resource-path').value = this.innerText;
  return false;
}

function getEncodedToken() {
  var data = document.getElementById("client-token").value;
  var sHead = JSON.stringify(jwtHeader);
  var head = KJUR.jws.JWS.readSafeJSONString(sHead);
  var sPayload = data;//newline_toDos(document.form1.jwspayload1.value);
  var sPemPrvKey = document.getElementById("client-key-cert").value;

  var jws = new KJUR.jws.JWS();
  var sResult = null;
  try {
    prv = KEYUTIL.getKey(sPemPrvKey);

    sResult = KJUR.jws.JWS.sign(head.alg, sHead, sPayload, prv);
    document.getElementById("auth-token-string").value = sResult;
  }
  catch (ex) {
    alert("Error: " + ex);
  }

  return false;
}

function addReqToLog(type, resource) {
  var newListItem = document.createElement("LI");
  newListItem.setAttribute('class', 'li-request');

  var newTypeDiv = document.createElement("div");
  newTypeDiv.setAttribute('class', 'liType');
  newTypeDiv.textContent = type;
  newListItem.appendChild(newTypeDiv);

  var newResourceDiv = document.createElement("div");
  newResourceDiv.setAttribute('class', 'liResource');
  newResourceDiv.textContent = resource;
  newResourceDiv.onclick = onListResourceClick;
  newListItem.appendChild(newResourceDiv);

  var list = document.getElementById("log-list");
  list.insertBefore(newListItem, list.childNodes[0]);

}

function addRespToLog(htmlStatus, message) {
  var type = 'SUCCESS';

  if (htmlStatus == 200) {
  } else {
    type = 'ERROR: HTML status = ' + htmlStatus;
    // if no response
    if (message == '') {
      message = 'No response, check server status';
    }
  }
  var newListItem = document.createElement("LI");
  if (htmlStatus == 200) {
    newListItem.setAttribute('class', 'li-response-ok');
  } else {
    newListItem.setAttribute('class', 'li-response-nok');
  }

  var newTypeDiv = document.createElement("div");
  newTypeDiv.setAttribute('class', 'liType');
  newTypeDiv.textContent = type;
  newListItem.appendChild(newTypeDiv);

  var newResourceDiv = document.createElement("div");
  newResourceDiv.setAttribute('class', 'liResource');
  newResourceDiv.textContent = message;
  newListItem.appendChild(newResourceDiv);

  var list = document.getElementById("log-list");
  list.insertBefore(newListItem, list.childNodes[0]);

}

function makeHttpRequest(type, requestUrl, resourcePath) {
  requestObj = new XMLHttpRequest();
  var completeUrl = requestUrl + resourcePath;
  requestObj.open(type, completeUrl, true);

  addReqToLog(type, resourcePath);

  // callback handler when positive response received
  requestObj.onreadystatechange = function () {
    if (this.readyState == XMLHttpRequest.DONE) {
      addRespToLog(this.status, this.responseText);
      if (this.status == 200) {
        let target = '#json-table';
        document.getElementById('json-table').innerHTML = '';
        jsonView.format(this.responseText, target);
      }
    }
  };
  requestObj.send(null);
}

function makeAuthRequest(type) {
  let server_type    = document.querySelector('input[name="HTTPType"]:checked').value;
  let server_address = document.getElementById('server-address').value;
  let server_port    = document.getElementById('server-port').value;
  let doc_root       = document.getElementById('server-doc-root').value;
  let resource       = document.getElementById('auth-token-string').value;

  if (server_address != "" && server_port != "") {
    requestUrl = server_type + '://' + server_address + ':' + server_port + '/' + doc_root + '/';
    makeHttpRequest(type, requestUrl, 'authorize?token=' + resource);
  }
  // prevent reloading of page
  return false;
}

function makeRestRequest(type) {
  let server_type    = document.querySelector('input[name="HTTPType"]:checked').value;
  let server_address = document.getElementById('server-address').value;
  let server_port    = document.getElementById('server-port').value;
  let doc_root       = document.getElementById('server-doc-root').value;
  let resource       = document.getElementById('resource-path').value;

  if (server_address != "" && server_port != "") {
    requestUrl = server_type + '://' + server_address + ':' + server_port + '/' + doc_root + '/';
    makeHttpRequest(type, requestUrl, resource);
  }
  // prevent reloadig of page
  return false;
}
