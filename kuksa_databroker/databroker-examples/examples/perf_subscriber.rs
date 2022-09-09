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

use databroker_proto::sdv::databroker as proto;

use std::time::Instant;

#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    match tonic::transport::Channel::from_static("http://127.0.0.1:55555")
        .connect()
        .await
    {
        Ok(channel) => {
            let mut client = proto::v1::broker_client::BrokerClient::new(channel.clone());

            let mut n: i32 = 0;

            let args = tonic::Request::new(proto::v1::SubscribeRequest {
                query: String::from("SELECT Vehicle.ADAS.ABS.Error"),
            });

            let mut now = Instant::now();
            // let mut last_status = Instant::now();

            match client.subscribe(args).await {
                Ok(response) => {
                    let mut stream = response.into_inner();
                    let mut running = true;
                    let mut started = false;
                    while running {
                        if let Some(update) = stream.message().await? {
                            for (_id, datapoint) in update.fields {
                                if let Some(value) = datapoint.value {
                                    match value {
                                        proto::v1::datapoint::Value::FailureValue(reason) => {
                                            if started {
                                                eprintln!("-> Failure: {:?}", reason);
                                            }
                                        }
                                        proto::v1::datapoint::Value::StringValue(string_value) => {
                                            match string_value.as_str() {
                                                "start" => {
                                                    eprintln!("START");
                                                    started = true;
                                                    now = Instant::now();
                                                }
                                                "end" => {
                                                    if started {
                                                        running = false;
                                                        let seconds = now.elapsed().as_secs_f64();
                                                        // eprintln!("\rReceived {} messages ({:.1} / s)", n, n as f64 / seconds);
                                                        eprintln!("\rEND                                                           ");
                                                        eprintln!(
                                                            "{} messages received ({:.1} / s)",
                                                            n,
                                                            n as f64 / seconds
                                                        );
                                                        eprintln!("Completed in {:.3} s", seconds);
                                                    }
                                                }
                                                _ => {
                                                    started = true;
                                                    n += 1;
                                                    if (n % 1000) == 0 {
                                                        let seconds = now.elapsed().as_secs_f64();
                                                        eprint!(
                                                            "\rReceived {} messages ({:.1} / s)",
                                                            n,
                                                            n as f64 / seconds
                                                        );
                                                    }
                                                }
                                            }
                                        }
                                        _ => eprintln!("-> Other value"),
                                    }
                                } else {
                                    eprintln!("-> Empty datapoint value");
                                }
                                // }
                            }
                        } else {
                            eprintln!("Server gone. Subscription stopped");
                            running = false;
                        }
                    }
                    // if let Ok(trailers) = stream.trailers().await {
                    //     eprintln!("{:?}", trailers);
                    // }
                }
                Err(err) => {
                    eprintln!("{}", err.message());
                }
            }
        }
        Err(err) => {
            eprintln!("{}", err);
        }
    }

    Ok(())
}
