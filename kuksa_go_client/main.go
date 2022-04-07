package main

import (
   "log"

   "kuksa.val/kuksa_go_client/kuksa_viss"
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
    value, err = kuksaClientComm.GetValueFromKuksaValServer("Vehicle.Speed")
    log.Printf("Vehicle.Speed: " + value)

    // Set Value of Vehicle.Speed
    err = kuksaClientComm.SetValueFromKuksaValServer("Vehicle.Speed", "70")

    // Get Value of Vehicle.Speed
    value, err = kuksaClientComm.GetValueFromKuksaValServer("Vehicle.Speed")
    log.Printf("Vehicle.Speed: " + value)

    // Get MetaData of Vehicle.Speed
    value, err = kuksaClientComm.GetMetadataFromKuksaValServer("Vehicle.Speed")
    log.Printf("Vehicle.Speed: " + value)

}
