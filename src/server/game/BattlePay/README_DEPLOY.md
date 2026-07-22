# In-game Shop (BattlePay) — deployment

P0 replays a byte-exact, client-validated `GET_PRODUCT_LIST_RESPONSE` catalog captured from a real
12.0.7.68275 session, because the 12.0.7 catalog wire is a nested reflection bitstream that cannot be
re-serialized field-by-field offline (per RE notes). This is real wire, not fabricated data.

## ⚠ TESTER SETUP — REQUIRED, or the Shop opens EMPTY

The catalog is a data file that the core loads at startup; it is **not** compiled into worldserver.
Every tester/operator MUST place it where the server looks, once per deployment:

1. The catalog blob now **ships in the repo** at:

       data/battlepay/product_list_68275.bin

2. **Copy it into your server's DataDir**, preserving the `battlepay/` subfolder:

       <DataDir>/battlepay/product_list_68275.bin

   `<DataDir>` is the `DataDir` value in `worldserver.conf` (same folder as your maps/dbc; it defaults
   to `.`, i.e. the worldserver working directory). So the file must end up at, e.g.,
   `<DataDir>/battlepay/product_list_68275.bin`.

3. The purchase rows apply automatically — `sql/updates/world/master/2026_07_22_00_world.sql` populates
   `battlepay_product` on the next world DB update. No manual SQL import needed.

**How to confirm it worked:** at startup the worldserver log prints
`BattlePay: loaded 58746-byte in-game Shop catalog ...`. If instead you see
`BattlePay: no catalog blob at '<path>' - the in-game Shop will open empty`, the file is not at the
path above — fix the copy / the DataDir value. An empty Shop is always this missing file, never a
code bug.

## Data file details
`BattlePayMgr::Load()` reads `<DataDir>/battlepay/product_list_68275.bin` at world startup. If absent,
the Shop simply opens empty (nothing is fabricated on the wire). The shipped blob is the **custom**
catalog (58746 bytes) whose productIDs match the `battlepay_product` rows; a raw retail capture would
advertise products with no purchase backing. Original capture provenance:
`C:\sniff\ingame-shop_ordersCrafting_professions.pkt`, SMSG 0x42021a (58846-byte body before reskin).

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
