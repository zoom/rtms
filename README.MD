# RTMS SDK

The base project that builds the Zoom RTMS SDKs for Node.js, Python and Go.

## Prerequisites

- Docker
- RTMS C SDK files
- Zoom Developer Credentials

## Getting Started

Place the RTMS C SDK files in the [lib/rtms_csdk](lib/rtms_csdk) directory

```bash
cp rtms_csdk.h rtms_common.h librtmsdk.so ./lib/rtms_csdk
```

Create a .env file with the following values:

```dotenv
MEETING_UUID=
SESSION_ID=
ZOOM_CLIENT_ID=
ZOOM_CLIENT_SECRET=
```

Run docker compose to build and run the test.js file

```bash
docker compose up
```


### Usage
See the test.js file for usage. When it comes to Node.JS, this is intended to be published as a package.

