# AWS Greengrass SDK Lite

The `aws-greengrass-sdk-lite` provides an API for making AWS IoT Greengrass IPC
calls with a small footprint. It enables Greengrass components to interact with
the Greengrass Nucleus (Lite or Classic) with less binary overhead and also
supports components written in C. Greengrass components can use this SDK as an
alternative to the `aws-iot-device-sdk-cpp-v2` or other language-specific device
SDKs.

### ⚠️ Important Notice ⚠️

This library is in pre-release status. We encourage users to test it and report
any bugs or missing features.

For development purposes, it is recommended to pin your usage to a specific
commit or tag.

## Building

For building the SDK and samples, see the [build guide](docs/BUILD.md).

## Security

See [CONTRIBUTING](docs/CONTRIBUTING.md#security-issue-notifications) for more
information.

## License

This project is licensed under the Apache-2.0 License.
