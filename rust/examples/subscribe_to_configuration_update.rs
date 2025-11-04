use std::{mem::MaybeUninit, sync::mpsc};

use ggl_sdk::Sdk;

fn main() {
    let sdk = Sdk::init();
    sdk.connect().expect("Failed to connect to IPC");
    println!("Connected to Greengrass IPC");

    let (tx, rx) = mpsc::sync_channel(1);

    let _sub = sdk
        .subscribe_to_configuration_update(
            None,
            &["test_str"],
            move |component_name, key_path| {
                println!("Configuration update received:");
                println!("  Component: {component_name}");
                print!("  Key path: [");
                for (i, key) in key_path.iter().enumerate() {
                    if i > 0 {
                        print!(", ");
                    }
                    print!("\"{key}\"");
                }
                println!("]");

                let _ = tx.try_send(());
            },
        )
        .expect("Failed to subscribe to configuration updates");

    println!("Subscribed to configuration updates. Waiting for updates...");

    let mut prev_value = String::new();
    let mut buf = [MaybeUninit::uninit(); 500];

    loop {
        rx.recv().unwrap();

        match sdk.get_config_str(&["test_str"], None, &mut buf) {
            Ok(config) => {
                if config != prev_value {
                    println!("Updated configuration value: {config}");
                    prev_value = config.to_string();
                }
            }
            Err(e) => eprintln!("Failed to get configuration: {e:?}"),
        }
    }
}
