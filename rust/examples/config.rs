use std::{mem::MaybeUninit, thread, time::Duration};

use gg_sdk::{Sdk, UnpackedObject};

fn main() {
    let sdk = Sdk::init();
    sdk.connect().expect("Failed to connect to GG nucleus");
    println!("Connected to GG nucleus.");

    let mut buf = [MaybeUninit::uninit(); 500];
    let test_str = sdk
        .get_config_str(&["test_str"], None, &mut buf)
        .expect("Failed to get test_str");
    println!("test_str value is {test_str}.");

    if let Ok(subkey2) = sdk.get_config_str(
        &["sample_map", "key2_map", "subkey2"],
        None,
        &mut buf,
    ) {
        println!("subkey2 value is {subkey2}.");
    }

    if let Ok(key2_map) =
        sdk.get_config(&["sample_map", "key2_map"], None, &mut buf)
    {
        if let UnpackedObject::Map(map) = key2_map.unpack() {
            println!("key2_map has {} entries", map.len());
            for kv in map.iter() {
                if let UnpackedObject::Buf(val) = kv.val().unpack() {
                    println!("  {}: {}", kv.key(), val);
                }
            }
        }
    }

    if let Ok(obj) = sdk.get_config(&["sample_map", "key3"], None, &mut buf) {
        if let UnpackedObject::I64(key3) = obj.unpack() {
            println!("key3 value is {key3}.");
        }
    }

    let init_val = 1i64;
    println!("Initializing test_num value to {init_val}.");

    sdk.update_config(&["test_num"], None, init_val)
        .expect("Failed to update test_num");

    loop {
        let val = match sdk
            .get_config(&["test_num"], None, &mut buf)
            .expect("Failed to get test_num")
            .unpack()
        {
            UnpackedObject::I64(v) => v,
            UnpackedObject::F64(v) => v as i64,
            _ => panic!("Unexpected type for test_num"),
        };
        println!("test_num value is {val}.");

        let val = val + 1;
        println!("Setting test_num value to {val}.");

        sdk.update_config(&["test_num"], None, val)
            .expect("Failed to update test_num");

        thread::sleep(Duration::from_secs(15));
    }
}
