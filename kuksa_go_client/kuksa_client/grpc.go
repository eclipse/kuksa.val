package kuksa_client

import (
	"context"
	"log"
	"time"

	grpcpb "github.com/eclipse/kuksa.val/kuksa_go_client/proto/kuksa/val/v1"
)

type FieldView struct {
	Field grpcpb.Field
	View  grpcpb.View
}

var dict map[string]FieldView

func (cg *KuksaClientCommGrpc) ConnectToKuksaVal() error {
	dict = map[string]FieldView{
		"value":       {grpcpb.Field_FIELD_VALUE, grpcpb.View_VIEW_CURRENT_VALUE},
		"targetValue": {grpcpb.Field_FIELD_ACTUATOR_TARGET, grpcpb.View_VIEW_TARGET_VALUE},
		"metadata":    {grpcpb.Field_FIELD_METADATA, grpcpb.View_VIEW_METADATA},
	}
	err := cg.startCommunication()
	return err
}

// Function to get attribute value from VSS tree
func (cg *KuksaClientCommGrpc) GetValueFromKuksaVal(path string, attr string) ([]string, error) {

	// Contact the server and return the Value
	ctx, cancel := context.WithTimeout(context.Background(), time.Second)
	defer cancel()
	view := dict[attr].View
	fields := []grpcpb.Field{dict[attr].Field}
	entries := grpcpb.EntryRequest{Path: path, View: view, Fields: fields}
	request := []*grpcpb.EntryRequest{&entries}
	resp, err := cg.client.Get(ctx, &grpcpb.GetRequest{Entries: request})
	if err != nil {
		log.Fatalf("Get call failed: %v", err)
	}
	respEntries := resp.GetEntries()
	values := []string{}
	for _, entry := range respEntries {
		values = append(values, entry.String())
	}

	return values, err
}

// // Function to subscribe attribute value from VSS tree
// func (cc *KuksaClientCommWs) SubscribeFromKuksaVal(path string, subsChannel *chan []byte, attr string) (string, error) {

// 	// Create new KuksaValRequest
// 	req := objx.New(make(map[string]interface{}))
// 	req.Set("requestId", uuid.New().String())
// 	req.Set("action", "subscribe")
// 	req.Set("path", path)
// 	req.Set("attribute", attr)

// 	// Create channel for subscription
// 	resp, err := cc.communicationHandler(req)
// 	var subscriptionId string
// 	if err == nil {
// 		subscriptionId = resp.Get("subscriptionId").String()
// 		cc.subscriptionStateMutex.Lock()
// 		cc.subscriptionState[subscriptionId] = subsChannel
// 		cc.subscriptionStateMutex.Unlock()

// 	}
// 	return subscriptionId, err
// }

// // Function to unsubscribe value from VSS tree
// func (cc *KuksaClientCommWs) UnsubscribeFromKuksaVal(id string) error {

// 	// Create new KuksaValRequest
// 	req := objx.New(make(map[string]interface{}))
// 	req.Set("requestId", uuid.New().String())
// 	req.Set("action", "unsubscribe")
// 	req.Set("subscriptionId", id)

// 	_, err := cc.communicationHandler(req)
// 	if err == nil {
// 		cc.subscriptionStateMutex.Lock()
// 		delete(cc.subscriptionState, id)
// 		cc.subscriptionStateMutex.Unlock()
// 	}
// 	return err
// }

// // Function to set value from VSS tree
// func (cc *KuksaClientCommWs) SetValueFromKuksaVal(path string, value string, attr string) error {

// 	// Create new KuksaValRequest
// 	respChannel := make(chan objx.Map)
// 	defer close(respChannel)
// 	req := objx.New(make(map[string]interface{}))
// 	uuid := uuid.New().String()
// 	req.Set("requestId", uuid)
// 	req.Set("action", "set")
// 	req.Set("path", path)
// 	req.Set("attribute", attr)
// 	req.Set(attr, value)

// 	_, err := cc.communicationHandler(req)
// 	return err
// }

// Function to authorize to kuksa.val server
func (cg *KuksaClientCommGrpc) AuthorizeKuksaValConn() error {
	log.Println("KUKSA.val databroker does not support authorization mechanism yet. If you want to use authorization please use KUKSA.val server")
	return nil
}

// Function to get metadata from kuksa.val server
// func (cc *KuksaClientCommWs) GetMetadataFromKuksaVal(path string) (string, error) {

// 	// Create new KuksaValRequest
// 	req := objx.New(make(map[string]interface{}))
// 	req.Set("requestId", uuid.New().String())
// 	req.Set("action", "getMetaData")
// 	req.Set("path", path)

// 	resp, err := cc.communicationHandler(req)
// 	var value string
// 	if resp.Has("metadata") {
// 		value, err = resp.Get("metadata").ObjxMap().JSON()
// 	}
// 	return value, err
// }
