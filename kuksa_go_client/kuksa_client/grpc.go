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
	"errors"
	"log"
	"os"
	"reflect"
	"regexp"
	"strconv"
	"strings"
	"time"

	"github.com/google/uuid"
	"google.golang.org/grpc/metadata"

	grpcpb "github.com/eclipse/kuksa.val/kuksa_go_client/proto/kuksa/val/v1"
	v1 "github.com/eclipse/kuksa.val/kuksa_go_client/proto/kuksa/val/v1"
)

type FieldView struct {
	Field grpcpb.Field
	View  grpcpb.View
}

type ParseError struct{}

func (e ParseError) Error() string {
	return "parse error"
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

// Function to get attribute value from databroker
func (cg *KuksaClientCommGrpc) GetValueFromKuksaVal(path string, attr string) ([]interface{}, error) {

	// Contact the server and return the Value
	ctx, cancel := context.WithTimeout(context.Background(), time.Second)
	ctx = metadata.AppendToOutgoingContext(ctx, "Authorization", cg.authorizationHeader)
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
	response := make([]interface{}, len(respEntries))
	for i, entry := range respEntries {
		response[i] = interface{}(entry)
	}
	return response, nil
}

func GetArrayFromInput[T any](input string) ([]T, error) {
	// Strip the brackets from the input
	input = strings.TrimSuffix(strings.TrimPrefix(input, "["), "]")
	
	// Split the input string into separate values
	// First alternative, not quotes including escaped double quote, ends at comma, single/double quote or EOL
	// Second group is double quoted string, may contain double quoted strings inside, ends at non-escaped
	// double quote
	pattern := `(?:\\"|\\'|[^"',])+|"(?:\\"|[^"])*"|'(?:\\'|[^'])*'`

	r := regexp.MustCompile(pattern)
	values := r.FindAllString(input, -1)

	// Parse each value as type T and append to the array
	array := make([]T, 0)
	for _, v := range values {

		v = strings.TrimSpace(v)
		var consider = len(v) > 0
		// make the ' or " disappear
		if len(v) > 1 && (v[0] == '"') && (v[len(v)-1] == '"')  {
			v = v[1:len(v)-1]
		}
		if len(v) > 1 && (v[0] == '\'') && (v[len(v)-1] == '\'')  {
			v = v[1:len(v)-1]
		}
		v = strings.ReplaceAll(v,"\\\"", "\"")
		v = strings.ReplaceAll(v,"\\'", "'")
		if consider {
			value, err := parseValue[T](v)
			if err != nil {
				return nil, ParseError{}
			}
			array = append(array,value)
		}
	}

	return array, nil
}

func parseValue[T any](value string) (T, error) {
	t := reflect.TypeOf((*T)(nil)).Elem()
	switch t.Kind() {
	case reflect.Bool:
		parsed, err := strconv.ParseBool(strings.TrimSpace(value))
		if err != nil {
			return reflect.Zero(t).Interface().(T), err
		}
		v := reflect.New(t).Elem().Interface().(T)
		reflect.ValueOf(&v).Elem().SetBool(parsed)
		return v, nil
	case reflect.Int32:
		parsed, err := strconv.ParseInt(strings.TrimSpace(value), 10, 32)
		if err != nil {
			return reflect.Zero(t).Interface().(T), err
		}
		v := reflect.New(t).Elem().Interface().(T)
		reflect.ValueOf(&v).Elem().SetInt(parsed)
		return v, nil
	case reflect.Int64:
		parsed, err := strconv.ParseInt(strings.TrimSpace(value), 10, 64)
		if err != nil {
			return reflect.Zero(t).Interface().(T), err
		}
		v := reflect.New(t).Elem().Interface().(T)
		reflect.ValueOf(&v).Elem().SetInt(parsed)
		return v, nil
	case reflect.Float32:
		parsed, err := strconv.ParseFloat(strings.TrimSpace(value), 32)
		if err != nil {
			return reflect.Zero(t).Interface().(T), err
		}
		v := reflect.New(t).Elem().Interface().(T)
		reflect.ValueOf(&v).Elem().SetFloat(parsed)
		return v, nil
	case reflect.Float64:
		parsed, err := strconv.ParseFloat(strings.TrimSpace(value), 64)
		if err != nil {
			return reflect.Zero(t).Interface().(T), err
		}
		v := reflect.New(t).Elem().Interface().(T)
		reflect.ValueOf(&v).Elem().SetFloat(parsed)
		return v, nil
	case reflect.Uint32:
		parsed, err := strconv.ParseUint(strings.TrimSpace(value), 10, 32)
		if err != nil {
			return reflect.Zero(t).Interface().(T), err
		}
		v := reflect.New(t).Elem().Interface().(T)
		reflect.ValueOf(&v).Elem().SetUint(parsed)
		return v, nil
	case reflect.Uint64:
		parsed, err := strconv.ParseUint(strings.TrimSpace(value), 10, 64)
		if err != nil {
			return reflect.Zero(t).Interface().(T), err
		}
		v := reflect.New(t).Elem().Interface().(T)
		reflect.ValueOf(&v).Elem().SetUint(parsed)
		return v, nil
	case reflect.String:
		v := reflect.New(t).Elem().Interface().(T)
		reflect.ValueOf(&v).Elem().SetString(value)
		return v, nil
	default:
		return reflect.Zero(t).Interface().(T), errors.New("unsupported type")
	}
}

func (cg *KuksaClientCommGrpc) SetValueFromKuksaVal(path string, value string, attr string) error {
	// Contact the server and set the Value
	ctx, cancel := context.WithTimeout(context.Background(), time.Second)
	ctx = metadata.AppendToOutgoingContext(ctx, "Authorization", cg.authorizationHeader)
	defer cancel()
	entries, getErr := cg.GetValueFromKuksaVal(path, "metadata")
	if getErr != nil {
		return getErr
	}

	var datatype int32
	for _, entry := range entries {
		metadata := entry.(*v1.DataEntry).GetMetadata()
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
		array, err := GetArrayFromInput[string](value)
		if err != nil {
			return err
		}
		datapoint = grpcpb.Datapoint{Value: &grpcpb.Datapoint_StringArray{StringArray: &grpcpb.StringArray{Values: array}}}
	case 21:
		array, err := GetArrayFromInput[bool](value)
		if err != nil {
			return err
		}
		datapoint = grpcpb.Datapoint{Value: &grpcpb.Datapoint_BoolArray{BoolArray: &grpcpb.BoolArray{Values: array}}}
	case 22:
		array, err := GetArrayFromInput[int32](value)
		if err != nil {
			return err
		}
		datapoint = grpcpb.Datapoint{Value: &grpcpb.Datapoint_Int32Array{Int32Array: &grpcpb.Int32Array{Values: array}}}
	case 23:
		array, err := GetArrayFromInput[int32](value)
		if err != nil {
			return err
		}
		datapoint = grpcpb.Datapoint{Value: &grpcpb.Datapoint_Int32Array{Int32Array: &grpcpb.Int32Array{Values: array}}}
	case 24:
		array, err := GetArrayFromInput[int32](value)
		if err != nil {
			return err
		}
		datapoint = grpcpb.Datapoint{Value: &grpcpb.Datapoint_Int32Array{Int32Array: &grpcpb.Int32Array{Values: array}}}
	case 25:
		array, err := GetArrayFromInput[int64](value)
		if err != nil {
			return err
		}
		datapoint = grpcpb.Datapoint{Value: &grpcpb.Datapoint_Int64Array{Int64Array: &grpcpb.Int64Array{Values: array}}}
	case 26:
		array, err := GetArrayFromInput[uint32](value)
		if err != nil {
			return err
		}
		datapoint = grpcpb.Datapoint{Value: &grpcpb.Datapoint_Uint32Array{Uint32Array: &grpcpb.Uint32Array{Values: array}}}
	case 27:
		array, err := GetArrayFromInput[uint32](value)
		if err != nil {
			return err
		}
		datapoint = grpcpb.Datapoint{Value: &grpcpb.Datapoint_Uint32Array{Uint32Array: &grpcpb.Uint32Array{Values: array}}}
	case 28:
		array, err := GetArrayFromInput[uint32](value)
		if err != nil {
			return err
		}
		datapoint = grpcpb.Datapoint{Value: &grpcpb.Datapoint_Uint32Array{Uint32Array: &grpcpb.Uint32Array{Values: array}}}
	case 29:
		array, err := GetArrayFromInput[uint64](value)
		if err != nil {
			return err
		}
		datapoint = grpcpb.Datapoint{Value: &grpcpb.Datapoint_Uint64Array{Uint64Array: &grpcpb.Uint64Array{Values: array}}}
	case 30:
		array, err := GetArrayFromInput[float32](value)
		if err != nil {
			return err
		}
		datapoint = grpcpb.Datapoint{Value: &grpcpb.Datapoint_FloatArray{FloatArray: &grpcpb.FloatArray{Values: array}}}
	case 31:
		array, err := GetArrayFromInput[float64](value)
		if err != nil {
			return err
		}
		datapoint = grpcpb.Datapoint{Value: &grpcpb.Datapoint_DoubleArray{DoubleArray: &grpcpb.DoubleArray{Values: array}}}
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
	ctx = metadata.AppendToOutgoingContext(ctx, "Authorization", cg.authorizationHeader)
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
func (cg *KuksaClientCommGrpc) AuthorizeKuksaValConn(TokenOrTokenfile string) error {
	var tokenString string
	// Data in config has precedence over hardcoded token
	if cg.Config.TokenOrTokenfile != ""{
		tokenString = cg.Config.TokenOrTokenfile
	}else{
		if TokenOrTokenfile == "" {
			return errors.New("No token given!")
		}
		tokenString = TokenOrTokenfile
	}
	
	log.Printf("Using token: %s", tokenString)

	info, err := os.Stat(tokenString)
	if err != nil {
		// the TokenOrTokenfile is read like its a token
	} else if info.Mode().IsRegular() {
		tokenByteString, err := os.ReadFile(tokenString)
		tokenString = string(tokenByteString)
		if err != nil {
			log.Fatal("Error reading token: ", err)
			return err
		}
	}
	cg.authorizationHeader = "Bearer " + tokenString
	return nil
}

// Function to get metadata from kuksa.val server
func (cg *KuksaClientCommGrpc) GetMetadataFromKuksaVal(path string) ([]interface{}, error) {
	metadata, err := cg.GetValueFromKuksaVal(path, "metadata")
	if err != nil {
		metadata = []interface{}{}
		return metadata, err
	}
	return metadata, nil
}
