# Deployment Guide for SlackShip

This guide explains how to build and deploy SlackShip for production.

## Prerequisites

### 1. Slack App Setup

First, create your Slack app:

1. Go to https://api.slack.com/apps
2. Click **"Create New App"** → **"From scratch"**
3. Name: **SlackShip**
4. Select a workspace

### 2. Configure OAuth

In your Slack app:

1. **OAuth & Permissions** → **Redirect URLs**
   - Add: `http://localhost:8080/callback`
   - Click **"Save URLs"**

2. **OAuth & Permissions** → **Scopes** → **User Token Scopes**
   - Add all scopes from `OAUTH_SETUP.md`
   - (channels:history, chat:write, users:read, etc.)

3. **Basic Information** → **App Credentials**
   - Copy your **Client ID**
   - Copy your **Client Secret**

## Local Development Build

### Step 1: Configure Credentials

```bash
cd /Users/nico/Documents/Dev/Personal/slackship

# Copy the example file
cp .env.example .env

# Edit .env and add your credentials
nano .env
```

In `.env`, replace with your actual values:
```bash
SLACKSHIP_CLIENT_ID=1234567890.9876543210
SLACKSHIP_CLIENT_SECRET=abcdef1234567890abcdef1234567890
```

### Step 2: Build

```bash
./build.sh
```

The script will:
- ✓ Load credentials from `.env`
- ✓ Verify they're set
- ✓ Build UI and daemon
- ✓ Create binaries

### Step 3: Test

```bash
./harbour-slackship
```

Click "Login with Slack" and verify it works!

## GitHub Actions Deployment

### Step 1: Add Secrets to GitHub

1. Go to your GitHub repo: https://github.com/nicosouv/slackship
2. Click **Settings** → **Secrets and variables** → **Actions**
3. Click **"New repository secret"**

Add two secrets:

**Secret 1:**
- Name: `SLACKSHIP_CLIENT_ID`
- Value: Your Slack Client ID (e.g., `1234567890.9876543210`)

**Secret 2:**
- Name: `SLACKSHIP_CLIENT_SECRET`
- Value: Your Slack Client Secret (e.g., `abcdef123...`)

### Step 2: Push a Tag

The GitHub Actions workflow automatically triggers on new tags:

```bash
# Commit your code
git add .
git commit -m "Prepare for release"

# Create a version tag
git tag v0.2.0

# Push code and tag
git push origin main
git push origin v0.2.0
```

### Step 3: Automatic Build

GitHub Actions will:
1. ✓ Load secrets as environment variables
2. ✓ Build for 3 architectures (armv7hl, aarch64, i486)
3. ✓ Create RPM packages
4. ✓ Create a GitHub Release
5. ✓ Attach RPMs to the release

### Step 4: Download Release

Go to: https://github.com/nicosouv/slackship/releases

Users can download the RPM for their device!

## Build Matrix

GitHub Actions builds for:

| Architecture | Devices |
|--------------|---------|
| **armv7hl** | Jolla 1, Jolla C, Xperia X, Xperia XA2 |
| **aarch64** | Xperia 10 II, Xperia 10 III, Xperia 10 IV |
| **i486** | Emulator |

## Security Notes

### ✅ What's Safe

The following are **safe to commit**:
- All source code
- `.env.example` (template without real credentials)
- `.gitignore` (prevents committing secrets)
- GitHub Actions workflow (uses secrets)

### ❌ What's NOT Safe

**NEVER commit these:**
- `.env` (contains real credentials)
- `oauth.conf` (if you create one)
- Any file with real Client ID/Secret

`.gitignore` already prevents this, but double-check!

## Environment Variables

### How They Work

```
Local Build:
  .env file → export → Environment → Code reads qgetenv()

GitHub Actions:
  Secrets → Environment → Docker → Code reads qgetenv()

Result:
  Credentials never stored in code or git!
```

### Verification

Check that credentials are NOT in the code:

```bash
# Should NOT find any real credentials
grep -r "1234567890" src/
grep -r "abcdef" src/

# Should find the env loading (OK)
grep -r "qgetenv" src/
```

## Troubleshooting

### Build fails: "OAuth credentials not set"

**Local:**
```bash
# Make sure .env exists
ls -la .env

# Make sure it's loaded
source .env
echo $SLACKSHIP_CLIENT_ID
```

**GitHub Actions:**
- Check that secrets are set in repo settings
- Verify secret names match exactly:
  - `SLACKSHIP_CLIENT_ID`
  - `SLACKSHIP_CLIENT_SECRET`

### Build succeeds but login fails

Check that:
1. Redirect URI is registered: `http://localhost:8080/callback`
2. All OAuth scopes are added to your Slack app
3. Client ID/Secret are correct (no typos)

### GitHub Actions can't access secrets

Secrets are only available:
- On `push` to main branch with tags
- On `workflow_dispatch` triggers
- NOT on pull requests from forks (security)

## Manual Distribution

If you don't want to use GitHub Actions:

### Build Locally

```bash
# Build for your architecture
./build.sh

# Find RPM
find RPMS -name "*.rpm"
```

### Install on Device

```bash
# Copy to device
scp RPMS/harbour-slackship-*.rpm nemo@192.168.1.100:

# SSH to device
ssh nemo@192.168.1.100

# Install
devel-su pkcon install-local harbour-slackship-*.rpm
```

## Updating Credentials

If you need to change your Slack app credentials:

### Local

```bash
# Edit .env
nano .env

# Update values
SLACKSHIP_CLIENT_ID=new_id
SLACKSHIP_CLIENT_SECRET=new_secret

# Rebuild
./build.sh
```

### GitHub Actions

1. Go to repo **Settings** → **Secrets**
2. Click on the secret to update
3. Click **"Update secret"**
4. Enter new value
5. Save
6. Create new tag to trigger rebuild

## Distribution Checklist

Before distributing SlackShip:

- [ ] Slack app created and configured
- [ ] All OAuth scopes added
- [ ] Redirect URI registered
- [ ] Client ID and Secret obtained
- [ ] `.env` file created locally (for dev)
- [ ] Secrets added to GitHub (for CI/CD)
- [ ] `.gitignore` prevents committing `.env`
- [ ] Local build tested and working
- [ ] GitHub Actions build tested (push a tag)
- [ ] Downloaded RPM from GitHub Release
- [ ] Installed and tested on actual device
- [ ] OAuth login flow tested end-to-end
- [ ] Documentation updated (README, OAUTH_SETUP)

## Next Steps

Once deployed:

1. **Monitor**: Check GitHub Actions for build status
2. **Test**: Install on multiple device types
3. **Distribute**: Share GitHub Release link with users
4. **Support**: Monitor issues on GitHub
5. **Update**: Push new tags for updates

## Support

- GitHub Issues: https://github.com/nicosouv/slackship/issues
- Slack API Docs: https://api.slack.com/authentication/oauth-v2
- Sailfish SDK: https://sailfishos.org/develop/

---

**Ready to deploy!** 🚀

Just follow the steps above and you're good to go!
