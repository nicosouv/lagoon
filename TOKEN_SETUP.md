# How to Get Your Slack Token for SlackShip

SlackShip uses Slack's User OAuth Token for authentication. Follow these simple steps:

## Step-by-Step Guide

### 1. Create a Slack App

1. Go to **https://api.slack.com/apps**
2. Click **"Create New App"**
3. Choose **"From scratch"**
4. Enter:
   - **App Name**: `SlackShip` (or any name you prefer)
   - **Pick a workspace**: Select your workspace
5. Click **"Create App"**

### 2. Add Permissions (Scopes)

1. In the left sidebar, click **"OAuth & Permissions"**
2. Scroll down to **"Scopes"**
3. Under **"User Token Scopes"**, click **"Add an OAuth Scope"**
4. Add these scopes one by one:

#### Required Scopes:
```
channels:history    - View messages in public channels
channels:read       - View basic channel info
channels:write      - Manage public channels
chat:write          - Send messages as you
groups:history      - View messages in private channels
groups:read         - View private channel info
im:history          - View direct messages
im:read             - View DM info
im:write            - Send direct messages
mpim:history        - View group DM messages
mpim:read           - View group DM info
users:read          - View people in workspace
reactions:read      - View emoji reactions
reactions:write     - Add/remove reactions
files:read          - View files
files:write         - Upload files
search:read         - Search messages and files
```

**Quick tip**: You can type `channels:` in the search box and add all that start with it, then repeat for `groups:`, `im:`, etc.

### 3. Install to Workspace

1. Scroll back to the top of **"OAuth & Permissions"** page
2. Click **"Install to Workspace"**
3. Review the permissions
4. Click **"Allow"**

### 4. Copy Your Token

1. After installation, you'll see **"User OAuth Token"**
2. It starts with `xoxp-`
3. Click **"Copy"** or select and copy the entire token
4. Example: `xoxp-1234567890-1234567890-1234567890-abcdefghijklmnopqrstuvwxyz123456`

### 5. Use Token in SlackShip

1. Open **SlackShip** app
2. Paste the token in the **"User OAuth Token"** field
3. Click **"Connect"**
4. Done! <‰

## Important Notes

  **Keep your token secure!**
- Never share your token publicly
- Don't commit it to GitHub
- Treat it like a password

= **Token doesn't expire**
- Your token will work until you revoke it
- You can revoke it anytime from the Slack app settings

<â **Multiple workspaces?**
- Create a separate app for each workspace
- Each workspace needs its own token
- SlackShip supports multiple workspaces

## Troubleshooting

### "Invalid auth" error
- Check that you copied the entire token (starts with `xoxp-`)
- Make sure there are no extra spaces
- Verify the app is installed to your workspace

### Missing permissions
- Go back to "OAuth & Permissions"
- Add any missing scopes
- Click "Reinstall to Workspace"
- Copy the new token

### Can't see all channels
- Make sure you added all `channels:*`, `groups:*`, and `im:*` scopes
- Some channels might be archived or you're not a member

## Need Help?

- Check the app's GitHub issues: https://github.com/nicosouv/slackship/issues
- Slack API docs: https://api.slack.com/authentication/token-types#user

## Why Manual Token?

Slack requires HTTPS for OAuth redirect URLs, which is difficult for native mobile apps without a backend server. Using a manual token is:
-  Simpler and more reliable
-  Standard for third-party Slack apps
-  Gives you full control
-  No complex OAuth flow needed
