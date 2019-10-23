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
document.getElementById('make-post-request').onclick = function req() {return makeRestRequest("POST");};


document.getElementById('client-token').addEventListener('paste', onTokenUpdate);
//document.getElementById('client-token').addEventListener('keyup', onTokenUpdate);
document.getElementById('client-token').addEventListener('change', onTokenUpdate);

document.getElementById('client-key-cert').addEventListener('paste', onTokenUpdate);
//document.getElementById('client-key-cert').addEventListener('keyup', onTokenUpdate);
document.getElementById('client-key-cert').addEventListener('change', onTokenUpdate);

///////////////////
// Implementation

function onTokenUpdate() {
  getEncodedToken();
  return false;
}

function getBase64url(source) {
  // make source base64 encoded
  encodedSource = CryptoJS.enc.Base64.stringify(source);
  // convert base64 base64url specifications
  encodedSource = encodedSource.replace(/=+$/, '');
  encodedSource = encodedSource.replace(/\+/g, '-');
  encodedSource = encodedSource.replace(/\//g, '_');

  return encodedSource;
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

function makeRestRequest(type) {
  let server_address = document.getElementById('server-address').value;
  let server_port    = document.getElementById('server-port').value;
  let doc_root       = document.getElementById('server-doc-root').value;
  let resource       = document.getElementById('resource-path').value;

  if (server_address != "" && server_port != "") {
    requestObj = new XMLHttpRequest();
    requestUrl = 'http://' + server_address + ':' + server_port + '/' + doc_root + '/' + resource;
    requestObj.open(type, requestUrl, true);

    // callback handler when positive response received
    requestObj.onreadystatechange = function () {
      if (this.readyState == XMLHttpRequest.DONE && this.status == 200) {
        document.getElementById('json-raw-output').innerHTML = this.responseText;
        let target = '#json-raw-output';
        jsonView.format(this.responseText, target);
      }
    };
    requestObj.send(null);
  }
  // prevent reloadig of page
  return false;
}
