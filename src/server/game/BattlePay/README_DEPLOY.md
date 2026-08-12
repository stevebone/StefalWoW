# In-game Shop (BattlePay) — deployment & administration

The 12.0.7.68275 `GET_PRODUCT_LIST_RESPONSE` catalog is a nested reflection bitstream that cannot be
re-serialized field-by-field offline. We ship a byte-exact, client-validated catalog blob captured from
a real 68275 session and treat it as a **TEMPLATE**: at startup and on `.reload shop_catalog`,
`BattlePayMgr` reskins the template's 9 simple-shape product slots from the `shop_product` DB tables via
the byte-exact `BattlePayCatalogWriter` (a C++ port of `battlepay_wire.py`, self-checked at load). The
offer set is therefore **DB-driven and reloadable without a restart**.

## ⚠ TESTER SETUP — REQUIRED, or the Shop opens EMPTY

The catalog template is a data file the core loads at startup; it is **not** compiled into worldserver.

1. Two blobs ship in the repo under `data/battlepay/`:
   - `product_list_68275.bin`      (catalog template, 58746 B)
   - `distribution_list_68275.bin` (distribution list, 107 B — unblocks the client shop panel)
2. **Copy the whole `battlepay/` folder into your server's DataDir**, preserving the subfolder:

       <DataDir>/battlepay/product_list_68275.bin
       <DataDir>/battlepay/distribution_list_68275.bin

   `<DataDir>` is the `DataDir` value in `worldserver.conf` (defaults to `.`).
3. Apply the DB updates (automatic on world/auth DB update):
   - `sql/updates/world/master/2026_08_09_00_world.sql` — creates `shop_product`,
     `shop_product_deliverable`, `shop_slot_override`, seeds all 9 template slots, drops the old
     `battlepay_product`.
   - `sql/updates/auth/master/2026_08_09_00_auth.sql` — RBAC perms 886 (`.reload shop_catalog`) and
     887 (`.shop`).

**Confirm it worked:** the worldserver log prints
`BattlePayCatalogWriter: self-check PASS - 9 simple slots, byte-exact round trip.` then
`BattlePay: assembled <N>-byte catalog (<M> routed slots) ...`. If the self-check FAILS the catalog is
served verbatim (no DB reskin) and the log says so — an empty/verbatim Shop is a data/path issue, not a
code bug.

## worldserver.conf toggles
- `Shop.Enabled` (default 1) — master switch: gates the feature-status availability flags (glue +
  in-world) AND every shop CMSG handler. Set 0 to fully disable the store.
- `Shop.PurchaseConfirmation` (default 0) — route purchases through the retail two-step confirm
  handshake. The `SMSG_BATTLE_PAY_CONFIRM_PURCHASE` layout is INFERRED (uncracked client struct, absent
  from all sniffs); leave OFF unless validating it live (the proven direct-grant path is used otherwise).
- `Shop.PlaceholderName` (optional, default "Currently unavailable") — name shown on unassigned slots.

## Admin model (DB is the source of truth)
`shop_product` — one row per sellable product (admin `productId`, decoupled from blob slot ids by the
routing map). Columns: enable/disable, name/description, currency+price (1 gold copper, 2 item token, 3
custom), optional `displayPrice`/`displayFlags`, `groupId`/`ordering`/`featured` for slot assignment,
`availableFrom`/`availableUntil` windows, and the purchase-time gates `reqLevel`/`reqFaction`/
`hideIfOwned`/`playerConditionId`.
`shop_product_deliverable` — payload(s); >1 row = bundle. `type` 1 item, 2 spell (both delivered on this
branch); 3 WoW Token (needs the wow-token branch's WowTokenMgr), 4 game-time / 5 service (reserved) —
types 3/4/5 currently abort the purchase with result 57 (no charge) until implemented.
`shop_slot_override` — pin a product to a physical slot; `productId = 0` forces an inert placeholder.

### Assembly
Candidates = enabled + in-window rows, ordered (featured DESC, ordering ASC, productId ASC). Overrides
pin first; the rest fill the 9 slots in order. Each assigned slot is reskinned (name/description/price/
flags) keeping the slot's advertised productID; the assembly records `slotProductId -> shop_product` so
the purchase handler resolves the buy. Unassigned slots become inert placeholders (purchase -> 57).
`availableFrom/Until` boundaries schedule an automatic rebuild on the world tick (restart-free rotation).

## Commands
- `.reload shop_catalog` — re-read the tables, rebuild, atomically swap the blob (clients pick it up on
  next shop open — the client re-requests the product list per open).
- `.shop list` / `.shop preview` — dump the assembled slot->product mapping + overflow warnings.
- `.shop enable <productId>` / `.shop disable <productId>`
- `.shop price <productId> <amount> [currency]`
- `.shop window <productId> <fromEpoch|-> <untilEpoch|->`
- `.shop feature <productId> <0|1>`
  All mutators write the DB then auto-reload.

## Purchase flow
`CMSG_BATTLE_PAY_START_PURCHASE` -> resolve advertised productID via routing -> `IsPurchasable`
(enabled/window/level/faction/owned/condition, server-authoritative) -> grant-before-charge over the
deliverable loop (item to bags with mail overflow; spell into the account-wide collection) -> charge ->
`SMSG_BATTLE_PAY_START_PURCHASE_RESPONSE` + `SMSG_BATTLE_PAY_PURCHASE_UPDATE(status=6)`. The account
purchase list (`CMSG_BATTLE_PAY_GET_PURCHASE_LIST`) and the distribution list are answered at the wire
layouts proven in the 68275 sniffs.

## Known limits (carried forward)
- 9 visible slots; no category/sort control until the ShopEntry region is cracked (SH-7). `groupId`/
  `ordering` are stored now so no schema change is needed then.
- Price renders as shop-currency fixed-point (mitigated by `displayPrice`/`HiddenPrice`); charging is
  server-side and exact.
- Products are never deleted to disable — placeholders/routing removal are used instead.
