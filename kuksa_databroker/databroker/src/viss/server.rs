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

use axum::{
    extract::ws::{CloseFrame, Message, WebSocket, WebSocketUpgrade},
    response::IntoResponse,
    routing::get,
    Router,
};
use std::{borrow::Cow, net::SocketAddr};

use tracing::{debug, error, info, trace};

use futures::{channel::mpsc, Sink};
use futures::{stream::StreamExt, Stream};

use crate::authorization::Authorization;
use crate::broker;

use super::v2::{self, server::Viss};

#[derive(Clone)]
struct AppState {
    broker: broker::DataBroker,
    authorization: Authorization,
}

pub async fn serve(
    addr: impl Into<std::net::SocketAddr>,
    broker: broker::DataBroker,
    // #[cfg(feature = "tls")] server_tls: ServerTLS,
    authorization: Authorization,
    // signal: F
) -> Result<(), Box<dyn std::error::Error>> {
    let app = Router::new()
        .route("/", get(handle_upgrade))
        .with_state(AppState {
            broker,
            authorization,
        });

    let addr = addr.into();
    let builder = axum::Server::try_bind(&addr).map_err(|err| {
        error!("Failed to bind address {addr}: {err}");
        err
    })?;

    info!("VISSv2 (websocket) service listening on {}", addr);
    builder
        .serve(app.into_make_service_with_connect_info::<SocketAddr>())
        .await
        .map_err(|e| e.into())
}

// Handle upgrade request
async fn handle_upgrade(
    ws: WebSocketUpgrade,
    axum::extract::ConnectInfo(addr): axum::extract::ConnectInfo<SocketAddr>,
    axum::extract::State(state): axum::extract::State<AppState>,
) -> impl IntoResponse {
    debug!("Received websocket upgrade request");
    ws.protocols(["VISSv2"])
        .on_upgrade(move |socket| handle_websocket(socket, addr, state.broker, state.authorization))
}

// Handle websocket (one per connection)
async fn handle_websocket(
    socket: WebSocket,
    addr: SocketAddr,
    broker: broker::DataBroker,
    authorization: Authorization,
) {
    let valid_subprotocol = match socket.protocol() {
        Some(subprotocol) => match subprotocol.to_str() {
            Ok("VISSv2") => {
                debug!("VISSv2 requested");
                true
            }
            Ok(_) | Err(_) => {
                debug!("Unsupported websocket subprotocol");
                false
            }
        },
        None => {
            debug!("Websocket subprotocol not specified, defaulting to VISSv2");
            true
        }
    };
    let mut socket = socket;
    if !valid_subprotocol {
        let _ = socket
            .send(Message::Close(Some(CloseFrame {
                code: axum::extract::ws::close_code::PROTOCOL,
                reason: Cow::from("Unsupported websocket subprotocol"),
            })))
            .await;
        return;
    }

    let (write, read) = socket.split();

    handle_viss_v2(write, read, addr, broker, authorization).await;
}

async fn handle_viss_v2<W, R>(
    write: W,
    mut read: R,
    client_addr: SocketAddr,
    broker: broker::DataBroker,
    authorization: Authorization,
) where
    W: Sink<Message> + Unpin + Send + 'static,
    <W as Sink<Message>>::Error: Send,
    R: Stream<Item = Result<Message, axum::Error>> + Unpin + Send + 'static,
{
    // Create a multi producer / single consumer channel, where the
    // single consumer will write to the socket.
    let (sender, receiver) = mpsc::channel::<Message>(10);

    let server = v2::server::Server::new(broker, authorization);
    let mut write_task = tokio::spawn(async move {
        let _ = receiver.map(Ok).forward(write).await;
    });

    let mut read_task = tokio::spawn(async move {
        while let Some(Ok(msg)) = read.next().await {
            match msg {
                Message::Text(msg) => {
                    debug!("Received request: {msg}");

                    // Handle it
                    let sender = sender.clone();
                    let serialized_response = match parse_v2_msg(&msg) {
                        Ok(request) => {
                            match request {
                                v2::Request::Get(request) => match server.get(request).await {
                                    Ok(response) => serialize(response),
                                    Err(error_response) => serialize(error_response),
                                },
                                v2::Request::Set(request) => match server.set(request).await {
                                    Ok(response) => serialize(response),
                                    Err(error_response) => serialize(error_response),
                                },
                                v2::Request::Subscribe(request) => {
                                    match server.subscribe(request).await {
                                        Ok((response, stream)) => {
                                            // Setup background stream
                                            let mut background_sender = sender.clone();

                                            tokio::spawn(async move {
                                                let mut stream = stream;
                                                while let Some(event) = stream.next().await {
                                                    let serialized_event = match event {
                                                        Ok(event) => serialize(event),
                                                        Err(error_event) => serialize(error_event),
                                                    };

                                                    if let Ok(text) = serialized_event {
                                                        debug!("Sending notification: {}", text);
                                                        if let Err(err) = background_sender
                                                            .try_send(Message::Text(text))
                                                        {
                                                            debug!("Failed to send notification: {err}");
                                                            if err.is_disconnected() {
                                                                break;
                                                            }
                                                        };
                                                    }
                                                }
                                            });

                                            // Return response
                                            serialize(response)
                                        }
                                        Err(error_response) => serialize(error_response),
                                    }
                                }
                                v2::Request::Unsubscribe(request) => {
                                    match server.unsubscribe(request).await {
                                        Ok(response) => serialize(response),
                                        Err(error_response) => serialize(error_response),
                                    }
                                }
                            }
                        }
                        Err(error_response) => serialize(error_response),
                    };

                    // Send it
                    if let Ok(text) = serialized_response {
                        debug!("Sending response: {}", text);
                        let mut sender = sender;
                        if let Err(err) = sender.try_send(Message::Text(text)) {
                            debug!("Failed to send response: {err}")
                        };
                    }
                }
                Message::Binary(msg) => {
                    debug!(
                        "Received binary message from {} (ignoring it) len={}: {:?}",
                        client_addr,
                        msg.len(),
                        msg
                    );
                }
                Message::Close(msg) => {
                    if let Some(close) = msg {
                        debug!(
                            "Received close connection request from {}: code = {}, reason: \"{}\"",
                            client_addr, close.code, close.reason
                        );
                    } else {
                        debug!(
                            "Received close connection request from {}: (missing CloseFrame)",
                            client_addr
                        );
                    }
                    break;
                }
                Message::Pong(msg) => {
                    trace!("Received pong message from {}: {:?}", client_addr, msg);
                }
                Message::Ping(msg) => {
                    // Handled automatically
                    trace!("Received ping message from {}: {:?}", client_addr, msg);
                }
            }
        }
    });

    // If any one of the tasks exit, abort the other.
    tokio::select! {
        _ = (&mut read_task) => {
            debug!("Read task completed, aborting write task.");
            write_task.abort();
        }
        _ = (&mut write_task) => {
            debug!("Write task completed, aborting read task.");
            read_task.abort();
        },
    }
    info!("Websocket connection closed ({})", client_addr);
}

fn parse_v2_msg(msg: &str) -> Result<v2::Request, v2::GenericErrorResponse> {
    let request: v2::Request =
        serde_json::from_str(msg).map_err(|_| {
            match serde_json::from_str::<v2::GenericRequest>(msg) {
                Ok(request) => v2::GenericErrorResponse {
                    action: request.action,
                    request_id: request.request_id,
                    error: v2::Error::BadRequest { msg: None },
                },
                Err(_) => v2::GenericErrorResponse {
                    action: None,
                    request_id: None,
                    error: v2::Error::BadRequest { msg: None },
                },
            }
        })?;
    Ok(request)
}

fn serialize(response: impl v2::Response) -> Result<String, serde_json::Error> {
    serde_json::to_string(&response)
}
