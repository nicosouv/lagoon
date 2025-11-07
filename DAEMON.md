# SlackShip Background Daemon

The SlackShip daemon is a background service that runs independently from the UI application, providing real-time message synchronization and notifications even when the app is closed.

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                     User Space                              │
│                                                             │
│  ┌──────────────┐              ┌───────────────────────┐   │
│  │  SlackShip   │              │  SlackShip Daemon     │   │
│  │  UI App      │◄────D-Bus────┤  (Background)         │   │
│  │              │              │                       │   │
│  │  - QML UI    │              │  - WebSocket Client   │   │
│  │  - Display   │              │  - Notifications      │   │
│  │  - User Input│              │  - Message Queue      │   │
│  └──────────────┘              │  - Sync Manager       │   │
│                                └───────────────────────┘   │
│                                          │                  │
│                                          │ WebSocket        │
└──────────────────────────────────────────┼──────────────────┘
                                           │
                                           ▼
                                  ┌─────────────────┐
                                  │  Slack API      │
                                  │  RTM WebSocket  │
                                  └─────────────────┘
```

## Features

### Real-time Messaging
- Maintains persistent WebSocket connection to Slack
- Receives messages even when UI is closed
- Automatic reconnection on network issues
- Ping/pong keep-alive mechanism (30s interval)

### Notifications
- Shows native Sailfish OS notifications for new messages
- Priority notifications for @mentions
- Click notification to open app at specific channel
- Respects user notification settings

### Background Sync
- Periodic sync every 5 minutes
- Incremental message fetching
- SQLite cache for offline access
- Unread count tracking

### D-Bus Communication
- Service name: `org.harbour.slackship`
- Object path: `/org/harbour/slackship`
- Interface: `org.harbour.slackship`

## D-Bus API

### Methods

```cpp
// Trigger immediate sync
void SyncNow()

// Switch to different workspace
void SetWorkspace(QString workspaceId)

// Mark channel as read (clears notifications)
void MarkChannelAsRead(QString channelId)

// Send message from UI
void SendMessage(QString channelId, QString text)

// Get current status
bool IsConnected()
int GetUnreadCount()
```

### Signals

```cpp
// New message received
signal NewMessageReceived(QString channelId, QString messageJson)

// Total unread count changed
signal UnreadCountChanged(int totalUnread)

// WebSocket connection state changed
signal ConnectionStateChanged(bool connected)

// Sync completed
signal SyncCompleted()
```

## Systemd Integration

### Service File
Located at: `/usr/lib/systemd/user/harbour-slackship-daemon.service`

### Service Management

```bash
# Start daemon
systemctl --user start harbour-slackship-daemon

# Stop daemon
systemctl --user stop harbour-slackship-daemon

# Enable auto-start on boot
systemctl --user enable harbour-slackship-daemon

# Check status
systemctl --user status harbour-slackship-daemon

# View logs
journalctl --user -u harbour-slackship-daemon -f
```

## Resource Usage

### Limits
- Memory: 100 MB max
- CPU: 20% quota
- Restart on failure with 10s delay

### Optimizations
- Offscreen Qt platform (no graphics)
- Minimal logging in production
- Connection pooling
- Message batching

## Security

### Sandboxing
- `NoNewPrivileges=true` - Cannot gain new privileges
- `PrivateTmp=true` - Private /tmp directory
- User context only (no root)

### Token Storage
- Tokens stored in Qt Settings (encrypted)
- Never logged or exposed
- Secure D-Bus communication

## Installation

The daemon is automatically installed with the SlackShip RPM package:

```bash
rpm -i harbour-slackship-*.rpm
```

After installation:
1. Daemon is registered with systemd
2. D-Bus service file installed
3. Auto-starts on first UI launch
4. Continues running after UI closes

## Development

### Building

```bash
# Build daemon separately
qmake harbour-slackship-daemon.pro
make

# Or use build script
./build.sh
```

### Testing D-Bus

```bash
# List D-Bus services
qdbus | grep slackship

# Call methods
qdbus org.harbour.slackship /org/harbour/slackship SyncNow

# Monitor signals
dbus-monitor "sender='org.harbour.slackship'"
```

### Debugging

```bash
# Run daemon in foreground with debug output
QT_LOGGING_RULES="*.debug=true" /usr/bin/harbour-slackship-daemon

# Check if daemon is running
qdbus org.harbour.slackship /org/harbour/slackship IsConnected
```

## Lifecycle

### Startup Sequence
1. systemd starts daemon on user login (or first app launch)
2. Daemon loads workspace configuration
3. Connects to Slack WebSocket
4. Starts periodic sync timer
5. Registers D-Bus service
6. Ready to receive messages

### Shutdown Sequence
1. User logs out or stops service
2. Daemon disconnects WebSocket gracefully
3. Saves state to cache
4. Unregisters D-Bus service
5. Process exits

### Error Recovery
- WebSocket disconnect → Auto-reconnect every 30s
- Network error → Exponential backoff
- Authentication failure → Notify UI
- Crash → systemd restarts service

## Communication Flow

### UI → Daemon
```
User sends message
    ↓
UI calls SendMessage() via D-Bus
    ↓
Daemon forwards to Slack API
    ↓
Slack processes message
    ↓
Daemon receives confirmation via WebSocket
    ↓
Daemon emits NewMessageReceived signal
    ↓
UI updates message list
```

### Daemon → UI
```
New message arrives via WebSocket
    ↓
Daemon processes message
    ↓
Daemon saves to cache
    ↓
Daemon shows notification
    ↓
Daemon emits NewMessageReceived signal
    ↓
UI (if running) updates display
```

## Configuration

### Environment Variables
```bash
# Qt platform (daemon runs headless)
QT_QPA_PLATFORM=offscreen

# Logging control
QT_LOGGING_RULES="*.debug=false"
```

### Settings Location
- Cache: `~/.local/share/harbour-slackship/slackship.db`
- Config: `~/.config/harbour-slackship/`
- Logs: `journalctl --user -u harbour-slackship-daemon`

## Troubleshooting

### Daemon not starting
```bash
# Check service status
systemctl --user status harbour-slackship-daemon

# Check logs
journalctl --user -u harbour-slackship-daemon --no-pager

# Verify D-Bus registration
qdbus | grep slackship
```

### No notifications
```bash
# Check if daemon is connected
qdbus org.harbour.slackship /org/harbour/slackship IsConnected

# Trigger manual sync
qdbus org.harbour.slackship /org/harbour/slackship SyncNow

# Check notification settings in UI
```

### High battery usage
```bash
# Check resource usage
systemctl --user status harbour-slackship-daemon

# Verify sync interval (should be 5 minutes)
# Disable if not needed via Settings
```

## Future Enhancements

- [ ] Voice/video call notifications
- [ ] Offline message queue
- [ ] Multi-account simultaneous connections
- [ ] Adaptive sync based on activity
- [ ] Bandwidth optimization
- [ ] E2E encryption for cache
