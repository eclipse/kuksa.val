package main

import (
	"log"
	"strconv"
	"time"

	"github.com/eclipse/kuksa.val/kuksa_go_client/kuksa_viss"
)

func main() {
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

	// Get Value of Vehicle.Speed1
	// This datapoint does not exist in the VSS and should result in an error
	var value string
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
