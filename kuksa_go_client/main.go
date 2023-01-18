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

//go:generate protoc --go_out=proto --go-grpc_out=proto -I ../proto kuksa/val/v1/types.proto
//go:generate protoc --go_out=proto --go-grpc_out=proto -I ../proto kuksa/val/v1/val.proto

package main

import (
	"flag"
	"log"

	"github.com/eclipse/kuksa.val/kuksa_go_client/kuksa_client"
	"github.com/eclipse/kuksa.val/kuksa_go_client/protocInstall"
)

func main() {

	protoc := flag.Bool("protoc", false, "")

	if *protoc {
		protocInstall.Protoc()
	} else {
		log.Println("Starting Kuksa Client!!")

		// Load the Kuksa Configuration
		var configKuksaClient kuksa_client.KuksaClientConfig
		kuksa_client.ReadConfig(&configKuksaClient)
		log.Println(configKuksaClient)
		protocol := flag.String("protocol", "undefined", "Could be grpc or ws. Per default the protocol variable is undefined. Then the value from the config file kuksa_client.json will be used.")
		flag.Parse()
		if *protocol == "undefined" {
			protocol = &configKuksaClient.TransportProtocol
		}

		var backend kuksa_client.KuksaBackend

		if *protocol == "ws" {
			backend = &kuksa_client.KuksaClientCommWs{Config: &configKuksaClient}
		} else if *protocol == "grpc" {
			backend = &kuksa_client.KuksaClientCommGrpc{Config: &configKuksaClient}
		} else {
			log.Println("Specify -protocol=ws or -protocol=grpc")
		}

		err := backend.ConnectToKuksaVal()
		if err != nil {
			log.Fatalf("Connection Error: %v", err)
		}
		defer backend.Close()

		//Authorize the connection
		err = backend.AuthorizeKuksaValConn()
		if err != nil {
			log.Fatalf("Authorization Error: %v", err)
		}

		err = backend.SetValueFromKuksaVal("Vehicle.ADAS.ABS.IsEnabled", "true", "value")
		if err != nil {
			log.Printf("Set Value Error: %v", err)
		} else {
			log.Printf("Vehicle.ADAS.ABS.IsEnabled Set: true")
		}

		values, err := backend.GetValueFromKuksaVal("Vehicle.ADAS.ABS.IsEnabled", "value")
		if err != nil {
			log.Printf("Get Value Error: %v", err)
		} else {
			for _, value := range values {
				log.Println("Vehicle.ADAS.ABS.IsEnabled: " + value)
			}
		}

		err = backend.SetValueFromKuksaVal("Vehicle.ADAS.ABS.IsEnabled", "true", "targetValue")
		if err != nil {
			log.Printf("Set Value Error: %v", err)
		} else {
			log.Printf("Vehicle.ADAS.ABS.IsEnabled Set: true")
		}

		tValues, err := backend.GetValueFromKuksaVal("Vehicle.ADAS.ABS.IsEnabled", "targetValue")
		if err != nil {
			log.Printf("Get Value Error: %v", err)
		} else {
			for _, value := range tValues {
				log.Println("Vehicle.ADAS.ABS.IsEnabled: " + value)
			}
		}

		// Get MetaData of Vehicle.Speed
		value, err := backend.GetMetadataFromKuksaVal("Vehicle.Speed")
		if err == nil {
			for _, val := range value {
				log.Printf("Vehicle.Speed Metadata: " + val)
			}
		} else {
			log.Printf("Error while getting metadata: %s", err)
		}

		//Subscribe to Vehicle.Speed

		id, err := backend.SubscribeFromKuksaVal("Vehicle.Speed", "value")
		if err == nil {
			log.Printf("Vehicle.Speed Subscription Id: %s", id)
		} else {
			log.Printf("Subscription Error %s", err)
		}
		err = backend.PrintSubscriptionMessages(id)
		if err != nil {
			log.Printf("Printing the subscription messages failed with: %s", err)
		}
       }
}
// More subscribing examples
// 		idT, err := backend.SubscribeFromKuksaVal("Vehicle.Speed", "targetValue")
// 		if err == nil {
// 			log.Printf("Vehicle.Speed Subscription Id: %s", idT)
// 		} else {
// 			log.Printf("Subscription Error %s", err)
// 		}
// 		err = backend.PrintSubscriptionMessages(idT)
// 		if err != nil {
// 			log.Printf("Printing the subscription messages failed with: %s", err)
// 		}

// 		err = backend.UnsubscribeFromKuksaVal(idT)
// 		if err != nil {
// 			log.Printf("Unsubscribing failed with: %s", err)
// 		}

// 		err = backend.UnsubscribeFromKuksaVal(id)
// 		if err != nil {
// 			log.Printf("Unsubscribing failed with: %s", err)
// 		}

// 		idM, err := backend.SubscribeFromKuksaVal("Vehicle.Speed", "metadata")
// 		if err == nil {
// 			log.Printf("Vehicle.Speed Subscription Id: %s", idM)
// 		} else {
// 			log.Printf("Subscription Error %s", err)
// 		}
// 		err = backend.PrintSubscriptionMessages(idM)
// 		if err != nil {
// 			log.Printf("Printing the subscription messages failed with: %s", err)
// 		}

// 		err = backend.UnsubscribeFromKuksaVal(idM)
// 		if err != nil {
// 			log.Printf("Unsubscribing failed with: %s", err)
// 		}
// 	}
// }
