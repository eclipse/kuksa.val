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

//go:generate protoc --go_out=proto -I $HOME/kuksa.val/proto kuksa/val/v1/types.proto kuksa/val/v1/val.proto --go_opt=module=github.com/eclipse/kuksa.val/proto/kuksa/val/v1

package main

import (
	"log"
	"strconv"
	"time"
	"flag"

	"github.com/eclipse/kuksa.val/kuksa_go_client/kuksa_viss"
	"github.com/eclipse/kuksa.val/kuksa_go_client/protocInstall"
)

func main() {
	protoc := flag.Bool("protoc", false, "")
    flag.Parse()

	if *protoc {
		protocInstall.Protoc()
	}else{
		log.Println("Starting Kuksa VISS Client!!")

		// Load the Kuksa Configuration
		var configKuksaClient kuksa_viss.KuksaVISSClientConfig
		kuksa_viss.ReadConfig(&configKuksaClient)
		log.Println(configKuksaClient)

		// Trigger connection to the Kuksa Server
		kuksaClientComm := kuksa_viss.KuksaClientComm{Config: &configKuksaClient}
		err := kuksaClientComm.ConnectToKuksaValServerWs()
		if err != nil {
			log.Fatalf("Connection Error: %v", err)
		}
		defer kuksaClientComm.Close()

		// Authorize the connection
		err = kuksaClientComm.AuthorizeKuksaValServerConn()
		if err != nil {
			log.Fatalf("Authorization Error: %v", err)
		}

		// Set and Get the attribute targetValue
		err = kuksaClientComm.SetAttrValueFromKuksaValServer("Vehicle.Body.Trunk.IsOpen", "true", "targetValue")
		if err != nil {
			log.Printf("Set Attribute Error: %v", err)
		} else {
			log.Printf("Vehicle.Body.Trunk.IsOpen Set: True")
		}

		var value string
		value, err = kuksaClientComm.GetAttrValueFromKuksaValServer("Vehicle.Body.Trunk.IsOpen", "targetValue")
		if err != nil {
			log.Printf("Get Attribute Error: %v", err)
		} else {
			log.Printf("Vehicle.Body.Trunk.IsOpen: " + value)
		}

		// Get Value of Vehicle.Speed1
		// This datapoint does not exist in the VSS and should result in an error
		value, err = kuksaClientComm.GetValueFromKuksaValServer("Vehicle.Speed1")
		if err != nil {
			log.Printf("Get Error: %v", err)
		} else {
			log.Printf("Vehicle.Speed: " + value)
		}

		// Go Routine for setting Vehicle Speed
		go func() {
			for i := 0; ; i++ {
				// Set Value of Vehicle.Speed
				err := kuksaClientComm.SetValueFromKuksaValServer("Vehicle.Speed", strconv.Itoa(i))
				if err != nil {
					log.Printf("Set Error: %v", err)
				} else {
					log.Printf("Vehicle.Speed Set: %d", i)
				}
				time.Sleep(10 * time.Millisecond)
			}
		}()

		// Go Routine for getting Vehicle Speed
		finish := make(chan int)
		go func() {
			tick := time.Tick(15 * time.Millisecond)
			done := time.After(1 * time.Second)
			for {
				select {
				case <-tick:
					// Get Value of Vehicle.Speed
					value, err = kuksaClientComm.GetValueFromKuksaValServer("Vehicle.Speed")
					if err != nil {
						log.Printf("Get Error: %v", err)
					} else {
						log.Printf("Vehicle.Speed Get: %s", value)
					}
					continue
				case <-done:
					log.Printf("Done now!!")
					finish <- 1
					return
				}
			}
		}()

		// Get MetaData of Vehicle.Speed
		value, err = kuksaClientComm.GetMetadataFromKuksaValServer("Vehicle.Speed")
		log.Printf("Vehicle.Speed Metadata: " + value)

		// Subscribe to Vehicle.Speed
		channel := make(chan []byte, 10)
		id, err := kuksaClientComm.SubscribeFromKuksaValServer("Vehicle.Speed", &channel)
		if err == nil {
			log.Printf("Vehicle.Speed Subscription Id: %s", id)
		} else {
			log.Printf("Subscription Error %s", err)
		}

		go func() {
			unsub := time.After(800 * time.Millisecond)
			for {
				select {
				case val := <-channel:
					log.Printf("Vehicle.Speed Subscribed: %s", val)
				case <-unsub:
					err := kuksaClientComm.UnsubscribeFromKuksaValServer(id)
					if err == nil {
						log.Printf("Vehicle.Speed Unsubscribed")
					} else {
						log.Printf("Unsubscription Error: %s", err)
					}
					close(channel)
					return
				}
			}
		}()

		<-finish
	}
}
