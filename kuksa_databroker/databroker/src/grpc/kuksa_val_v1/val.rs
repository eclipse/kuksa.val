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
use crate::broker::EntryReadAccess;
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
            /*
             * valid_requests: A collection of valid requests, each represented as a tuple with five fields:
             * - Regex: The regular expression created from the string path request.
             * - Fields: A HashSet of proto::Field objects extracted from the request.
             * - RequestPath: The original request path, used for error reporting when no entries match.
             * - IsMatch: A boolean flag indicating whether the current request matches any entry.
             * - Error: An optional ReadError representing a permission error that may occur when querying a valid path entry.
             */
            let mut valid_requests: Vec<(
                regex::Regex,
                HashSet<proto::Field>,
                String,
                bool,
                Option<ReadError>,
            )> = Vec::new();

            // Fill valid_requests structure.
            for request in requested {
                if request.path.contains('*') && !glob::is_valid_pattern(&request.path) {
                    errors.push(proto::DataEntryError {
                        path: request.path,
                        error: Some(proto::Error {
                            code: 400,
                            reason: "bad_request".to_owned(),
                            message: "Bad Wildcard Pattern Request".to_owned(),
                        }),
                    });
                    continue;
                }

                let view = proto::View::from_i32(request.view).ok_or_else(|| {
                    tonic::Status::invalid_argument(format!("Invalid View (id: {}", request.view))
                })?;
                let fields = HashSet::<proto::Field>::from_iter(request.fields.iter().filter_map(
                    |id| proto::Field::from_i32(*id), // Ignore unknown fields for now
                ));
                let view_fields = combine_view_and_fields(view, fields);
                debug!("Getting fields: {:?}", view_fields);

                let regex_exp = glob::to_regex(&request.path);
                match regex_exp {
                    Ok(value) => {
                        valid_requests.push((value, view_fields, request.path, false, None));
                    }
                    Err(_) => {
                        errors.push(proto::DataEntryError {
                            path: request.path,
                            error: Some(proto::Error {
                                code: 400,
                                reason: "bad regex".to_owned(),
                                message: "Regex can't be created for provided path".to_owned(),
                            }),
                        });
                    }
                }
            }
            if !valid_requests.is_empty() {
                broker
                    .for_each_entry(|entry| {
                        let mut result_fields: HashSet<proto::Field> = HashSet::new();
                        for (regex, view_fields, _, is_match, op_error) in &mut valid_requests {
                            let path = &entry.metadata().path;
                            if regex.is_match(path) {
                                // Update the `is_match` to indicate a valid and used request path.
                                *is_match = true;
                                if view_fields.contains(&proto::Field::Metadata) {
                                    result_fields.extend(view_fields.clone());
                                }
                                if view_fields.contains(&proto::Field::ActuatorTarget)
                                    || view_fields.contains(&proto::Field::Value)
                                {
                                    match entry.datapoint() {
                                        Ok(_) => {
                                            // If the entry's path matches the regex and there is access permission,
                                            // add the result fields to the current entry.
                                            result_fields.extend(view_fields.clone());
                                        }
                                        Err(error) => {
                                            //Propagate the error
                                            *op_error = Some(error);
                                        }
                                    }
                                }
                            }
                        }

                        // If there are result fields, add them to the entries list.
                        if !result_fields.is_empty() {
                            let proto_entry =
                                proto_entry_from_entry_and_fields(entry, result_fields);
                            debug!("Getting datapoint: {:?}", proto_entry);
                            entries.push(proto_entry);
                        }
                    })
                    .await;
            }

            /*
             * Handle Unmatched or Permission Errors
             *
             * After processing valid requests, this section iterates over the `valid_requests` vector
             * to check if any requests didn't have matching entries or encountered permission errors.
             *
             * For each unmatched request, a "not_found" error message is added to the `errors` list.
             * For requests with permission errors, a "forbidden" error message is added.
             */
            for (_, _, path, is_match, error) in valid_requests {
                if !is_match {
                    errors.push(proto::DataEntryError {
                        path: path.to_owned(),
                        error: Some(proto::Error {
                            code: 404,
                            reason: "not_found".to_owned(),
                            message: "No entries found for the provided path".to_owned(),
                        }),
                    });
                } else if let Some(_error) = error {
                    // clear the entries vector since we only want to return rerrors
                    // and not partial success
                    entries.clear();
                    errors.push(proto::DataEntryError {
                        path: path.to_owned(),
                        error: Some(proto::Error {
                            code: 403,
                            reason: "forbidden".to_owned(),
                            message: "Permission denied for some entries".to_owned(),
                        }),
                    });
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

        let mut valid_requests: HashMap<String, (regex::Regex, HashSet<broker::Field>)> =
            HashMap::new();

        for entry in &request.entries {
            if entry.path.contains('*') && !glob::is_valid_pattern(&entry.path) {
                tonic::Status::new(tonic::Code::InvalidArgument, "Invalid Pattern Argument");
                continue;
            }

            let regex_exp = glob::to_regex(&entry.path);
            if let Ok(regex) = regex_exp {
                let mut fields = HashSet::new();
                for id in &entry.fields {
                    if let Some(field) = proto::Field::from_i32(*id) {
                        match field {
                            proto::Field::Value => {
                                fields.insert(broker::Field::Datapoint);
                            }
                            proto::Field::ActuatorTarget => {
                                fields.insert(broker::Field::ActuatorTarget);
                            }
                            _ => {
                                // Just ignore other fields for now
                            }
                        }
                    };
                }
                valid_requests.insert(entry.path.clone(), (regex, fields));
            }
        }

        let mut entries: HashMap<i32, HashSet<broker::Field>> = HashMap::new();

        if !valid_requests.is_empty() {
            for (path, (regex, fields)) in valid_requests {
                let mut requested_path_found = false;
                let mut permission_error = false;
                broker
                    .for_each_entry(|entry| {
                        let entry_path = &entry.metadata().path;
                        if regex.is_match(entry_path) {
                            requested_path_found = true;
                            entries
                                .entry(entry.metadata().id)
                                .and_modify(|existing_fields| {
                                    existing_fields.extend(fields.clone());
                                })
                                .or_insert(fields.clone());

                            match entry.datapoint() {
                                Ok(_) => {}
                                Err(_) => permission_error = true,
                            }
                        }
                    })
                    .await;
                if !requested_path_found {
                    let message = format!("No entries found for the provided. Path: {}", path);
                    return Err(tonic::Status::new(tonic::Code::NotFound, message));
                }
                if permission_error {
                    let message = format!("Permission denied for some entries. Path: {}", path);
                    return Err(tonic::Status::new(tonic::Code::PermissionDenied, message));
                }
            }
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
    entry: EntryReadAccess,
    fields: HashSet<proto::Field>,
) -> proto::DataEntry {
    let path = entry.metadata().path.to_string();
    let value = if fields.contains(&proto::Field::Value) {
        match entry.datapoint() {
            Ok(value) => Option::<proto::Datapoint>::from(value.clone()),
            Err(_) => None,
        }
    } else {
        None
    };
    let actuator_target = if fields.contains(&proto::Field::ActuatorTarget) {
        match entry.actuator_target() {
            Ok(value) => match value {
                Some(value) => Option::<proto::Datapoint>::from(value.clone()),
                None => None,
            },
            Err(_) => None,
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
            metadata.data_type = proto::DataType::from(entry.metadata().data_type.clone()) as i32;
        }
        if all || fields.contains(&proto::Field::MetadataDescription) {
            metadata_is_set = true;
            metadata.description = Some(entry.metadata().description.clone());
        }
        if all || fields.contains(&proto::Field::MetadataEntryType) {
            metadata_is_set = true;
            metadata.entry_type = proto::EntryType::from(&entry.metadata().entry_type) as i32;
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
            metadata.entry_specific = match entry.metadata().entry_type {
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
            metadata.entry_specific = match entry.metadata().entry_type {
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
            metadata.entry_specific = match entry.metadata().entry_type {
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
