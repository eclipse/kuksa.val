package main

import (
   "log"
   "time"
   "strconv"

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
        log.Fatal("Connection Error:", err)
    }
    defer kuksaClientComm.Close()

    // Authorize the connection
    err = kuksaClientComm.AuthorizeKuksaValServerConn()
    if err != nil {
        log.Fatalf("Authorization Error: ", err)
    }

   // Get Value of Vehicle.Speed
    var value string
    value, err = kuksaClientComm.GetValueFromKuksaValServer("Vehicle.Speed1")
    if err != nil {
        log.Printf("Get Error: %v", err)
    } else {
        log.Printf("Vehicle.Speed: " + value)
    }

    // Go Routine for setting Vehicle Speed
    go func() {
        for i:=0;;i++ {
            // Set Value of Vehicle.Speed
            err := kuksaClientComm.SetValueFromKuksaValServer("Vehicle.Speed", strconv.Itoa(i))
            if err != nil { 
                log.Printf("Set Error: %v", err)
            }
            time.Sleep(10 * time.Millisecond)
        }
    }()

    // Go Routine for getting Vehicle Speed
    finish := make(chan int)
    go func() {
	tick := time.Tick(15*time.Millisecond)
	done := time.After(1 * time.Second)
        for {
            select {
                case <-tick:
                    // Get Value of Vehicle.Speed
                    value, _ = kuksaClientComm.GetValueFromKuksaValServer("Vehicle.Speed")
                    log.Printf("Vehicle.Speed: " + value)
                case <-done:
                    log.Printf("Done now!!")
                    finish<-1
                    return
            }
        }
    }()

    // Get MetaData of Vehicle.Speed
    value, err = kuksaClientComm.GetMetadataFromKuksaValServer("Vehicle.Speed")
    log.Printf("Vehicle.Speed: " + value)

    <-finish
}
