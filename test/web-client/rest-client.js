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
document.getElementById('make-set-request').onclick = function req() {return makeRestRequest('SET');};


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
  var stringifiedHeader = CryptoJS.enc.Utf8.parse(JSON.stringify(jwtHeader));
  var encodedHeader = getBase64url(stringifiedHeader);

  var data = document.getElementById("client-token").value;
  var encodedData = getBase64url(CryptoJS.enc.Utf8.parse(data));

  var signatureInput = encodedHeader + "." + encodedData;
  var secret = document.getElementById("client-key-cert").value;
  var signature = CryptoJS.SHA256(signatureInput, secret);
  signature = getBase64url(signature);

  // update on-screen token for user
  document.getElementById("auth-token-string").value = signatureInput + "." + signature;

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
    requestObj.onload = function () {
      if (this.readyState == 4 && this.status == 200) {
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
