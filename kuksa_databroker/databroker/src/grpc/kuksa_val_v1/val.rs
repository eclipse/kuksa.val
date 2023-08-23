/********************************************************************************
* Copyright (c) 2022, 2023 Contributors to the Eclipse Foundation
*
* See the NOTICE file(s) distributed with this work for additional
* information regarding copyright ownership.
*
* This program and the accompanying materials are made available under the
* terms of the Apache License 2.0 which is available at
* http://www.apache.org/licenses/LICENSE-2.0
*
* SPDX-License-Identifier: Apache-2.0
********************************************************************************/

use std::collections::HashMap;
use std::collections::HashSet;
use std::iter::FromIterator;
use std::pin::Pin;

use databroker_proto::kuksa::val::v1 as proto;
use databroker_proto::kuksa::val::v1::DataEntryError;
use tokio_stream::Stream;
use tokio_stream::StreamExt;
use tracing::debug;

use crate::broker;
use crate::broker::ReadError;
use crate::broker::SubscriptionError;
use crate::glob;
use crate::permissions::Permissions;

#[tonic::async_trait]
impl proto::val_server::Val for broker::DataBroker {
    async fn get(
        &self,
        request: tonic::Request<proto::GetRequest>,
    ) -> Result<tonic::Response<proto::GetResponse>, tonic::Status> {
        debug!(?request);
        let permissions = match request.extensions().get::<Permissions>() {
            Some(permissions) => {
                debug!(?permissions);
                permissions.clone()
            }
            None => return Err(tonic::Status::unauthenticated("Unauthenticated")),
        };
        let broker = self.authorized_access(&permissions);

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
                match glob::is_valid_pattern(&request.path) {
                    true => {
                        match broker.get_entry_by_path(&request.path).await {
                            Ok(entry) => {
                                let view =
                                    proto::View::from_i32(request.view).ok_or_else(|| {
                                        tonic::Status::invalid_argument(format!(
                                            "Invalid View (id: {}",
                                            request.view
                                        ))
                                    })?;
                                let fields = HashSet::<proto::Field>::from_iter(
                                    request.fields.iter().filter_map(
                                        |id| proto::Field::from_i32(*id), // Ignore unknown fields for now
                                    ),
                                );
                                let fields = combine_view_and_fields(view, fields);
                                debug!("Getting fields: {:?}", fields);
                                let proto_entry = proto_entry_from_entry_and_fields(entry, fields);
                                debug!("Getting datapoint: {:?}", proto_entry);
                                entries.push(proto_entry);
                            }
                            Err(ReadError::NotFound) => {
                                let sub_path_new = request.path.clone();
                                let regex = glob::to_regex(sub_path_new.as_ref());
                                match broker.get_entries_by_regex(regex.unwrap()).await {
                                    Ok(entries_result) => {
                                        for data_entry in entries_result {
                                            let view = proto::View::from_i32(request.view)
                                                .ok_or_else(|| {
                                                    tonic::Status::invalid_argument(format!(
                                                        "Invalid View (id: {}",
                                                        request.view
                                                    ))
                                                })?;

                                            let fields = HashSet::<proto::Field>::from_iter(
                                                request.fields.iter().filter_map(
                                                    |id| proto::Field::from_i32(*id), // Ignore unknown fields for now
                                                ),
                                            );
                                            let fields = combine_view_and_fields(view, fields);
                                            debug!("Getting fields: {:?}", fields);

                                            let entry_type = data_entry.metadata.entry_type.clone();

                                            let proto_entry = proto_entry_from_entry_and_fields(
                                                data_entry, fields,
                                            );
                                            debug!("Getting datapoint: {:?}", proto_entry);

                                            if view == proto::View::TargetValue {
                                                if entry_type == broker::EntryType::Actuator {
                                                    entries.push(proto_entry);
                                                }
                                            } else {
                                                entries.push(proto_entry);
                                            }
                                        }
                                        if entries.is_empty() {
                                            errors.push(proto::DataEntryError {
                                                path: request.path,
                                                error: Some(proto::Error {
                                                    code: 404,
                                                    reason: "not_found".to_owned(),
                                                    message: "No entries found for the provided call parameters"
                                                        .to_owned(),
                                                }),
                                            });
                                        }
                                    }
                                    Err(ReadError::NotFound) => {
                                        errors.push(proto::DataEntryError {
                                            path: request.path,
                                            error: Some(proto::Error {
                                                code: 404,
                                                reason: "not_found".to_owned(),
                                                message: "No entries found for the provided path"
                                                    .to_owned(),
                                            }),
                                        });
                                    }
                                    Err(ReadError::PermissionExpired) => {}
                                    Err(ReadError::PermissionDenied) => {}
                                }
                            }
                            Err(ReadError::PermissionExpired) => {
                                errors.push(proto::DataEntryError {
                                    path: request.path,
                                    error: Some(proto::Error {
                                        code: 401,
                                        reason: "unauthorized".to_owned(),
                                        message: "Authorization expired".to_owned(),
                                    }),
                                });
                            }
                            Err(ReadError::PermissionDenied) => {
                                errors.push(proto::DataEntryError {
                                    path: request.path,
                                    error: Some(proto::Error {
                                        code: 403,
                                        reason: "forbidden".to_owned(),
                                        message: "Permission denied".to_owned(),
                                    }),
                                });
                            }
                        }
                    }
                    false => {
                        errors.push(proto::DataEntryError {
                            path: request.path,
                            error: Some(proto::Error {
                                code: 400,
                                reason: "bad_request".to_owned(),
                                message: "Bad Path Request".to_owned(),
                            }),
                        });
                    }
                };
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
        debug!(?request);
        let permissions = match request.extensions().get::<Permissions>() {
            Some(permissions) => {
                debug!(?permissions);
                permissions.clone()
            }
            None => return Err(tonic::Status::unauthenticated("Unauthenticated")),
        };

        let broker = self.authorized_access(&permissions);

        // Collect errors encountered
        let mut errors = Vec::<DataEntryError>::new();

        let mut updates = Vec::<(i32, broker::EntryUpdate)>::new();
        for request in request.into_inner().updates {
            match &request.entry {
                Some(entry) => match broker.get_id_by_path(&entry.path).await {
                    Some(id) => {
                        let fields =
                            HashSet::<proto::Field>::from_iter(request.fields.iter().filter_map(
                                |id| proto::Field::from_i32(*id), // Ignore unknown fields for now
                            ));

                        if entry.actuator_target.is_some() {
                            if let Some(metadata) = broker.get_metadata(id).await {
                                if metadata.entry_type != broker::EntryType::Actuator {
                                    return Err(tonic::Status::invalid_argument(
                                        "Tried to set a target value for a non-actuator. Non-actuators have no target value.".to_string(),
                                    ));
                                }
                            }
                        }

                        let entry = match &request.entry {
                            Some(entry) => entry,
                            None => {
                                return Err(tonic::Status::invalid_argument(
                                    "Empty entry".to_string(),
                                ))
                            }
                        };
                        debug!("Settings fields: {:?}", fields);
                        let update =
                            broker::EntryUpdate::from_proto_entry_and_fields(entry, fields);
                        updates.push((id, update));
                    }
                    None => {
                        let message = format!("{} not found", entry.path);
                        errors.push(proto::DataEntryError {
                            path: entry.path.clone(),
                            error: Some(proto::Error {
                                code: 404,
                                reason: "not_found".to_string(),
                                message,
                            }),
                        })
                    }
                },
                None => {
                    return Err(tonic::Status::invalid_argument(
                        "Path is required".to_string(),
                    ))
                }
            }
        }

        match broker.update_entries(updates).await {
            Ok(()) => {}
            Err(err) => {
                debug!("Failed to set datapoint: {:?}", err);
                for (id, error) in err.into_iter() {
                    if let Some(metadata) = broker.get_metadata(id).await {
                        let path = metadata.path.clone();
                        let data_entry_error = match error {
                            broker::UpdateError::NotFound => DataEntryError {
                                path: path.clone(),
                                error: Some(proto::Error {
                                    code: 404,
                                    reason: String::from("not found"),
                                    message: format!("no datapoint registered for path {path}"),
                                }),
                            },
                            broker::UpdateError::WrongType => DataEntryError {
                                path,
                                error: Some(proto::Error {
                                    code: 400,
                                    reason: String::from("type mismatch"),
                                    message:
                                        "cannot set existing datapoint to value of different type"
                                            .to_string(),
                                }),
                            },
                            broker::UpdateError::UnsupportedType => DataEntryError {
                                path,
                                error: Some(proto::Error {
                                    code: 400,
                                    reason: String::from("unsupported type"),
                                    message: "cannot set datapoint to value of unsupported type"
                                        .to_string(),
                                }),
                            },
                            broker::UpdateError::OutOfBounds => DataEntryError {
                                path,
                                error: Some(proto::Error {
                                    code: 400,
                                    reason: String::from("value out of bounds"),
                                    message: String::from("given value exceeds type's boundaries"),
                                }),
                            },
                            broker::UpdateError::PermissionDenied => DataEntryError {
                                path: path.clone(),
                                error: Some(proto::Error {
                                    code: 403,
                                    reason: String::from("forbidden"),
                                    message: format!("Access was denied for {path}"),
                                }),
                            },
                            broker::UpdateError::PermissionExpired => DataEntryError {
                                path,
                                error: Some(proto::Error {
                                    code: 401,
                                    reason: String::from("unauthorized"),
                                    message: String::from("Unauthorized"),
                                }),
                            },
                        };
                        errors.push(data_entry_error);
                    }
                }
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
        debug!(?request);
        let permissions = match request.extensions().get::<Permissions>() {
            Some(permissions) => {
                debug!(?permissions);
                permissions.clone()
            }
            None => return Err(tonic::Status::unauthenticated("Unauthenticated")),
        };
        let broker = self.authorized_access(&permissions);

        let request = request.into_inner();

        if request.entries.is_empty() {
            return Err(tonic::Status::invalid_argument(
                "Subscription request must contain at least one entry.",
            ));
        }

        let mut entries = HashMap::new();

        for entry in request.entries {
            let mut fields = HashSet::new();
            for id in entry.fields {
                match proto::Field::from_i32(id) {
                    Some(field) => match field {
                        proto::Field::Value => {
                            fields.insert(broker::Field::Datapoint);
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
                            "Invalid Field (id: {id})"
                        )))
                    }
                };
            }
            entries.insert(entry.path.clone(), fields);
        }

        match broker.subscribe(entries).await {
            Ok(stream) => {
                let stream = convert_to_proto_stream(stream);
                Ok(tonic::Response::new(Box::pin(stream)))
            }
            Err(SubscriptionError::NotFound) => {
                Err(tonic::Status::new(tonic::Code::NotFound, "Path not found"))
            }
            Err(SubscriptionError::InvalidInput) => Err(tonic::Status::new(
                tonic::Code::InvalidArgument,
                "Invalid Argument",
            )),
            Err(SubscriptionError::InternalError) => {
                Err(tonic::Status::new(tonic::Code::Internal, "Internal Error"))
            }
        }
    }

    async fn get_server_info(
        &self,
        _request: tonic::Request<proto::GetServerInfoRequest>,
    ) -> Result<tonic::Response<proto::GetServerInfoResponse>, tonic::Status> {
        let server_info = proto::GetServerInfoResponse {
            name: "databroker".to_owned(),
            version: self.get_version().to_owned(),
        };
        Ok(tonic::Response::new(server_info))
    }
}

fn convert_to_proto_stream(
    input: impl Stream<Item = broker::EntryUpdates>,
) -> impl Stream<Item = Result<proto::SubscribeResponse, tonic::Status>> {
    input.map(move |item| {
        let mut updates = Vec::new();
        for update in item.updates {
            updates.push(proto::EntryUpdate {
                entry: Some(proto::DataEntry::from(update.update)),
                fields: update
                    .fields
                    .iter()
                    .map(|field| proto::Field::from(field) as i32)
                    .collect(),
            });
        }
        let response = proto::SubscribeResponse { updates };
        Ok(response)
    })
}

fn proto_entry_from_entry_and_fields(
    entry: broker::Entry,
    fields: HashSet<proto::Field>,
) -> proto::DataEntry {
    let path = entry.metadata.path;
    let value = if fields.contains(&proto::Field::Value) {
        Option::<proto::Datapoint>::from(entry.datapoint)
    } else {
        None
    };
    let actuator_target = if fields.contains(&proto::Field::ActuatorTarget) {
        match entry.actuator_target {
            Some(actuator_target) => Option::<proto::Datapoint>::from(actuator_target),
            None => None,
        }
    } else {
        None
    };
    let metadata = {
        let mut metadata = proto::Metadata::default();
        let mut metadata_is_set = false;

        let all = fields.contains(&proto::Field::Metadata);

        if all || fields.contains(&proto::Field::MetadataDataType) {
            metadata_is_set = true;
            metadata.data_type = proto::DataType::from(entry.metadata.data_type) as i32;
        }
        if all || fields.contains(&proto::Field::MetadataDescription) {
            metadata_is_set = true;
            metadata.description = Some(entry.metadata.description);
        }
        if all || fields.contains(&proto::Field::MetadataEntryType) {
            metadata_is_set = true;
            metadata.entry_type = proto::EntryType::from(&entry.metadata.entry_type) as i32;
        }
        if all || fields.contains(&proto::Field::MetadataComment) {
            metadata_is_set = true;
            // TODO: Add to Metadata
            metadata.comment = None;
        }
        if all || fields.contains(&proto::Field::MetadataDeprecation) {
            metadata_is_set = true;
            // TODO: Add to Metadata
            metadata.deprecation = None;
        }
        if all || fields.contains(&proto::Field::MetadataUnit) {
            metadata_is_set = true;
            // TODO: Add to Metadata
            metadata.unit = None;
        }
        if all || fields.contains(&proto::Field::MetadataValueRestriction) {
            metadata_is_set = true;
            // TODO: Add to Metadata
        }
        if all || fields.contains(&proto::Field::MetadataActuator) {
            metadata_is_set = true;
            // TODO: Add to Metadata
            metadata.entry_specific = match entry.metadata.entry_type {
                broker::EntryType::Actuator => {
                    // Some(proto::metadata::EntrySpecific::Actuator(
                    //     proto::Actuator::default(),
                    // ));
                    None
                }
                broker::EntryType::Sensor | broker::EntryType::Attribute => None,
            };
        }
        if all || fields.contains(&proto::Field::MetadataSensor) {
            metadata_is_set = true;
            // TODO: Add to Metadata
            metadata.entry_specific = match entry.metadata.entry_type {
                broker::EntryType::Sensor => {
                    // Some(proto::metadata::EntrySpecific::Sensor(
                    //     proto::Sensor::default(),
                    // ));
                    None
                }
                broker::EntryType::Attribute | broker::EntryType::Actuator => None,
            };
        }
        if all || fields.contains(&proto::Field::MetadataAttribute) {
            metadata_is_set = true;
            // TODO: Add to Metadata
            metadata.entry_specific = match entry.metadata.entry_type {
                broker::EntryType::Attribute => {
                    // Some(proto::metadata::EntrySpecific::Attribute(
                    //     proto::Attribute::default(),
                    // ));
                    None
                }
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

impl broker::EntryUpdate {
    fn from_proto_entry_and_fields(
        entry: &proto::DataEntry,
        fields: HashSet<proto::Field>,
    ) -> Self {
        let datapoint = if fields.contains(&proto::Field::Value) {
            entry
                .value
                .as_ref()
                .map(|value| broker::Datapoint::from(value.clone()))
        } else {
            None
        };
        let actuator_target = if fields.contains(&proto::Field::ActuatorTarget) {
            match &entry.actuator_target {
                Some(datapoint) => Some(Some(broker::Datapoint::from(datapoint.clone()))),
                None => Some(None),
            }
        } else {
            None
        };
        Self {
            path: None,
            datapoint,
            actuator_target,
            entry_type: None,
            data_type: None,
            description: None,
            allowed: None,
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::{broker::DataBroker, permissions};

    #[tokio::test]
    async fn test_update_datapoint_using_wrong_type() {
        let broker = DataBroker::default();
        let authorized_access = broker.authorized_access(&permissions::ALLOW_ALL);

        authorized_access
            .add_entry(
                "test.datapoint1".to_owned(),
                broker::DataType::Bool,
                broker::ChangeType::OnChange,
                broker::EntryType::Sensor,
                "Test datapoint 1".to_owned(),
                None,
            )
            .await
            .expect("Register datapoint should succeed");

        let mut req = tonic::Request::new(proto::SetRequest {
            updates: vec![proto::EntryUpdate {
                fields: vec![proto::Field::Value as i32],
                entry: Some(proto::DataEntry {
                    path: "test.datapoint1".to_owned(),
                    value: Some(proto::Datapoint {
                        timestamp: Some(std::time::SystemTime::now().into()),
                        value: Some(proto::datapoint::Value::Int32(1456)),
                    }),
                    metadata: None,
                    actuator_target: None,
                }),
            }],
        });

        // Manually insert permissions
        req.extensions_mut().insert(permissions::ALLOW_ALL.clone());

        match proto::val_server::Val::set(&broker, req)
            .await
            .map(|res| res.into_inner())
        {
            Ok(set_response) => {
                assert!(
                    !set_response.errors.is_empty(),
                    "databroker should not allow updating boolean datapoint with an int32"
                );
                let error = set_response.errors[0]
                    .to_owned()
                    .error
                    .expect("error details are missing");
                assert_eq!(error.code, 400, "unexpected error code");
            }
            Err(_status) => panic!("failed to execute set request"),
        }
    }
}
