/********************************************************************************
* Copyright (c) 2022 Contributors to the Eclipse Foundation
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

pub use crate::types;

use crate::query;
pub use crate::types::{ChangeType, DataType, DataValue, EntryType};

use tokio::sync::{broadcast, mpsc, RwLock};
use tokio_stream::wrappers::ReceiverStream;
use tokio_stream::Stream;

use std::collections::{HashMap, HashSet};
use std::convert::TryFrom;
use std::sync::atomic::{AtomicI32, Ordering};
use std::sync::Arc;
use std::time::SystemTime;

use tracing::{debug, info};

#[derive(Debug)]
pub enum UpdateError {
    NotFound,
    WrongType,
    OutOfBounds,
    UnsupportedType,
}

#[derive(Debug, Clone)]
pub struct Metadata {
    pub id: i32,
    pub path: String,
    pub data_type: DataType,
    pub entry_type: EntryType,
    pub change_type: ChangeType,
    pub description: String,
}

#[derive(Debug, Clone)]
pub struct Datapoint {
    pub ts: SystemTime,
    pub value: DataValue,
}

#[derive(Debug, Clone)]
pub struct Entry {
    pub datapoint: Datapoint,
    pub actuator_target: Option<DataValue>,
    pub metadata: Metadata,
}

#[derive(PartialEq, Eq, Hash)]
pub enum Field {
    Datapoint,
    ActuatorTarget,
}

#[derive(Default)]
pub struct Entries {
    next_id: AtomicI32,
    path_to_id: HashMap<String, i32>,
    entries: HashMap<i32, Entry>,
}

#[derive(Default)]
pub struct Subscriptions {
    subscriptions: Vec<Subscription>,
}

#[derive(Debug, Clone)]
pub struct QueryResponse {
    pub fields: Vec<QueryField>,
}

#[derive(Debug, Clone)]
pub struct QueryField {
    pub name: String,
    pub value: DataValue,
}

#[derive(Debug)]
pub enum SubscriptionError {
    CompilationError(String),
    InternalError,
}

#[derive(Clone)]
pub struct DataBroker {
    pub entries: Arc<RwLock<Entries>>,
    pub subscriptions: Arc<RwLock<Subscriptions>>,
    shutdown_trigger: broadcast::Sender<()>,
}

pub struct Subscription {
    query: query::CompiledQuery,
    sender: mpsc::Sender<QueryResponse>,
}

#[derive(Debug)]
pub struct NotificationError {}

#[derive(Debug)]
pub struct RegistrationError {}

#[derive(Debug, Clone, Default)]
pub struct EntryUpdate {
    pub path: Option<String>,

    pub datapoint: Option<Datapoint>,

    // Actuator target is wrapped in an additional Option<> in
    // order to be able to convey "update it to None" which would
    // mean setting it to `Some(None)`.
    pub actuator_target: Option<Option<DataValue>>, // only for actuators

    // Metadata
    pub entry_type: Option<EntryType>,
    pub data_type: Option<DataType>,
    pub description: Option<String>,
}

impl EntryUpdate {
    pub fn is_empty(self) -> bool {
        self.path.is_none()
            && self.datapoint.is_none()
            && self.actuator_target.is_none()
            && self.entry_type.is_none()
            && self.data_type.is_none()
            && self.description.is_none()
    }
}

impl Entry {
    pub fn diff(&self, mut update: EntryUpdate) -> EntryUpdate {
        if let Some(datapoint) = &update.datapoint {
            if self.metadata.change_type != ChangeType::Continuous {
                // TODO: Compare timestamps as well?
                if datapoint.value == self.datapoint.value {
                    update.datapoint = None;
                }
            }
        }

        if let Some(actuator_target) = &update.actuator_target {
            if actuator_target == &self.actuator_target {
                update.actuator_target = None;
            }
        }

        // TODO: Implement for .path
        //                     .entry_type
        //                     .data_type
        //                     .description

        update
    }

    pub fn validate(&self, update: &EntryUpdate) -> Result<(), UpdateError> {
        if let Some(datapoint) = &update.datapoint {
            self.validate_value(&datapoint.value)?;
        }
        if let Some(Some(value)) = &update.actuator_target {
            self.validate_value(value)?;
        }
        Ok(())
    }

    fn validate_value(&self, value: &DataValue) -> Result<(), UpdateError> {
        // Validate value
        match self.metadata.data_type {
            DataType::Bool => match value {
                DataValue::Bool(_) => Ok(()),
                _ => Err(UpdateError::WrongType),
            },
            DataType::String => match value {
                DataValue::String(_) => Ok(()),
                _ => Err(UpdateError::WrongType),
            },
            DataType::Int8 => match value {
                DataValue::Int32(value) => match i8::try_from(*value) {
                    Ok(_) => Ok(()),
                    Err(_) => Err(UpdateError::OutOfBounds),
                },
                _ => Err(UpdateError::WrongType),
            },
            DataType::Int16 => match value {
                DataValue::Int32(value) => match i16::try_from(*value) {
                    Ok(_) => Ok(()),
                    Err(_) => Err(UpdateError::OutOfBounds),
                },
                _ => Err(UpdateError::WrongType),
            },
            DataType::Int32 => match value {
                DataValue::Int32(_) => Ok(()),
                _ => Err(UpdateError::WrongType),
            },

            DataType::Int64 => match value {
                DataValue::Int64(_) => Ok(()),
                _ => Err(UpdateError::WrongType),
            },
            DataType::Uint8 => match value {
                DataValue::Uint32(value) => match u8::try_from(*value) {
                    Ok(_) => Ok(()),
                    Err(_) => Err(UpdateError::OutOfBounds),
                },
                _ => Err(UpdateError::WrongType),
            },
            DataType::Uint16 => match value {
                DataValue::Uint32(value) => match u16::try_from(*value) {
                    Ok(_) => Ok(()),
                    Err(_) => Err(UpdateError::OutOfBounds),
                },
                _ => Err(UpdateError::WrongType),
            },
            DataType::Uint32 => match value {
                DataValue::Uint32(_) => Ok(()),
                _ => Err(UpdateError::WrongType),
            },
            DataType::Uint64 => match value {
                DataValue::Uint64(_) => Ok(()),
                _ => Err(UpdateError::WrongType),
            },
            DataType::Float => match value {
                DataValue::Float(_) => Ok(()),
                _ => Err(UpdateError::WrongType),
            },
            DataType::Double => match value {
                DataValue::Double(_) => Ok(()),
                _ => Err(UpdateError::WrongType),
            },
            DataType::BoolArray => match value {
                DataValue::BoolArray(_) => Ok(()),
                _ => Err(UpdateError::WrongType),
            },
            DataType::StringArray => match value {
                DataValue::StringArray(_) => Ok(()),
                _ => Err(UpdateError::WrongType),
            },
            DataType::Int8Array => match &value {
                DataValue::Int32Array(array) => {
                    let mut out_of_bounds = false;
                    for value in array {
                        match i8::try_from(*value) {
                            Ok(_) => {}
                            Err(_) => {
                                out_of_bounds = true;
                                break;
                            }
                        }
                    }
                    if out_of_bounds {
                        Err(UpdateError::OutOfBounds)
                    } else {
                        Ok(())
                    }
                }
                _ => Err(UpdateError::WrongType),
            },
            DataType::Int16Array => match &value {
                DataValue::Int32Array(array) => {
                    let mut out_of_bounds = false;
                    for value in array {
                        match i16::try_from(*value) {
                            Ok(_) => {}
                            Err(_) => {
                                out_of_bounds = true;
                                break;
                            }
                        }
                    }
                    if out_of_bounds {
                        Err(UpdateError::OutOfBounds)
                    } else {
                        Ok(())
                    }
                }
                _ => Err(UpdateError::WrongType),
            },
            DataType::Int32Array => match value {
                DataValue::Int32Array(_) => Ok(()),
                _ => Err(UpdateError::WrongType),
            },
            DataType::Int64Array => match value {
                DataValue::Int64Array(_) => Ok(()),
                _ => Err(UpdateError::WrongType),
            },
            DataType::Uint8Array => match &value {
                DataValue::Uint32Array(array) => {
                    let mut out_of_bounds = false;
                    for value in array {
                        match u8::try_from(*value) {
                            Ok(_) => {}
                            Err(_) => {
                                out_of_bounds = true;
                                break;
                            }
                        }
                    }
                    if out_of_bounds {
                        Err(UpdateError::OutOfBounds)
                    } else {
                        Ok(())
                    }
                }
                _ => Err(UpdateError::WrongType),
            },
            DataType::Uint16Array => match &value {
                DataValue::Uint32Array(array) => {
                    let mut out_of_bounds = false;
                    for value in array {
                        match u16::try_from(*value) {
                            Ok(_) => {}
                            Err(_) => {
                                out_of_bounds = true;
                                break;
                            }
                        }
                    }
                    if out_of_bounds {
                        Err(UpdateError::OutOfBounds)
                    } else {
                        Ok(())
                    }
                }
                _ => Err(UpdateError::WrongType),
            },
            DataType::Uint32Array => match value {
                DataValue::Uint32Array(_) => Ok(()),
                _ => Err(UpdateError::WrongType),
            },
            DataType::Uint64Array => match value {
                DataValue::Uint64Array(_) => Ok(()),
                _ => Err(UpdateError::WrongType),
            },
            DataType::FloatArray => match value {
                DataValue::FloatArray(_) => Ok(()),
                _ => Err(UpdateError::WrongType),
            },
            DataType::DoubleArray => match value {
                DataValue::DoubleArray(_) => Ok(()),
                _ => Err(UpdateError::WrongType),
            },
            DataType::Timestamp => Err(UpdateError::UnsupportedType),
            DataType::TimestampArray => Err(UpdateError::UnsupportedType),
        }
    }

    pub fn apply(&mut self, update: EntryUpdate) -> HashSet<Field> {
        let mut updated = HashSet::new();
        if let Some(datapoint) = update.datapoint {
            self.datapoint = datapoint;
            updated.insert(Field::Datapoint);
        }
        if let Some(actuator_target) = update.actuator_target {
            self.actuator_target = actuator_target;
            updated.insert(Field::ActuatorTarget);
        }

        // TODO: Apply the other fields as well

        updated
    }
}

#[derive(Debug)]
pub enum SuccessfulUpdate {
    NoChange,
    ValueChanged,
}

impl Subscriptions {
    pub fn push(&mut self, subscription: Subscription) {
        self.subscriptions.push(subscription)
    }

    pub fn iter(&self) -> impl Iterator<Item = &Subscription> + '_ {
        self.subscriptions.iter()
    }

    pub fn clear(&mut self) {
        self.subscriptions.clear()
    }

    pub fn cleanup(&mut self) {
        self.subscriptions.retain(|sub| {
            if sub.sender.is_closed() {
                info!("Subscription ended: removing");
                false
            } else {
                true
            }
        });
    }
}

impl Subscription {
    fn generate_input(
        &self,
        changed: Option<&Vec<(i32, HashSet<Field>)>>,
        all: &Entries,
    ) -> Option<impl query::ExecutionInput> {
        let id_used_in_query = {
            let mut query_uses_id = false;
            match changed {
                Some(changed) => {
                    for (id, fields) in changed {
                        if let Some(entry) = all.get_by_id(*id) {
                            if self.query.input_spec.contains(&entry.metadata.path)
                                && fields.contains(&Field::Datapoint)
                            {
                                query_uses_id = true;
                                break;
                            }
                        }
                    }
                }
                None => {
                    // Always generate input if `changed` is None
                    query_uses_id = true;
                }
            }
            query_uses_id
        };

        if id_used_in_query {
            let mut input = query::ExecutionInputImpl::new();
            for name in self.query.input_spec.iter() {
                match all.get_by_path(name) {
                    Some(entry) => input.add(name.to_owned(), entry.datapoint.value.to_owned()),
                    None => {
                        // TODO: This should probably generate an error
                        input.add(name.to_owned(), DataValue::NotAvailable)
                    }
                }
            }
            Some(input)
        } else {
            None
        }
    }

    async fn notify(&self, input: &impl query::ExecutionInput) -> Result<(), NotificationError> {
        // Execute query (if anything queued)
        match self.query.execute(input) {
            Ok(result) => match result {
                Some(fields) => match self
                    .sender
                    .send(QueryResponse {
                        fields: fields
                            .iter()
                            .map(|e| QueryField {
                                name: e.0.to_owned(),
                                value: e.1.to_owned(),
                            })
                            .collect(),
                    })
                    .await
                {
                    Ok(()) => Ok(()),
                    Err(_) => Err(NotificationError {}),
                },
                None => Ok(()),
            },
            Err(e) => {
                // TODO: send error to subscriber
                debug!("{:?}", e);
                Ok(()) // no cleanup needed
            }
        }
    }
}

impl Entries {
    pub fn new() -> Self {
        Self {
            next_id: Default::default(),
            path_to_id: Default::default(),
            entries: Default::default(),
        }
    }

    pub fn get_by_id(&self, id: i32) -> Option<&Entry> {
        self.entries.get(&id)
    }

    pub fn get_by_path(&self, name: impl AsRef<str>) -> Option<&Entry> {
        match self.path_to_id.get(name.as_ref()) {
            Some(id) => self.get_by_id(*id),
            None => None,
        }
    }

    pub fn update_by_path(
        &mut self,
        path: impl AsRef<str>,
        update: EntryUpdate,
    ) -> Result<HashSet<Field>, UpdateError> {
        match self.path_to_id.get(path.as_ref()) {
            Some(id) => self.update(*id, update),
            None => Err(UpdateError::NotFound),
        }
    }

    pub fn update(&mut self, id: i32, update: EntryUpdate) -> Result<HashSet<Field>, UpdateError> {
        match self.entries.get_mut(&id) {
            Some(entry) => {
                // Reduce update to only include changes
                let update = entry.diff(update);
                // Validate update
                match entry.validate(&update) {
                    Ok(_) => {
                        let changed_fields = entry.apply(update);
                        Ok(changed_fields)
                    }
                    Err(err) => Err(err),
                }
            }
            None => Err(UpdateError::NotFound),
        }
    }

    pub fn add(
        &mut self,
        name: String,
        data_type: DataType,
        change_type: ChangeType,
        entry_type: EntryType,
        description: String,
        datapoint: Option<Datapoint>,
    ) -> Result<i32, RegistrationError> {
        if let Some(datapoint) = self.get_by_path(&name) {
            // It already exists
            return Ok(datapoint.metadata.id);
        };

        // Get next id (and bump it)
        let id = self.next_id.fetch_add(1, Ordering::SeqCst);

        // Map name -> id
        self.path_to_id.insert(name.clone(), id);

        // Add entry (mapped by id)
        self.entries.insert(
            id,
            Entry {
                metadata: Metadata {
                    id,
                    path: name,
                    data_type,
                    change_type,
                    entry_type,
                    description,
                },
                datapoint: match datapoint {
                    Some(datapoint) => datapoint,
                    None => Datapoint {
                        ts: SystemTime::now(),
                        value: DataValue::NotAvailable,
                    },
                },
                actuator_target: None,
            },
        );

        // Return the id
        Ok(id)
    }

    pub fn iter(&self) -> impl Iterator<Item = &Entry> + '_ {
        self.entries.values()
    }
}

impl query::CompilationInput for Entries {
    fn get_datapoint_type(&self, field: &str) -> Result<DataType, query::CompilationError> {
        match self.get_by_path(field) {
            Some(entry) => Ok(entry.metadata.data_type.to_owned()),
            None => Err(query::CompilationError::UnknownField(field.to_owned())),
        }
    }
}

impl DataBroker {
    pub fn new() -> Self {
        let (shutdown_trigger, _) = broadcast::channel::<()>(1);

        DataBroker {
            entries: Default::default(),
            subscriptions: Default::default(),
            shutdown_trigger,
        }
    }

    pub fn start_housekeeping_task(&self) {
        info!("Starting housekeeping task");
        let subscriptions = self.subscriptions.clone();
        tokio::spawn(async move {
            let mut interval = tokio::time::interval(std::time::Duration::from_secs(1));

            loop {
                interval.tick().await;
                // Cleanup dropped subscriptions
                subscriptions.write().await.cleanup();
            }
        });
    }

    pub async fn add_entry(
        &self,
        name: String,
        data_type: DataType,
        change_type: ChangeType,
        entry_type: EntryType,
        description: String,
    ) -> Result<i32, RegistrationError> {
        self.entries
            .write()
            .await
            .add(name, data_type, change_type, entry_type, description, None)
    }

    pub async fn get_id_by_path(&self, name: &str) -> Option<i32> {
        self.entries
            .read()
            .await
            .get_by_path(name)
            .map(|entry| entry.metadata.id)
    }

    pub async fn get_datapoint(&self, id: i32) -> Option<Datapoint> {
        self.entries
            .read()
            .await
            .get_by_id(id)
            .map(|entry| entry.datapoint.clone())
    }

    pub async fn get_datapoint_by_path(&self, name: &str) -> Option<Datapoint> {
        self.entries
            .read()
            .await
            .get_by_path(name)
            .map(|entry| entry.datapoint.clone())
    }

    pub async fn get_metadata(&self, id: i32) -> Option<Metadata> {
        self.entries
            .read()
            .await
            .get_by_id(id)
            .map(|item| item.metadata.clone())
    }

    pub async fn get_entry_by_path(&self, path: impl AsRef<str>) -> Option<Entry> {
        self.entries
            .read()
            .await
            .get_by_path(path.as_ref())
            .cloned()
    }

    pub async fn get_entry(&self, id: i32) -> Option<Entry> {
        self.entries.read().await.get_by_id(id).cloned()
    }

    pub async fn update_entries(
        &self,
        updates: impl IntoIterator<Item = (i32, EntryUpdate)>,
    ) -> Result<(), Vec<(i32, UpdateError)>> {
        let mut errors = Vec::new();
        let mut entries = self.entries.write().await;

        let cleanup_needed = {
            let updated: Vec<(i32, HashSet<Field>)> = updates
                .into_iter()
                .filter_map(|(id, update)| {
                    debug!("setting id {} to {:?}", id, update);
                    match entries.update(id, update) {
                        Ok(updated_fields) => {
                            if updated_fields.is_empty() {
                                None
                            } else {
                                Some((id, updated_fields))
                            }
                        }
                        Err(err) => {
                            errors.push((id, err));
                            None
                        }
                    }
                })
                .collect();

            // Downgrade to reader (to allow other readers) while holding on
            // to a read lock in order to ensure a consistent state while
            // notifying subscribers (no writes in between)
            let entries = entries.downgrade();

            // Notify
            let mut cleanup_needed = false;
            for sub in self.subscriptions.read().await.iter() {
                if let Some(input) = sub.generate_input(Some(&updated), &entries) {
                    match sub.notify(&input).await {
                        Ok(()) => {}
                        Err(_) => cleanup_needed = true,
                    }
                }
            }
            cleanup_needed
        };

        // Cleanup closed subscriptions
        if cleanup_needed {
            self.subscriptions.write().await.cleanup();
        }

        // Return errors if any
        if !errors.is_empty() {
            Err(errors)
        } else {
            Ok(())
        }
    }

    pub async fn subscribe(
        &self,
        query: &str,
    ) -> Result<impl Stream<Item = QueryResponse>, SubscriptionError> {
        let reader = self.entries.read().await;
        let compiled_query = query::compile(query, &*reader);

        match compiled_query {
            Ok(compiled_query) => {
                let (sender, receiver) = mpsc::channel(10);

                // Send the initial execution of query
                let subscription = Subscription {
                    query: compiled_query,
                    sender,
                };

                match subscription.generate_input(None, &reader) {
                    Some(input) => match subscription.notify(&input).await {
                        Ok(_) => self.subscriptions.write().await.push(subscription),
                        Err(_) => return Err(SubscriptionError::InternalError),
                    },
                    None => return Err(SubscriptionError::InternalError),
                };
                let stream = ReceiverStream::new(receiver);
                Ok(stream)
            }
            Err(e) => Err(SubscriptionError::CompilationError(format!("{:?}", e))),
        }
    }

    pub async fn shutdown(&self) {
        // Drain subscriptions
        let mut subscriptions = self.subscriptions.write().await;
        subscriptions.clear();

        // Signal shutdown
        let _ = self.shutdown_trigger.send(());
    }

    pub fn get_shutdown_trigger(&self) -> broadcast::Receiver<()> {
        self.shutdown_trigger.subscribe()
    }
}

impl Default for DataBroker {
    fn default() -> Self {
        Self::new()
    }
}

#[tokio::test]
async fn test_register_datapoint() {
    let datapoints = DataBroker::new();
    let id1 = datapoints
        .add_entry(
            "test.datapoint1".to_owned(),
            DataType::Bool,
            ChangeType::OnChange,
            EntryType::Sensor,
            "Test datapoint 1".to_owned(),
        )
        .await
        .expect("Register datapoint should succeed");

    {
        match datapoints.entries.read().await.get_by_id(id1) {
            Some(entry) => {
                assert_eq!(entry.metadata.id, id1);
                assert_eq!(entry.metadata.path, "test.datapoint1");
                assert_eq!(entry.metadata.data_type, DataType::Bool);
                assert_eq!(entry.metadata.description, "Test datapoint 1");
            }
            None => {
                panic!("datapoint should exist");
            }
        }
    }

    let id2 = datapoints
        .add_entry(
            "test.datapoint2".to_owned(),
            DataType::String,
            ChangeType::OnChange,
            EntryType::Sensor,
            "Test datapoint 2".to_owned(),
        )
        .await
        .expect("Register datapoint should succeed");

    {
        match datapoints.entries.read().await.get_by_id(id2) {
            Some(entry) => {
                assert_eq!(entry.metadata.id, id2);
                assert_eq!(entry.metadata.path, "test.datapoint2");
                assert_eq!(entry.metadata.data_type, DataType::String);
                assert_eq!(entry.metadata.description, "Test datapoint 2");
            }
            None => {
                panic!("no metadata returned");
            }
        }
    }

    let id3 = datapoints
        .add_entry(
            "test.datapoint1".to_owned(),
            DataType::Bool,
            ChangeType::OnChange,
            EntryType::Sensor,
            "Test datapoint 1 (modified)".to_owned(),
        )
        .await
        .expect("Register datapoint should succeed");

    assert_eq!(id3, id1);
}

#[tokio::test]
async fn test_get_set_datapoint() {
    let datapoints = DataBroker::new();

    let id1 = datapoints
        .add_entry(
            "test.datapoint1".to_owned(),
            DataType::Int32,
            ChangeType::OnChange,
            EntryType::Sensor,
            "Test datapoint 1".to_owned(),
        )
        .await
        .expect("Register datapoint should succeed");

    // Data point exists with value NotAvailable
    match datapoints.get_datapoint(id1).await {
        Some(datapoint) => {
            assert_eq!(datapoint.value, DataValue::NotAvailable);
        }
        None => {
            panic!("data point expected to exist");
        }
    }

    datapoints
        .update_entries([(
            id1,
            EntryUpdate {
                path: None,
                datapoint: Some(Datapoint {
                    ts: SystemTime::now(),
                    value: DataValue::Int32(100),
                }),
                actuator_target: None,
                entry_type: None,
                data_type: None,
                description: None,
            },
        )])
        .await
        .expect("setting datapoint #1");

    // Data point exists with value 100
    match datapoints.get_datapoint(id1).await {
        Some(datapoint) => {
            assert_eq!(datapoint.value, DataValue::Int32(100));
        }
        None => {
            panic!("data point expected to exist");
        }
    }
}

#[tokio::test]
async fn test_subscribe_and_get() {
    use tokio_stream::StreamExt;

    let datapoints = DataBroker::new();
    let id1 = datapoints
        .add_entry(
            "test.datapoint1".to_owned(),
            DataType::Int32,
            ChangeType::OnChange,
            EntryType::Sensor,
            "Test datapoint 1".to_owned(),
        )
        .await
        .expect("Register datapoint should succeed");

    let mut stream = datapoints
        .subscribe("SELECT test.datapoint1")
        .await
        .expect("Setup subscription");

    // Expect an initial query response
    // No value has been set yet, so value should be NotAvailable
    match stream.next().await {
        Some(query_resp) => {
            assert_eq!(query_resp.fields.len(), 1);
            assert_eq!(query_resp.fields[0].name, "test.datapoint1");
            assert_eq!(query_resp.fields[0].value, DataValue::NotAvailable);
        }
        None => {
            panic!("did not expect stream end")
        }
    }

    datapoints
        .update_entries([(
            id1,
            EntryUpdate {
                path: None,
                datapoint: Some(Datapoint {
                    ts: SystemTime::now(),
                    value: DataValue::Int32(101),
                }),
                actuator_target: None,
                entry_type: None,
                data_type: None,
                description: None,
            },
        )])
        .await
        .expect("setting datapoint #1");

    // Value has been set, expect the next item in stream to match.
    match stream.next().await {
        Some(query_resp) => {
            assert_eq!(query_resp.fields.len(), 1);
            assert_eq!(query_resp.fields[0].name, "test.datapoint1");
            assert_eq!(query_resp.fields[0].value, DataValue::Int32(101));
        }
        None => {
            panic!("did not expect stream end")
        }
    }

    // Check that the data point has been stored as well
    match datapoints.get_datapoint(id1).await {
        Some(datapoint) => {
            assert_eq!(datapoint.value, DataValue::Int32(101));
        }
        None => {
            panic!("data point expected to exist");
        }
    }
}

#[tokio::test]
async fn test_multi_subscribe() {
    use tokio_stream::StreamExt;

    let datapoints = DataBroker::new();
    let id1 = datapoints
        .add_entry(
            "test.datapoint1".to_owned(),
            DataType::Int32,
            ChangeType::OnChange,
            EntryType::Sensor,
            "Test datapoint 1".to_owned(),
        )
        .await
        .expect("Register datapoint should succeed");

    let mut subscription1 = datapoints
        .subscribe("SELECT test.datapoint1")
        .await
        .expect("setup first subscription");

    let mut subscription2 = datapoints
        .subscribe("SELECT test.datapoint1")
        .await
        .expect("setup second subscription");

    // Expect an initial query response
    // No value has been set yet, so value should be NotAvailable
    match subscription1.next().await {
        Some(query_resp) => {
            assert_eq!(query_resp.fields.len(), 1);
            assert_eq!(query_resp.fields[0].name, "test.datapoint1");
            assert_eq!(query_resp.fields[0].value, DataValue::NotAvailable);
        }
        None => {
            panic!("did not expect stream end")
        }
    }

    // Expect an initial query response
    // No value has been set yet, so value should be NotAvailable
    match subscription2.next().await {
        Some(query_resp) => {
            assert_eq!(query_resp.fields.len(), 1);
            assert_eq!(query_resp.fields[0].name, "test.datapoint1");
            assert_eq!(query_resp.fields[0].value, DataValue::NotAvailable);
        }
        None => {
            panic!("did not expect stream end")
        }
    }

    for i in 0..100 {
        datapoints
            .update_entries([(
                id1,
                EntryUpdate {
                    path: None,
                    datapoint: Some(Datapoint {
                        ts: SystemTime::now(),
                        value: DataValue::Int32(i),
                    }),
                    actuator_target: None,
                    entry_type: None,
                    data_type: None,
                    description: None,
                },
            )])
            .await
            .expect("setting datapoint #1");

        if let Some(query_resp) = subscription1.next().await {
            assert_eq!(query_resp.fields.len(), 1);
            assert_eq!(query_resp.fields[0].name, "test.datapoint1");
            if let DataValue::Int32(value) = query_resp.fields[0].value {
                assert_eq!(value, i);
            } else {
                panic!("expected test.datapoint1 to contain a value");
            }
        } else {
            panic!("did not expect end of stream");
        }

        if let Some(query_resp) = subscription2.next().await {
            assert_eq!(query_resp.fields.len(), 1);
            assert_eq!(query_resp.fields[0].name, "test.datapoint1");
            if let DataValue::Int32(value) = query_resp.fields[0].value {
                assert_eq!(value, i);
            } else {
                panic!("expected test.datapoint1 to contain a value");
            }
        } else {
            panic!("did not expect end of stream");
        }
    }
}

#[tokio::test]
async fn test_subscribe_after_new_registration() {
    use tokio_stream::StreamExt;

    let datapoints = DataBroker::new();
    let id1 = datapoints
        .add_entry(
            "test.datapoint1".to_owned(),
            DataType::Int32,
            ChangeType::OnChange,
            EntryType::Sensor,
            "Test datapoint 1".to_owned(),
        )
        .await
        .expect("Register datapoint should succeed");

    let mut subscription = datapoints
        .subscribe("SELECT test.datapoint1")
        .await
        .expect("Setup subscription");

    // Expect an initial query response
    // No value has been set yet, so value should be NotAvailable
    match subscription.next().await {
        Some(query_resp) => {
            assert_eq!(query_resp.fields.len(), 1);
            assert_eq!(query_resp.fields[0].name, "test.datapoint1");
            assert_eq!(query_resp.fields[0].value, DataValue::NotAvailable);
        }
        None => {
            panic!("did not expect stream end")
        }
    }

    datapoints
        .update_entries([(
            id1,
            EntryUpdate {
                path: None,
                datapoint: Some(Datapoint {
                    ts: SystemTime::now(),
                    value: DataValue::Int32(200),
                }),
                actuator_target: None,
                entry_type: None,
                data_type: None,
                description: None,
            },
        )])
        .await
        .expect("setting datapoint #1");

    match subscription.next().await {
        Some(query_resp) => {
            assert_eq!(query_resp.fields.len(), 1);
            assert_eq!(query_resp.fields[0].name, "test.datapoint1");
            assert_eq!(query_resp.fields[0].value, DataValue::Int32(200));
        }
        None => {
            panic!("did not expect stream end")
        }
    }

    match datapoints.get_datapoint(id1).await {
        Some(datapoint) => {
            assert_eq!(datapoint.value, DataValue::Int32(200));
        }
        None => {
            panic!("data point expected to exist");
        }
    }

    let id2 = datapoints
        .add_entry(
            "test.datapoint1".to_owned(),
            DataType::Int32,
            ChangeType::OnChange,
            EntryType::Sensor,
            "Test datapoint 1 (new description)".to_owned(),
        )
        .await
        .expect("Registration should succeed");

    assert_eq!(id1, id2, "Re-registration should result in the same id");

    datapoints
        .update_entries([(
            id1,
            EntryUpdate {
                path: None,
                datapoint: Some(Datapoint {
                    ts: SystemTime::now(),
                    value: DataValue::Int32(102),
                }),
                actuator_target: None,
                entry_type: None,
                data_type: None,
                description: None,
            },
        )])
        .await
        .expect("setting datapoint #1 (second time)");

    match subscription.next().await {
        Some(query_resp) => {
            assert_eq!(query_resp.fields.len(), 1);
            assert_eq!(query_resp.fields[0].name, "test.datapoint1");
            assert_eq!(query_resp.fields[0].value, DataValue::Int32(102));
        }
        None => {
            panic!("did not expect stream end")
        }
    }

    match datapoints.get_datapoint(id1).await {
        Some(datapoint) => {
            assert_eq!(datapoint.value, DataValue::Int32(102));
        }
        None => {
            panic!("data point expected to exist");
        }
    }
}

#[tokio::test]
async fn test_subscribe_set_multiple() {
    use tokio_stream::StreamExt;

    let datapoints = DataBroker::new();
    let id1 = datapoints
        .add_entry(
            "test.datapoint1".to_owned(),
            DataType::Int32,
            ChangeType::OnChange,
            EntryType::Sensor,
            "Test datapoint 1".to_owned(),
        )
        .await
        .expect("Register datapoint should succeed");

    let id2 = datapoints
        .add_entry(
            "test.datapoint2".to_owned(),
            DataType::Int32,
            ChangeType::OnChange,
            EntryType::Sensor,
            "Test datapoint 2".to_owned(),
        )
        .await
        .expect("Register datapoint should succeed");

    let mut subscription = datapoints
        .subscribe("SELECT test.datapoint1, test.datapoint2")
        .await
        .expect("setup first subscription");

    // Expect an initial query response
    // No value has been set yet, so value should be NotAvailable
    match subscription.next().await {
        Some(query_resp) => {
            assert_eq!(query_resp.fields.len(), 2);
            assert_eq!(query_resp.fields[0].name, "test.datapoint1");
            assert_eq!(query_resp.fields[0].value, DataValue::NotAvailable);
            assert_eq!(query_resp.fields[1].name, "test.datapoint2");
            assert_eq!(query_resp.fields[1].value, DataValue::NotAvailable);
        }
        None => {
            panic!("did not expect stream end")
        }
    }

    for i in 0..10 {
        datapoints
            .update_entries([
                (
                    id1,
                    EntryUpdate {
                        path: None,
                        datapoint: Some(Datapoint {
                            ts: SystemTime::now(),
                            value: DataValue::Int32(-i),
                        }),
                        actuator_target: None,
                        entry_type: None,
                        data_type: None,
                        description: None,
                    },
                ),
                (
                    id2,
                    EntryUpdate {
                        path: None,
                        datapoint: Some(Datapoint {
                            ts: SystemTime::now(),
                            value: DataValue::Int32(i),
                        }),
                        actuator_target: None,
                        entry_type: None,
                        data_type: None,
                        description: None,
                    },
                ),
            ])
            .await
            .expect("setting datapoint #1");
    }

    for i in 0..10 {
        if let Some(query_resp) = subscription.next().await {
            assert_eq!(query_resp.fields.len(), 2);
            assert_eq!(query_resp.fields[0].name, "test.datapoint1");
            if let DataValue::Int32(value) = query_resp.fields[0].value {
                assert_eq!(value, -i);
            } else {
                panic!("expected test.datapoint1 to contain a values");
            }
            assert_eq!(query_resp.fields[1].name, "test.datapoint2");
            if let DataValue::Int32(value) = query_resp.fields[1].value {
                assert_eq!(value, i);
            } else {
                panic!("expected test.datapoint2 to contain a values");
            }
        } else {
            panic!("did not expect end of stream");
        }
    }
}

#[tokio::test]
async fn test_bool_array() {
    let broker = DataBroker::new();
    let id = broker
        .add_entry(
            "Vehicle.TestArray".to_owned(),
            DataType::BoolArray,
            ChangeType::OnChange,
            EntryType::Sensor,
            "Run of the mill test array".to_owned(),
        )
        .await
        .unwrap();

    let ts = std::time::SystemTime::now();
    match broker
        .update_entries([(
            id,
            EntryUpdate {
                path: None,
                datapoint: Some(Datapoint {
                    ts,
                    value: DataValue::BoolArray(vec![true, true, false, true]),
                }),
                actuator_target: None,
                entry_type: None,
                data_type: None,
                description: None,
            },
        )])
        .await
    {
        Ok(()) => {}
        Err(e) => {
            panic!(
                "Expected set_datapoints to succeed ( with Ok(()) ), instead got: Err({:?})",
                e
            )
        }
    }

    match broker.get_datapoint(id).await {
        Some(datapoint) => {
            assert_eq!(datapoint.ts, ts);
            assert_eq!(
                datapoint.value,
                DataValue::BoolArray(vec![true, true, false, true])
            );
        }
        None => panic!("Expected get_datapoint to return a datapoint, instead got: None"),
    }
}

#[tokio::test]
async fn test_string_array() {
    let broker = DataBroker::new();
    let id = broker
        .add_entry(
            "Vehicle.TestArray".to_owned(),
            DataType::StringArray,
            ChangeType::OnChange,
            EntryType::Sensor,
            "Run of the mill test array".to_owned(),
        )
        .await
        .unwrap();

    let ts = std::time::SystemTime::now();
    match broker
        .update_entries([(
            id,
            EntryUpdate {
                path: None,
                datapoint: Some(Datapoint {
                    ts,
                    value: DataValue::StringArray(vec![
                        String::from("yes"),
                        String::from("no"),
                        String::from("maybe"),
                        String::from("nah"),
                    ]),
                }),
                actuator_target: None,
                entry_type: None,
                data_type: None,
                description: None,
            },
        )])
        .await
    {
        Ok(_) => {}
        Err(e) => {
            panic!(
                "Expected set_datapoints to succeed ( with Ok(()) ), instead got: Err({:?})",
                e
            )
        }
    }

    match broker.get_datapoint(id).await {
        Some(datapoint) => {
            assert_eq!(datapoint.ts, ts);
            assert_eq!(
                datapoint.value,
                DataValue::StringArray(vec![
                    String::from("yes"),
                    String::from("no"),
                    String::from("maybe"),
                    String::from("nah"),
                ])
            );
        }
        None => panic!("Expected get_datapoint to return a datapoint, instead got: None"),
    }
}

#[tokio::test]
async fn test_int8_array() {
    let broker = DataBroker::new();
    let id = broker
        .add_entry(
            "Vehicle.TestArray".to_owned(),
            DataType::Int8Array,
            ChangeType::OnChange,
            EntryType::Sensor,
            "Run of the mill test array".to_owned(),
        )
        .await
        .unwrap();

    let ts = std::time::SystemTime::now();
    match broker
        .update_entries([(
            id,
            EntryUpdate {
                path: None,
                datapoint: Some(Datapoint {
                    ts,
                    value: DataValue::Int32Array(vec![10, 20, 30, 40]),
                }),
                actuator_target: None,
                entry_type: None,
                data_type: None,
                description: None,
            },
        )])
        .await
    {
        Ok(()) => {}
        Err(e) => {
            panic!(
                "Expected set_datapoints to succeed ( with Ok(()) ), instead got: Err({:?})",
                e
            )
        }
    }

    if broker
        .update_entries([(
            id,
            EntryUpdate {
                path: None,
                datapoint: Some(Datapoint {
                    ts,
                    value: DataValue::Int32Array(vec![100, 200, 300, 400]),
                }),
                actuator_target: None,
                entry_type: None,
                data_type: None,
                description: None,
            },
        )])
        .await
        .is_ok()
    {
        panic!("Expected set_datapoints to fail ( with Err() ), instead got: Ok(())",)
    }

    match broker.get_datapoint(id).await {
        Some(datapoint) => {
            assert_eq!(datapoint.ts, ts);
            assert_eq!(datapoint.value, DataValue::Int32Array(vec![10, 20, 30, 40]));
        }
        None => panic!("Expected get_datapoint to return a datapoint, instead got: None"),
    }
}

#[tokio::test]
async fn test_uint8_array() {
    let broker = DataBroker::new();
    let id = broker
        .add_entry(
            "Vehicle.TestArray".to_owned(),
            DataType::Uint8Array,
            ChangeType::OnChange,
            EntryType::Sensor,
            "Run of the mill test array".to_owned(),
        )
        .await
        .unwrap();

    let ts = std::time::SystemTime::now();
    match broker
        .update_entries([(
            id,
            EntryUpdate {
                path: None,
                datapoint: Some(Datapoint {
                    ts,
                    value: DataValue::Uint32Array(vec![10, 20, 30, 40]),
                }),
                actuator_target: None,
                entry_type: None,
                data_type: None,
                description: None,
            },
        )])
        .await
    {
        Ok(()) => {}
        Err(e) => {
            panic!(
                "Expected set_datapoints to succeed ( with Ok(()) ), instead got: Err({:?})",
                e
            )
        }
    }

    if broker
        .update_entries([(
            id,
            EntryUpdate {
                path: None,
                datapoint: Some(Datapoint {
                    ts,
                    value: DataValue::Uint32Array(vec![100, 200, 300, 400]),
                }),
                actuator_target: None,
                entry_type: None,
                data_type: None,
                description: None,
            },
        )])
        .await
        .is_ok()
    {
        panic!("Expected set_datapoints to fail ( with Err() ), instead got: Ok(())",)
    }

    match broker.get_datapoint(id).await {
        Some(datapoint) => {
            assert_eq!(datapoint.ts, ts);
            assert_eq!(
                datapoint.value,
                DataValue::Uint32Array(vec![10, 20, 30, 40])
            );
        }
        None => panic!("Expected get_datapoint to return a datapoint, instead got: None"),
    }
}

#[tokio::test]
async fn test_float_array() {
    let broker = DataBroker::new();
    let id = broker
        .add_entry(
            "Vehicle.TestArray".to_owned(),
            DataType::FloatArray,
            ChangeType::OnChange,
            EntryType::Sensor,
            "Run of the mill test array".to_owned(),
        )
        .await
        .unwrap();

    let ts = std::time::SystemTime::now();
    match broker
        .update_entries([(
            id,
            EntryUpdate {
                path: None,
                datapoint: Some(Datapoint {
                    ts,
                    value: DataValue::FloatArray(vec![10.0, 20.0, 30.0, 40.0]),
                }),
                actuator_target: None,
                entry_type: None,
                data_type: None,
                description: None,
            },
        )])
        .await
    {
        Ok(()) => {}
        Err(e) => {
            panic!(
                "Expected set_datapoints to succeed ( with Ok(()) ), instead got: Err({:?})",
                e
            )
        }
    }

    match broker.get_datapoint(id).await {
        Some(datapoint) => {
            assert_eq!(datapoint.ts, ts);
            assert_eq!(
                datapoint.value,
                DataValue::FloatArray(vec![10.0, 20.0, 30.0, 40.0])
            );
        }
        None => panic!("Expected get_datapoint to return a datapoint, instead got: None"),
    }
}
