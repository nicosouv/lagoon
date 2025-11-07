#!/bin/bash

# Build script for SlackShip (UI + Daemon)

set -e

echo "Building SlackShip..."

# Check for .env file and load it
if [ -f .env ]; then
    echo "Loading OAuth credentials from .env..."
    export $(cat .env | grep -v '^#' | xargs)
else
    echo "⚠️  Warning: .env file not found!"
    echo "Create .env from .env.example and add your Slack OAuth credentials"
    echo ""
fi

# Verify OAuth credentials are set
if [ -z "$SLACKSHIP_CLIENT_ID" ] || [ -z "$SLACKSHIP_CLIENT_SECRET" ]; then
    echo "❌ ERROR: OAuth credentials not set!"
    echo ""
    echo "Please set the following environment variables:"
    echo "  - SLACKSHIP_CLIENT_ID"
    echo "  - SLACKSHIP_CLIENT_SECRET"
    echo ""
    echo "Either:"
    echo "  1. Copy .env.example to .env and fill in your credentials"
    echo "  2. Or export them manually:"
    echo "     export SLACKSHIP_CLIENT_ID='your_client_id'"
    echo "     export SLACKSHIP_CLIENT_SECRET='your_client_secret'"
    echo ""
    exit 1
fi

echo "✓ OAuth credentials loaded"
echo "  Client ID: ${SLACKSHIP_CLIENT_ID:0:10}..."
echo ""

# Build main application
echo "Building main application..."
qmake harbour-slackship.pro
make clean
make -j$(nproc)

# Build daemon
echo "Building daemon..."
qmake harbour-slackship-daemon.pro
make clean
make -j$(nproc)

echo ""
echo "✓ Build completed successfully!"
echo ""
echo "Binaries:"
echo "  - harbour-slackship (UI)"
echo "  - harbour-slackship-daemon (Background service)"
