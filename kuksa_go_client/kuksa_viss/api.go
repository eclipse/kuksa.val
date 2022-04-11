package kuksa_viss

import (
    "net/url"
    "log"
    "os"
    "errors"
    "crypto/tls"
    "crypto/x509"
    "io/ioutil"
    "sync"

    "github.com/gorilla/websocket"
    "github.com/google/uuid"
    "github.com/stretchr/objx"
)

type KuksaClientComm struct {
     Config *KuksaVISSClientConfig
     connSocket *websocket.Conn
     sendChannel chan KuksaValRequest
     requestState map[string]chan objx.Map
     sendAndRecvMutex sync.Mutex
}

type KuksaValRequest struct {
     Request       objx.Map
     RespChannel   chan objx.Map
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

    // Create request state handling
    cc.requestState = make(map [string] chan objx.Map)

    // Create send sub routine
    cc.sendChannel = make(chan KuksaValRequest)
    go func() {
        for {
            req := <-cc.sendChannel
            reqString, _ := req.Request.JSON()
            cc.connSocket.WriteMessage(websocket.TextMessage, []byte(reqString))

            // Store Request ID for request state handling
            cc.sendAndRecvMutex.Lock()
            cc.requestState[req.Request.Get("requestId").String()]=req.RespChannel
            cc.sendAndRecvMutex.Unlock()
        }
    }()

    // Create receive sub routine
    go func() {
        for {
            _, respString, _ := cc.connSocket.ReadMessage()
            resp, _ := objx.FromJSON(string(respString))

            // Get the correct channel to send back response
            cc.sendAndRecvMutex.Lock()
            respChannel := cc.requestState[resp.Get("requestId").String()]
            delete(cc.requestState, resp.Get("requestId").String())
            cc.sendAndRecvMutex.Unlock()

            respChannel<-resp
       }

    }()

    return nil
}

// Close the Kuksa Client
func (cc *KuksaClientComm) Close() error {
    cc.connSocket.Close()
    return nil
}


// Function to get value from VSS tree
func (cc *KuksaClientComm) GetValueFromKuksaValServer(path string) (string, error) {

    // Create new KuksaValRequest
    respChannel := make(chan objx.Map)
    req := objx.New(make (map [string] interface{}))
    req.Set("requestId", uuid.New().String())
    req.Set("action", "get")
    req.Set("path",  path)
    kuksaReq := KuksaValRequest{Request: req, RespChannel: respChannel}

    cc.sendChannel <- kuksaReq
    response := <-respChannel
    if response.Has("error") {
        errString, _ := response.Get("error").ObjxMap().JSON()
        return "", errors.New(errString)
    }
    valStr:=response.Get("data.dp.value").String() 

    return valStr, nil
}


// Function to set value from VSS tree
func (cc *KuksaClientComm) SetValueFromKuksaValServer(path string, value string) error {

    // Create new KuksaValRequest
    respChannel := make(chan objx.Map)
    req := objx.New(make (map [string] interface{}))
    req.Set("requestId", uuid.New().String())
    req.Set("action", "set")
    req.Set("path",  path)
    req.Set("value",  value)
    kuksaReq := KuksaValRequest{Request: req, RespChannel: respChannel}

    cc.sendChannel <- kuksaReq
    response := <-respChannel
    if response.Has("error") {
        errString, _ := response.Get("error").ObjxMap().JSON()
        return errors.New(errString)
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

    // Create new KuksaValRequest
    respChannel := make(chan objx.Map)
    req := objx.New(make (map [string] interface{}))
    req.Set("requestId", uuid.New().String())
    req.Set("action", "authorize")
    req.Set("tokens",  string(tokenByteString))
    kuksaReq := KuksaValRequest{Request: req, RespChannel: respChannel}

    cc.sendChannel <- kuksaReq
    response := <-respChannel
    if response.Has("error") {
        errString, _ := response.Get("error").ObjxMap().JSON()
        return errors.New(errString)
    }
    return nil
}

// Function to get metadata from kuksa.val server
func (cc *KuksaClientComm) GetMetadataFromKuksaValServer(path string) (string, error) {

    // Create new KuksaValRequest
    respChannel := make(chan objx.Map)
    req := objx.New(make (map [string] interface{}))
    req.Set("requestId", uuid.New().String())
    req.Set("action", "getMetaData")
    req.Set("path",  path)
    kuksaReq := KuksaValRequest{Request: req, RespChannel: respChannel}

    cc.sendChannel <- kuksaReq
    response := <-respChannel
    if response.Has("error") {
        errString, _ := response.Get("error").ObjxMap().JSON()
        return "", errors.New(errString)
    }
    valStr, _:=response.Get("metadata").ObjxMap().JSON() 

    return valStr, nil
}
