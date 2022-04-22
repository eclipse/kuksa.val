package kuksa_viss

import (
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
)

type KuksaClientComm struct {
	Config                 *KuksaVISSClientConfig
	connSocket             *websocket.Conn
	sendChannel            chan []byte
	requestState           map[string]*chan []byte
	requestStateMutex      sync.RWMutex
	subscriptionState      map[string]*chan []byte
	subscriptionStateMutex sync.RWMutex
}

const (
	TIMEOUT = 1 * time.Second
)

func (cc *KuksaClientComm) startCommunication() error {

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
		log.Printf(string(cc.connSocket.LocalAddr().String()))
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
		log.Printf(string(cc.connSocket.LocalAddr().String()))
	}

	// Create send sub routine
	cc.sendChannel = make(chan []byte, 10)
	go func() {
		for {
			req := <-cc.sendChannel
			cc.connSocket.WriteMessage(websocket.TextMessage, []byte(req))
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
func (cc *KuksaClientComm) Close() {
	close(cc.sendChannel)
	cc.connSocket.Close()
}

// Communication Handler
func (cc *KuksaClientComm) communicationHandler(req objx.Map) (objx.Map, error) {
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
