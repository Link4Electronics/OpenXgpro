# 05 — Strategy

1. Reconstruct the interface from the embedded dialog/menu resources (this is
   the current milestone). The layout of every dialog can be dumped with
   `tools/dump_resources.py` — see `01-ui-inventory.md`.
2. Reproduce dialogs as native Qt widgets.
3. Reverse the USB transport / WINUSB protocol to drive real hardware — see
   `02-usb-transport.md`.
4. Implement the `.alg` algorithm plugin loader so chip support can be reused
   from the original distribution — see `04-algorithm-format.md`.

The hex-view/buffer reimplementation is covered separately in
`03-buffer-and-hexview.md`.
