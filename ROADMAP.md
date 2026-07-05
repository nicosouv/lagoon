# Lagoon Roadmap

Actionable roadmap from the 2026-07 performance/security/quality audit.
Work through phases in order; items inside a phase are ordered by impact.
Check items off (`[x]`) as they land, and note the commit hash next to each item
so work can be resumed across sessions.

Conventions for every item:
- Code, comments, commits in English; commits are concise one-liners.
- New user-facing strings must be added to the translation files.
- On each git tag: update version on LoginPage and SettingsPage (until item 1.5 removes this chore).

Status legend: `[ ]` todo — `[~]` in progress — `[x]` done (add commit hash).

---

## Phase 1 — Performance quick wins

Goal: remove the main UI lag sources with small, localized changes.
Expected outcome: smooth scrolling in FirstPage and ConversationPage, no list flicker.

### 1.1 `[x]` Hash-based user lookup in UserModel (89d5dda)
- File: `src/models/usermodel.cpp` / `.h`
- `findUserIndex()` (`usermodel.cpp:171`) is a linear scan, called from every
  message delegate binding (`getUserName`, `getUserAvatar`), channel delegate,
  typing indicator and mention search.
- Add `QHash<QString, int> m_userIndex` (userId -> index in `m_users`).
  Rebuild it in `updateUsers()`, `loadUsersFromCache()`, `addUser()`, `clear()`.
  `findUserIndex()` becomes a hash lookup.
- Acceptance: no behavior change; scrolling a 100-message conversation in a
  500+ user workspace no longer stutters.

### 1.2 `[x]` Stop resetting the conversation list model from QML (c10d2a1)
- File: `qml/pages/FirstPage.qml:114-123`
- Remove the `conversationListView.model = null / = conversationModel` hack in
  the `onUsersUpdated` handler. It destroys and recreates every delegate.
- Replacement: DM names must update via data, not via model reset. Two options:
  a) In `ConversationModel`, add a slot `refreshDmNames()` that emits
     `dataChanged` with `NameRole` for `im` rows; call it from `main.cpp` when
     `UserModel::usersUpdated` fires (C++-to-C++ connection).
  b) Or resolve the DM display name in `ChannelDelegate` through a binding that
     re-evaluates on a `userModel.usersUpdated` signal (Connections + property).
  Option (a) is cleaner and testable.
- Acceptance: after login with cold cache, DM names appear without the list
  visibly rebuilding (no scroll jump, no flicker).

### 1.3 `[x]` Remove full-model resets on unread/sort changes (fa74d08)
- File: `src/models/conversationmodel.cpp:149, 179, 221, 416, 533`
- Every read/unread transition triggers `beginResetModel(); sortConversations(); endResetModel();`.
  During the unread batch fetch this can fire every second.
- Changes:
  - During batch unread fetching, do NOT re-sort per result. Accumulate, then
    sort once when `allUnreadsFetched` arrives (add a public
    `resortAndNotify()` slot; call it from `harbour-lagoon.qml`
    `onAllUnreadsFetched`, or better from a C++ connection in `main.cpp`).
  - For single transitions (markAsRead, toggleStar, incrementUnread), replace
    reset with `beginMoveRows`/`endMoveRows` to the new sorted position
    (the list is already sorted; compute target index with std::lower_bound
    over the existing comparator).
- Acceptance: marking a channel read/unread moves only that row; scroll
  position preserved; no flicker during startup unread fetch.

### 1.4 `[x]` Cheapen avatar rendering (GPU) (22c7fec)
- Files: `qml/components/MessageDelegate.qml:148-163`, `qml/pages/FirstPage.qml:237-273`
- Remove `layer.enabled + OpacityMask` (MessageDelegate) and the inline
  `ShaderEffect` (FirstPage header). Each one is an offscreen render pass per item.
- Replacement: a single shared `RoundedAvatar` component in `qml/components/`
  using one of:
  - `Image` + pre-rounded look: draw a circular border `Rectangle` on top and
    accept square-ish avatars (Slack avatars are square; a thin circular
    border + `clip` is visually close), or
  - one shared `OpacityMask` texture via `layer.smooth` alternatives is NOT
    acceptable — the point is zero per-delegate layers.
- Also reuse `RoundedAvatar` in the mention suggestion list
  (`qml/pages/ConversationPage.qml:320-352`).
- Acceptance: `MessageDelegate` and FirstPage header contain no `layer.effect`;
  scrolling stays >50fps on device.

### 1.5 `[x]` Single source of truth for the version (0ac0dc4)
- Implementation note: CI already rewrites `Version:` in the spec from the git
  tag on release; the `.pro` reads the spec via `sed`, defines `APP_VERSION`,
  exposed to QML as the `appVersion` context property.
- Files: `harbour-lagoon.pro`, `rpm/harbour-lagoon.spec`, `src/updatechecker.cpp:11`,
  `qml/pages/LoginPage.qml:278`, `qml/pages/SettingsPage.qml:405`
- Today three versions diverge: spec `0.37.0`, QML `0.37.29`, UpdateChecker
  `0.36.0` (which makes the app always think an update exists).
- Add to `harbour-lagoon.pro`: `DEFINES += APP_VERSION=\\\"$$VERSION\\\"` fed
  from the spec version (qmake `VERSION` variable, kept in sync by the build
  scripts / spec `%{version}` passed by mb2). UpdateChecker uses `APP_VERSION`;
  expose it to QML via a context property (e.g. `appVersion`) and bind both
  pages to it.
- Acceptance: bumping the spec version alone updates LoginPage, SettingsPage
  and UpdateChecker. Remove the manual-update note from CLAUDE.md afterwards.

### 1.6 `[x]` Lighten MessageDelegate bindings (22c7fec)
- File: `qml/components/MessageDelegate.qml`
- `isGrouped` (`:18-39`) calls `listView.model.data()` with hardcoded role ids
  (257/258). Move grouping to C++: add an `isGroupedWithPrevious` role in
  `MessageModel` computed at parse time (same user + <300s gap), recomputed on
  insert/remove of neighbors.
- Replace the inline `menu: ContextMenu {...}` (`:440`) with a `Component`
  instantiated on demand (Silica supports assigning the menu lazily), so
  delegates don't each carry a menu + emoji Repeater.
- Acceptance: no hardcoded role numbers in QML; delegate creation time drops.

---

## Phase 2 — Network architecture

Goal: stop the permanent polling churn; make real-time actually real-time.
Expected outcome: near-zero background traffic when idle, instant message delivery.

### 2.1 `[ ]` WebSocket auto-reconnect with backoff
- File: `src/websocketclient.cpp` / `.h`
- No reconnect logic exists; after Sailfish suspends the app the RTM socket is
  dead and everything degrades to polling.
- Add: reconnect timer with exponential backoff (1s, 2s, 4s... cap 60s) on
  `disconnected`/`error` while a URL is known; `rtm.connect` must be re-called
  (the old wss URL expires) — emit a `reconnectNeeded` signal that `SlackAPI`
  handles by calling `connectWebSocket()` again.
- Handle app resume: on `QGuiApplication::applicationStateChanged` to Active
  (connect in `main.cpp`), if socket is not connected, trigger reconnect and a
  one-shot conversations refresh to resync missed events.
- Rename `disconnect()` to `close()` while here (it shadows `QObject::disconnect`).
- Acceptance: kill wifi 30s then restore -> messages flow again without
  restarting the app; suspend/resume the app -> same.

### 2.2 `[ ]` Event-driven unreads instead of per-channel polling
- Files: `src/slackapi.cpp:85-180, 835-843`, `qml/harbour-lagoon.qml:87-94`
- Today: every 30s, `users.conversations` + one `conversations.info` per
  channel (batches of 2, 1s apart). For 60 channels that is continuous traffic.
- Target model:
  - Full unread fetch ONCE after authentication (keep current batch code).
  - Afterwards, unreads are maintained locally from RTM events (`message`,
    `im_marked`/`channel_marked` if available on rtm, own `conversations.mark`
    calls) — most of this local bookkeeping already exists
    (`incrementUnread`, `markAsRead`).
  - The 30s poll becomes a fallback ONLY when the websocket is down
    (gate `handleRefreshTimer` on `!m_webSocketClient->isConnected()`),
    and it should only refresh the conversation list, not per-channel info.
- Acceptance: with a live websocket, steady-state network traffic is zero
  (verify with the in-app bandwidth counter over 5 minutes idle).

### 2.3 `[ ]` Optimistic message send and reaction updates
- Files: `qml/pages/ConversationPage.qml:450-461, 590-599`, `src/slackapi.cpp:745-759`,
  `src/models/messagemodel.cpp`
- Today: send -> wait 1s -> refetch ENTIRE history; reaction -> refetch message.
- Changes:
  - `chat.postMessage` response contains the final message object (`ok`, `ts`,
    `message`) — emit it and `MessageModel::addMessage()` it directly. Remove
    `refreshTimer` full refetch. Keep a pending/failed state role if the app
    should show sending status (optional, phase 4 polish).
  - Reactions: apply add/remove locally in `MessageModel` (mutate the
    reactions array for that ts), keep the single-message refetch only as
    error fallback.
- Acceptance: sending a message shows it instantly; adding a reaction updates
  the bubble with no network round-trip visible; no history refetch on send.

### 2.4 `[ ]` Format message text in C++ once
- Files: `src/models/messagemodel.cpp`, new `src/slacktextformatter.cpp` / `.h`,
  `qml/components/MessageDelegate.qml:267-278`, `qml/components/EmojiHelper.js`
- `EmojiHelper.formatSlackText()` runs 10+ regexes per delegate per binding
  evaluation, with a QML->C++ callback per mention.
- Port `formatSlackText` (including HTML escaping — keep it! — and the `"`
  escaping fix from item 3.6) to a C++ helper. `MessageModel` stores
  `formattedText` computed at parse time (it can resolve mentions via a
  `UserModel*` pointer injected in `main.cpp`). Expose as a role; delegate
  binds `text: model.formattedText`.
- Keep `EmojiHelper.js` for reaction emoji mapping only.
- Acceptance: `formatSlackText` no longer called from MessageDelegate;
  identical rendering (compare a channel with links, mentions, code, emoji).

### 2.5 `[ ]` Move user cache from QSettings to SQLite
- Files: `src/models/usermodel.cpp:225-349`, new `src/cache/userdb.cpp` (or
  extend `src/cache/cachemanager.cpp`)
- `saveFullUserCache` writes 9 INI keys per user + `sync()` on the UI thread;
  same cost on load.
- Use QtSql (allowed module) with a `users` table keyed by (teamId, userId);
  batch insert in a transaction. Run save on a worker thread
  (`QtConcurrent::run` is fine); load can stay synchronous (single indexed
  SELECT is fast) or async with a `cacheLoaded` signal.
- Migrate: if the old QSettings cache exists, import once then clear it.
- Acceptance: cold start with 1000 cached users shows no UI freeze; the
  `users-full` INI file is gone after first run.

---

## Phase 3 — Security

Goal: protect the token and the OAuth flow. Do 3.1 and 3.2 first.

### 3.1 `[ ]` Remove redirectmeto.com from the OAuth flow
- File: `src/oauthmanager.cpp:27`
- The authorization code (and state) currently transit through a third-party
  service. Combined with the embedded client secret (3.2) this allows full
  token theft by whoever operates/compromises that service.
- Decision to make (pick one):
  a) Make the manual token flow (MANUAL_TOKEN.md) the primary login path and
     drop the OAuth browser flow entirely. Simplest, most honest for a
     client-side app. LoginPage keeps a clear guided UI.
  b) Host a minimal token-exchange proxy (e.g. a tiny serverless function you
     control) that holds the client secret and forwards `code` -> token.
     The app then needs NO embedded secret (also solves 3.2).
  Recommendation: (a) unless you want to run infrastructure.
- Acceptance: no third-party host appears anywhere in the auth flow; grep for
  `redirectmeto` returns nothing.

### 3.2 `[ ]` Stop embedding the client secret (follows from 3.1 choice)
- Files: `src/oauthmanager.cpp:15-25`, `harbour-lagoon.pro:13-20`
- If 3.1(a): delete `CLIENT_SECRET`, `exchangeCodeForToken`, the local TCP
  server, and the qmake defines. If 3.1(b): the proxy holds the secret; the
  app keeps only `CLIENT_ID`.
- Acceptance: `strings` on the built binary does not reveal a Slack secret.

### 3.3 `[ ]` Protect stored tokens
- File: `src/workspacemanager.cpp:274-328`
- Tokens are plaintext in `~/.config/harbour-lagoon/workspaces.conf`.
- Steps (incremental):
  1. `QFile::setPermissions` 0600 on the settings file after every save
     (cheap, do immediately).
  2. Evaluate Sailfish Secrets API availability on the minimum supported SFOS
     version; if usable, store tokens there and keep only workspace metadata
     in QSettings. Document the decision in this file.
  3. Stop exposing `TokenRole` to QML (`workspacemanager.cpp:53`); QML only
     needs `workspaceSwitched(index, token)` — audit QML usages first.
- Acceptance: config file is 0600 and contains no token (if step 2 lands);
  QML model no longer exposes `token`.

### 3.4 `[ ]` Cryptographically random OAuth state
- File: `src/oauthmanager.cpp:297-316`
- `qsrand(time)` is predictable. Qt 5.6 has no QRandomGenerator: read 32 bytes
  from `/dev/urandom` and hex-encode.
- Note: becomes moot if 3.1(a) removes the OAuth flow — then delete instead.

### 3.5 `[ ]` Purge sensitive logging
- Files: `src/oauthmanager.cpp:116` (full callback request incl. auth code),
  `src/slackapi.cpp:539` (full POST body = message contents), notification
  lambdas in `src/main.cpp:139-208` (message text), token-length logs.
- Remove or guard behind a compile-time `LAGOON_VERBOSE_LOG` define that is
  off in release builds. journald persists logs on device.
- Acceptance: a normal session's journal contains no message bodies, tokens,
  or OAuth codes.

### 3.6 `[ ]` Escape quotes in generated hrefs + kill duplicate formatter
- Files: `qml/components/EmojiHelper.js:343-344`, `qml/js/storage.js`
- HTML is escaped (good) but `"` inside a URL can break out of the `href`
  attribute. Escape `"` -> `&quot;` before building `<a href>`.
- `storage.js:parseMarkdown` duplicates the formatter WITHOUT escaping —
  check usages; delete it or delegate to the C++ formatter (2.4).
- Acceptance: message containing `<https://x.test/a"b|click>` renders inert.

### 3.7 `[ ]` Reduce OAuth scopes (only if OAuth flow survives 3.1)
- File: `src/oauthmanager.cpp:30` — legacy scope `client` = full access via
  deprecated `oauth.access`. Move to granular scopes covering: channels/ims
  read+history+write, users:read, reactions, pins, bookmarks, search, files,
  emoji, rtm:stream.

---

## Phase 4 — Tests

Goal: safety net so phases 1-3 refactors don't regress. Can start in parallel
with Phase 1; at minimum land 4.1 + 4.2 before Phase 2 (network refactor).

### 4.1 `[~]` Unit test harness (QtTest) (8f4fafe harness + tst_conversationmodel, b386ccf tst_messagemodel; usermodel/updatechecker/workspacemanager targets pending)
- New: `tests/unit/` with one `.pro` per test target, plus `tests/tests.pro`
  (SUBDIRS). Qt 5.6-compatible QtTest only.
- Wire into `harbour-lagoon.pro` as an optional SUBDIRS or standalone qmake
  project runnable on the build host (models are pure Qt, no Silica needed).
- Add a `run-tests.sh` (or extend `build.sh`) target that builds and runs all
  unit tests; must exit non-zero on failure.
- First targets (pure logic, no network):
  - `tst_conversationmodel`: parseConversation (channel/group/im/mpim, unread
    calculation from last_read vs latest, null latest), sortConversations
    ordering (starred > unread > type > alpha), markAsRead/incrementUnread
    transitions, get()/section mapping.
  - `tst_messagemodel`: parseMessage fields, addMessage prepend order,
    updateMessage by ts, getLatestTimestamp, isGroupedWithPrevious (after 1.6).
  - `tst_usermodel`: name priority (displayName > realName > name), hash
    lookup after 1.1, searchUsers matching/limits, cache round-trip.
  - `tst_updatechecker`: isNewerVersion matrix (equal, patch/minor/major
    greater/lesser, short versions).
  - `tst_workspacemanager`: add/update dedup by teamId, removeDuplicates,
    current index stability across remove/sort (use QSettings with
    `QStandardPaths::setTestModeEnabled(true)` or a temp config path).
- Acceptance: `run-tests.sh` green locally; document how to run in README.

### 4.2 `[ ]` SlackAPI integration tests against a mock server
- New: `tests/integration/tst_slackapi/`
- Spin an in-process HTTP mock (QTcpServer serving canned JSON per endpoint,
  like the OAuth callback server does) and point `SlackAPI` at it — requires
  making `API_BASE_URL` injectable (constructor arg or setter, default
  unchanged).
- Cover: auth.test success/failure, users.conversations -> signal payload,
  conversations.info unread extraction (unread_count_display / last_read
  paths / null latest), batch pacing of unread fetches, error responses
  (`ok:false`) emitting apiError, POST vs GET endpoint routing.
- Also `tst_websocketclient` with a mock `QWebSocketServer`: hello/message/
  error dispatch, ping timer, and (after 2.1) reconnect backoff.
- Acceptance: network refactors in Phase 2 are developed against these tests.

### 4.3 `[ ]` QML unit tests (Quick Test)
- New: `tests/qml/` using `qmltestrunner` (QtQuickTest, available in 5.6).
- Silica components don't run off-device easily: restrict to pure-JS/logic
  pieces — `EmojiHelper.js` (convertEmoji, emojiToReactionName,
  emojiToTwemojiUrl surrogate pairs, formatSlackText escaping incl. the
  `"` case from 3.6 while it still lives in JS), mention parsing helpers if
  extracted to a .js lib.
- Acceptance: runs on the build host without a device.

### 4.4 `[ ]` E2E smoke tests on device/emulator
- New: `tests/e2e/README.md` + scripts.
- Pragmatic approach for Sailfish (no Appium): a scripted scenario driven by
  the mock Slack server from 4.2 running on the host:
  1. Deploy RPM to emulator (`mb2` + `sfdk deploy` or ssh), launch app with
     `LAGOON_API_BASE_URL` env override (add support: read base URL from env
     in debug builds only).
  2. App logs structured checkpoints (login ok, N conversations loaded,
     message displayed, message sent) to stdout; the host script asserts the
     sequence via ssh journal/stdout capture.
  - Scenarios: fresh login (manual token) -> conversation list loads ->
    open channel -> history renders -> send message -> appears optimistically
    (2.3) -> websocket event injects an incoming message -> unread badge on
    another channel increments.
- Manual fallback: a written checklist in `tests/e2e/CHECKLIST.md` for
  pre-release runs on real hardware (login, scroll perf, suspend/resume
  reconnect, workspace switch, notifications).
- Acceptance: one command runs the smoke scenario against the emulator; the
  checklist is part of the release routine.

### 4.5 `[x]` CI hook (optional, after 4.1) (1f3d72a — tests.yml, plain Qt 5 on ubuntu-latest)
- GitHub Actions job building the unit tests with a plain Qt 5 image
  (models don't need Sailfish SDK) and running them on every push.
  Device/emulator e2e stays manual.

---

## Phase 5 — Structural cleanup (continuous)

### 5.1 `[ ]` QSortFilterProxyModel for FirstPage search/sections
- Replaces the delegate `visible/height:0` filtering (`FirstPage.qml:398-404`)
  and the per-keystroke `updateVisibleCount()` JS loop (`:148-157`).
  Filtering and section-collapse become proxy invalidations.

### 5.2 `[ ]` Extract notification logic from main.cpp
- `src/main.cpp:139-208` lambdas -> `NotificationCoordinator` class.
  Fix mention detection: match `<@CURRENT_USER_ID>` instead of
  `text.contains("@")`. Unit-test it (extends 4.1).

### 5.3 `[ ]` Deduplicate OAuth URL building
- `src/oauthmanager.cpp` builds the same URL in 3 places -> one private
  helper. (May disappear entirely with 3.1(a).)

### 5.4 `[ ]` Typing indicator without Qt.createQmlObject
- `qml/pages/ConversationPage.qml:105` compiles a QML Timer per typing event.
  Use a single declared Timer + expiry map, and debounce checkForMention
  (it currently runs twice per keystroke via onTextChanged +
  onCursorPositionChanged).

### 5.5 `[ ]` Pagination for conversation history
- `fetchConversationHistory` loads a fixed window; add `oldest`/`cursor`
  support + "load more" on scroll top in ConversationPage.

---

## Suggested session plan

| Session | Scope |
|---------|-------|
| 1 | 1.1, 1.2, 1.5 (small, independent, high impact) |
| 2 | 1.3 (model sort/move refactor) + 4.1 harness with tst_conversationmodel first |
| 3 | 1.4, 1.6 (delegate rendering) |
| 4 | 4.2 mock server + tst_slackapi (prep for network work) |
| 5 | 2.1 websocket reconnect, 2.2 event-driven unreads |
| 6 | 2.3 optimistic send, 2.4 C++ formatter (+ 3.6) |
| 7 | 3.1/3.2 OAuth decision + implementation, 3.4, 3.5 |
| 8 | 3.3 token storage, 2.5 SQLite cache |
| 9 | 4.3, 4.4 e2e smoke, 4.5 CI |
| 10+ | Phase 5 items as maintenance |
