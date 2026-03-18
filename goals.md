# Meteor — Completion Goals

## What's Done
- Setup wizard UI flow (mainpage → servername → users → local → streaming → select_user → new_user)
- Page transition animations (setup-preview.js)
- Flask backend with `/api/users` and `/api/users/new`
- CSS design system (dark theme)
- Voice config page in controlpanel

## What's Missing

### Critical
- [ ] Setup wizard inputs don't save anything — servername, admin user, local paths, streaming all navigate forward without persisting data
- [DONE] `select_user.html` login is a `// TODO` — clicking a user does nothing
- [ ] `main.py` redirects to setup even when `server: true` — no main UI is served
- [ ] `main/home.html` and `main/users.html` are empty shells
- [ ] Setup never writes `presetup: false` / `server: true` to `serverinfo.json`

### Medium
- [ ] `controlpanel.py` was deleted
- [ ] `meteorserver.py` is empty (unclear scope)

## Day-by-Day Plan

### Day 1 — Wire up the setup wizard
- Add `POST /api/setup/config` to save server name → `serverinfo.json`
- Fix `servername.html` to POST before navigating
- Fix `users.html` to call `/api/users/new` for the admin account
- Fix `local.html` to save paths to `serverinfo.json`
- At end of `streaming.html`, call API to set `presetup: false`, `server: true`

### Day 2 — Login/sessions
- Add Flask session support
- Add `POST /api/login` endpoint that checks password hash
- Fix `select_user.html` to POST credentials and redirect to main UI on success
- Add auth middleware to protect non-setup routes

### Day 3 — Main server UI
- Fix `main.py` to serve `main/home.html` when `server: true`
- Add route for the `main/` directory
- Build `main/home.html` (dashboard: server name, user count, status)
- Build `main/users.html` (list users, add/remove)

### Day 4 — Controlpanel
- Re-create `controlpanel.py`
- Hook up voice config page to save settings
- Wire up user management in `controlpanel/users/`

### Day 5 — Polish & test
- End-to-end test: fresh setup → login → main UI
- Fix edge cases (empty users.json, bad passwords, re-running setup)
- Determine what to do with `sysreq` data
