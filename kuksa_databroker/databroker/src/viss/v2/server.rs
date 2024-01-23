/********************************************************************************
* Copyright (c) 2023 Contributors to the Eclipse Foundation
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

use std::{
    collections::{HashMap, HashSet},
    convert::TryFrom,
    pin::Pin,
    sync::Arc,
    time::SystemTime,
};

use futures::{
    stream::{AbortHandle, Abortable},
    Stream, StreamExt,
};
use tokio::sync::RwLock;
use tracing::warn;

use crate::{
    authorization::Authorization,
    broker::{self, AuthorizedAccess, UpdateError},
    permissions::{self, Permissions},
};

use super::{conversions, types::*};

#[tonic::async_trait]
pub(crate) trait Viss: Send + Sync + 'static {
    async fn get(&self, request: GetRequest) -> Result<GetSuccessResponse, GetErrorResponse>;
    async fn set(&self, request: SetRequest) -> Result<SetSuccessResponse, SetErrorResponse>;

    type SubscribeStream: Stream<Item = Result<SubscriptionEvent, SubscriptionErrorEvent>>
        + Send
        + 'static;

    async fn subscribe(
        &self,
        request: SubscribeRequest,
    ) -> Result<(SubscribeSuccessResponse, Self::SubscribeStream), SubscribeErrorResponse>;

    async fn unsubscribe(
        &self,
        request: UnsubscribeRequest,
    ) -> Result<UnsubscribeSuccessResponse, UnsubscribeErrorResponse>;
}

pub struct SubscriptionHandle {
    abort_handle: AbortHandle,
}

impl From<AbortHandle> for SubscriptionHandle {
    fn from(abort_handle: AbortHandle) -> Self {
        Self { abort_handle }
    }
}

impl Drop for SubscriptionHandle {
    fn drop(&mut self) {
        self.abort_handle.abort();
    }
}

pub struct Server {
    broker: broker::DataBroker,
    authorization: Authorization,
    subscriptions: Arc<RwLock<HashMap<SubscriptionId, SubscriptionHandle>>>,
}

impl Server {
    pub fn new(broker: broker::DataBroker, authorization: Authorization) -> Self {
        Self {
            broker,
            authorization,
            subscriptions: Arc::new(RwLock::new(HashMap::new())),
        }
    }
}

#[tonic::async_trait]
impl Viss for Server {
    async fn get(&self, request: GetRequest) -> Result<GetSuccessResponse, GetErrorResponse> {
        let request_id = request.request_id;

        if let Some(Filter::StaticMetadata(_)) = &request.filter {
            // Authorization not required for metadata, don't bail if an
            // access token is missing.
            let broker = self.broker.authorized_access(&permissions::ALLOW_NONE);
            let metadata = generate_metadata(&broker, request.path.as_ref()).await;
            return Ok(GetSuccessResponse::Metadata(MetadataResponse {
                request_id,
                metadata,
            }));
        }
        let permissions = resolve_permissions(&self.authorization, &request.authorization)
            .map_err(|error| GetErrorResponse {
                request_id: request_id.clone(),
                error,
                ts: SystemTime::now().into(),
            })?;
        let broker = self.broker.authorized_access(&permissions);

        // Get datapoints
        match broker.get_datapoint_by_path(request.path.as_ref()).await {
            Ok(datapoint) => {
                let dp = DataPoint::from(datapoint);
                Ok(GetSuccessResponse::Data(DataResponse {
                    request_id,
                    data: Data::Object(DataObject {
                        path: request.path,
                        dp,
                    }),
                }))
            }
            Err(err) => Err(GetErrorResponse {
                request_id,
                ts: SystemTime::now().into(),
                error: match err {
                    broker::ReadError::NotFound => Error::NotFoundInvalidPath,
                    broker::ReadError::PermissionDenied => Error::Forbidden,
                    broker::ReadError::PermissionExpired => Error::UnauthorizedTokenExpired,
                },
            }),
        }
    }

    async fn set(&self, request: SetRequest) -> Result<SetSuccessResponse, SetErrorResponse> {
        let request_id = request.request_id;
        let permissions = resolve_permissions(&self.authorization, &request.authorization)
            .map_err(|error| SetErrorResponse {
                request_id: request_id.clone(),
                error,
                ts: SystemTime::now().into(),
            })?;
        let broker = self.broker.authorized_access(&permissions);

        match broker.get_metadata_by_path(request.path.as_ref()).await {
            Some(metadata) => {
                if metadata.entry_type != broker::EntryType::Actuator {
                    return Err(SetErrorResponse {
                        request_id,
                        error: Error::UnauthorizedReadOnly,
                        ts: SystemTime::now().into(),
                    });
                }

                let value = request.value;

                let update = value
                    .try_into_type(&metadata.data_type)
                    .map(|actuator_target| broker::EntryUpdate {
                        path: None,
                        datapoint: None,
                        actuator_target: Some(Some(broker::Datapoint {
                            value: actuator_target,
                            ts: SystemTime::now(),
                        })),
                        entry_type: None,
                        data_type: None,
                        description: None,
                        allowed: None,
                    })
                    .map_err(|err| SetErrorResponse {
                        request_id: request_id.clone(),
                        error: match err {
                            conversions::Error::ParseError => Error::BadRequest {
                                msg: Some(format!(
                                    "Failed to parse the value as a {}",
                                    DataType::from(metadata.data_type.clone())
                                )),
                            },
                        },
                        ts: SystemTime::now().into(),
                    })?;

                let updates = vec![(metadata.id, update)];
                match broker.update_entries(updates).await {
                    Ok(()) => Ok(SetSuccessResponse {
                        request_id,
                        ts: SystemTime::now().into(),
                    }),
                    Err(errors) => {
                        let error = if let Some((_, error)) = errors.first() {
                            match error {
                                UpdateError::NotFound => Error::NotFoundInvalidPath,
                                UpdateError::WrongType => Error::BadRequest {
                                    msg: Some("Wrong data type.".into()),
                                },
                                UpdateError::OutOfBounds => Error::BadRequest {
                                    msg: Some("Value out of bounds.".into()),
                                },
                                UpdateError::UnsupportedType => Error::BadRequest {
                                    msg: Some("Unsupported data type.".into()),
                                },
                                UpdateError::PermissionDenied => Error::Forbidden,
                                UpdateError::PermissionExpired => Error::UnauthorizedTokenExpired,
                            }
                        } else {
                            Error::InternalServerError
                        };

                        Err(SetErrorResponse {
                            request_id,
                            error,
                            ts: SystemTime::now().into(),
                        })
                    }
                }
            }
            None => {
                // Not found
                Err(SetErrorResponse {
                    request_id,
                    error: Error::NotFoundInvalidPath,
                    ts: SystemTime::now().into(),
                })
            }
        }
    }

    type SubscribeStream = Pin<
        Box<
            dyn Stream<Item = Result<SubscriptionEvent, SubscriptionErrorEvent>>
                + Send
                + Sync
                + 'static,
        >,
    >;

    async fn subscribe(
        &self,
        request: SubscribeRequest,
    ) -> Result<(SubscribeSuccessResponse, Self::SubscribeStream), SubscribeErrorResponse> {
        let request_id = request.request_id;
        let permissions = resolve_permissions(&self.authorization, &request.authorization)
            .map_err(|error| SubscribeErrorResponse {
                request_id: request_id.clone(),
                error,
                ts: SystemTime::now().into(),
            })?;
        let broker = self.broker.authorized_access(&permissions);

        let Some(entries) = broker
            .get_id_by_path(request.path.as_ref())
            .await
            .map(|id| HashMap::from([(id, HashSet::from([broker::Field::Datapoint]))]))
        else {
            return Err(SubscribeErrorResponse {
                request_id,
                error: Error::NotFoundInvalidPath,
                ts: SystemTime::now().into(),
            });
        };

        match broker.subscribe(entries).await {
            Ok(stream) => {
                let subscription_id = SubscriptionId::new();

                let (abort_handle, abort_registration) = AbortHandle::new_pair();

                // Make the stream abortable
                let stream = Abortable::new(stream, abort_registration);

                // Register abort handle
                self.subscriptions.write().await.insert(
                    subscription_id.clone(),
                    SubscriptionHandle::from(abort_handle),
                );

                let stream = convert_to_viss_stream(subscription_id.clone(), stream);

                Ok((
                    SubscribeSuccessResponse {
                        request_id,
                        subscription_id,
                        ts: SystemTime::now().into(),
                    },
                    Box::pin(stream),
                ))
            }
            Err(err) => Err(SubscribeErrorResponse {
                request_id,
                error: match err {
                    broker::SubscriptionError::NotFound => Error::NotFoundInvalidPath,
                    broker::SubscriptionError::InvalidInput => Error::NotFoundInvalidPath,
                    broker::SubscriptionError::InternalError => Error::InternalServerError,
                },
                ts: SystemTime::now().into(),
            }),
        }
    }

    async fn unsubscribe(
        &self,
        request: UnsubscribeRequest,
    ) -> Result<UnsubscribeSuccessResponse, UnsubscribeErrorResponse> {
        let subscription_id = request.subscription_id;
        let request_id = request.request_id;
        match self.subscriptions.write().await.remove(&subscription_id) {
            Some(_) => {
                // Stream is aborted when handle is dropped
                Ok(UnsubscribeSuccessResponse {
                    request_id,
                    subscription_id,
                    ts: SystemTime::now().into(),
                })
            }
            None => Err(UnsubscribeErrorResponse {
                request_id,
                subscription_id,
                error: Error::NotFoundInvalidSubscriptionId,
                ts: SystemTime::now().into(),
            }),
        }
    }
}

fn convert_to_viss_stream(
    subscription_id: SubscriptionId,
    stream: impl Stream<Item = broker::EntryUpdates>,
) -> impl Stream<Item = Result<SubscriptionEvent, SubscriptionErrorEvent>> {
    stream.map(move |mut item| {
        let ts = SystemTime::now().into();
        let subscription_id = subscription_id.clone();
        match item.updates.pop() {
            Some(item) => match (item.update.path, item.update.datapoint) {
                (Some(path), Some(datapoint)) => Ok(SubscriptionEvent {
                    subscription_id,
                    data: Data::Object(DataObject {
                        path: path.into(),
                        dp: datapoint.into(),
                    }),
                    ts,
                }),
                (_, _) => Err(SubscriptionErrorEvent {
                    subscription_id,
                    error: Error::InternalServerError,
                    ts,
                }),
            },
            None => Err(SubscriptionErrorEvent {
                subscription_id,
                error: Error::InternalServerError,
                ts,
            }),
        }
    })
}

fn resolve_permissions(
    authorization: &Authorization,
    token: &Option<String>,
) -> Result<Permissions, Error> {
    match authorization {
        Authorization::Disabled => Ok(permissions::ALLOW_ALL.clone()),
        Authorization::Enabled { token_decoder } => match token {
            Some(token) => match token_decoder.decode(token) {
                Ok(claims) => match Permissions::try_from(claims) {
                    Ok(permissions) => Ok(permissions),
                    Err(_) => Err(Error::UnauthorizedTokenInvalid),
                },
                Err(_) => Err(Error::UnauthorizedTokenInvalid),
            },
            None => Err(Error::UnauthorizedTokenMissing),
        },
    }
}

async fn generate_metadata<'a>(
    db: &'a AuthorizedAccess<'_, '_>,
    path: &str,
) -> HashMap<String, MetadataEntry> {
    let mut metadata: HashMap<String, MetadataEntry> = HashMap::new();

    // We want to remove all but the last "component" present in the path.
    // For example, if requesting "Vehicle.Driver", we want to match
    // everything starting with "Vehicle.Driver" but include "Driver"
    // as the top level entry returned as metadata.
    let prefix_to_strip = match path.rsplit_once('.') {
        Some((prefix_excl_dot, _leaf)) => &path[..=prefix_excl_dot.len()],
        None => "",
    };

    // Include everything that starts with the requested path.
    // Insert into the metadata tree, with the top level being the last
    // "component" of the branch as described above.
    db.for_each_entry(|entry| {
        let entry_metadata = entry.metadata();
        let entry_path = &entry_metadata.path;
        if entry_path.starts_with(path) {
            if let Some(path) = entry_path.strip_prefix(prefix_to_strip) {
                insert_entry(&mut metadata, path, entry_metadata.into());
            }
        }
    })
    .await;

    metadata
}

fn insert_entry(entries: &mut HashMap<String, MetadataEntry>, path: &str, entry: MetadataEntry) {
    // Get the leftmost path component by splitting at '.'.
    // `split_once` will return None if there is only one component,
    // which means it's the leaf.
    match path.split_once('.') {
        Some((key, path)) => match entries.get_mut(key) {
            Some(MetadataEntry::Branch(branch_entry)) => {
                insert_entry(&mut branch_entry.children, path, entry);
            }
            Some(_) => {
                warn!("Should only be possible for branches to exist here");
                // ignore
            }
            None => {
                let mut branch = BranchEntry {
                    description: "".into(),
                    children: HashMap::default(),
                };
                insert_entry(&mut branch.children, path, entry);
                entries.insert(key.to_owned(), MetadataEntry::Branch(branch));
            }
        },
        None => {
            entries.insert(path.to_owned(), entry);
        }
    }
}
