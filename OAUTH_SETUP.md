# OAuth Setup Guide for SlackShip

This guide explains how to configure OAuth authentication for SlackShip so users can log in with "Login with Slack" button.

## Why OAuth?

Instead of requiring each user to create their own Slack app and manually copy tokens, OAuth provides:
- ✅ One-click login experience
- ✅ No technical knowledge required from users
- ✅ Automatic token refresh
- ✅ Proper security with scopes
- ✅ Standard Slack authentication flow

## Step 1: Create Slack App

1. Go to https://api.slack.com/apps
2. Click **"Create New App"**
3. Choose **"From scratch"**
4. Fill in:
   - **App Name**: `SlackShip` (or your preferred name)
   - **Development Workspace**: Choose any workspace
5. Click **"Create App"**

## Step 2: Configure OAuth & Permissions

### A. Add Redirect URLs

1. In your app settings, go to **OAuth & Permissions**
2. Under **Redirect URLs**, click **"Add New Redirect URL"**
3. Add: `http://localhost:8080/callback`
4. Click **"Save URLs"**

> **Note**: For production, you may want to use a custom URL scheme like `slackship://oauth/callback`

### B. Add OAuth Scopes

Scroll down to **Scopes** section and add these **User Token Scopes**:

#### Conversations & Channels
- `channels:history` - Read messages from public channels
- `channels:read` - View basic info about public channels
- `channels:write` - Join and leave public channels
- `groups:history` - Read messages from private channels
- `groups:read` - View basic info about private channels

#### Direct Messages
- `im:history` - Read direct message history
- `im:read` - View basic info about direct messages
- `im:write` - Start and manage direct messages
- `mpim:history` - Read group direct message history
- `mpim:read` - View basic info about group DMs

#### Chat & Messages
- `chat:write` - Send messages as the user

#### Users
- `users:read` - View people in the workspace

#### Files
- `files:read` - View files shared in channels/DMs
- `files:write` - Upload files

#### Reactions & Search
- `reactions:read` - View emoji reactions
- `reactions:write` - Add and remove emoji reactions
- `search:read` - Search messages and files

## Step 3: Get Your Credentials

1. Go to **Basic Information**
2. Under **App Credentials**, you'll find:
   - **Client ID**: `1234567890.1234567890`
   - **Client Secret**: `abcdef1234567890abcdef1234567890`

3. Copy both values

## Step 4: Configure SlackShip

### Option A: Build from Source

Edit `src/oauthmanager.cpp` and replace:

```cpp
const QString OAuthManager::CLIENT_ID = "YOUR_SLACK_CLIENT_ID";
const QString OAuthManager::CLIENT_SECRET = "YOUR_SLACK_CLIENT_SECRET";
```

With your actual credentials:

```cpp
const QString OAuthManager::CLIENT_ID = "1234567890.1234567890";
const QString OAuthManager::CLIENT_SECRET = "abcdef1234567890abcdef1234567890";
```

### Option B: Environment Variables (Recommended)

For better security, use environment variables:

```bash
export SLACKSHIP_CLIENT_ID="1234567890.1234567890"
export SLACKSHIP_CLIENT_SECRET="abcdef1234567890abcdef1234567890"
```

Then modify the code to read from environment:

```cpp
const QString OAuthManager::CLIENT_ID = qgetenv("SLACKSHIP_CLIENT_ID");
const QString OAuthManager::CLIENT_SECRET = qgetenv("SLACKSHIP_CLIENT_SECRET");
```

### Option C: Configuration File

Create `/usr/share/harbour-slackship/oauth.conf`:

```ini
[OAuth]
ClientId=1234567890.1234567890
ClientSecret=abcdef1234567890abcdef1234567890
```

And load it at runtime (safer for distribution).

## Step 5: Distribute Your App

### For Public Distribution

If you want to distribute SlackShip publicly:

1. **Submit app for Slack App Directory**
   - Go to **Manage Distribution** in your app settings
   - Click **"Activate Public Distribution"**
   - Fill in required information
   - Submit for review

2. **Once approved**, users can install directly from Slack

### For Private Distribution

If keeping it private:
- Users will see a warning "This app is not listed in the Slack App Directory"
- They can still authorize the app
- Only works for workspaces where the app is installed

## OAuth Flow Explained

### User Experience

1. User opens SlackShip
2. Clicks **"Login with Slack"**
3. Browser opens with Slack authorization page
4. User reviews permissions and clicks **"Allow"**
5. Browser redirects back to `localhost:8080/callback`
6. SlackShip receives the authorization code
7. SlackShip exchanges code for access token
8. User is logged in!

### Technical Flow

```
┌─────────────┐
│  SlackShip  │
│     App     │
└──────┬──────┘
       │
       │ 1. User clicks "Login"
       ▼
┌─────────────────────────────────────────┐
│ Generate state (CSRF protection)        │
│ Start local HTTP server (port 8080)     │
│ Open browser with authorization URL     │
└──────┬──────────────────────────────────┘
       │
       │ 2. Browser opens
       ▼
┌────────────────────────────────────┐
│  Slack Authorization Page          │
│  https://slack.com/oauth/authorize │
│                                    │
│  [x] Read messages                 │
│  [x] Send messages                 │
│  [x] View users                    │
│                                    │
│          [Allow] [Deny]            │
└──────┬─────────────────────────────┘
       │
       │ 3. User clicks "Allow"
       ▼
┌────────────────────────────────────┐
│  Slack redirects to:               │
│  http://localhost:8080/callback    │
│  ?code=xoxp-...                    │
│  &state=abc123                     │
└──────┬─────────────────────────────┘
       │
       │ 4. Local server receives callback
       ▼
┌─────────────────────────────────────────┐
│ SlackShip validates state               │
│ Extracts authorization code             │
│ Sends code to Slack token endpoint      │
└──────┬──────────────────────────────────┘
       │
       │ 5. Token exchange
       ▼
┌────────────────────────────────────┐
│  POST /api/oauth.v2.access         │
│  client_id + client_secret + code  │
│                                    │
│  Response:                         │
│  {                                 │
│    "access_token": "xoxp-..."      │
│    "team": {...}                   │
│    "authed_user": {...}            │
│  }                                 │
└──────┬─────────────────────────────┘
       │
       │ 6. Token received
       ▼
┌─────────────────────────────────────────┐
│ SlackShip:                              │
│ - Saves token securely                  │
│ - Connects WebSocket                    │
│ - Loads conversations                   │
│ - User is logged in!                    │
└─────────────────────────────────────────┘
```

## Security Considerations

### CSRF Protection
The OAuth flow uses a random `state` parameter to prevent CSRF attacks:
1. Generate random state before redirect
2. Store state locally
3. Verify state matches when callback is received
4. Reject if state doesn't match

### Token Storage
Tokens are stored in:
- `WorkspaceManager` using Qt Settings
- Encrypted by the OS keychain (on supported systems)
- Never logged or exposed to console

### Client Secret
The client secret should NOT be hardcoded in production. Use one of:
- Environment variables
- Secure configuration file (not in repo)
- Server-side proxy for token exchange (most secure)

### PKCE Flow (Future Enhancement)
For even better security, consider implementing PKCE (Proof Key for Code Exchange):
- No client secret needed
- Code verifier/challenge mechanism
- Safer for native apps

## Troubleshooting

### "Invalid redirect_uri"
- Make sure you added `http://localhost:8080/callback` to Redirect URLs
- Check for typos
- Ensure no trailing slash

### "Port 8080 already in use"
- SlackShip will try ports 8080-8089
- If all busy, authentication will fail
- Close other apps using these ports

### "This app is not listed"
- Your app is not in the Slack App Directory
- Users can still authorize it
- Or submit app for review to get listed

### "Browser doesn't open"
- Check if default browser is set
- Try manually opening the URL shown in logs
- Ensure `QDesktopServices` is available

### "Token exchange failed"
- Check CLIENT_ID and CLIENT_SECRET
- Verify scopes match between app config and code
- Check network connectivity

## Testing

### Test OAuth Flow

1. Build and run SlackShip
2. Click "Login with Slack"
3. Browser should open automatically
4. Authorize the app
5. Should redirect to success page
6. SlackShip should log you in

### Debug Logs

Enable debug logging:
```bash
QT_LOGGING_RULES="*.debug=true" harbour-slackship
```

You'll see:
- OAuth URL being opened
- Local server starting
- Callback received
- Token exchange request
- Authentication result

## Production Checklist

Before distributing SlackShip:

- [ ] Replace placeholder CLIENT_ID and CLIENT_SECRET
- [ ] Use secure token storage (keychain)
- [ ] Implement token refresh mechanism
- [ ] Add proper error messages for users
- [ ] Test on all supported Sailfish OS versions
- [ ] Submit app to Slack App Directory (optional)
- [ ] Add privacy policy and terms of service
- [ ] Document required permissions clearly
- [ ] Test OAuth flow on clean install
- [ ] Verify redirect URI is registered

## Next Steps

Once OAuth is working:
1. Implement token refresh (tokens expire)
2. Add "Switch Account" feature
3. Support multiple workspaces simultaneously
4. Add "Logout" functionality
5. Handle token revocation gracefully

## Support

If you encounter issues:
- Check SlackShip logs: `journalctl --user -u harbour-slackship`
- Check daemon logs: `journalctl --user -u harbour-slackship-daemon`
- Review Slack API documentation: https://api.slack.com/authentication/oauth-v2
- Open issue on GitHub: https://github.com/nicosouv/slackship/issues
