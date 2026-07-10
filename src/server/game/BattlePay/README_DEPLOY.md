# In-game Shop (BattlePay) — deployment

P0 replays a byte-exact, client-validated `GET_PRODUCT_LIST_RESPONSE` catalog captured from a real
12.0.7.68275 session, because the 12.0.7 catalog wire is a nested reflection bitstream that cannot be
re-serialized field-by-field offline (per RE notes). This is real wire, not fabricated data.

## Required data file
Deploy the captured catalog blob (message body, opcode dword stripped) to:

    <DataDir>/battlepay/product_list_68275.bin

`BattlePayMgr::Load()` reads it at world startup. If absent, the Shop simply opens empty (nothing is
fabricated on the wire). Source blob provenance: extracted from
`C:\sniff\ingame-shop_ordersCrafting_professions.pkt`, SMSG 0x42021a, 58846 bytes.

## Scope
- P0 (this commit): catalog DISPLAY. Shop opens and shows real products.
- NOT yet: purchase/deliver-for-gold. Retail catalog products route their "buy" through the Battle.net
  RPC web-checkout (confirmed in sniff: CMSG_OPEN_CHECKOUT, 0x the in-game START_PURCHASE opcode), so a
  gold/token purchase path needs either a custom (in-game-purchasable) catalog — blocked on the reflection
  writer — or a live sniff of the in-game purchase flow. Tracked separately.
