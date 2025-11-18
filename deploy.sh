#!/bin/bash

# Build the project
pio run

# Check if build succeeded
if [ $? -ne 0 ]; then
    echo "Build failed"
    exit 1
fi

# AWS S3 bucket name
S3_BUCKET="files.opind.co"

# Copy firmware files to S3 with public-read ACL
aws s3 cp .pio/build/opta_m7/firmware.bin s3://${S3_BUCKET}/firmware_m7.bin --acl public-read
aws s3 cp .pio/build/opta_m4/firmware.bin s3://${S3_BUCKET}/firmware_m4.bin --acl public-read

echo "Deployment complete!"
echo "M7 Firmware: https://${S3_BUCKET}/firmware_m7.bin"
echo "M4 Firmware: https://${S3_BUCKET}/firmware_m4.bin"
