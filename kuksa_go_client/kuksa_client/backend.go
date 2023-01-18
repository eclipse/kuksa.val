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

type KuksaBackend interface {
	ConnectToKuksaVal() error
	Close()
	AuthorizeKuksaValConn() error
	GetValueFromKuksaVal(path string, attr string) ([]string, error)
	SetValueFromKuksaVal(path string, value string, attr string) error
	SubscribeFromKuksaVal(path string, attr string) (string, error)
	UnsubscribeFromKuksaVal(id string) error
	PrintSubscriptionMessages(id string) error
	GetMetadataFromKuksaVal(path string) ([]string, error)
}
