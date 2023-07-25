// Copyright 2023 Robert Bosch GmbH
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

package kuksa_client

import (
	"errors"
	"log"
	"os"

	"github.com/google/uuid"
	"github.com/stretchr/objx"
)

// Function to connect to kuksa.val
func (cc *KuksaClientCommWs) ConnectToKuksaVal() error {
	err := cc.startCommunication()
	log.Println("Websocket connected")
	return err
}

// Function to get attribute value from VSS tree
func (cc *KuksaClientCommWs) GetValueFromKuksaVal(path string, attr string) ([]interface{}, error) {

	// Create new KuksaValRequest
	req := objx.New(make(map[string]interface{}))
	req.Set("requestId", uuid.New().String())
	req.Set("action", "get")
	req.Set("path", path)
	req.Set("attribute", attr)

	resp, err := cc.communicationHandler(req)
	var value string
	if resp.Has("data.dp." + attr) {
		value = resp.Get("data.dp." + attr).String()
	}
	values := []interface{}{}
	values = append(values, value)
	return values, err
}

func (cc *KuksaClientCommWs) PrintSubscriptionMessages(id string) error {
	for {
		val := <-*cc.subscriptionState[id]
		log.Printf("Vehicle.Speed Subscribed: %s", val)
	}
}

// Function to subscribe attribute value from VSS tree
func (cc *KuksaClientCommWs) SubscribeFromKuksaVal(path string, attr string) (string, error) {

	// Create new channel for subscribing
	id := uuid.New().String()
	subsChannel := make(chan []byte, 10)

	// Create new KuksaValRequest
	req := objx.New(make(map[string]interface{}))
	req.Set("requestId", id)
	req.Set("action", "subscribe")
	req.Set("path", path)
	req.Set("attribute", attr)

	// Create channel for subscription
	resp, err := cc.communicationHandler(req)
	var subscriptionId string
	if err == nil {
		subscriptionId = resp.Get("subscriptionId").String()
		cc.subscriptionStateMutex.Lock()
		cc.subscriptionState[subscriptionId] = &subsChannel
		cc.subscriptionStateMutex.Unlock()

	}
	return subscriptionId, err
}

// Function to unsubscribe value from VSS tree
func (cc *KuksaClientCommWs) UnsubscribeFromKuksaVal(id string) error {

	// Create new KuksaValRequest
	req := objx.New(make(map[string]interface{}))
	req.Set("requestId", uuid.New().String())
	req.Set("action", "unsubscribe")
	req.Set("subscriptionId", id)

	_, err := cc.communicationHandler(req)
	if err == nil {
		cc.subscriptionStateMutex.Lock()
		delete(cc.subscriptionState, id)
		cc.subscriptionStateMutex.Unlock()
	}

	close(*cc.subscriptionState[id])
	return err
}

// Function to set value from VSS tree
func (cc *KuksaClientCommWs) SetValueFromKuksaVal(path string, value string, attr string) error {

	// Create new KuksaValRequest
	respChannel := make(chan objx.Map)
	defer close(respChannel)
	req := objx.New(make(map[string]interface{}))
	uuid := uuid.New().String()
	req.Set("requestId", uuid)
	req.Set("action", "set")
	req.Set("path", path)
	req.Set("attribute", attr)
	// Note: Line below currently gives problems if value is a string representation of a JSON object
	// (array and theoretically also struct)
	// The Set method handles it as a string, which the gives problems at the server side which expects for example
	// a JSON array rather than a string containing an array
	req.Set(attr, value)

	_, err := cc.communicationHandler(req)
	return err
}

// Function to authorize to kuksa.val server
func (cc *KuksaClientCommWs) AuthorizeKuksaValConn(TokenOrTokenfile string) error {

	var tokenString string
	// Data in config has precedence over hardcoded token
	if cc.Config.TokenOrTokenfile != ""{
		tokenString = cc.Config.TokenOrTokenfile
	}else{
		if TokenOrTokenfile == "" {
			return errors.New("No token given!")
		}
		tokenString = TokenOrTokenfile
	}

	log.Printf("Using token: %s", tokenString)

	info, err := os.Stat(tokenString)
    if err != nil {
        // the TokenOrTokenfile is read like its a token
    } else if info.Mode().IsRegular() {
        tokenByteString, err := os.ReadFile(tokenString)
		tokenString = string(tokenByteString)
		if err != nil {
			log.Fatal("Error reading token: ", err)
			return err
		}
    }

	// Create new KuksaValRequest
	req := objx.New(make(map[string]interface{}))
	req.Set("requestId", uuid.New().String())
	req.Set("action", "authorize")
	req.Set("tokens", tokenString)

	_, err = cc.communicationHandler(req)
	return err
}

// Function to get metadata from kuksa.val server
func (cc *KuksaClientCommWs) GetMetadataFromKuksaVal(path string) ([]interface{}, error) {

	// Create new KuksaValRequest
	req := objx.New(make(map[string]interface{}))
	req.Set("requestId", uuid.New().String())
	req.Set("action", "getMetaData")
	req.Set("path", path)

	resp, err := cc.communicationHandler(req)
	var value string
	if resp.Has("metadata") {
		value, err = resp.Get("metadata").ObjxMap().JSON()
	}
	values := []interface{}{}
	values = append(values, value)
	return values, err
}
