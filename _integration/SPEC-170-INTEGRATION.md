# SPEC-170 — Drag tab → grid slot (wireup)

QML side is done. The chip is now a real drag source, the empty slot is a
real drop target, and GridWorkspace switches the cell to a confirmation card
on a successful drop. The remaining work is on the C++ side so the assignment
actually persists across sessions and so the live tab session can be rendered
inside the cell.

## AppState.h additions

- `Q_INVOKABLE void attachTabToSlot(const QString& sourceTabId,
                                   const QString& hostTabId,
                                   int cellIndex);`
- `Q_INVOKABLE QString tabAtSlot(const QString& hostTabId,
                                  int cellIndex) const;`
- `Q_INVOKABLE void detachTabFromSlot(const QString& hostTabId,
                                       int cellIndex);` (optional — used when
   the user closes the cell)
- Optional polish (lets the placeholder card show the assigned tab's title +
  emoji instead of just the tabId): expose
  `Q_INVOKABLE QString tabTitle(const QString& tabId) const;` and
  `Q_INVOKABLE QString tabEmoji(const QString& tabId) const;`. Both already
  exist on TabsModel via roles, but a thin AppState wrapper makes them
  callable from QML without reaching into the model API.

## AppState.cpp

- Store the mapping inside the host `Tab.gridSpans` entries with an extra
  `"tabId"` key (so the same QVariantMap already round-tripped to
  `session.json` carries the assignment without a schema bump). Example
  span shape after attaching:

      gridSpans = {
        2: { "cols": 1, "rows": 1, "tabId": "tab-09a3" },
        ...
      }

- `attachTabToSlot` must:
  1. Reject self-attach (sourceTabId == hostTabId).
  2. Clear any existing assignment of `sourceTabId` on any other host (a
     tab can only live in one slot at a time).
  3. Update `gridSpans[cellIndex]["tabId"] = sourceTabId`.
  4. Emit a new signal `tabSlotAssignmentsChanged(QString hostTabId)` and
     persist to `session.json`.
- `tabAtSlot` reads the same map and returns "" when no assignment.
- The TabsModel already exposes `tabTitle` / `tabEmoji` Q_INVOKABLEs that
  the temporary placeholder card uses — no changes needed there.

## GridWorkspace.qml (already adjusted)

- Added `hostTabId` property + `assignmentsTick` so a drop refreshes the
  per-cell `assignedTabId` lookup.
- The Repeater renders an `EmptySlot` when `appState.tabAtSlot(hostId, idx)`
  is empty, and a lightweight stand-in card when it isn't.
- **Next step in C++**: once an embeddable `TerminalView` component is
  registered (the same one Terminal.qml already uses for the active tab),
  swap the stand-in `Rectangle` for `TerminalView { sessionId:
  appState.tabSessionId(parent.parent.assignedTabId); ... }`. Until that
  lands, the placeholder card with the tab's title + emoji communicates
  that the drop landed and the assignment is persisted.

## main.cpp / Terminal.qml

- Terminal.qml already passes `cols/rows/spans` to GridWorkspace; add
  `hostTabId: appState.activeTabId` to the same instantiation so slots
  can address their host correctly when multiple tabs each host a grid.

## Verification

- Drag a TabChip onto an empty slot in a 2×2 grid → cell shows the tab's
  emoji + title (confirmation card).
- The original chip stays in the TabBar (the chip is a drag *source*, not
  a move operation).
- Close + reopen the app → the assignment survives (gridSpans + session.json).
- Right-click / middle-click / double-click on the chip still work
  (rename / close / context menu) — drag threshold is 6px so taps never
  spuriously promote into drags.
- Hover with a drag over a slot → border pulses accent.
