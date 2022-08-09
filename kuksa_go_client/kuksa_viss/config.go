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

package kuksa_viss

import (
	"log"
	"strconv"

	"github.com/spf13/viper"
)

type Transport string

const (
	UndefTransport Transport = ""
	Grpc                     = "GRPC"
	WebSocket      Transport = "WebSocket"
)

type KuksaVISSClientConfig struct {
	ServerAddress     string    `mapstructure:"serverAddress"`
	ServerPort        string    `mapstructure:"serverPort"`
	Insecure          bool      `mapstructure:"insecure"`
	CertsDir          string    `mapstructure:"certsDir"`
	TokenPath         string    `mapstructure:"tokenPath"`
	TransportProtocol Transport `mapstructure:"transport"`
}

func ReadConfig(config *KuksaVISSClientConfig) {

	// Read in the configuration of the switcher
	log.Println("Reading Config ...")

	viper.SetConfigName("kuksa-viss-client") // name of config file (without extension)
	viper.AddConfigPath("./")                // path to look for the config file in

	viper.SetEnvPrefix("kuksa_viss_client")
	viper.AutomaticEnv()
	err := viper.ReadInConfig() // Find and read the config file
	if err != nil {             // Handle errors reading the config file
		log.Panicf("Fatal error config file: %s \n", err)
	}

	err = viper.Unmarshal(&config)
	if err != nil {
		log.Panicf("Unable to decode config into struct, %v", err)
	}
}

func (config KuksaVISSClientConfig) String() string {

	var retString string

	retString += "\nKuksa VISS Client Config\n"
	retString += "\tServer Address: " + config.ServerAddress + "\n"
	retString += "\tServer Port: " + config.ServerPort + "\n"
	retString += "\tInsecure: " + strconv.FormatBool(config.Insecure) + "\n"
	retString += "\tCertsDir: " + config.CertsDir + "\n"
	retString += "\tTokenPath: " + config.TokenPath + "\n"
	retString += "\tTransport Protocol: " + string(config.TransportProtocol) + "\n"

	return retString
}
