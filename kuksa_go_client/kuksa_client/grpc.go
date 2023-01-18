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
	"log"
	"strconv"
	"time"

	grpcpb "github.com/eclipse/kuksa.val/kuksa_go_client/proto/kuksa/val/v1"
	"github.com/google/uuid"
)

type FieldView struct {
	Field grpcpb.Field
	View  grpcpb.View
}

var dict map[string]FieldView

func (cg *KuksaClientCommGrpc) ConnectToKuksaVal() error {
	dict = map[string]FieldView{
		"value":       {grpcpb.Field_FIELD_VALUE, grpcpb.View_VIEW_CURRENT_VALUE},
		"targetValue": {grpcpb.Field_FIELD_ACTUATOR_TARGET, grpcpb.View_VIEW_TARGET_VALUE},
		"metadata":    {grpcpb.Field_FIELD_METADATA, grpcpb.View_VIEW_METADATA},
	}
	err := cg.startCommunication()
	return err
}

// function to get back go values to further work with them
func (cg *KuksaClientCommGrpc) GetGoValuesFromKuksaVal(path string, attr string) ([]*grpcpb.DataEntry, error) {
	// Contact the server and return the Value
	ctx, cancel := context.WithTimeout(context.Background(), time.Second)
	defer cancel()
	view := dict[attr].View
	fields := []grpcpb.Field{dict[attr].Field}
	entries := grpcpb.EntryRequest{Path: path, View: view, Fields: fields}
	request := []*grpcpb.EntryRequest{&entries}
	resp, err := cg.client.Get(ctx, &grpcpb.GetRequest{Entries: request})
	if err != nil {
		log.Fatalf("Get call failed: %v", err)
	}
	respEntries := resp.GetEntries()
	return respEntries, nil
}

// Function to get attribute value in string format from VSS tree
func (cg *KuksaClientCommGrpc) GetValueFromKuksaVal(path string, attr string) ([]string, error) {

	entries, err := cg.GetGoValuesFromKuksaVal(path, attr)
	values := []string{}
	for _, entry := range entries {
		values = append(values, entry.String())
	}

	return values, err
}

func (cg *KuksaClientCommGrpc) SetValueFromKuksaVal(path string, value string, attr string) error {
	// Contact the server and set the Value
	ctx, cancel := context.WithTimeout(context.Background(), time.Second)
	defer cancel()
	entries, getErr := cg.GetGoValuesFromKuksaVal(path, "metadata")
	if getErr != nil {
		return getErr
	}

	var datatype int32
	for _, entry := range entries {
		metadata := entry.GetMetadata()
		datatype = int32(metadata.GetDataType().Number())
	}

	// because of the lack of int8/16 and uint8/16 there are multiple times Datapoint_U/Int32 in the switch case
	// the switch case needs to be modified every time you add a new datapoint or datatype
	// the solution has been chosen because the compiler needs to know the type of a variable at runtime
	// second option would have been to have a setValue function for every type
	// this option has been not chosen because firstly the user needs to know what datatype his path requires (user exxperience low)
	// and secondly this would have been not less code only would have saved a little bit of runtime
	// there are maybe other options
	var datapoint grpcpb.Datapoint
	switch datatype {
	case 1:
		datapoint = grpcpb.Datapoint{Value: &grpcpb.Datapoint_String_{String_: value}}
	case 2:
		boolean, convErr := strconv.ParseBool(value)
		if convErr != nil {
			return convErr
		}
		datapoint = grpcpb.Datapoint{Value: &grpcpb.Datapoint_Bool{Bool: boolean}}
	case 3:
		int32_, convErr := strconv.ParseInt(value, 10, 8)
		if convErr != nil {
			return convErr
		}
		datapoint = grpcpb.Datapoint{Value: &grpcpb.Datapoint_Int32{Int32: int32(int32_)}}
	case 4:
		int32_, convErr := strconv.ParseInt(value, 10, 16)
		if convErr != nil {
			return convErr
		}
		datapoint = grpcpb.Datapoint{Value: &grpcpb.Datapoint_Int32{Int32: int32(int32_)}}
	case 5:
		int32_, convErr := strconv.ParseInt(value, 10, 32)
		if convErr != nil {
			return convErr
		}
		datapoint = grpcpb.Datapoint{Value: &grpcpb.Datapoint_Int32{Int32: int32(int32_)}}
	case 6:
		int64_, convErr := strconv.ParseInt(value, 10, 64)
		if convErr != nil {
			return convErr
		}
		datapoint = grpcpb.Datapoint{Value: &grpcpb.Datapoint_Int64{Int64: int64_}}
	case 7:
		uint32_, convErr := strconv.ParseUint(value, 10, 8)
		if convErr != nil {
			return convErr
		}
		datapoint = grpcpb.Datapoint{Value: &grpcpb.Datapoint_Uint32{Uint32: uint32(uint32_)}}
	case 8:
		uint32_, convErr := strconv.ParseUint(value, 10, 16)
		if convErr != nil {
			return convErr
		}
		datapoint = grpcpb.Datapoint{Value: &grpcpb.Datapoint_Uint32{Uint32: uint32(uint32_)}}
	case 9:
		uint32_, convErr := strconv.ParseUint(value, 10, 32)
		if convErr != nil {
			return convErr
		}
		datapoint = grpcpb.Datapoint{Value: &grpcpb.Datapoint_Uint32{Uint32: uint32(uint32_)}}
	case 10:
		uint64_, convErr := strconv.ParseUint(value, 10, 64)
		if convErr != nil {
			return convErr
		}
		datapoint = grpcpb.Datapoint{Value: &grpcpb.Datapoint_Uint64{Uint64: uint64_}}
	case 11:
		float_, convErr := strconv.ParseFloat(value, 32)
		if convErr != nil {
			return convErr
		}
		datapoint = grpcpb.Datapoint{Value: &grpcpb.Datapoint_Float{Float: float32(float_)}}
	case 12:
		float_, convErr := strconv.ParseFloat(value, 64)
		if convErr != nil {
			return convErr
		}
		datapoint = grpcpb.Datapoint{Value: &grpcpb.Datapoint_Double{Double: float_}}
	case 20:
		datapoint = grpcpb.Datapoint{Value: &grpcpb.Datapoint_StringArray{StringArray: &grpcpb.StringArray{Values: []string{value}}}}
	case 21:
		boolean, convErr := strconv.ParseBool(value)
		if convErr != nil {
			return convErr
		}
		datapoint = grpcpb.Datapoint{Value: &grpcpb.Datapoint_BoolArray{BoolArray: &grpcpb.BoolArray{Values: []bool{boolean}}}}
	case 22:
		int32_, convErr := strconv.ParseInt(value, 10, 8)
		if convErr != nil {
			return convErr
		}
		datapoint = grpcpb.Datapoint{Value: &grpcpb.Datapoint_Int32Array{Int32Array: &grpcpb.Int32Array{Values: []int32{int32(int32_)}}}}
	case 23:
		int32_, convErr := strconv.ParseInt(value, 10, 16)
		if convErr != nil {
			return convErr
		}
		datapoint = grpcpb.Datapoint{Value: &grpcpb.Datapoint_Int32Array{Int32Array: &grpcpb.Int32Array{Values: []int32{int32(int32_)}}}}
	case 24:
		int32_, convErr := strconv.ParseInt(value, 10, 32)
		if convErr != nil {
			return convErr
		}
		datapoint = grpcpb.Datapoint{Value: &grpcpb.Datapoint_Int32Array{Int32Array: &grpcpb.Int32Array{Values: []int32{int32(int32_)}}}}
	case 25:
		int64_, convErr := strconv.ParseInt(value, 10, 32)
		if convErr != nil {
			return convErr
		}
		datapoint = grpcpb.Datapoint{Value: &grpcpb.Datapoint_Int64Array{Int64Array: &grpcpb.Int64Array{Values: []int64{int64_}}}}
	case 26:
		uint32_, convErr := strconv.ParseUint(value, 10, 8)
		if convErr != nil {
			return convErr
		}
		datapoint = grpcpb.Datapoint{Value: &grpcpb.Datapoint_Uint32Array{Uint32Array: &grpcpb.Uint32Array{Values: []uint32{uint32(uint32_)}}}}
	case 27:
		uint32_, convErr := strconv.ParseUint(value, 10, 16)
		if convErr != nil {
			return convErr
		}
		datapoint = grpcpb.Datapoint{Value: &grpcpb.Datapoint_Uint32Array{Uint32Array: &grpcpb.Uint32Array{Values: []uint32{uint32(uint32_)}}}}
	case 28:
		uint32_, convErr := strconv.ParseUint(value, 10, 32)
		if convErr != nil {
			return convErr
		}
		datapoint = grpcpb.Datapoint{Value: &grpcpb.Datapoint_Uint32Array{Uint32Array: &grpcpb.Uint32Array{Values: []uint32{uint32(uint32_)}}}}
	case 29:
		uint64_, convErr := strconv.ParseUint(value, 10, 64)
		if convErr != nil {
			return convErr
		}
		datapoint = grpcpb.Datapoint{Value: &grpcpb.Datapoint_Uint64Array{Uint64Array: &grpcpb.Uint64Array{Values: []uint64{uint64_}}}}
	case 30:
		float_, convErr := strconv.ParseFloat(value, 32)
		if convErr != nil {
			return convErr
		}
		datapoint = grpcpb.Datapoint{Value: &grpcpb.Datapoint_FloatArray{FloatArray: &grpcpb.FloatArray{Values: []float32{float32(float_)}}}}
	case 31:
		float_, convErr := strconv.ParseFloat(value, 64)
		if convErr != nil {
			return convErr
		}
		datapoint = grpcpb.Datapoint{Value: &grpcpb.Datapoint_DoubleArray{DoubleArray: &grpcpb.DoubleArray{Values: []float64{float_}}}}
	}

	fields := []grpcpb.Field{dict[attr].Field}
	var entry grpcpb.DataEntry
	if attr == "value" {
		entry = grpcpb.DataEntry{Path: path, Value: &datapoint}
	} else if attr == "targetValue" {
		entry = grpcpb.DataEntry{Path: path, ActuatorTarget: &datapoint}
	}

	requestargs := []*grpcpb.EntryUpdate{}
	requestargs = append(requestargs, &grpcpb.EntryUpdate{Entry: &entry, Fields: fields})
	_, err := cg.client.Set(ctx, &grpcpb.SetRequest{Updates: requestargs})
	return err
}

func (cg *KuksaClientCommGrpc) PrintSubscriptionMessages(id string) error {
	for {
		channel := *cg.subsChannel[id]
		resp, err := channel.Recv()
		if err != nil {
			return err
		}
		updates := resp.GetUpdates()
		log.Println(updates)

		for _, update := range updates {
			var val string
			entry := update.GetEntry()
			switch cg.subsAttr {
			case "value":
				val = entry.GetValue().String()
			case "targetValue":
				val = entry.GetActuatorTarget().String()
			case "metadata":
				val = entry.GetMetadata().String()
			default:
				continue
			}
			log.Printf("Vehicle.Speed Subscribed: %s", val)
		}
	}
}

func (cg *KuksaClientCommGrpc) SubscribeFromKuksaVal(path string, attr string) (string, error) {
	// Contact the server and return SubscriptionId
	cg.subsAttr = attr
	ctx, cancel := context.WithCancel(context.Background())
	id := uuid.New().String()
	cg.cancel[id] = &cancel
	view := dict[attr].View
	fields := []grpcpb.Field{dict[attr].Field}
	subEntry := []*grpcpb.SubscribeEntry{{Path: path, View: view, Fields: fields}}
	subReq := grpcpb.SubscribeRequest{Entries: subEntry}
	client, err := cg.client.Subscribe(ctx, &subReq)
	if err != nil {
		return "", err
	}
	cg.subscriptionStateMutex.Lock()
	cg.subsChannel[id] = &client
	cg.subscriptionStateMutex.Unlock()
	return id, nil
}

func (cg *KuksaClientCommGrpc) UnsubscribeFromKuksaVal(id string) error {
	cancel := *cg.cancel[id]
	cancel()
	client := *cg.subsChannel[id]
	client.CloseSend()

	return nil
}

// Function to authorize to kuksa.val server
func (cg *KuksaClientCommGrpc) AuthorizeKuksaValConn() error {
	log.Println("KUKSA.val databroker does not support authorization mechanism yet. If you want to use authorization please use KUKSA.val server")
	return nil
}

// Function to get metadata from kuksa.val server
func (cg *KuksaClientCommGrpc) GetMetadataFromKuksaVal(path string) ([]string, error) {
	metadata, err := cg.GetValueFromKuksaVal(path, "metadata")
	if err != nil {
		return []string{""}, err
	}
	return metadata, nil
}
