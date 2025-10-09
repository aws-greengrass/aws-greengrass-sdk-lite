# Subscribe to Configuration Update Sample

This sample demonstrates how to subscribe to configuration updates for a
Greengrass component.

## Overview

The sample subscribes to it's own configuration updates and prints notifications
whenever the configuration changes. This is useful for components that need to
react to configuration changes in real-time.

## Building

Build the sample from the project root:

```sh
cmake -B build -D CMAKE_BUILD_TYPE=MinSizeRel -D BUILD_SAMPLES=ON
make -C build -j$(nproc)
```

The binary will be available at
`./build/bin/sample_subscribe_to_configuration_update`.

## Usage

### ⚠️ Important Notice ⚠️

Always subscribe to configuration updates before calling GetConfiguration to
ensure you don't miss any updates.

### Deployment

Deploy this component using the provided `recipe.yml`. Update the S3 bucket
location in the recipe before deployment.

### Step by Step guide

1. Deploy the Component to your Greengrass device. A detailed guid can be found
   [here](https://docs.aws.amazon.com/greengrass/v2/developerguide/create-deployments.html).

2. Check the initial logs:

   ```sh
   journalctl -fau ggl.aws-greengrass-sdk-lite.samples.subscribe_to_configuration_update.service
   ```

3. Revise the deployment to update the `test_str` configuration value in the
   `aws-greengrass-sdk-lite.samples.SubscribeToConfigurationUpdate` component
   using the AWS Console:
   - Navigate to AWS IoT Greengrass console
   - Select `Deployments` and find the deployment targeting your device/group.
   - Click `Actions` > `Revise` > `Revise Deployments` > `Next` > `Next`.
   - Find `aws-greengrass-sdk-lite.samples.SubscribeToConfigurationUpdate` in
     the component list
   - Click `Configure component`
   - Under `Configuration to merge`, add:

     ```json
     {
       "test_str": "updated_value"
     }
     ```

   - Click `Confirm` and then `Next` through the remaining steps
   - Click `Deploy` to apply the changes

4. After the deployment completes, check the logs again. You should see the
   callback triggered with a message indicating the configuration update was
   received

5. The callback handler will print the component name and key paths that were
   updated

The sample subscribes to the `test_str` key path. To subscribe to all
configuration updates, use an empty key path list.

#### Alternative: Using AWS CLI

To update configuration using AWS CLI:

```sh
aws greengrassv2 create-deployment \
  --target-arn "arn:aws:iot:REGION:ACCOUNT_ID:thinggroup/THING_GROUP_NAME" \
  --components '{
    "aws-greengrass-sdk-lite.samples.SubscribeToConfigurationUpdate": {
      "componentVersion": "VERSION",
      "configurationUpdate": {
        "merge": "{\"test_str\":\"updated_value\"}"
      }
    }
  }'
```

Replace `REGION`, `ACCOUNT_ID`, `THING_GROUP_NAME`, and `VERSION` with your
values.

## Known Issues

Subscription callbacks may be invoked even when the configuration value has not
changed. You should call `ggipc_get_config` right after to verify the value has
changed before taking action.
