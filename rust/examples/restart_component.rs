use std::{thread, time::Duration};

use gg_sdk::Sdk;

fn main() {
    let sdk = Sdk::init();
    sdk.connect().expect("Failed to connect to GG nucleus");
    println!("Connected to GG nucleus.");

    println!("Sleeping for 15 seconds before restart...");
    for i in (1..=15).rev() {
        println!("Restart in {i} seconds");
        thread::sleep(Duration::from_secs(1));
    }

    println!("Restarting component 'aws-greengrass-component-sdk.samples.restart_component'...");
    sdk.restart_component(
        "aws-greengrass-component-sdk.samples.restart_component",
    )
    .expect("Failed to restart component");

    println!("Restart request sent successfully.");
}
