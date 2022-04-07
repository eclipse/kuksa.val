package kuksa_viss

import (
    "net/url"
    "log"
    "os"
    "encoding/json"
    "errors"
    "time"
    "crypto/tls"
    "crypto/x509"
    "io/ioutil"

    "github.com/gorilla/websocket"
    "github.com/google/uuid"
    "github.com/stretchr/objx"
)

type KuksaClientComm struct {
     Config *KuksaVISSClientConfig
     connSocket *websocket.Conn
}

type KuksaValRequest struct {
     RequestId string `json:"requestId"`
     Action    string `json:"action"`
     Token     string `json:"tokens,omitempty"` 
     Path      string `json:"path,omitempty"` 
     Value     string `json:"value,omitempty"`
     Metadata   string `json:"metadata,omitempty"`
}

// Function to send and receive Kuksa Requests over Websocket
func (cc *KuksaClientComm) sendAndRecv(reqJson []byte) ([]byte, error) {
    resultChannel := make(chan []byte)

    // Send the request
    go func() {
        err := cc.connSocket.WriteMessage(websocket.TextMessage, reqJson)
        if err != nil {
            log.Fatal("Sending Error: ", err)
        }
    }()

    // Receive the response
    go func() {
        _, message, err := cc.connSocket.ReadMessage()
        if err != nil {
            log.Fatal("Receive Error:", err)
        }
        resultChannel<-message
    }()

    // Wait for finishing or timeout
    select {
        case result:= <-resultChannel:
            return result, nil
        case <-time.After(3 * time.Second):
            return nil, errors.New("Communication Timeout")
    }

}

// Function to connect to kuksa.val
func (cc *KuksaClientComm) ConnectToKuksaValServerWs() error {

    if cc.Config.Insecure {
        serverUrl := url.URL{Scheme: "ws", Host: cc.Config.ServerAddress+":"+cc.Config.ServerPort}
        log.Printf("Connecting to " + serverUrl.String())

        // Connect to the Kuksa Websocket
        var err error
        cc.connSocket, _, err = websocket.DefaultDialer.Dial(serverUrl.String(), nil)
        if err != nil {
            log.Fatal("Connection error: ", err)
            return err
        }
        log.Printf(string(cc.connSocket.LocalAddr().String()))
    } else {
        serverUrl := url.URL{Scheme: "wss", Host: cc.Config.ServerAddress+":"+cc.Config.ServerPort}
        log.Printf("Connecting to " + serverUrl.String())

        // Load Client cert
        cert, err := tls.LoadX509KeyPair(cc.Config.CertsDir+"/Client.pem", cc.Config.CertsDir+"/Client.key")
        if err != nil {
            log.Fatal("Certificate Error: ", err)
        }

        // Load CA Certificate
        caCert, err := ioutil.ReadFile(cc.Config.CertsDir+"/CA.pem")
        caCertPool := x509.NewCertPool()
        caCertPool.AppendCertsFromPEM(caCert)

        tlsConfig := &tls.Config {
            Certificates: []tls.Certificate{cert},
            //RootCAs:      caCertPool,
            InsecureSkipVerify: true,
        }

        dialer := websocket.Dialer{TLSClientConfig: tlsConfig}
        cc.connSocket, _, err = dialer.Dial(serverUrl.String(), nil)
        if err != nil {
            log.Fatal("Connection error: ", err)
            return err
        }
        log.Printf(string(cc.connSocket.LocalAddr().String()))
    }
    return nil
}

// Close the Kuksa Client
func (cc *KuksaClientComm) Close() error {
    cc.connSocket.Close()
    return nil
}

// Function to get value from VSS tree
func (cc *KuksaClientComm) GetValueFromKuksaValServer(path string) (string, error) {

    req := KuksaValRequest{RequestId: uuid.New().String(), Action: "get", Path: path}
    reqJson, _ := json.Marshal(req)
    var resultJson []byte
    resultJson, _ = cc.sendAndRecv(reqJson)
    parsedResult, _ := objx.FromJSON(string(resultJson))
    if errStr:=parsedResult.Get("error").String(); errStr != "" {
        return "",errors.New(errStr)
    }
    valStr:=parsedResult.Get("data.dp.value").String() 

    return valStr, nil
}


// Function to set value from VSS tree
func (cc *KuksaClientComm) SetValueFromKuksaValServer(path string, value string) error {

    req := KuksaValRequest{RequestId: uuid.New().String(), Action: "set", Path: path, Value: value}
    reqJson, _ := json.Marshal(req)
    var resultJson []byte
    resultJson, _ = cc.sendAndRecv(reqJson)
    parsedResult, _ := objx.FromJSON(string(resultJson))
    if errStr:=parsedResult.Get("error").String(); errStr != "" {
        return errors.New(errStr)
    }

    return nil
}


// Function to authorize to kuksa.val server
func (cc *KuksaClientComm) AuthorizeKuksaValServerConn() error {

    // Get the authorization token
    tokenByteString, err := os.ReadFile(cc.Config.TokenPath)
    if err != nil {
        log.Fatal("Error reading token: ",  err)
        return err
    }

    // Create the authorization request 
    req := KuksaValRequest{RequestId: uuid.New().String(), Action: "authorize", Token: string(tokenByteString)}
    reqJson, _ := json.Marshal(req)
    var resultJson []byte
    resultJson, _ = cc.sendAndRecv(reqJson)
    parsedResult, _ := objx.FromJSON(string(resultJson))
    if errStr:=parsedResult.Get("error").String(); errStr != "" {
        return errors.New(errStr)
    }

    return nil
}

// Function to get metadata from kuksa.val server
func (cc *KuksaClientComm) GetMetadataFromKuksaValServer(path string) (string, error) {

    req := KuksaValRequest{RequestId: uuid.New().String(), Action: "getMetaData", Path: path}
    reqJson, _ := json.Marshal(req)
    var resultJson []byte
    resultJson, _ = cc.sendAndRecv(reqJson)
    parsedResult, _ := objx.FromJSON(string(resultJson))
    if errStr:=parsedResult.Get("error").String(); errStr != "" {
        return "",errors.New(errStr)
    }
    valStr, _:=parsedResult.Get("metadata").ObjxMap().JSON()

    return valStr, nil
}

// Function to update metadata at kuksa.val server
func (cc *KuksaClientComm) UpdateMetadatAtKuksaValServer(path string, metadata string) error {
   
    req := KuksaValRequest{RequestId: uuid.New().String(), Action: "updateMetaData", Path: path, Metadata: metadata}
    reqJson, _ := json.Marshal(req)
    var resultJson []byte
    resultJson, _ = cc.sendAndRecv(reqJson)
    log.Printf(string(resultJson))

    return nil

}
