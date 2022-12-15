// Copyright 2022 Robert Bosch GmbH
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
	"context"
	"crypto/tls"
	"crypto/x509"
	"errors"
	"io/ioutil"
	"log"
	"net/url"
	"sync"
	"time"

	"github.com/gorilla/websocket"
	"github.com/stretchr/objx"
	"google.golang.org/grpc"
	"google.golang.org/grpc/credentials/insecure"

	// pb "github.com/eclipse/kuksa.val/kuksa_go_client/proto"
	grpcpb "github.com/eclipse/kuksa.val/kuksa_go_client/proto/kuksa/val/v1"
)

type KuksaClientCommWs struct {
	Config                 *KuksaClientConfig
	connSocket             *websocket.Conn
	sendChannel            chan []byte
	requestState           map[string]*chan []byte
	requestStateMutex      sync.RWMutex
	subscriptionState      map[string]*chan []byte
	subscriptionStateMutex sync.RWMutex
}

type KuksaClientCommGrpc struct {
	Config                 *KuksaClientConfig
	conn                   *grpc.ClientConn
	client                 grpcpb.VALClient
	cancel                 map[string]*context.CancelFunc
	subsChannel            map[string]*grpcpb.VAL_SubscribeClient
	subsAttr               string
	subscriptionStateMutex sync.RWMutex
}

const (
	TIMEOUT = 1 * time.Second
)

func (cg *KuksaClientCommGrpc) startCommunication() error {
	var opts []grpc.DialOption
	opts = append(opts, grpc.WithTransportCredentials(insecure.NewCredentials()))
	serverUrl := cg.Config.ServerAddress + ":" + cg.Config.ServerPort
	var err error
	cg.conn, err = grpc.Dial(serverUrl, opts...)
	if err != nil {
		return err
	}
	cg.client = grpcpb.NewVALClient(cg.conn)
	cg.cancel = make(map[string]*context.CancelFunc)
	cg.subsChannel = make(map[string]*grpcpb.VAL_SubscribeClient)
	log.Println("gRPC channel Connected")
	return nil
}

func (cc *KuksaClientCommWs) startCommunication() error {

	if cc.Config.Insecure {
		// Open an insecure websocket
		serverUrl := url.URL{Scheme: "ws", Host: cc.Config.ServerAddress + ":" + cc.Config.ServerPort}
		log.Printf("Connecting to " + serverUrl.String())

		// Connect to the Kuksa Websocket
		var err error
		cc.connSocket, _, err = websocket.DefaultDialer.Dial(serverUrl.String(), nil)
		if err != nil {
			log.Fatal("Connection error: ", err)
			return err
		}
		log.Println(string(cc.connSocket.LocalAddr().String()))
	} else {
		// Open a secure websocket
		serverUrl := url.URL{Scheme: "wss", Host: cc.Config.ServerAddress + ":" + cc.Config.ServerPort}
		log.Printf("Connecting to " + serverUrl.String())

		// Load Client cert
		cert, err := tls.LoadX509KeyPair(cc.Config.CertsDir+"/Client.pem", cc.Config.CertsDir+"/Client.key")
		if err != nil {
			log.Fatal("Certificate Error: ", err)
		}

		// Load CA Certificate
		caCert, err := ioutil.ReadFile(cc.Config.CertsDir + "/CA.pem")
		if err != nil {
			return err
		}
		caCertPool := x509.NewCertPool()
		caCertPool.AppendCertsFromPEM(caCert)

		tlsConfig := &tls.Config{
			Certificates:       []tls.Certificate{cert},
			RootCAs:            caCertPool,
			InsecureSkipVerify: true,
		}

		dialer := websocket.Dialer{TLSClientConfig: tlsConfig}
		cc.connSocket, _, err = dialer.Dial(serverUrl.String(), nil)
		if err != nil {
			log.Fatal("Connection error: ", err)
			return err
		}
		log.Println(string(cc.connSocket.LocalAddr().String()))
	}

	// Create send sub routine
	cc.sendChannel = make(chan []byte, 10)
	go func() {
		for {
			req, ok := <-cc.sendChannel
			if ok {
				cc.connSocket.WriteMessage(websocket.TextMessage, []byte(req))
			}
		}
	}()

	// Create receive sub routine and the request state
	cc.requestState = make(map[string]*chan []byte)
	cc.subscriptionState = make(map[string]*chan []byte)
	go func() {
		for {
			_, respString, err := cc.connSocket.ReadMessage()

			if err != nil {
				return
			}
			resp, _ := objx.FromJSON(string(respString))
			// Get the correct channel to send back response
			if resp.Has("requestId") {
				cc.requestStateMutex.RLock()
				respChannel, ok := cc.requestState[resp.Get("requestId").String()]
				if ok {
					select {
					case *respChannel <- respString:
					default:
						continue
					}
				}
				cc.requestStateMutex.RUnlock()
			} else if resp.Get("action").String() == "subscription" {
				cc.subscriptionStateMutex.RLock()
				respChannel, ok := cc.subscriptionState[resp.Get("subscriptionId").String()]
				if ok {
					select {
					case *respChannel <- []byte(resp.Get("data.dp.value").String()):
					default:
						continue
					}
				}
				cc.subscriptionStateMutex.RUnlock()
			}

		}

	}()

	return nil
}

// Close the Kuksa Client
func (cg *KuksaClientCommGrpc) Close() {
	defer cg.conn.Close()
}

func (cc *KuksaClientCommWs) Close() {
	close(cc.sendChannel)
	cc.connSocket.Close()
}

// Communication Handler
func (cc *KuksaClientCommWs) communicationHandler(req objx.Map) (objx.Map, error) {
	// Create channel for response and store it in the Request state
	respChannel := make(chan []byte)
	defer close(respChannel)
	cc.requestStateMutex.Lock()
	cc.requestState[req.Get("requestId").String()] = &respChannel
	cc.requestStateMutex.Unlock()

	reqString, _ := req.JSON()
	cc.sendChannel <- []byte(reqString)

	timeOut := time.After(TIMEOUT)
	var err error
	var response objx.Map
	select {
	case responseString := <-respChannel:
		response, _ = objx.FromJSON(string(responseString))
		if response.Has("error") {
			errString, _ := response.Get("error").ObjxMap().JSON()
			err = errors.New(errString)
		}
	case <-timeOut:
		err = errors.New("Timeout")
	}

	cc.requestStateMutex.Lock()
	delete(cc.requestState, req.Get("requestId").String())
	cc.requestStateMutex.Unlock()
	return response, err
}
