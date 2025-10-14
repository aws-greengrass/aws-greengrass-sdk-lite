# Subscribe to Configuration Update Sample

This sample demonstrates how to subscribe to configuration updates for a
Greengrass component.

## Overview

The sample subscribes to its own configuration updates and prints notifications
whenever the configuration changes. This is useful for components that need to
react to configuration changes in real-time.

## Usage

### ⚠️ Important Notice ⚠️

Always subscribe to configuration updates before calling GetConfiguration to
ensure you don't miss any updates.

### Testing Configuration Updates

After deploying the component, update its configuration to trigger the
subscription callback.

**Using AWS Console:**

1. Navigate to AWS IoT Greengrass console
2. Select `Deployments` and find the deployment targeting your thing group
3. Click `Actions` > `Revise` > `Next` > `Next`
4. Find `aws-greengrass-sdk-lite.samples.SubscribeToConfigurationUpdate` in the
   component list
5. Click `Configure component`
6. Under `Configuration to merge`, add:
   ```json
   {
     "test_str": "updated_value"
   }
   ```
7. Click `Confirm` > `Next` > `Deploy`

**Using AWS CLI:**

```bash
aws greengrassv2 create-deployment \
  --target-arn "arn:aws:iot:REGION:ACCOUNT_ID:thinggroup/YOUR_THING_GROUP" \
  --components '{
    "aws-greengrass-sdk-lite.samples.SubscribeToConfigurationUpdate": {
      "componentVersion": "VERSION",
      "configurationUpdate": {
        "merge": "{\"test_str\":\"updated_value\"}"
      }
    }
  }' \
  --region YOUR_REGION
```

The sample subscribes to the `test_str` key path. To subscribe to all
configuration updates, use an empty key path list.

## Known Issues

Subscription callbacks may be invoked even when the configuration value has not
changed. You should call `ggipc_get_config` right after to verify the value has
changed before taking action.
