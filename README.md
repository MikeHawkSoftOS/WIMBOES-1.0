# Wimboes 1.0

`make` builds `build/wimboes.img` in about a second (needs gcc with -m32, binutils). `make run` boots it in QEMU.

VirtualBox: `make vdi` (needs qemu-img), then attach the VDI to a VM (Other/Unknown 32-bit, BIOS, 128 MB).
VMware: convert the image the same way or attach as a raw disk.

Accounts: `Admin` and `Guest` (no passwords). Add more in Settings > Accounts. The disk is RAM only after reboot you lose all your shit.
Features: boot splash, login + user accounts, desktop icons and context menu, taskbar, Start menu with live
search, draggable/resizable windows, File Explorer, Notepad, Calculator, Paint, Command Prompt, Settings, Task Manager.

`make test` runs the same UI natively with sanitizers and writes screenshots to /tmp/shots.
Layout: src/boot.S (BIOS+VBE) -> src/entry.S -> src/main.c; hal_x86.c is the only hardware code.

<img width="1026" height="765" alt="image" src="https://github.com/user-attachments/assets/a1c246ae-51b6-4065-8fb2-65f118cf16aa" />
<img width="1024" height="764" alt="image" src="https://github.com/user-attachments/assets/2ea463f7-07a4-4fda-b200-7436db8214fd" />
