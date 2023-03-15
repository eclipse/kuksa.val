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

use prost_types::Timestamp;

use tokio::sync::mpsc;
use tokio_stream::wrappers::ReceiverStream;

use std::collections::HashMap;
use std::env;
use std::time::{Instant, SystemTime};

const DEFAULT_ITERATIONS: i32 = 1000;
const DEFAULT_NTH_MESSAGE: i32 = 1;

fn create_payload(value: &str, id: i32) -> proto::v1::StreamDatapointsRequest {
    let ts = Timestamp::from(SystemTime::now());
    proto::v1::StreamDatapointsRequest {
        datapoints: HashMap::from([(
            id,
            proto::v1::Datapoint {
                timestamp: Some(ts),
                value: Some(proto::v1::datapoint::Value::StringValue(value.to_string())),
            },
        )]),
    }
}

async fn run_streaming_set_test(iterations: i32, n_th_message: i32) {
    let connect = tonic::transport::Channel::from_static("http://127.0.0.1:55555")
        .connect()
        .await;
    match connect {
        Ok(channel) => {
            let mut client = proto::v1::collector_client::CollectorClient::with_interceptor(
                channel,
                |mut req: tonic::Request<()>| {
                    req.metadata_mut().append("authorization",
                tonic::metadata::AsciiMetadataValue::from_str("Bearer eyJ0eXAiOiJKV1QiLCJhbGciOiJSUzI1NiJ9.eyJzdWIiOiJsb2NhbCBkZXYiLCJpc3MiOiJjcmVhdGVUb2tlbi5weSIsImF1ZCI6WyJrdWtzYS52YWwiXSwiaWF0IjoxNTE2MjM5MDIyLCJleHAiOjE3NjcyMjU1OTksInNjb3BlIjoicHJvdmlkZSBjcmVhdGUifQ.h7F2q5pXJ0VAGKQJQKrRwj_RZIhWRb5y6_7YXwhnAv-sH5tk_LSXy7UQPEE2pO8Bzp2xCOD3gjkulJZZ49Xk-0sLgedb9YLgONfgcsySaknOTLB0PSbdBMVXhtfuNyTN9RMoeW4gLsNaitVw-_QM027nmaqzaQmgNb8GyBqonqrZD8jjFTf2e6wHYg1DuvxooMoI1gn8r_weXYmK8ksBJrL4097FD5jghF9mJKzKG25oM6wl2vkibmv3l1ZmHdilA_QIbt7ZXAh0VO8NVwUfWWxitimSs27w3CE2wvcwWI7hgLHOR2wJEqkgaMaOckVbkbJPXDrWQz-uDyCwOr2oICpUELOp1j-lbHTIH1dunmilldLMJlVbatMDmYtGYDmvZQ470aAH-Df_fN4WtoxdWHQekNBjr0TIM-vP1-uucFPprMWFtUjjejwQVNVBRz_HaZiOObt4jku0VHv1fP3y7MJmJ6M--RSQlGNnvtKzJXG9exeN3wmkyBpdnVFQ4s_EUn88kkneiPZAas7zAhRQhBjwLS7n0j761wXAUrEQPQ0YQ7AhhopZLmrhiJeGAoZQ5kazUb8p2qIiIroLDGoO6gvd6RK_M-3EJ3mGaUdqrEKxWk9-r5lu6UcE1Zl7XXr_AHD2sD9mwVpuFJ_mYHN5hvvcZUjkmrBdGTkryAfatwY").unwrap());
                    Ok(req)
                },
            );

            let datapoint1_id = match client
                .register_datapoints(tonic::Request::new(proto::v1::RegisterDatapointsRequest {
                    list: vec![proto::v1::RegistrationMetadata {
                        name: "Vehicle.ADAS.ABS.Error".to_owned(),
                        description: "Vehicle.ADAS.ABS.Error".to_owned(),
                        data_type: proto::v1::DataType::String as i32,
                        change_type: proto::v1::ChangeType::Continuous as i32,
                    }],
                }))
                .await
            {
                Ok(metadata) => metadata.into_inner().results["Vehicle.ADAS.ABS.Error"],
                Err(err) => {
                    println!("Couldn't retrieve metadata: {err:?}");
                    -1
                }
            };

            if datapoint1_id == -1 {
                return;
            }
            let (tx, rx) = mpsc::channel(10);
            let now = Instant::now();

            let sender = tokio::spawn(async move {
                match client.stream_datapoints(ReceiverStream::new(rx)).await {
                    Ok(response) => {
                        let mut stream = response.into_inner();

                        while let Ok(message) = stream.message().await {
                            match message {
                                Some(message) => {
                                    for error in message.errors {
                                        println!(
                                            "Error setting datapoint {}: {:?}",
                                            error.0,
                                            proto::v1::DatapointError::from_i32(error.1)
                                        )
                                    }
                                }
                                None => {
                                    break;
                                }
                            }
                        }
                        // match PropertyStatus::from_i32(message.status) {
                        //     Some(status) => {
                        //         println!("-> status: {:?}", status)
                        //     }
                        //     None => println!("-> status: Unknown"),
                        // }
                    }
                    Err(err) => {
                        // println!("-> status: {}",code_to_text(&err.code()));
                        println!("{}", err.message());
                    }
                }
            });

            let feeder = tokio::spawn(async move {
                // send start message
                match tx.send(create_payload("start", datapoint1_id)).await {
                    Ok(_) => {
                        eprintln!("START");
                    }
                    Err(err) => eprint!("{err}"),
                };

                let mut n: i32 = 0;
                let mut n_id: i32 = 0;

                // send event messages
                for i in 0..iterations {
                    // Every N:th message is of the subscribed type
                    let id = if (n % n_th_message) == 0 {
                        datapoint1_id
                    } else {
                        11
                    };

                    match tx.send(create_payload("event", id)).await {
                        Ok(_) => {
                            if (i % 1000) == 0 {
                                // eprint!("#");
                                let seconds = now.elapsed().as_secs_f64();
                                eprint!("\r{} messages sent ({:.1} / s)", n, n as f64 / seconds);
                            }
                            if id == datapoint1_id {
                                n_id += 1;
                            }
                            n += 1;
                        }
                        Err(err) => eprint!("{err}"),
                    };
                }

                // send end message
                match tx.send(create_payload("end", datapoint1_id)).await {
                    Ok(_) => {
                        eprintln!("\rEND                                                    ");
                    }
                    Err(err) => eprint!("{err}"),
                };

                (n, n_id)
            });

            let (n, n_id) = feeder.await.unwrap();
            match sender.await {
                Ok(_) => {}
                Err(err) => eprint!("{err}"),
            };

            let seconds = now.elapsed().as_secs_f64();
            println!(
                "Pushed {} total messages ({:.1} / s)",
                n,
                n as f64 / seconds
            );
            println!(
                "Pushed {} matching messages ({:.1} / s)",
                n_id,
                n_id as f64 / seconds
            );
            println!("Completed in {seconds:.3} s");
        }
        Err(err) => {
            println!("{err}");
        }
    }
}

#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    let args: Vec<String> = env::args().collect();
    let iterations = match args.get(1) {
        Some(arg1) => match arg1.parse::<i32>() {
            Ok(number) => number,
            Err(_) => DEFAULT_ITERATIONS,
        },
        None => DEFAULT_ITERATIONS,
    };

    let queue_size = match args.get(2) {
        Some(arg1) => match arg1.parse::<i32>() {
            Ok(number) => number,
            Err(_) => DEFAULT_NTH_MESSAGE,
        },
        None => DEFAULT_NTH_MESSAGE,
    };

    println!("INPUT: Set {iterations} times");

    // run_set_test(iterations).await;
    run_streaming_set_test(iterations, queue_size).await;

    Ok(())
}
