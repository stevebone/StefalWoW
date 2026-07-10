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

## Custom catalog + purchase
The reflection catalog writer was cracked (`c:\dumps\battlepay_wire.py`), so we ship a **custom** catalog:
`gen_shop_catalog.py` reskins the first N retail entries into our shop items (keeping each entry's productID
so its ShopEntry keeps it visible) and writes `battlepay_custom_product_list.bin` -> deploy it as
`<DataDir>/battlepay/product_list_68275.bin`. Regenerate it together with the SQL below if the product set changes.

Apply `sql/custom/battlepay/battlepay_product.sql` to the **world** DB. Each row maps a catalog productId to a
gold/token cost + a grant (item or spell). `BattlePayMgr::LoadProducts()` loads it.

## Purchase flow (StartPurchase path)
On CMSG_BATTLE_PAY_START_PURCHASE the handler reads the productID (strong candidate = the u32 scalar; all
scalars are logged so a live purchase confirms it), validates + charges gold/token, grants the item/spell,
and replies with SMSG_BATTLE_PAY_START_PURCHASE_RESPONSE + SMSG_BATTLE_PAY_PURCHASE_UPDATE(status=Done). Wire
layouts were recovered offline (`c:\dumps\battlepay_purchase_wire.py`); the response packets are byte-aligned
(walletName sent empty) so no bit-packing risk. Delivery-detail packets (opaque blobs) are intentionally NOT
sent — the item arrives via the normal item/collection packets regardless.

## Runtime-confirmable assumptions (do NOT fabricate — logged for a live test)
- Which CMSG_START_PURCHASE scalar is the productID (candidate: u32). The handler logs all three.
- Whether the client uses StartPurchase (handled) vs OpenCheckout (retail web path; logged, not granted) for
  these reskinned products. A single live purchase (or a purchase sniff) confirms both; adjust if needed.
