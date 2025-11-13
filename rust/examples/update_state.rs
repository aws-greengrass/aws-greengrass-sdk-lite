use std::{thread, time::Duration};

use gg_sdk::{ComponentState, Sdk};

fn main() {
    let sdk = Sdk::init();
    sdk.connect().expect("Failed to connect to GG nucleus");
    println!("Connected to GG nucleus.");

    println!("Sleeping for 10 seconds");
    thread::sleep(Duration::from_secs(10));

    println!("Updating component state to RUNNING.");
    sdk.update_state(ComponentState::Running)
        .expect("Failed to update component state");

    println!("Component state updated successfully");

    loop {
        thread::sleep(Duration::from_secs(600));
    }
}
