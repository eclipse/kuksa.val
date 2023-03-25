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
            let mut client = proto::v1::broker_client::BrokerClient::with_interceptor(
                channel,
                |mut req: tonic::Request<()>| {
                    req.metadata_mut().append("authorization",
                        "Bearer eyJ0eXAiOiJKV1QiLCJhbGciOiJSUzI1NiJ9.eyJzdWIiOiJsb2NhbCBkZXYiLCJpc3MiOiJjcmVhdGVUb2tlbi5weSIsImF1ZCI6WyJrdWtzYS52YWwiXSwiaWF0IjoxNTE2MjM5MDIyLCJleHAiOjE3NjcyMjU1OTksInNjb3BlIjoicmVhZCJ9.P6tJPRSJWB51UOFDFs8qQ-lGqb1NoWgCekHUKyMiYcs8sR3FGVKSRjSkcqv1tXOlILvqhUwyuTKui25_kFKkTPv47GI0xAqcXtaTmDwHAWZHFC6HWGWGXohu7XvURrim5kMRVHy_VGlzasGgVap0JFk3wmaY-nyFYL_PLDjvGjIQuOwFiUtKK1PfiKviZKyc5EzPUEAoHxFL_BSOsTdDDcaydFe9rSKJzpYrj7qXY0hMJCje2BUGlSUIttR95aSjOZflSxiGystWHME8fKMmDERAx749Jpt37M3taCxBsUzER5olPz65MGzFSikfC-jH_KGmJ4zNYS65_OM1a-CPfW7Ts__pyAXxFULNMHRMIfh8Wiig4UcooMy_ZJO_DN2rq95XdaBbzRua5mxvO2wM6iu5kv4lhNxhjVNGuWFRLLJ_icBUZlvAuC3eqp66B-Y3jJNI0cSnIvsVX8YFVS3ebW8tf40OdeVou8fWZPcQsFAAafBhIxNOW8FbLZ9sRvQ-FGwZy-GyF52IJ5ZKeGfAkeEh9ZLIcyJ2YlGp4q0EOKIdwIBsWfCFtZbAvi2ornO3XvJm94NBqprpvQYN_IB7yyRxDduLjNKqqcFqnrlWYI-ZhvghWH2rEblplgHZdyVD1G9Mbv0_zdNTKFs6J7IP96aV6-4hBOt3kROlS1G7ObA"
                        .parse::<tonic::metadata::AsciiMetadataValue>()
                        .unwrap()
                    );
                    Ok(req)
                },
            );
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
                                                eprintln!("-> Failure: {reason:?}");
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
                                                        eprintln!("Completed in {seconds:.3} s");
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
            eprintln!("{err}");
        }
    }

    Ok(())
}
