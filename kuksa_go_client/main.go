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

//go:generate protoc --go_out=proto --go-grpc_out=proto -I ../proto kuksa/val/v1/types.proto
//go:generate protoc --go_out=proto --go-grpc_out=proto -I ../proto kuksa/val/v1/val.proto

package main

import (
	"flag"
	"log"

	"github.com/eclipse/kuksa.val/kuksa_go_client/kuksa_client"
)

func main() {
	log.Println("Starting Kuksa Client!!")

	// Load the Kuksa Configuration
	var configKuksaClient kuksa_client.KuksaClientConfig
	kuksa_client.ReadConfig(&configKuksaClient)
	protocol := flag.String("protocol", "undefined", "Could be grpc or ws. Per default the protocol variable is undefined. Then the value from the config file kuksa_client.json will be used.")
	flag.Parse()
	if *protocol == "undefined" {
		protocol = &configKuksaClient.TransportProtocol
	}

	var backend kuksa_client.KuksaBackend

var token string
if *protocol == "ws" {
	// example token from kuksa.val/kuksa_certificates/jwt/all-read-write.json.token
	token = "eyJ0eXAiOiJKV1QiLCJhbGciOiJSUzI1NiJ9.eyJzdWIiOiJrdWtzYS52YWwiLCJpc3MiOiJFY2xpcHNlIEtVS1NBIERldiIsImFkbWluIjp0cnVlLCJpYXQiOjE1MTYyMzkwMjIsImV4cCI6MTc2NzIyNTU5OSwia3Vrc2EtdnNzIjp7IioiOiJydyJ9fQ.QQcVR0RuRJIoasPXYsMGZhdvhLjUalk4GcRaxhh3-0_j3CtVSZ0lTbv_Z3As5BfIYzaMlwUzFGvCVOq2MXVjRK81XOAZ6wIsyKOxva16zjbZryr2V_m3yZ4twI3CPEzJch11_qnhInirHltej-tGg6ySfLaTYeAkw4xYGwENMBBhN5t9odANpScZP_xx5bNfwdW1so6FkV1WhpKlCywoxk_vYZxo187d89bbiu-xOZUa5D-ycFkd1-1rjPXLGE_g5bc4jcQBvNBc-5FDbvt4aJlTQqjpdeppxhxn_gjkPGIAacYDI7szOLC-WYajTStbksUju1iQCyli11kPx0E66me_ZVwOX07f1lRF6D2brWm1LcMAHM3bQUK0LuyVwWPxld64uSAEsvSKsRyJERc7nZUgLf7COnUrrkxgIUNjukbdT2JVN_I-3l3b4YXg6JVD7Y5g0QYBKgXEFpZrDbBVhzo7PXPAhJD6-c3DcUQyRZExbrnFV56RwWuExphw8lYnbMvxPWImiVmB9nRVgFKD0TYaw1sidPSSlZt8Uw34VZzHWIZQAQY0BMjR33fefg42XQ1YzIwPmDx4GYXLl7HNIIVbsRsibKaJnf49mz2qnLC1K272zXSPljO11Ke1MNnsnKyUH7mcwEs9nhTsnMgEOx_TyMLRYo-VEHBDLuEOiBo"
	backend = &kuksa_client.KuksaClientCommWs{Config: &configKuksaClient}
} else if *protocol == "grpc" {
	// example token from kuksa.val/jwt/provide-all.token
	token = "eyJ0eXAiOiJKV1QiLCJhbGciOiJSUzI1NiJ9.eyJzdWIiOiJsb2NhbCBkZXYiLCJpc3MiOiJjcmVhdGVUb2tlbi5weSIsImF1ZCI6WyJrdWtzYS52YWwiXSwiaWF0IjoxNTE2MjM5MDIyLCJleHAiOjE3NjcyMjU1OTksInNjb3BlIjoicHJvdmlkZSJ9.OJWzTvDjcmeWyg3vmBR5TEtqYaHq8HrpFLlTKZAfDBAQBUHpyUEboJ97jfWuWgBnTpnfboyfAbwvLqo6bEVZ6tXzF8n9LtW6HmPbIWoDqXuobM2grUCVaGKuOcnCpMCQYChziqHbYwRJYP9nkYgbQU1kE4dN7880Io4xzq0GEbWksB2CVpOoExQUmCZpCohPs-XEkdmXhcUKnWnOeiSsRGKusx987vpY_WOXh6WE7DfJgzAgpPDo33qI7zQuTzUILORQsiHmsrQO0-zcvokNjaQUzlt5ETZ7MQLCtiUQaN0NMbDMCWkmSfNvZ5hKCNbfr2FaiMzrGBOQdvQiFo-DqZKGNweaGpufYXuaKfn3SXKoDr8u1xDE5oKgWMjxDR9pQYGzIF5bDXITSywCm4kN5DIn7e2_Ga28h3rBl0t0ZT0cwlszftQRueDTFcMns1u9PEDOqf7fRrhjq3zqpxuMAoRANVd2z237eBsS0AvdSIxL52N4xO8P_h93NN8Vaum28fTPxzm8p9WlQh4mgUelggtT415hLcxizx15ARIRG0RiW91Pglzt4WRtXHnsg93Ixd3yXXzZ2i4Y0hqhj_L12SsXunK2VxKup2sFCQz6wM-t_7ADmNYcs80idzsadY8rYKDV8N1WqOOd4ANG_nzWa86Tyu6wAwhDVag5nbFmLZQ"
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
err = backend.AuthorizeKuksaValConn(token)
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

	err = backend.SetValueFromKuksaVal("Vehicle.OBD.DTCList", "[dtc1, dtc2, dtc3]", "value")
	if err != nil {
		log.Printf("Set Value Error: %v", err)
	} else {
		log.Printf("Vehicle.OBD.DTCList Set: [dtc1, dtc2, dtc3]")
	}

	values, err = backend.GetValueFromKuksaVal("Vehicle.OBD.DTCList", "value")
	if err != nil {
		log.Printf("Get Value Error: %v", err)
	} else {
		for _, value := range values {
			log.Println("Vehicle.OBD.DTCList: " + value)
		}
	}

	// set string with "" and \"
	err = backend.SetValueFromKuksaVal("Vehicle.OBD.DTCList", "['dtc1, dtc2', dtc2, \"dtc3, dtc3\"]", "value")
	if err != nil {
		log.Printf("Set Value Error: %v", err)
	} else {
		log.Printf("Vehicle.OBD.DTCList Set: [dtc1, dtc2, dtc3]")
	}

	values, err = backend.GetValueFromKuksaVal("Vehicle.OBD.DTCList", "value")
	if err != nil {
		log.Printf("Get Value Error: %v", err)
	} else {
		for _, value := range values {
			log.Println("Vehicle.OBD.DTCList: " + value)
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