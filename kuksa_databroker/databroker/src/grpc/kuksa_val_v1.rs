use std::collections::HashSet;
use std::pin::Pin;
use std::time::SystemTime;

use databroker_api::kuksa::val::v1 as proto;
use tokio_stream::Stream;
use tokio_stream::StreamExt;
use tracing::debug;

use crate::broker;
use crate::types;
// use crate::types::{ChangeType, DataType, DataValue};

#[tonic::async_trait]
impl proto::val_server::Val for broker::DataBroker {
    async fn get(
        &self,
        request: tonic::Request<proto::GetRequest>,
    ) -> Result<tonic::Response<proto::GetResponse>, tonic::Status> {
        let requested = request.into_inner().entries;
        if requested.is_empty() {
            Err(tonic::Status::new(
                tonic::Code::InvalidArgument,
                "No datapoints requested".to_string(),
            ))
        } else {
            let mut entries = Vec::new();
            let mut errors = Vec::new();

            for request in requested {
                match self.get_entry_by_path(&request.path).await {
                    Some(entry) => {
                        let view = match proto::View::from_i32(request.view) {
                            Some(view) => view,
                            None => {
                                return Err(tonic::Status::invalid_argument(format!(
                                    "Invalid View (id: {}",
                                    request.view
                                )))
                            }
                        };
                        let fields = {
                            let mut fields = vec![];
                            for id in request.fields {
                                match proto::Field::from_i32(id) {
                                    Some(field) => fields.push(field),
                                    None => {
                                        return Err(tonic::Status::invalid_argument(format!(
                                            "Invalid Field (id: {})",
                                            id
                                        )))
                                    }
                                };
                            }
                            fields
                        };
                        let fields = combine_view_and_fields(view, fields);
                        debug!("Getting fields: {:?}", fields);
                        let proto_entry = proto_entry_from_entry_and_fields(entry, fields);
                        debug!("Getting datapoint: {:?}", proto_entry);
                        entries.push(proto_entry);
                    }
                    None => {
                        // Entry doesn't exist
                        let message = format!("Path '{}' not found", &request.path);
                        errors.push(proto::DataEntryError {
                            path: request.path,
                            error: Some(proto::Error {
                                code: 404,
                                reason: "not_found".to_owned(),
                                message,
                            }),
                        });
                    }
                }
            }

            // Not sure how to handle the "global error".
            // Fall back to just use the first path specific error if any
            let error = match errors.get(0) {
                Some(first) => first.error.clone(),
                None => None,
            };

            let response = proto::GetResponse {
                entries,
                errors,
                error,
            };
            Ok(tonic::Response::new(response))
        }
    }

    async fn set(
        &self,
        request: tonic::Request<proto::SetRequest>,
    ) -> Result<tonic::Response<proto::SetResponse>, tonic::Status> {
        // Collect errors encountered
        let mut errors = Vec::new();

        let mut updates = Vec::<(i32, broker::EntryUpdate)>::new();
        for request in request.into_inner().updates {
            match self.get_id_by_path(&request.path).await {
                Some(id) => {
                    let fields = {
                        let mut fields = HashSet::new();
                        for id in request.fields {
                            match proto::Field::from_i32(id) {
                                Some(field) => fields.insert(field),
                                None => {
                                    return Err(tonic::Status::invalid_argument(format!(
                                        "Invalid Field (id: {})",
                                        id
                                    )))
                                }
                            };
                        }
                        fields
                    };

                    let entry = match &request.entry {
                        Some(entry) => entry,
                        None => {
                            return Err(tonic::Status::invalid_argument("Empty entry".to_string()))
                        }
                    };
                    debug!("Settings fields: {:?}", fields);
                    let update = broker::EntryUpdate::from_proto_entry(entry, fields);
                    updates.push((id, update));
                }
                None => {
                    let message = format!("{} not found", request.path);
                    errors.push(proto::DataEntryError {
                        path: request.path,
                        error: Some(proto::Error {
                            code: 404,
                            reason: "not_found".to_string(),
                            message,
                        }),
                    })
                }
            }
        }

        match self.update_entries(updates).await {
            Ok(()) => {}
            Err(err) => {
                debug!("Failed to set datapoint: {:?}", err);
                // errors = err
                //     .iter()
                //     .map(|(id, error)| (*id, proto::DataEntryError::from(error) as i32))
                //     .collect();
            }
        }

        Ok(tonic::Response::new(proto::SetResponse {
            error: None,
            errors,
        }))
    }

    type SubscribeStream = Pin<
        Box<
            dyn Stream<Item = Result<proto::SubscribeResponse, tonic::Status>>
                + Send
                + Sync
                + 'static,
        >,
    >;

    async fn subscribe(
        &self,
        request: tonic::Request<proto::SubscribeRequest>,
    ) -> Result<tonic::Response<Self::SubscribeStream>, tonic::Status> {
        let request = request.into_inner();
        let mut paths = Vec::new();
        let mut fields = HashSet::new();
        for entry in request.entries {
            paths.push(entry.path.clone());
            for id in entry.fields {
                match proto::Field::from_i32(id) {
                    Some(field) => match field {
                        proto::Field::Value => {
                            fields.insert(broker::Field::Value);
                        }
                        proto::Field::ActuatorTarget => {
                            fields.insert(broker::Field::ActuatorTarget);
                        }
                        _ => {
                            // Just ignore other fields for now
                        }
                    },
                    None => {
                        return Err(tonic::Status::invalid_argument(format!(
                            "Invalid Field (id: {})",
                            id
                        )))
                    }
                };
            }
        }
        match self.subscribe(paths, fields).await {
            Ok(stream) => {
                let stream = convert_to_proto_stream(stream);
                debug!("Subscribed to new query");
                Ok(tonic::Response::new(Box::pin(stream)))
            }
            Err(e) => Err(tonic::Status::new(
                tonic::Code::InvalidArgument,
                format!("{:?}", e),
            )),
        }
    }

    async fn get_server_info(
        &self,
        _request: tonic::Request<proto::GetServerInfoRequest>,
    ) -> Result<tonic::Response<proto::GetServerInfoResponse>, tonic::Status> {
        let server_info = proto::GetServerInfoResponse {
            name: "databroker".to_owned(),
            version: "0.17.0".to_owned(),
        };
        Ok(tonic::Response::new(server_info))
    }
}

fn convert_to_proto_stream(
    input: impl Stream<Item = broker::ChangeNotificationResponse>,
) -> impl Stream<Item = Result<proto::SubscribeResponse, tonic::Status>> {
    input.map(move |item| {
        let mut notifications = Vec::new();
        for notification in item.notifications {
            notifications.push(proto::ChangeNotification {
                entry: Some(proto_entry_from_update(notification.update)),
                fields: proto_fields_from_fields(notification.fields),
            });
        }
        let response = proto::SubscribeResponse { notifications };
        Ok(response)
    })
}

fn proto_entry_from_entry_and_fields(
    entry: broker::Entry,
    fields: HashSet<proto::Field>,
) -> proto::DataEntry {
    let path = entry.metadata.path;
    let value = match fields.contains(&proto::Field::Value) {
        true => proto_datapoint_from_datapoint(entry.datapoint),
        false => None,
    };
    let actuator_target = match fields.contains(&proto::Field::ActuatorTarget) {
        true => match entry.actuator_target {
            Some(actuator_target) => proto_datapoint_from_value(actuator_target),
            None => None,
        },
        false => None,
    };
    let metadata = {
        let mut metadata = proto::Metadata::default();
        let mut metadata_is_set = false;

        let all = fields.contains(&proto::Field::Metadata);

        if all || fields.contains(&proto::Field::MetadataDataType) {
            metadata_is_set = true;
            metadata.data_type = match entry.metadata.data_type {
                broker::DataType::String => Some(proto::DataType::String as i32),
                broker::DataType::Bool => Some(proto::DataType::Boolean as i32),
                broker::DataType::Int8 => Some(proto::DataType::Int8 as i32),
                broker::DataType::Int16 => Some(proto::DataType::Int16 as i32),
                broker::DataType::Int32 => Some(proto::DataType::Int32 as i32),
                broker::DataType::Int64 => Some(proto::DataType::Int64 as i32),
                broker::DataType::Uint8 => Some(proto::DataType::Uint8 as i32),
                broker::DataType::Uint16 => Some(proto::DataType::Uint16 as i32),
                broker::DataType::Uint32 => Some(proto::DataType::Uint32 as i32),
                broker::DataType::Uint64 => Some(proto::DataType::Uint64 as i32),
                broker::DataType::Float => Some(proto::DataType::Float as i32),
                broker::DataType::Double => Some(proto::DataType::Double as i32),
                broker::DataType::Timestamp => Some(proto::DataType::Timestamp as i32),
                broker::DataType::StringArray => Some(proto::DataType::StringArray as i32),
                broker::DataType::BoolArray => Some(proto::DataType::BooleanArray as i32),
                broker::DataType::Int8Array => Some(proto::DataType::Int8Array as i32),
                broker::DataType::Int16Array => Some(proto::DataType::Int16Array as i32),
                broker::DataType::Int32Array => Some(proto::DataType::Int32Array as i32),
                broker::DataType::Int64Array => Some(proto::DataType::Int64Array as i32),
                broker::DataType::Uint8Array => Some(proto::DataType::Uint8Array as i32),
                broker::DataType::Uint16Array => Some(proto::DataType::Uint16Array as i32),
                broker::DataType::Uint32Array => Some(proto::DataType::Uint32Array as i32),
                broker::DataType::Uint64Array => Some(proto::DataType::Uint64Array as i32),
                broker::DataType::FloatArray => Some(proto::DataType::FloatArray as i32),
                broker::DataType::DoubleArray => Some(proto::DataType::DoubleArray as i32),
                broker::DataType::TimestampArray => Some(proto::DataType::TimestampArray as i32),
            };
        }
        if all || fields.contains(&proto::Field::MetadataDescription) {
            metadata_is_set = true;
            metadata.description = Some(entry.metadata.description);
        }
        if all || fields.contains(&proto::Field::MetadataEntryType) {
            metadata_is_set = true;
            metadata.entry_type = match entry.metadata.entry_type {
                broker::EntryType::Sensor => Some(proto::EntryType::Sensor as i32),
                broker::EntryType::Attribute => Some(proto::EntryType::Attribute as i32),
                broker::EntryType::Actuator => Some(proto::EntryType::Actuator as i32),
            }
        }
        if all || fields.contains(&proto::Field::MetadataComment) {
            metadata_is_set = true;
            // TODO: Add to Metadata
            metadata.comment = Some("".to_string())
        }
        if all || fields.contains(&proto::Field::MetadataDeprecation) {
            metadata_is_set = true;
            // TODO: Add to Metadata
            metadata.deprecation = Some("".to_string());
        }
        if all || fields.contains(&proto::Field::MetadataUnit) {
            metadata_is_set = true;
            // TODO: Add to Metadata
            metadata.unit = Some("".to_string());
        }
        if all || fields.contains(&proto::Field::MetadataValueRestriction) {
            metadata_is_set = true;
            // TODO: Add to Metadata
        }
        if all || fields.contains(&proto::Field::MetadataActuator) {
            metadata_is_set = true;
            // TODO: Add to Metadata
            metadata.entry_specific = match entry.metadata.entry_type {
                broker::EntryType::Actuator => Some(proto::metadata::EntrySpecific::Actuator(
                    proto::Actuator::default(),
                )),
                broker::EntryType::Sensor | broker::EntryType::Attribute => None,
            };
        }
        if all || fields.contains(&proto::Field::MetadataSensor) {
            metadata_is_set = true;
            // TODO: Add to Metadata
            metadata.entry_specific = match entry.metadata.entry_type {
                broker::EntryType::Sensor => Some(proto::metadata::EntrySpecific::Sensor(
                    proto::Sensor::default(),
                )),
                broker::EntryType::Attribute | broker::EntryType::Actuator => None,
            };
        }
        if all || fields.contains(&proto::Field::MetadataAttribute) {
            metadata_is_set = true;
            // TODO: Add to Metadata
            metadata.entry_specific = match entry.metadata.entry_type {
                broker::EntryType::Attribute => Some(proto::metadata::EntrySpecific::Attribute(
                    proto::Attribute::default(),
                )),
                broker::EntryType::Sensor | broker::EntryType::Actuator => None,
            };
        }

        if metadata_is_set {
            Some(metadata)
        } else {
            None
        }
    };
    proto::DataEntry {
        path,
        value,
        actuator_target,
        metadata,
    }
}

fn combine_view_and_fields(
    view: proto::View,
    fields: impl IntoIterator<Item = proto::Field>,
) -> HashSet<proto::Field> {
    let mut combined = HashSet::new();

    combined.extend(fields);

    match view {
        proto::View::Unspecified => {
            // If no fields are specified, View::Unspecified will
            // default to the equivalent of View::CurrentValue
            if combined.is_empty() {
                combined.insert(proto::Field::Path);
                combined.insert(proto::Field::Value);
            }
        }
        proto::View::CurrentValue => {
            combined.insert(proto::Field::Path);
            combined.insert(proto::Field::Value);
        }
        proto::View::TargetValue => {
            combined.insert(proto::Field::Path);
            combined.insert(proto::Field::ActuatorTarget);
        }
        proto::View::Metadata => {
            combined.insert(proto::Field::Path);
            combined.insert(proto::Field::Metadata);
        }
        proto::View::Fields => {}
        proto::View::All => {
            combined.insert(proto::Field::Path);
            combined.insert(proto::Field::Value);
            combined.insert(proto::Field::ActuatorTarget);
            combined.insert(proto::Field::Metadata);
        }
    }

    combined
}

fn value_from_proto_value(value: Option<proto::datapoint::Value>) -> broker::DataValue {
    match value {
        Some(value) => match value {
            proto::datapoint::Value::String(value) => types::DataValue::String(value),
            proto::datapoint::Value::Bool(value) => types::DataValue::Bool(value),
            proto::datapoint::Value::Int32(value) => types::DataValue::Int32(value),
            proto::datapoint::Value::Int64(value) => types::DataValue::Int64(value),
            proto::datapoint::Value::Uint32(value) => types::DataValue::Uint32(value),
            proto::datapoint::Value::Uint64(value) => types::DataValue::Uint64(value),
            proto::datapoint::Value::Float(value) => types::DataValue::Float(value),
            proto::datapoint::Value::Double(value) => types::DataValue::Double(value),
            proto::datapoint::Value::StringArray(array) => {
                types::DataValue::StringArray(array.values)
            }
            proto::datapoint::Value::BoolArray(array) => types::DataValue::BoolArray(array.values),
            proto::datapoint::Value::Int32Array(array) => {
                types::DataValue::Int32Array(array.values)
            }
            proto::datapoint::Value::Int64Array(array) => {
                types::DataValue::Int64Array(array.values)
            }
            proto::datapoint::Value::Uint32Array(array) => {
                types::DataValue::Uint32Array(array.values)
            }
            proto::datapoint::Value::Uint64Array(array) => {
                types::DataValue::Uint64Array(array.values)
            }
            proto::datapoint::Value::FloatArray(array) => {
                types::DataValue::FloatArray(array.values)
            }
            proto::datapoint::Value::DoubleArray(array) => {
                types::DataValue::DoubleArray(array.values)
            }
        },
        None => types::DataValue::NotAvailable,
    }
}

fn datapoint_from_proto_datapoint(datapoint: proto::Datapoint) -> broker::Datapoint {
    let value = value_from_proto_value(datapoint.value);

    broker::Datapoint {
        ts: match datapoint.timestamp {
            Some(ts) => match std::convert::TryInto::try_into(ts) {
                Ok(ts) => ts,
                Err(_) => SystemTime::now(),
            },
            None => SystemTime::now(),
        },
        value,
    }
}

fn proto_datapoint_from_datapoint(datapoint: broker::Datapoint) -> Option<proto::Datapoint> {
    match datapoint.value {
        types::DataValue::NotAvailable => None,
        types::DataValue::Bool(value) => Some(proto::Datapoint {
            value: Some(proto::datapoint::Value::Bool(value)),
            timestamp: Some(datapoint.ts.into()),
        }),
        types::DataValue::String(value) => Some(proto::Datapoint {
            value: Some(proto::datapoint::Value::String(value)),
            timestamp: Some(datapoint.ts.into()),
        }),
        types::DataValue::Int32(value) => Some(proto::Datapoint {
            value: Some(proto::datapoint::Value::Int32(value)),
            timestamp: Some(datapoint.ts.into()),
        }),
        types::DataValue::Int64(value) => Some(proto::Datapoint {
            value: Some(proto::datapoint::Value::Int64(value)),
            timestamp: Some(datapoint.ts.into()),
        }),
        types::DataValue::Uint32(value) => Some(proto::Datapoint {
            value: Some(proto::datapoint::Value::Uint32(value)),
            timestamp: Some(datapoint.ts.into()),
        }),
        types::DataValue::Uint64(value) => Some(proto::Datapoint {
            value: Some(proto::datapoint::Value::Uint64(value)),
            timestamp: Some(datapoint.ts.into()),
        }),
        types::DataValue::Float(value) => Some(proto::Datapoint {
            value: Some(proto::datapoint::Value::Float(value)),
            timestamp: Some(datapoint.ts.into()),
        }),
        types::DataValue::Double(value) => Some(proto::Datapoint {
            value: Some(proto::datapoint::Value::Double(value)),
            timestamp: Some(datapoint.ts.into()),
        }),
        types::DataValue::BoolArray(values) => Some(proto::Datapoint {
            value: Some(proto::datapoint::Value::BoolArray(proto::BoolArray {
                values,
            })),
            timestamp: Some(datapoint.ts.into()),
        }),
        types::DataValue::StringArray(values) => Some(proto::Datapoint {
            value: Some(proto::datapoint::Value::StringArray(proto::StringArray {
                values,
            })),
            timestamp: Some(datapoint.ts.into()),
        }),
        types::DataValue::Int32Array(values) => Some(proto::Datapoint {
            value: Some(proto::datapoint::Value::Int32Array(proto::Int32Array {
                values,
            })),
            timestamp: Some(datapoint.ts.into()),
        }),
        types::DataValue::Int64Array(values) => Some(proto::Datapoint {
            value: Some(proto::datapoint::Value::Int64Array(proto::Int64Array {
                values,
            })),
            timestamp: Some(datapoint.ts.into()),
        }),
        types::DataValue::Uint32Array(values) => Some(proto::Datapoint {
            value: Some(proto::datapoint::Value::Uint32Array(proto::Uint32Array {
                values,
            })),
            timestamp: Some(datapoint.ts.into()),
        }),
        types::DataValue::Uint64Array(values) => Some(proto::Datapoint {
            value: Some(proto::datapoint::Value::Uint64Array(proto::Uint64Array {
                values,
            })),
            timestamp: Some(datapoint.ts.into()),
        }),
        types::DataValue::FloatArray(values) => Some(proto::Datapoint {
            value: Some(proto::datapoint::Value::FloatArray(proto::FloatArray {
                values,
            })),
            timestamp: Some(datapoint.ts.into()),
        }),
        types::DataValue::DoubleArray(values) => Some(proto::Datapoint {
            value: Some(proto::datapoint::Value::DoubleArray(proto::DoubleArray {
                values,
            })),
            timestamp: Some(datapoint.ts.into()),
        }),
    }
}

fn proto_datapoint_from_value(value: broker::DataValue) -> Option<proto::Datapoint> {
    match value {
        types::DataValue::NotAvailable => None,
        types::DataValue::Bool(value) => Some(proto::Datapoint {
            value: Some(proto::datapoint::Value::Bool(value)),
            timestamp: None,
        }),
        types::DataValue::String(value) => Some(proto::Datapoint {
            value: Some(proto::datapoint::Value::String(value)),
            timestamp: None,
        }),
        types::DataValue::Int32(value) => Some(proto::Datapoint {
            value: Some(proto::datapoint::Value::Int32(value)),
            timestamp: None,
        }),
        types::DataValue::Int64(value) => Some(proto::Datapoint {
            value: Some(proto::datapoint::Value::Int64(value)),
            timestamp: None,
        }),
        types::DataValue::Uint32(value) => Some(proto::Datapoint {
            value: Some(proto::datapoint::Value::Uint32(value)),
            timestamp: None,
        }),
        types::DataValue::Uint64(value) => Some(proto::Datapoint {
            value: Some(proto::datapoint::Value::Uint64(value)),
            timestamp: None,
        }),
        types::DataValue::Float(value) => Some(proto::Datapoint {
            value: Some(proto::datapoint::Value::Float(value)),
            timestamp: None,
        }),
        types::DataValue::Double(value) => Some(proto::Datapoint {
            value: Some(proto::datapoint::Value::Double(value)),
            timestamp: None,
        }),
        types::DataValue::BoolArray(values) => Some(proto::Datapoint {
            value: Some(proto::datapoint::Value::BoolArray(proto::BoolArray {
                values,
            })),
            timestamp: None,
        }),
        types::DataValue::StringArray(values) => Some(proto::Datapoint {
            value: Some(proto::datapoint::Value::StringArray(proto::StringArray {
                values,
            })),
            timestamp: None,
        }),
        types::DataValue::Int32Array(values) => Some(proto::Datapoint {
            value: Some(proto::datapoint::Value::Int32Array(proto::Int32Array {
                values,
            })),
            timestamp: None,
        }),
        types::DataValue::Int64Array(values) => Some(proto::Datapoint {
            value: Some(proto::datapoint::Value::Int64Array(proto::Int64Array {
                values,
            })),
            timestamp: None,
        }),
        types::DataValue::Uint32Array(values) => Some(proto::Datapoint {
            value: Some(proto::datapoint::Value::Uint32Array(proto::Uint32Array {
                values,
            })),
            timestamp: None,
        }),
        types::DataValue::Uint64Array(values) => Some(proto::Datapoint {
            value: Some(proto::datapoint::Value::Uint64Array(proto::Uint64Array {
                values,
            })),
            timestamp: None,
        }),
        types::DataValue::FloatArray(values) => Some(proto::Datapoint {
            value: Some(proto::datapoint::Value::FloatArray(proto::FloatArray {
                values,
            })),
            timestamp: None,
        }),
        types::DataValue::DoubleArray(values) => Some(proto::Datapoint {
            value: Some(proto::datapoint::Value::DoubleArray(proto::DoubleArray {
                values,
            })),
            timestamp: None,
        }),
    }
}

impl broker::EntryUpdate {
    fn from_proto_entry(entry: &proto::DataEntry, fields: HashSet<proto::Field>) -> Self {
        let path = match fields.contains(&proto::Field::Path) {
            true => Some(entry.path.clone()),
            false => None,
        };
        let datapoint = match fields.contains(&proto::Field::Value) {
            true => entry
                .value
                .as_ref()
                .map(|value| datapoint_from_proto_datapoint(value.clone())),
            false => None,
        };
        let actuator_target = match fields.contains(&proto::Field::ActuatorTarget) {
            true => match &entry.actuator_target {
                Some(datapoint) => Some(Some(value_from_proto_value(datapoint.value.clone()))),
                None => Some(None),
            },
            false => None,
        };
        Self {
            path,
            datapoint,
            actuator_target,
            entry_type: None,
            data_type: None,
            description: None,
        }
    }
}

fn proto_entry_from_update(update: broker::EntryUpdate) -> proto::DataEntry {
    proto::DataEntry {
        path: update.path.unwrap(),
        value: match update.datapoint {
            Some(datapoint) => proto_datapoint_from_datapoint(datapoint),
            None => None,
        },
        actuator_target: match update.actuator_target {
            Some(Some(actuator_target)) => proto_datapoint_from_value(actuator_target),
            Some(None) => None,
            None => None,
        },
        metadata: None,
    }
}

fn proto_fields_from_fields(fields: impl IntoIterator<Item = broker::Field>) -> Vec<i32> {
    fields
        .into_iter()
        .map(|field| match field {
            broker::Field::Value => proto::Field::Value as i32,
            broker::Field::ActuatorTarget => proto::Field::ActuatorTarget as i32,
        })
        .collect()
}
