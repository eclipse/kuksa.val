package kuksa_viss

import (
    "log"
    "os"

    "github.com/google/uuid"
    "github.com/stretchr/objx"
)

// Function to connect to kuksa.val
func (cc *KuksaClientComm) ConnectToKuksaValServerWs() error {
    err := cc.startCommunication()
    return err
}

// Function to get value from VSS tree
func (cc *KuksaClientComm) GetValueFromKuksaValServer(path string) (string, error) {

    // Create new KuksaValRequest
    req := objx.New(make (map [string] interface{}))
    req.Set("requestId", uuid.New().String())
    req.Set("action", "get")
    req.Set("path",  path)

    resp, err := cc.communicationHandler(req)
    var value string
    if resp.Has("data.dp.value") {
        value = resp.Get("data.dp.value").String()
    }
    return value, err
}

// Function to subscribe value from VSS tree
func (cc *KuksaClientComm) SubscribeFromKuksaValServer(path string, subsChannel *chan []byte) (string, error) {

    // Create new KuksaValRequest
    req := objx.New(make (map [string] interface{}))
    req.Set("requestId", uuid.New().String())
    req.Set("action", "subscribe")
    req.Set("path",  path)

    // Create channel for subscription
    resp, err := cc.communicationHandler(req)
    var subscriptionId string
    if err == nil {
        subscriptionId = resp.Get("subscriptionId").String()
        cc.subscriptionStateMutex.Lock()
        cc.subscriptionState[subscriptionId] = subsChannel
        cc.subscriptionStateMutex.Unlock()

    }
    return subscriptionId, err
}

// Function to unsubscribe value from VSS tree
func (cc *KuksaClientComm) UnsubscribeFromKuksaValServer(id string) error {

    // Create new KuksaValRequest
    req := objx.New(make (map [string] interface{}))
    req.Set("requestId", uuid.New().String())
    req.Set("action", "unsubscribe")
    req.Set("subscriptionId",  id)

    _, err := cc.communicationHandler(req)
    if err == nil {
        cc.subscriptionStateMutex.Lock()
        delete(cc.subscriptionState, id)
        cc.subscriptionStateMutex.Unlock()
    }
    return err
}

// Function to set value from VSS tree
func (cc *KuksaClientComm) SetValueFromKuksaValServer(path string, value string) error {

    // Create new KuksaValRequest
    respChannel := make(chan objx.Map)
    defer close(respChannel)
    req := objx.New(make (map [string] interface{}))
    uuid := uuid.New().String()
    req.Set("requestId", uuid)
    req.Set("action", "set")
    req.Set("path",  path)
    req.Set("value",  value)

    _, err := cc.communicationHandler(req)
    return err
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
    req := objx.New(make (map [string] interface{}))
    req.Set("requestId", uuid.New().String())
    req.Set("action", "authorize")
    req.Set("tokens",  string(tokenByteString))

    _, err = cc.communicationHandler(req)
    return err
}

// Function to get metadata from kuksa.val server
func (cc *KuksaClientComm) GetMetadataFromKuksaValServer(path string) (string, error) {

    // Create new KuksaValRequest
    req := objx.New(make (map [string] interface{}))
    req.Set("requestId", uuid.New().String())
    req.Set("action", "getMetaData")
    req.Set("path",  path)

    resp, err := cc.communicationHandler(req)
    var value string
    if resp.Has("metadata") {
        value, err  = resp.Get("metadata").ObjxMap().JSON()
    }
    return value, err
}
