# Release Process

This document explains how to create a new release of SlackShip.

## Automated Release Pipeline

The project uses GitHub Actions to automatically build and publish releases when you create a new tag.

### Creating a New Release

1. **Update the version** in the following files:
   - `rpm/harbour-slackship.spec`
   - `rpm/harbour-slackship.yaml`

2. **Commit your changes**:
   ```bash
   git add .
   git commit -m "Prepare release v0.2.0"
   ```

3. **Create and push a tag**:
   ```bash
   git tag v0.2.0
   git push origin v0.2.0
   ```

4. **GitHub Actions will automatically**:
   - Build RPM packages for all architectures (armv7hl, aarch64, i486)
   - Create a GitHub release
   - Attach the RPM files to the release
   - Generate release notes

## Build Architectures

The pipeline builds for three architectures:

- **armv7hl**: 32-bit ARM (Jolla 1, Jolla C, Xperia X, Xperia XA2)
- **aarch64**: 64-bit ARM (Xperia 10 II, III, IV and newer devices)
- **i486**: x86 (Emulator only)

## Workflow Files

### build-docker.yml (Recommended)

Uses Docker containers with Sailfish SDK to build packages. This is the recommended approach as it:
- Works reliably in CI environments
- Uses official Sailfish SDK Docker images
- Builds all architectures in parallel
- Creates GitHub releases automatically

### build-release.yml (Alternative)

Attempts to install Sailfish SDK directly on the runner. This may be less reliable but doesn't require Docker.

## Manual Build

If you need to build manually:

```bash
# Install Sailfish SDK
# Then use mb2 tool:

mb2 -t SailfishOS-4.5.0.19-armv7hl build
mb2 -t SailfishOS-4.5.0.19-aarch64 build
mb2 -t SailfishOS-4.5.0.19-i486 build
```

## Testing Before Release

1. Build locally with the Sailfish SDK
2. Test on device or emulator
3. Verify all features work
4. Check for crashes or memory leaks
5. Update changelog

## Version Numbering

Follow semantic versioning (MAJOR.MINOR.PATCH):
- MAJOR: Breaking changes
- MINOR: New features (backwards compatible)
- PATCH: Bug fixes

Examples:
- v0.1.0 - Initial release
- v0.2.0 - Added reactions support
- v0.2.1 - Fixed crash on startup

## Troubleshooting

### Build fails in CI
- Check the GitHub Actions logs
- Verify the Sailfish SDK version is correct
- Ensure all dependencies are declared in the spec file

### RPM not generated
- Check the .spec and .yaml files for syntax errors
- Verify all source files are listed in the .pro file
- Check for compilation errors in the logs

### Release not created
- Ensure you pushed the tag: `git push origin v0.2.0`
- Check that the tag format is correct: `v*.*.*`
- Verify GitHub Actions has permission to create releases
