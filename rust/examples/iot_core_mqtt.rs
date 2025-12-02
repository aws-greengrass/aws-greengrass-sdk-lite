use std::{thread, time::Duration};

use gg_sdk::{Qos, Sdk};

fn main() {
    let sdk = Sdk::init();
    sdk.connect().expect("Failed to connect to GG nucleus");
    println!("Connected to GG nucleus.");

    let callback = move |topic: &str, payload: &[u8]| {
        let payload_str = String::from_utf8_lossy(payload);
        println!("Received [{payload_str}] on [{topic}].");
    };

    let _sub = sdk
        .subscribe_to_iot_core("hello", Qos::AtMostOnce, &callback)
        .expect("Failed to subscribe to topic");
    println!("Subscribed to topic.");

    loop {
        sdk.publish_to_iot_core("hello", b"world", Qos::AtMostOnce)
            .expect("Failed to publish to topic");
        println!("Published to topic.");

        thread::sleep(Duration::from_secs(15));
    }
}
